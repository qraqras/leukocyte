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
        'Layout/IndentationConsistency' => { 'EnforcedStyle' => 'tab', 'IndentWidth' => 4 }
      }
    }
  }
}

tmp_in = File.join(Dir.tmpdir, "leuko_test_input_snake.json")
File.write(tmp_in, JSON.pretty_generate(input))

out_dir = File.join(Dir.tmpdir, "leuko_test_out_snake")
FileUtils.rm_rf(out_dir)

cmd = ['ruby', SCRIPT, '--in', tmp_in, '--out-dir', out_dir, '--schemas', SCHEMAS_DIR]
system(*cmd) or raise "normalize_config failed"

files = Dir.glob(File.join(out_dir, '*.json'))
raise "no output file" if files.empty?

json = JSON.parse(File.read(files.first))
rule = json.dig('categories','layout','rules','indentation_consistency')
raise 'rule missing' unless rule.is_a?(Hash)
raise 'enforced_style not normalized' unless rule['enforced_style'] == 'tab'
raise 'indent_width not normalized' unless rule['indent_width'] == 4

puts 'ok'
