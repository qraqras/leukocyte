#!/usr/bin/env ruby
require 'json'
require 'fileutils'
require 'tmpdir'

ROOT = File.expand_path('../../../', __FILE__)
NORM = File.join(ROOT, 'scripts', 'normalize_config.rb')
GEN = File.join(ROOT, 'scripts', 'run_generator.sh')
SCHEMAS = File.join(ROOT, 'scripts', 'rule_schemas')

# Prepare input that sets indentation_consistency to tab/4
input = {
  'schema_version' => '1.0.0',
  'categories' => {
    'Layout' => {
      'rules' => {
        'Layout/IndentationConsistency' => { 'EnforcedStyle' => 'tab', 'IndentWidth' => 4 }
      }
    }
  }
}

tmp_in = File.join(Dir.tmpdir, "leuko_integration.json")
File.write(tmp_in, JSON.pretty_generate(input))

out_dir = File.join(Dir.tmpdir, "leuko_integration_out")
FileUtils.rm_rf(out_dir)

# Run normalize (this should produce a canonical JSON under out_dir)
system('ruby', NORM, '--in', tmp_in, '--out-dir', out_dir, '--schemas', SCHEMAS) or raise 'normalize failed'
files = Dir.glob(File.join(out_dir, '*.json'))
raise 'no normalized file' if files.empty?

# Copy normalized file to /tmp for the C test to pick up
FileUtils.cp(files.first, '/tmp/leuko_integration.json')

# Run generator
system('bash', GEN, 'scripts/rule_schemas/layout.indentation_consistency.json', 'generated') or raise 'generator failed'

# Compile C integration test
system('gcc', '-std=c99', '-Ivendor/cjson', '-Igenerated/configs', '-o', 'build/test_leuko_config_integration', 'tests/c/test_leuko_config_integration.c', 'generated/configs/leuko_config.c', 'generated/configs/categories/leuko_category_layout.c', 'generated/configs/rules/leuko_layout_indentation_consistency.c', 'vendor/cjson/cJSON.c', '-lm') or raise 'compile failed'

# Run
system('./build/test_leuko_config_integration') or raise 'integration test failed'

puts 'ok'
