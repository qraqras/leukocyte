#!/usr/bin/env bash
set -euo pipefail

ROOT=$(pwd)
BUILD=${ROOT}/build
BENCH_DIR=${ROOT}/tests/bench
mkdir -p "$BENCH_DIR"

sizes=(2000 5000 10000 100000)

# Generate Ruby file with approx 'lines' lines. Each method is 5 lines.
generate_file() {
  local lines=$1
  local out=$2
  if [ -f "$out" ]; then
    echo "File exists: $out"
    return
  fi
  echo "Generating $out ($lines lines)"
  echo "# frozen_string_literal: true" > "$out"
  echo "class Bench" >> "$out"
  methods=$(( (lines - 2) / 5 ))
  for i in $(seq 1 $methods); do
    cat >> "$out" <<'RUBY'
  def mPLACEHOLDER
    a = 1
    b = 2
    a + b
  end
RUBY
  done
  sed -i "s/PLACEHOLDER/$i/" "$out" || true
  echo "end" >> "$out"
  actual=$(wc -l < "$out")
  echo "Wrote $actual lines to $out"
}

# Run a command n times and print timings (seconds, max RSS) per run
run_times() {
  local runs=$1
  shift
  local cmd=("$@")
  python3 tests/bench/bench_runner.py "$runs" "${cmd[@]}"
}


# Ensure rubocop exists
if ! command -v rubocop >/dev/null 2>&1; then
  echo "rubocop not found" >&2
  exit 2
fi

# Build toolbox for leuko path
LEUKO=${BUILD}/leuko
if [ ! -x "$LEUKO" ]; then
  echo "Leuko binary not found at $LEUKO" >&2
  exit 2
fi

# Create files
for s in "${sizes[@]}"; do
  out="$BENCH_DIR/bench_${s}.rb"
  generate_file "$s" "$out"
done

# Run benchmarks
for s in "${sizes[@]}"; do
  f="$BENCH_DIR/bench_${s}.rb"
  echo "\n====== SIZE: $s lines ($f) ======"
  echo "Leuko runs (3):"
  run_times --quiet 3 "$LEUKO" "$f"

  echo "Rubocop runs (3):"
  # Use --only for the cop; suppress output and RuboCop's banners
  run_times --quiet 3 rubocop --only Layout/IndentationConsistency --format quiet "$f" 2>/dev/null
done

echo "Done."
