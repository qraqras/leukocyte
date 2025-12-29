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
        'Style/SomeRule' => true
      }
    }
  }
}

tmp_in = File.join(Dir.tmpdir, "leuko_test_unknown_input.json")
File.write(tmp_in, JSON.pretty_generate(input))

out_dir = File.join(Dir.tmpdir, "leuko_test_unknown_out")
FileUtils.rm_rf(out_dir)

cmd = ['ruby', SCRIPT, '--in', tmp_in, '--out-dir', out_dir, '--schemas', SCHEMAS_DIR]
system(*cmd) or raise "normalize_config failed"

# verify there's a json file in out_dir
files = Dir.glob(File.join(out_dir, '*.json'))
raise "no output file" if files.empty?

json = JSON.parse(File.read(files.first))
# trailing_whitespace was unknown in schema and should be removed
raise 'trailing_whitespace should be absent (ignored)' unless json.dig('categories','layout','rules','trailing_whitespace').nil?
# UnknownCop should be absent
raise 'UnknownCop should be absent' unless json.dig('categories','layout','rules','unknown_cop').nil?
# Style.SomeRule is a different category and should be ignored from Layout
raise 'Style.SomeRule should be absent' unless json.dig('categories','layout','rules','some_rule').nil?
# No annotations present
raise 'annotations should not exist' if json.key?('annotations')

puts 'ok'
