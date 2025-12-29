#!/usr/bin/env ruby
# scripts/sync_configs.rb
# Find RuboCop config files for a base directory using RuboCop APIs, then
# generate resolved JSON for each using scripts/export_rubocop_config.rb and
# write them under a target .leukocyte directory with an index.json.

require 'optparse'
require 'fileutils'
require 'json'
require 'time'

begin
  require 'rubocop'
rescue LoadError
  warn 'Rubocop gem not found. Install with: gem install rubocop'
  exit 1
end

options = {
  dir: Dir.pwd,
  outdir: '.leukocyte/configs',
  index: '.leukocyte/index.json',
  exporter: 'scripts/export_rubocop_config.rb',
  verbose: false
}

OptionParser.new do |opt|
  opt.on('--dir DIR', 'Base directory to search for configs (default: cwd)') { |v| options[:dir] = v }
  opt.on('--outdir DIR', 'Directory to place generated JSONs (default: .leukocyte/configs)') { |v| options[:outdir] = v }
  opt.on('--index PATH', 'Index JSON file (default: .leukocyte/index.json)') { |v| options[:index] = v }
  opt.on('--exporter PATH', 'Path to exporter script (default: scripts/export_rubocop_config.rb)') { |v| options[:exporter] = v }
  opt.on('--verbose', 'Verbose') { options[:verbose] = true }
  opt.on('--help', 'Show help') { puts opt; exit }
end.parse!

# Helpers
def sanitize_name(path)
  s = path.gsub(/^\//, '')
  s = s.gsub(/[^A-Za-z0-9._\-]/, '_')
  s = s.gsub(/[_]{2,}/, '_')
  s = s[0, 120]
  s
end

# Use RuboCop::ConfigFinder to find project root
project_root = nil
begin
  project_root = RuboCop::ConfigFinder.project_root
rescue => _e
  project_root = nil
end

# Collect candidates
candidates = []
start_dir = File.expand_path(options[:dir])
cur = start_dir.dup

# Collect .rubocop.yml upwards until project_root (inclusive)
loop do
  candidate = File.join(cur, RuboCop::ConfigFinder::DOTFILE)
  candidates << candidate if File.exist?(candidate)
  break if cur == '/' || (project_root && File.expand_path(cur) == File.expand_path(project_root))
  parent = File.dirname(cur)
  break if parent == cur
  cur = parent
end

# project-level .config entries
if project_root
  p1 = File.join(project_root, '.config', RuboCop::ConfigFinder::DOTFILE)
  candidates << p1 if File.exist?(p1)
  p2 = File.join(project_root, '.config', 'rubocop', 'config.yml')
  candidates << p2 if File.exist?(p2)
end

# user home
if ENV['HOME']
  u = File.join(ENV['HOME'], RuboCop::ConfigFinder::DOTFILE)
  candidates << u if File.exist?(u)
end

# XDG config
xdg = ENV['XDG_CONFIG_HOME'] || File.join(ENV['HOME'] || '', '.config')
if xdg && !xdg.empty?
  x = File.join(xdg, 'rubocop', 'config.yml')
  candidates << x if File.exist?(x)
end

# Deduplicate preserving order
candidates.uniq!

if candidates.empty?
  warn 'No RuboCop configuration files found.'
  exit 1
end

# Prepare output dirs
index_path = options[:index]
outdir = options[:outdir]
index_dir = File.dirname(index_path)
FileUtils.mkdir_p(outdir, mode: 0o700)
FileUtils.mkdir_p(index_dir, mode: 0o700) unless File.directory?(index_dir)

index_entries = []
count = 0

candidates.each do |cfg|
  count += 1
  base = sanitize_name(cfg)
  name = format('%04d_%s.json', count, base)
  outpath = File.join(outdir, name)
  tmp_out = outpath + '.tmp'

  # Call exporter
  exporter = options[:exporter]
  cmd = ["ruby", exporter, "--config", cfg, "--out", tmp_out]
  if options[:verbose]
    puts "Running: #{cmd.join(' ')}"
  end
  rc = system(*cmd)
  unless rc
    warn "Exporter failed for #{cfg}"
    next
  end

  # Atomic move
  File.rename(tmp_out, outpath)

  index_entries << {
    'src' => cfg,
    'out' => outpath,
    'ts' => Time.now.utc.iso8601
  }
  puts "Wrote #{outpath} (from #{cfg})" if options[:verbose]
end

# Write index file (pretty)
File.open(index_path + '.tmp', 'w', 0o644) do |f|
  f.write(JSON.pretty_generate(index_entries))
end
File.rename(index_path + '.tmp', index_path)
puts "Wrote index #{index_path}"

exit 0
