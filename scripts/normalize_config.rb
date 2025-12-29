#!/usr/bin/env ruby
# scripts/normalize_config.rb
# Normalize RuboCop resolved JSON into canonical form, inject defaults from rule schemas,
# canonicalize keys, compute SHA256, and write atomically to .leukocyte/<hash>.json

require 'json'
require 'digest'
require 'fileutils'
require 'optparse'

options = {
  input: nil,
  out_dir: '.leukocyte',
  schemas_dir: 'scripts/rule_schemas'
}

OptionParser.new do |opts|
  opts.banner = "Usage: normalize_config.rb --in resolved.json [--out-dir .leukocyte]"
  opts.on("--in FILE", "Input resolved JSON from export_rubocop_config.rb") { |v| options[:input] = v }
  opts.on("--out-dir DIR", "Output directory for canonical JSON") { |v| options[:out_dir] = v }
  opts.on("--schemas DIR", "Directory containing rule_schemas") { |v| options[:schemas_dir] = v }
  opts.on_tail("-h", "--help", "Show this message") { puts opts; exit }
end.parse!

unless options[:input]
  warn "--in is required"
  exit 2
end

input_json = JSON.parse(File.read(options[:input]))

# helper: convert CamelCase or mixed to snake_case
def to_snake_case(s)
  return nil unless s.is_a?(String)
  # handle already snake/camel/mixed: Insert underscore between lower-uppercase, then downcase
  s = s.gsub(/::/, '/')
  s = s.gsub(/([A-Z]+)([A-Z][a-z])/, '\1_\2')
  s = s.gsub(/([a-z\d])([A-Z])/, '\1_\2')
  s = s.tr("- ", "__")
  s.downcase
end

# Load rule schemas defaults (canonicalize names to snake_case, e.g., layout.indentation_consistency -> layout/indentation_consistency)
rule_schemas = {}
if Dir.exist?(options[:schemas_dir])
  Dir.glob(File.join(options[:schemas_dir], "*.json")).each do |f|
    begin
      j = JSON.parse(File.read(f))
      # canonical name: filename like Layout.IndentationConsistency.json -> Layout/IndentationConsistency -> snake -> layout/indentation_consistency
      parts = File.basename(f, '.json').split('.')
      name = parts.map { |p| to_snake_case(p) }.join('/')
      rule_schemas[name] = j
    rescue => e
      warn "Failed to parse rule schema #{f}: #{e}"
    end
  end
end

# Load compiled schema definitions for general/category defaults
$compiled_schema = {}
compiled_schema_path = 'scripts/compiled_config_schema.json'
if File.exist?(compiled_schema_path)
  $compiled_schema = JSON.parse(File.read(compiled_schema_path))
end

def get_default_from_compiled(defs, key, prop)
  return nil unless defs && defs[key] && defs[key]['properties'] && defs[key]['properties'][prop]
  prop_def = defs[key]['properties'][prop]
  return prop_def['default'] if prop_def.key?('default')
  if prop_def.key?('allOf') && prop_def['allOf'].is_a?(Array)
    prop_def['allOf'].each do |entry|
      return entry['default'] if entry.is_a?(Hash) && entry.key?('default')
    end
  end
  nil
end

general_defaults = $compiled_schema.dig('definitions', 'general_common') || {}
category_defaults = $compiled_schema.dig('definitions', 'category_common') || {}

# Default fallback for enabled/severity
DEFAULT_ENABLED = true
DEFAULT_SEVERITY = 'warning'

# Inject defaults into a rule value hash
def validate_against_schema(value_obj, schema)
  errors = []
  return [true, errors] unless schema

  # If schema has allOf, validate against each sub-schema
  if schema['allOf'].is_a?(Array)
    schema['allOf'].each do |sub|
      v, e = validate_against_schema(value_obj, sub)
      errors.concat(e) unless v
    end
    return [errors.empty?, errors]
  end

  return [true, errors] unless schema['properties']

  # required properties
  if schema['required'].is_a?(Array)
    schema['required'].each do |r|
      errors << "missing required #{r}" unless value_obj.key?(r)
    end
  end

  schema['properties'].each do |prop, prop_schema|
    next unless value_obj.key?(prop)
    val = value_obj[prop]
    # type check
    if prop_schema['type']
      case prop_schema['type']
      when 'boolean'
        errors << "#{prop} should be boolean" unless val == true || val == false
      when 'integer'
        errors << "#{prop} should be integer" unless val.is_a?(Integer)
      when 'string'
        errors << "#{prop} should be string" unless val.is_a?(String)
      when 'array'
        errors << "#{prop} should be array" unless val.is_a?(Array)
      when 'object'
        errors << "#{prop} should be object" unless val.is_a?(Hash)
      end
    end
    # enum
    if prop_schema['enum'] && !prop_schema['enum'].include?(val)
      errors << "#{prop} must be one of #{prop_schema['enum'].join(', ')}"
    end
    # minimum
    if prop_schema['minimum'] && val.is_a?(Numeric) && val < prop_schema['minimum']
      errors << "#{prop} must be >= #{prop_schema['minimum']}"
    end
  end

  [errors.empty?, errors]
end


def inject_defaults_for_rule(rule_name, value_obj, rule_schemas)
  # value_obj is a Hash (we ensure boolean becomes {enabled: bool} before calling)
  schema = rule_schemas[rule_name]
  # Inject enabled default if missing
  unless value_obj.key?('enabled')
    if schema && schema.dig('properties', 'enabled', 'default')
      value_obj['enabled'] = schema['properties']['enabled']['default']
    else
      value_obj['enabled'] = DEFAULT_ENABLED
    end
  end

  # Inject other defaults from schema or from compiled rule_common
  if schema && schema['properties']
    schema['properties'].each do |prop, prop_schema|
      next if prop == 'enabled'
      if !value_obj.key?(prop)
        if prop_schema.key?('default')
          value_obj[prop] = prop_schema['default']
        else
          # fallback to compiled schema defaults
          d = get_default_from_compiled($compiled_schema['definitions'], 'rule_common', prop)
          value_obj[prop] = d if d
        end
      end
    end
  end

  # severity default (try rule schema, compiled schema, then fallback constant)
  unless value_obj.key?('severity')
    if schema && schema.dig('properties','severity','default')
      value_obj['severity'] = schema['properties']['severity']['default']
    else
      d = get_default_from_compiled($compiled_schema['definitions'], 'rule_common', 'severity')
      value_obj['severity'] = d || DEFAULT_SEVERITY
    end
  end

  value_obj
end

# Convert keys in nested schema nodes to collect patternProperties
def collect_pattern_properties(node, patterns)
  return unless node.is_a?(Hash)
  if node['patternProperties'].is_a?(Hash)
    patterns.concat(node['patternProperties'].keys)
  end
  if node['properties'].is_a?(Hash)
    node['properties'].each do |k,v|
      collect_pattern_properties(v, patterns)
    end
  end
  if node['allOf'].is_a?(Array)
    node['allOf'].each do |sub|
      collect_pattern_properties(sub, patterns)
    end
  end
end

# Deep convert hash keys to snake_case recursively
def deep_snake_case(obj)
  case obj
  when Hash
    obj.each_with_object({}) do |(k, v), h|
      key = k.is_a?(String) ? to_snake_case(k) : k
      h[key] = deep_snake_case(v)
    end
  when Array
    obj.map { |e| deep_snake_case(e) }
  else
    obj
  end
end


def rule_allowed_in_category?(cat_name, rule_name, category_schemas)
  schema = category_schemas[cat_name]
  return true unless schema
  patterns = []
  collect_pattern_properties(schema, patterns)
  return true if patterns.empty?
  # rule_name is expected to be snake_case short name (e.g., indentation_consistency)
  patterns.any? { |pat| Regexp.new(pat).match?(rule_name) }
end


def normalize_rules(categories, rule_schemas, category_schemas)
  return unless categories.is_a?(Hash)
  categories.each do |cat_name, cat_obj|
    next unless cat_obj.is_a?(Hash) && cat_obj['rules'].is_a?(Hash)
    rules = cat_obj['rules']
    # iterate over a copy of keys because we may delete keys
    rules.keys.dup.each do |orig_rule_name|
      val = rules[orig_rule_name]

      # Normalize rule name: take last segment after '/' and convert to snake_case
      short = orig_rule_name.to_s.split('/').last
      rule_name = to_snake_case(short)

      # handle name collisions after normalization: silently drop the duplicate
      if rule_name != orig_rule_name && rules.key?(rule_name)
        rules.delete(orig_rule_name)
        next
      end

      # Move value to normalized key if needed
      if rule_name != orig_rule_name
        rules[rule_name] = rules.delete(orig_rule_name)
      end

      # If rule schema is unknown, silently ignore it
      fullname = "#{cat_name}/#{rule_name}"
      unless rule_schemas.key?(fullname)
        rules.delete(rule_name)
        next
      end

      # If rule is not allowed by the category schema, silently ignore it
      unless rule_allowed_in_category?(cat_name, rule_name, category_schemas)
        rules.delete(rule_name)
        next
      end

      if val == true || val == false
        rules[rule_name] = { 'enabled' => !!val }
      elsif val.is_a?(Hash)
        # normalize property keys to snake_case, ensure enabled present, and inject defaults from schema
        rules[rule_name] = deep_snake_case(val.dup)
        rules[rule_name] = inject_defaults_for_rule(fullname, rules[rule_name], rule_schemas)
      else
        # unexpected type: move to raw
        rules[rule_name] = { 'enabled' => DEFAULT_ENABLED, 'raw' => val }
      end

      # Also ensure rule-specific defaults are injected
      rules[rule_name] = inject_defaults_for_rule(fullname, rules[rule_name], rule_schemas)

      # Validate against schema; if invalid, exit immediately
      schema = rule_schemas[fullname]
      valid, errs = validate_against_schema(rules[rule_name], schema)
      unless valid
        exit 2
      end
    end

    # ensure include/exclude arrays exist (normalize: keep absent as empty list? we leave absent)
  end
end

# Recursively sort object keys to produce canonical JSON
def canonicalize(obj)
  case obj
  when Hash
    sorted = obj.keys.sort.each_with_object({}) do |k, h|
      h[k] = canonicalize(obj[k])
    end
    sorted
  when Array
    obj.map { |e| canonicalize(e) }
  else
    obj
  end
end

# Perform normalization
# - convert booleans to objects with enabled
# - inject defaults from schemas
# - inject severity default
# - canonicalize (key sort)

# Clone input to avoid mutating original
normalized = Marshal.load(Marshal.dump(input_json))

# Ensure top-level structure presence
normalized['metadata'] ||= {}
normalized['schema_version'] ||= '1.0.0'
# Ensure categories exist
normalized['categories'] ||= {}
# No annotations are recorded in this mode (unknown keys are silently ignored; validation errors abort immediately)

# Load category schemas (canonicalize filename -> snake_case name)
category_schemas = {}
if Dir.exist?('scripts/category_schemas')
  Dir.glob(File.join('scripts/category_schemas','*.json')).each do |f|
    begin
      j = JSON.parse(File.read(f))
      name = to_snake_case(File.basename(f, '.json'))
      category_schemas[name] = j
    rescue => e
      warn "Failed to parse category schema #{f}: #{e}"
    end
  end
end

# Inject defaults for general
normalized['general'] ||= {}
if general_defaults && general_defaults['properties']
  ['enabled','severity','include','exclude'].each do |p|
    if !normalized['general'].key?(p) && general_defaults['properties'][p] && general_defaults['properties'][p]['default']
      normalized['general'][p] = general_defaults['properties'][p]['default']
    end
  end
end

# Normalize category names to snake_case and ignore unknown categories; inject category defaults
normalized['categories'].keys.dup.each do |orig_cat_name|
  cat_name = to_snake_case(orig_cat_name)
  # handle collisions: if normalized already has this cat name from another original, silently drop the duplicate
  if normalized['categories'].key?(cat_name)
    normalized['categories'].delete(orig_cat_name)
    next
  end
  # move/rename key if needed (silent)
  if cat_name != orig_cat_name
    normalized['categories'][cat_name] = normalized['categories'].delete(orig_cat_name)
  end

  unless category_schemas.key?(cat_name)
    normalized['categories'].delete(cat_name)
    next
  end

  schema = category_schemas[cat_name]
  # inject category defaults: prefer category schema defaults, fallback to compiled category_common
  cat = normalized['categories'][cat_name]
  # Ensure rules property exists (empty allowed)
  cat['rules'] ||= {}
  if cat && (schema || category_defaults)
    ['enabled','severity','include','exclude'].each do |p|
      unless cat.key?(p)
        # check category schema for default
        defval = nil
        if schema && schema['properties'] && schema['properties'][p] && schema['properties'][p]['default']
          defval = schema['properties'][p]['default']
        else
          # check allOf entries
          if schema && schema['allOf'].is_a?(Array)
            schema['allOf'].each do |sub|
              if sub['properties'] && sub['properties'][p] && sub['properties'][p]['default']
                defval = sub['properties'][p]['default']; break
              end
            end
          end
        end
        if defval.nil? && category_defaults && category_defaults['properties'] && category_defaults['properties'][p] && category_defaults['properties'][p]['default']
          defval = category_defaults['properties'][p]['default']
        end
        cat[p] = defval if defval
      end
    end
  end
end

normalize_rules(normalized['categories'], rule_schemas, category_schemas)

# Canonicalize the whole object
canonical = canonicalize(normalized)
canonical_str = JSON.generate(canonical)
checksum = Digest::SHA256.hexdigest(canonical_str)

out_dir = options[:out_dir]
FileUtils.mkdir_p(out_dir)
out_path_tmp = File.join(out_dir, ".#{checksum}.tmp")
out_path = File.join(out_dir, "#{checksum}.json")
File.open(out_path_tmp, 'w') do |f|
  f.write(canonical_str)
end
File.rename(out_path_tmp, out_path)

puts out_path
puts checksum
exit 0
