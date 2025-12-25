#!/usr/bin/env ruby

require 'json'
require 'yaml'
require 'optparse'

begin
  require 'rubocop'
rescue LoadError
  warn 'Rubocop gem not found. Install with: gem install rubocop'
  exit 1
end

# Constants: defaults
DEFAULT_OUT = '.leukocyte.json'

options = {
  config: nil,
  files: [],
  out: DEFAULT_OUT
}

OptionParser.new do |opt|
  opt.on('--config PATH', 'RuboCop config file (optional)') { |v| options[:config] = v }
  opt.on('--files GLOB', 'Comma-separated globs of files to sample (optional)') { |v| options[:files] = v.split(',') }
  opt.on('--out PATH', 'Output JSON file') { |v| options[:out] = v }
  opt.on('--help', 'Show help') {
    puts opt; puts "Note: pretty output and config-file-only mode are always enabled."; exit
  }
end.parse!

store = RuboCop::ConfigStore.new
store.options_config = options[:config] if options[:config]

# Helper: deep merge two hashes (child overrides parent). Arrays/scalars are replaced by child.
def deep_merge(parent, child)
  result = parent.dup
  child.each do |k, v|
    if result[k].is_a?(Hash) && v.is_a?(Hash)
      result[k] = deep_merge(result[k], v)
    else
      result[k] = v
    end
  end
  result
end

# Helper: load YAML config and recursively resolve inherit_from WITHOUT merging Cop defaults
def load_and_merge_yaml(path, visited = {})
  unless File.exist?(path)
    warn "Config file not found: #{path}"
    return {}
  end
  real = File.expand_path(path)
  return {} if visited[real]

  visited[real] = true
  h = YAML.load_file(real) || {}
  base = {}
  parents = h['inherit_from']
  if parents
    parents = [parents] unless parents.is_a?(Array)
    parents.each do |p|
      ppath = File.expand_path(p, File.dirname(real))
      base = deep_merge(base, load_and_merge_yaml(ppath, visited))
    end
  end
  deep_merge(base, h)
end

# Determine sample files
sample_files = options[:files]
if sample_files.empty?
  sample_files = Dir.glob('**/*.rb').reject { |p| p.start_with?('vendor/') }
end
if sample_files.empty?
  warn 'No Ruby files found to sample. Use --files to specify.'
  exit 1
end

# Resolve config files (config-file-only behavior is always enabled)
if options[:config].nil?
  warn 'Config-file-only mode requires --config PATH'
  exit 1
end
# load the configuration file(s) directly by parsing YAML and resolving inherit_from
# This avoids RuboCop's merging of Cop defaults so only explicit keys are present
raw_hash = load_and_merge_yaml(options[:config])

# Build output structure (direct)
out = {
  'general' => {},
  'categories' => {}
}

# helper: normalize key names to snake_case (simple)
def normalize_key(k)
  k.to_s.gsub(/([^A-Za-z0-9])/, '_').gsub(/([A-Z])/) { "_#{$1.downcase}" }.gsub(/^_+|_+$/, '').downcase
end

# helper: deep copy values and normalize hash keys recursively
def copy_normalized_value(val)
  case val
  when Array
    val.map { |e| copy_normalized_value(e) }
  when Hash
    h = {}
    val.each do |kk, vv|
      h[normalize_key(kk)] = copy_normalized_value(vv)
    end
    h
  else
    val
  end
end

# Populate a section from raw YAML config. If is_category is true, handle rules and cop-name checks
def populate_section(out_section, raw_section, is_category = false, cat_name = nil)
  return unless raw_section.is_a?(Hash)

  raw_section.each do |kk, vv|
    if is_category && kk.is_a?(String) && kk.include?('/')
      c2, r2 = kk.split('/', 2)
      ensure_category(out, c2)
      out['categories'][c2]['rules'][r2] = format_rule_obj(vv)
    elsif is_category && kk.is_a?(String)
      qname = "#{cat_name}/#{kk}"
      reg = RuboCop::Cop::Registry.global
      if reg.names.include?(qname) || vv.is_a?(Hash)
        out_section['rules'] ||= {}
        out_section['rules'][kk] = format_rule_obj(vv)
      else
        if vv.is_a?(Array)
          out_section[normalize_key(kk)] = vv.dup
        elsif vv.is_a?(Hash)
          out_section[normalize_key(kk)] = copy_normalized_value(vv)
        else
          out_section[normalize_key(kk)] = vv
        end
      end
    else
      nk = normalize_key(kk)
      if vv.is_a?(Hash)
        out_section[nk] = copy_normalized_value(vv)
      elsif vv.is_a?(Array)
        out_section[nk] = vv.dup
      else
        out_section[nk] = vv
      end
    end
  end
end

# copy AllCops into `general` section
if raw_hash['AllCops'] && raw_hash['AllCops'].is_a?(Hash)
  populate_section(out['general'], raw_hash['AllCops'])
end

# iterate over resolved keys (simplified)
def ensure_category(o, cat)
  o['categories'][cat] ||= { 'rules' => {} }
end

def format_rule_obj(v)
  if v.is_a?(Hash)
    obj = {}
    v.each do |kk, vv|
      nk = normalize_key(kk)
      obj[nk] = copy_normalized_value(vv)
    end
    obj
  else
    { 'enabled' => (v != false) }
  end
end

reg = RuboCop::Cop::Registry.global

raw_hash.each do |k, v|
  next if k == 'AllCops'
  next unless k.is_a?(String)

  if k.include?('/')
    cat, rule = k.split('/', 2)
    ensure_category(out, cat)
    out['categories'][cat]['rules'][rule] = format_rule_obj(v)
  elsif v.is_a?(Hash)
    cat = k
    # Check against RuboCop registry for known department names
    is_known_category = if reg.respond_to?(:departments)
                          deps = reg.departments
                          deps.include?(cat.to_sym) || deps.map(&:to_s).include?(cat)
                        else
                          false
                        end
    unless is_known_category
      warn "Unknown category '#{cat}' — not found in RuboCop registry"
    end
    ensure_category(out, cat)
    populate_section(out['categories'][cat], v, true, cat)
  end
end

# Post-process: coerce certain keys to Array based on resolved/default RuboCop config
begin
  sample_file = sample_files.first
  resolved_hash = store.for(sample_file).to_h
rescue => _e
  resolved_hash = {}
end
begin
  default_cfg = RuboCop::ConfigLoader.default_configuration
rescue => _e
  default_cfg = {}
end

# Helper: build a map from normalized key -> original key (prefer raw -> resolved -> default order)
def build_normalized_map(list)
  map = {}
  list.each do |orig|
    nk = normalize_key(orig)
    map[nk] ||= orig
  end
  map
end

# Coerce keys to Array when resolved/default indicate Array using map lookup
if out['general'].is_a?(Hash)
  general_key_list = ((raw_hash['AllCops'] || {}).keys + (resolved_hash['AllCops'] || {}).keys + (default_cfg['AllCops'] || {}).keys).compact.uniq
  all_map = build_normalized_map(general_key_list)
  out['general'].each do |nk, val|
    orig = all_map[nk]
    next unless orig

    res_val = resolved_hash.dig('AllCops', orig)
    def_val = default_cfg.dig('AllCops', orig)
    if res_val.is_a?(Array) || def_val.is_a?(Array)
      out['general'][nk] = Array(val)
    end
  end
end

# Categories: coerce keys based on resolved/default types using map lookup
out['categories'].each do |cat, catobj|
  key_list = ((raw_hash[cat] || {}).keys + (resolved_hash[cat] || {}).keys + (default_cfg[cat] || {}).keys).compact.uniq
  key_map = build_normalized_map(key_list)
  catobj.each do |nk, val|
    next if nk == 'rules'

    orig = key_map[nk]
    next unless orig

    res_val = resolved_hash.dig(cat, orig)
    def_val = default_cfg.dig(cat, orig)
    if res_val.is_a?(Array) || def_val.is_a?(Array)
      catobj[nk] = Array(val)
    end
  end
end

# Sanitize values that JSON can't encode (Infinity, NaN)
def sanitize_values(v)
  case v
  when Hash
    h = {}
    v.each { |k, vv| h[k] = sanitize_values(vv) }
    h
  when Array
    v.map { |e| sanitize_values(e) }
  when Float
    if v.infinite? || v.nan?
      nil
    else
      v
    end
  else
    v
  end
end

safe_out = sanitize_values(out)

# Sort hash keys recursively for stable, diff-friendly output
def sort_keys(v)
  case v
  when Hash
    sorted = {}
    v.keys.sort.each do |k|
      sorted[k] = sort_keys(v[k])
    end
    sorted
  when Array
    v.map { |e| sort_keys(e) }
  else
    v
  end
end

sorted_out = sort_keys(safe_out)
json = JSON.pretty_generate(sorted_out)
json = json.gsub(/\[\s*\n\s*\](,?)/, '[]\1')
File.write(options[:out], json)
puts "Wrote #{options[:out]}"
