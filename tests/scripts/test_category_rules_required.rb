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
      # no 'rules' provided
    }
  }
}

tmp_in = File.join(Dir.tmpdir, "leuko_test_input_category.json")
File.write(tmp_in, JSON.pretty_generate(input))
out_dir = File.join(Dir.tmpdir, "leuko_test_out_cat")
FileUtils.rm_rf(out_dir)

cmd = ['ruby', SCRIPT, '--in', tmp_in, '--out-dir', out_dir, '--schemas', SCHEMAS_DIR]
system(*cmd) or raise "normalize_config failed"

files = Dir.glob(File.join(out_dir, '*.json'))
raise "no output file" if files.empty?
json = JSON.parse(File.read(files.first))
raise 'Category rules not injected' unless json.dig('categories','layout','rules').is_a?(Hash)
puts 'ok'
