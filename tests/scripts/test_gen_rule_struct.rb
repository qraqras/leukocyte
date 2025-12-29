#!/usr/bin/env ruby
require 'json'
require 'fileutils'

OUT = File.expand_path('../../..', __FILE__) + '/generated'
FileUtils.rm_rf(OUT)
system('bash', 'scripts/run_generator.sh', 'scripts/rule_schemas/layout.indentation_consistency.json', OUT) or raise 'generator failed'

hdr = File.read(File.join(OUT, 'configs', 'rules', 'leuko_layout_indentation_consistency.h'))
src = File.read(File.join(OUT, 'configs', 'rules', 'leuko_layout_indentation_consistency.c'))
raise 'header missing struct name' unless hdr.include?('leuko_layout_indentation_consistency_t')
raise 'default enforced_style missing' unless src.include?('strdup("space")')
raise 'default indent width' unless src.include?('out->indent_width = 2')
# check general files present
raise 'no leuko_general.h' unless File.exist?(File.join(OUT, 'configs', 'leuko_general.h'))
raise 'no leuko_general.c' unless File.exist?(File.join(OUT, 'configs', 'leuko_general.c'))
puts 'ok'
