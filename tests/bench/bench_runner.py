#!/usr/bin/env python3
import sys, subprocess, time, resource

if len(sys.argv) < 3:
    print("Usage: bench_runner.py <runs> <cmd...>", file=sys.stderr)
    sys.exit(2)

# Optional --quiet flag: if present as first arg after runs, silence child stdout/stderr
quiet = False
arg0 = sys.argv[1]
if arg0 == '--quiet':
    quiet = True
    runs = int(sys.argv[2])
    cmd = sys.argv[3:]
else:
    runs = int(arg0)
    cmd = sys.argv[2:]

for i in range(runs):
    start = time.time()
    if quiet:
        subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        subprocess.run(cmd)
    end = time.time()
    usage = resource.getrusage(resource.RUSAGE_CHILDREN)
    # ru_maxrss is in kilobytes on Linux
    print(f"{end-start:.3f} {usage.ru_maxrss}")
