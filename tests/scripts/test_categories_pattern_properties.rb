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
      'rules' => {}
    },
    'FooBar' => {
      'rules' => {
        'Layout/TrailingWhitespace' => true
      }
    }
  }
}

tmp_in = File.join(Dir.tmpdir, "leuko_test_input_cat.json")
File.write(tmp_in, JSON.pretty_generate(input))

out_dir = File.join(Dir.tmpdir, "leuko_test_out_cat")
FileUtils.rm_rf(out_dir)

cmd = ['ruby', SCRIPT, '--in', tmp_in, '--out-dir', out_dir, '--schemas', SCHEMAS_DIR]
system(*cmd) or raise "normalize_config failed"

files = Dir.glob(File.join(out_dir, '*.json'))
raise "no output file" if files.empty?

json = JSON.parse(File.read(files.first))
# Known category exists (normalized to snake_case)
raise 'layout must exist' unless json.dig('categories','layout')
# Unknown category should be removed
raise 'FooBar should be removed' if json.dig('categories','foo_bar')
# No annotations should be present in the output (unknowns are silently ignored)
raise 'annotations should not exist' if json.key?('annotations')

puts 'ok'
