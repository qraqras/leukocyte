#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd $(dirname $0)/../..; pwd)
LEUKO=${ROOT}/build/leuko
BENCH_DIR=${ROOT}/tests/bench
OUT=${BENCH_DIR}/sweep_results.csv

if [ ! -x "$LEUKO" ]; then
  echo "Leuko binary not found at $LEUKO" >&2
  exit 2
fi

sizes=(5000 10000 100000)
chunks=(65536 262144 1048576)
smalls=(4096 8192 16384 32768)

echo "chunk,small,size,run,secs,rss_kb" > "$OUT"

for c in "${chunks[@]}"; do
  for s in "${smalls[@]}"; do
    export LEUKO_ARENA_CHUNK=$c
    export LEUKO_ARENA_SMALL_LIMIT=$s
    echo "==== chunk=$c small=$s ===="
    for size in "${sizes[@]}"; do
      f="$BENCH_DIR/bench_${size}.rb"
      # 3 runs, quiet
      echo "running size=$size"
      python3 "$BENCH_DIR/bench_runner.py" --quiet 3 "$LEUKO" "$f" | nl -ba -w1 -s, | while IFS=, read -r rn line; do
        # line like: <time> <rss>
        secs=$(echo "$line" | awk '{print $1}')
        rss=$(echo "$line" | awk '{print $2}')
        echo "$c,$s,$size,$rn,$secs,$rss" >> "$OUT"
      done
    done
  done
done

echo "Sweep done, results at $OUT"
