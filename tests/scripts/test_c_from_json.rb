#!/usr/bin/env ruby
require 'fileutils'

OUT_BIN = 'build/test_leuko_indentation'
# compile step
system('gcc', '-std=c99', '-Ivendor/cjson', '-Igenerated/configs', '-o', OUT_BIN, 'tests/c/test_leuko_indentation.c', 'generated/configs/rules/leuko_layout_indentation_consistency.c', 'generated/configs/categories/leuko_category_layout.c', 'generated/configs/leuko_config.c', 'vendor/cjson/cJSON.c', '-lm') or raise 'compile failed'
# run
system(OUT_BIN) or raise 'c test failed'
puts 'ok'
