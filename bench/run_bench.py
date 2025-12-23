#!/usr/bin/env python3
import sys, subprocess, time, os, math

if len(sys.argv) < 2:
    print("Usage: run_bench_custom.py <runs>", file=sys.stderr)
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

# generate file if missing (simple version of run_bench.sh generate_file)
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

results = {}
for s in sizes:
    f = os.path.join(BENCH_DIR, f'bench_{s}.rb')
    generate_file(s, f)
    print(f"\n====== SIZE: {s} lines ({f}) ======")
    for name, cmd in [("Leuko", [LEUKO, f]), ("Rubocop", ["rubocop", "--cache", "false", "--only", "Layout/IndentationConsistency", "--format", "quiet", f])]:
        # Remove RuboCop cache to ensure cache-off benchmarking
        if name == "Rubocop":
            cache_dir = os.path.join(os.getcwd(), '.rubocop_cache')
            if os.path.exists(cache_dir):
                print(f"Removing existing RuboCop cache at {cache_dir}")
                import shutil
                shutil.rmtree(cache_dir)
        print(f"{name} runs ({runs}):")
        # invoke bench_runner.py to run the command runs times quietly
        p = subprocess.Popen([bench_runner, '--quiet', str(runs)] + cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        out, err = p.communicate()
        if p.returncode != 0:
            print(f"Command failed: {' '.join(cmd)}", file=sys.stderr)
            print(err, file=sys.stderr)
            sys.exit(2)
        times = []
        rss = []
        for line in out.strip().splitlines():
            if not line.strip():
                continue
            parts = line.strip().split()
            if len(parts) >= 2:
                t = float(parts[0])
                r = int(parts[1])
                times.append(t)
                rss.append(r)
        times.sort()
        rss_max = max(rss) if rss else 0
        # median
        median = times[len(times)//2]
        # p90: use ceil(0.9*n)-1 index
        idx90 = int(math.ceil(0.9*len(times))) - 1
        p90 = times[idx90]
        print(f"times: {times}")
        print(f"median: {median:.3f} s, p90: {p90:.3f} s, ru_maxrss_max: {rss_max} KB")
        results[(s, name)] = {'times': times, 'median': median, 'p90': p90, 'ru_maxrss_max': rss_max}

print("\nDone.")
