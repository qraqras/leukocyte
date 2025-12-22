#!/usr/bin/env python3
import sys, subprocess, time, os, math

if len(sys.argv) < 2:
    print("Usage: run_bench_parallel.py <runs> [workers...]", file=sys.stderr)
    sys.exit(2)

runs = int(sys.argv[1])
ROOT = os.getcwd()
BUILD = os.path.join(ROOT, 'build')
BENCH_DIR = os.path.join(ROOT, 'tests', 'bench')
LEUKO = os.path.join(BUILD, 'leuko')
if not os.path.exists(LEUKO) or not os.access(LEUKO, os.X_OK):
    print(f"Leuko binary not found at {LEUKO}", file=sys.stderr)
    sys.exit(2)

sizes = [2000, 5000, 10000, 100000, 200000]

# generate file if missing
def generate_file(lines, out):
    if os.path.exists(out):
        return
    print(f"Generating {out} ({lines} lines)")
    with open(out, 'w') as f:
        f.write("# frozen_string_literal: true\n")
        f.write("class Bench\n")
        methods = (lines - 2) // 5
        for i in range(1, methods + 1):
            f.write(f"  def m{i}\n")
            f.write("    a = 1\n")
            f.write("    b = 2\n")
            f.write("    a + b\n")
            f.write("  end\n")
        f.write("end\n")

bench_runner = os.path.join(BENCH_DIR, 'bench_runner.py')
if not os.path.exists(bench_runner):
    print("bench_runner.py not found", file=sys.stderr)
    sys.exit(2)

# number of files per size when generating multi-file directories (override via env BENCH_FILES_PER_SIZE)
FILES_PER_SIZE = int(os.getenv('BENCH_FILES_PER_SIZE', '10'))

def generate_many_files(lines, out_dir, count):
    os.makedirs(out_dir, exist_ok=True)
    methods = (lines - 2) // 5
    for i in range(count):
        out = os.path.join(out_dir, f'bench_{lines}_{i}.rb')
        if os.path.exists(out):
            continue
        with open(out, 'w') as f:
            f.write("# frozen_string_literal: true\n")
            f.write("class Bench\n")
            for j in range(1, methods + 1):
                f.write(f"  def m{j}\n")
                f.write("    a = 1\n")
                f.write("    b = 2\n")
                f.write("    a + b\n")
                f.write("  end\n")
            f.write("end\n")

results = {}
for s in sizes:
    f = os.path.join(BENCH_DIR, f'bench_{s}.rb')
    generate_file(s, f)
    print(f"\n====== SIZE: {s} lines ({f}) ======")

    # baseline (no parallel)
    print("Baseline (sequential) runs:")
    p = subprocess.Popen([bench_runner, '--quiet', str(runs), LEUKO, f], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    out, err = p.communicate()
    if p.returncode != 0:
        print("Command failed: baseline", file=sys.stderr)
        print(err, file=sys.stderr)
        sys.exit(2)
    times = []
    rss = []
    for line in out.strip().splitlines():
        if not line.strip():
            continue
        parts = line.strip().split()
        if len(parts) >= 2:
            t = float(parts[0]); r = int(parts[1]);
            times.append(t); rss.append(r)
    times.sort(); rss_max = max(rss) if rss else 0
    median = times[len(times)//2]; idx90 = int(math.ceil(0.9*len(times))) - 1; p90 = times[idx90]
    print(f"sequential median: {median:.3f}s p90: {p90:.3f}s rss_max: {rss_max} KB")
    results[(s, 'sequential')] = {'median': median, 'p90': p90, 'rss_max': rss_max}

    # Leuko parallel (auto workers)
    print("Leuko --parallel (auto) runs:")
    p = subprocess.Popen([bench_runner, '--quiet', str(runs), LEUKO, '--parallel', f], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    out, err = p.communicate()
    if p.returncode != 0:
        print("Command failed: leuko parallel", file=sys.stderr)
        print(err, file=sys.stderr)
        sys.exit(2)
    times = []; rss = []
    for line in out.strip().splitlines():
        if not line.strip():
            continue
        parts = line.strip().split()
        if len(parts) >= 2:
            t = float(parts[0]); r = int(parts[1]);
            times.append(t); rss.append(r)
    times.sort(); rss_max = max(rss) if rss else 0
    median = times[len(times)//2]; idx90 = int(math.ceil(0.9*len(times))) - 1; p90 = times[idx90]
    print(f"leuko median: {median:.3f} s, p90: {p90:.3f} s, ru_maxrss_max: {rss_max} KB")
    results[(s, 'leuko_parallel_auto')] = {'times': times, 'median': median, 'p90': p90, 'ru_maxrss_max': rss_max}

    # RuboCop parallel (auto)
    print("Rubocop --parallel runs:")
    # remove cache to ensure cache-off benchmarking
    cache_dir = os.path.join(os.getcwd(), '.rubocop_cache')
    if os.path.exists(cache_dir):
        import shutil
        shutil.rmtree(cache_dir)
    rcmd = ['rubocop', '--cache', 'false', '--parallel', '--only', 'Layout/IndentationConsistency', '--format', 'quiet', f]
    p = subprocess.Popen([bench_runner, '--quiet', str(runs)] + rcmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    out, err = p.communicate()
    if p.returncode != 0:
        print("Command failed: rubocop parallel", file=sys.stderr)
        print(err, file=sys.stderr)
        sys.exit(2)
    times = []; rss = []
    for line in out.strip().splitlines():
        if not line.strip():
            continue
        parts = line.strip().split()
        if len(parts) >= 2:
            t = float(parts[0]); r = int(parts[1]);
            times.append(t); rss.append(r)
    times.sort(); rss_max = max(rss) if rss else 0
    median = times[len(times)//2]; idx90 = int(math.ceil(0.9*len(times))) - 1; p90 = times[idx90]
    print(f"rubocop median: {median:.3f} s, p90: {p90:.3f} s, ru_maxrss_max: {rss_max} KB")
    results[(s, 'rubocop_parallel_auto')] = {'times': times, 'median': median, 'p90': p90, 'ru_maxrss_max': rss_max}

    # Multi-file directory: generate and run on directory containing multiple bench files
    multi_dir = os.path.join(BENCH_DIR, f'multi_{s}')
    print(f"Generating multi-file directory {multi_dir} with {FILES_PER_SIZE} files...")
    generate_many_files(s, multi_dir, FILES_PER_SIZE)

    # Leuko on directory
    print("Leuko --parallel on directory runs:")
    p = subprocess.Popen([bench_runner, '--quiet', str(runs), LEUKO, '--parallel', multi_dir], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    out, err = p.communicate()
    if p.returncode != 0:
        print("Command failed: leuko parallel on directory", file=sys.stderr)
        print(err, file=sys.stderr)
        sys.exit(2)
    times = []; rss = []
    for line in out.strip().splitlines():
        if not line.strip():
            continue
        parts = line.strip().split()
        if len(parts) >= 2:
            t = float(parts[0]); r = int(parts[1]);
            times.append(t); rss.append(r)
    times.sort(); rss_max = max(rss) if rss else 0
    median = times[len(times)//2]; idx90 = int(math.ceil(0.9*len(times))) - 1; p90 = times[idx90]
    print(f"leuko (dir) median: {median:.3f} s, p90: {p90:.3f} s, ru_maxrss_max: {rss_max} KB")
    results[(s, 'leuko_parallel_dir')] = {'times': times, 'median': median, 'p90': p90, 'ru_maxrss_max': rss_max}

    # RuboCop on directory (cache off)
    print("Rubocop --parallel on directory runs:")
    cache_dir = os.path.join(os.getcwd(), '.rubocop_cache')
    if os.path.exists(cache_dir):
        import shutil
        shutil.rmtree(cache_dir)
    rcmd = ['rubocop', '--cache', 'false', '--parallel', '--only', 'Layout/IndentationConsistency', '--format', 'quiet', multi_dir]
    p = subprocess.Popen([bench_runner, '--quiet', str(runs)] + rcmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    out, err = p.communicate()
    if p.returncode != 0:
        print("Command failed: rubocop parallel on directory", file=sys.stderr)
        print(err, file=sys.stderr)
        sys.exit(2)
    times = []; rss = []
    for line in out.strip().splitlines():
        if not line.strip():
            continue
        parts = line.strip().split()
        if len(parts) >= 2:
            t = float(parts[0]); r = int(parts[1]);
            times.append(t); rss.append(r)
    times.sort(); rss_max = max(rss) if rss else 0
    median = times[len(times)//2]; idx90 = int(math.ceil(0.9*len(times))) - 1; p90 = times[idx90]
    print(f"rubocop (dir) median: {median:.3f} s, p90: {p90:.3f} s, ru_maxrss_max: {rss_max} KB")
    results[(s, 'rubocop_parallel_dir')] = {'times': times, 'median': median, 'p90': p90, 'ru_maxrss_max': rss_max}

print("\nDone.")
