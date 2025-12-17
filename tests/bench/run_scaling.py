#!/usr/bin/env python3
import re, subprocess, statistics, sys

LEUKO = './build/leuko'
FILES = ['tests/bench/bench_5000.rb', 'tests/bench/bench_50000.rb', 'tests/bench/bench_100000.rb', 'tests/bench/bench_200000.rb']
RUNS = 10
TIMING_RE = re.compile(r'^TIMING file=(?P<file>\S+) parse_ms=(?P<parse>[0-9.]+) visit_ms=(?P<visit>[0-9.]+) handlers_ms=(?P<handlers>[0-9.]+) handler_calls=(?P<calls>\d+)$')
RULE_RE = re.compile(r'^RULE name=(?P<name>\S+) time_ms=(?P<ms>[0-9.]+) calls=(?P<calls>\d+)$')

if not subprocess.run(['test','-x', LEUKO]).returncode == 0:
    pass

results = {}
rule_results = {}  # rule_results[file][rulename] = list of ms
for f in FILES:
    results[f] = {'parse':[], 'visit':[], 'handlers':[], 'calls':[]}
    rule_results[f] = {}
    for i in range(RUNS):
        # Disable ASAN leak-fatal for long-running bench runs to avoid failing on known leaks
        env = dict(**subprocess.os.environ)
        env['ASAN_OPTIONS'] = 'detect_leaks=0'
        proc = subprocess.run([LEUKO, '--timings', f], capture_output=True, text=True, env=env)
        out = proc.stdout.strip().splitlines()
        # find last TIMING line
        timing_line = None
        for line in out[::-1]:
            m = TIMING_RE.match(line)
            if m:
                timing_line = m
                break
        if not timing_line:
            print('No TIMING line found in output for', f)
            print('\n'.join(out))
            sys.exit(1)
        results[f]['parse'].append(float(timing_line.group('parse')))
        results[f]['visit'].append(float(timing_line.group('visit')))
        results[f]['handlers'].append(float(timing_line.group('handlers')))
        results[f]['calls'].append(int(timing_line.group('calls')))

        # collect RULE lines that appear after TIMING in stdout
        seen_timing = False
        for line in out:
            if seen_timing:
                m = RULE_RE.match(line)
                if m:
                    name = m.group('name')
                    ms = float(m.group('ms'))
                    calls = int(m.group('calls'))
                    rule_results[f].setdefault(name, []).append((ms, calls))
            else:
                if TIMING_RE.match(line):
                    seen_timing = True

# Report medians
print('file,parse_ms_med,visit_ms_med,handlers_ms_med,handler_calls_med')
for f in FILES:
    p = statistics.median(results[f]['parse'])
    v = statistics.median(results[f]['visit'])
    h = statistics.median(results[f]['handlers'])
    c = int(statistics.median(results[f]['calls']))
    print(f'{f},{p:.3f},{v:.3f},{h:.3f},{c}')

# Compute scaling: per-1000-lines handlers_ms per file
print('\nDerived handlers_ms per 1000 lines (approx):')
for f in FILES:
    lines = int(f.split('_')[-1].split('.')[0])
    med = statistics.median(results[f]['handlers'])
    per_k = med / (lines / 1000.0)
    print(f'{f}: {per_k:.6f} ms per 1k lines')

# Per-rule medians per file
print('\nPer-rule medians (ms) per file:')
for f in FILES:
    print('\nFILE:', f)
    items = []
    for name, values in rule_results[f].items():
        ms_list = [v[0] for v in values]
        calls_list = [v[1] for v in values]
        med_ms = statistics.median(ms_list)
        med_calls = int(statistics.median(calls_list))
        items.append((med_ms, med_calls, name))
    items.sort(reverse=True)
    for med_ms, med_calls, name in items:
        print(f'{name},{med_ms:.3f},{med_calls}')
