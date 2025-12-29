#!/usr/bin/env ruby
require 'fileutils'
require 'json'
ROOT = File.expand_path('../../../', __FILE__)
GEN = File.join(ROOT, 'scripts', 'run_generator.sh')
SCHEMAS = File.join(ROOT, 'scripts', 'rule_schemas')
OUT = File.join(ROOT, 'generated')

# Clean
FileUtils.rm_rf(OUT)
# Run generator
system('bash', GEN, SCHEMAS, OUT) or raise 'generator failed'
# Check aggregated files
raise 'no leuko_config.h' unless File.exist?(File.join(OUT, 'configs', 'leuko_config.h'))
raise 'no leuko_config.c' unless File.exist?(File.join(OUT, 'configs', 'leuko_config.c'))
# Check canonical per-rule header exists
p = File.join(OUT, 'configs', 'rules', 'leuko_layout_indentation_consistency.h')
ch = File.join(OUT, 'configs', 'categories', 'leuko_category_layout.h')
puts "OUT=#{OUT}\nchecking #{p} exists=#{File.exist?(p)}\nchecking #{ch} exists=#{File.exist?(ch)}"
raise 'no per-rule header' unless File.exist?(p)
raise 'no category header' unless File.exist?(ch)

hdr = File.read(File.join(OUT, 'configs', 'leuko_config.h'))
raise 'init prototype missing' unless hdr.include?('leuko_config_init_defaults')

puts 'ok'
