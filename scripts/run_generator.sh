#!/usr/bin/env bash
set -euo pipefail
SCHEMAS_DIR=${1:-scripts/rule_schemas}
OUTDIR=${2:-generated}
OUTDIR_CONFIG="$OUTDIR/configs"
mkdir -p "$OUTDIR_CONFIG"
# Build generator (PoC compile locally)
gcc -std=c99 -Ivendor/cjson -o tools/gen_rule_struct tools/gen_rule_struct.c vendor/cjson/cJSON.c
./tools/gen_rule_struct "$SCHEMAS_DIR" "$OUTDIR_CONFIG"
