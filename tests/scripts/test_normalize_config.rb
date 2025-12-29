#!/usr/bin/env ruby
require 'json'
require 'fileutils'
require 'tmpdir'

ROOT = File.expand_path('../../../', __FILE__)
SCRIPT = File.join(ROOT, 'scripts', 'normalize_config.rb')
SCHEMAS_DIR = File.join(ROOT, 'scripts', 'rule_schemas')

input = {
  'schema_version' => '1.0.0',
  'categories' => {
    'Layout' => {
      'rules' => {
        'Layout/TrailingWhitespace' => true,
        'Layout/UnknownCop' => { 'SomeOption' => 123 },
        'Layout/IndentationConsistency' => { 'EnforcedStyle' => 'invalid_style' },
        'Style/SomeRule' => true
      }
    }
  }
}

tmp_in = File.join(Dir.tmpdir, "leuko_test_input.json")
File.write(tmp_in, JSON.pretty_generate(input))

out_dir = File.join(Dir.tmpdir, "leuko_test_out")
FileUtils.rm_rf(out_dir)

cmd = ['ruby', SCRIPT, '--in', tmp_in, '--out-dir', out_dir, '--schemas', SCHEMAS_DIR]
# This input includes an invalid value for IndentationConsistency, so normalize should fail (exit non-zero)
success = system(*cmd)
raise "normalize_config should fail on invalid rule value" if success

# Ensure no output file was produced
files = Dir.glob(File.join(out_dir, '*.json'))
raise "output file should not be produced on invalid input" unless files.empty?

puts 'ok'
