# TODO / Design Notes

## Config discovery & merging (RuboCop-compatible)

### Goal
- Provide RuboCop-compatible configuration file discovery for files and directories.
- Implementation must support discovery, `inherit_from` resolution, merging rules, caching, and tests; errors should follow Leuko policy (single short message on fatal parse/resolve errors).

### Discovery algorithm
- Input: target file path OR working directory.
- Start directory: `dirname(file)` when file provided, else working directory.
- Candidate filenames (in order): `.rubocop.yml`, `rubocop.yml`.
- Walk upward from start directory to filesystem root collecting candidate config files. Stop upward walk if any discovered config has `root: true`.
- If CLI `--config <path>` is provided, skip upward discovery and use that file as the sole discovery entry (still resolve its `inherit_from`).

### inherit_from
- Support `inherit_from` as a string or array of strings.
- Resolve relative paths relative to the config file's directory.
- Recursively resolve inherited files (chain). Detect cycles and treat as fatal (single short error message).
- Missing referenced files may be treated as an error (short message) or configurable ignore; default: report short error.

### Merge rules
- Priority: near file (lower/closer) overrides far (higher). Merge order is: AllCops (global) -> Category -> Cop (closest wins).
- Scalars (Severity, Enabled, EnforcedStyle, etc.): last-write-wins (closest config overrides).
- Maps: recursive merge (keys merged with the same rules per key).
- Arrays (Include, Exclude): concatenated in discovery order (higher -> lower), preserving order.
- Type mismatches: ignore the invalid value and fall back to previous/default value; do not emit many diagnostics (follow RuboCop behavior and project policy).

### Behavior for directory input
- When a directory is passed on CLI: recursively enumerate candidate files (Ruby file patterns).
- For each file:
  - Determine the merged config by discovery + merge (as above).
  - Evaluate `Include`/`Exclude` from the merged config to quickly skip files not to be analyzed.
  - **Collect the applicable rules for that file** by applying the merged config to the rule registry (i.e., build a per-file `rules_by_type` using `build_rules_by_type_for_file` or equivalent). Because Include/Exclude, Enabled, and other settings vary by file, rules must be selected per-file rather than globally per-run.
- Optimization and parallelism:
  - Collect unique merged configs and parse/resolve `inherit_from` in parallel (worker pool). Cache merged configs per-directory (mtime-based) to avoid redundant work.
  - After merged configs are ready, compute per-file applicable rule sets (cache results keyed by config+file path or directory) and reuse where possible.
  - Process Ruby files in a second parallel stage: parse with Prism, initialize `leuko_processed_source_t`, then apply the precomputed `rules_by_type` to the parsed nodes. Use bounded worker pools to control CPU and memory usage.
  - Ensure all caches and shared data structures are thread-safe (locks or atomic swap on initialization) and that output ordering is stable (e.g., sort by path before final reporting) to avoid nondeterminism.

### Caching & performance
- Cache merged configuration per-directory path (key = directory path or `--config` path) with mtime validation.
- In LSP/daemon mode, support optional FS-watcher invalidation for immediate updates.
- Keep reads/merges idempotent and memoized to reduce I/O in large projects.

### APIs (proposal)
- `int leuko_config_discover_for_file(const char *file_path, const char *cli_config_path, leuko_raw_config_t **out_raw, char **err)`
  - Discover and return merged raw config (parsed YAML AST equivalent). Returns 0 on success.
- `int leuko_config_load_file(const char *path, leuko_raw_config_t **out, char **err)`
- `int leuko_config_resolve_inherits(leuko_raw_config_t *cfg, const char *base_dir, char **err)`
- `int leuko_config_merge(const leuko_raw_config_t *a, const leuko_raw_config_t *b, leuko_raw_config_t **out)`
- `int leuko_config_to_runtime(config_t *out, const leuko_raw_config_t *raw, char **err)`
- `void leuko_config_clear_cache(void);`

### Error handling
- Fatal parse / resolution errors: return a short single-line error message (no diagnostic flood). Follow project rule: "YAML read errors do not emit diagnostics; produce a single error message".
- Non-fatal invalid entries: ignore and fall back to existing/default values.

### Tests (must be automated)
- Discovery: nested directories with `.rubocop.yml` at different levels, verify merged result for files in each directory.
- `inherit_from`: relative/absolute resolution, chain resolution, cycle detection.
- Merge semantics: scalars, maps, arrays (Include/Exclude concatenation and order verification).
- `--config` precedence: ensure it overrides discovery.
- Directory input behavior: early-exclude works and skipped files are not analyzed.
- **Per-file rule collection**: verify `build_rules_by_type_for_file` (or equivalent) yields different rule sets for files that are included/excluded differently by merged configs; test that enabled/disabled rules and Include/Exclude affect per-file rule sets correctly.
- Parallel pipeline: YAML parsing stage and per-file rule set computation stage can be executed in parallel without races; tests should run the pipeline with a synthetic repo to assert deterministic results.
- Cache invalidation: mtime change triggers reload.

### Implementation plan (sprint)
1. Create `leuko_raw_config_t` + YAML loader wrapper and tests for basic parse.
2. Implement `leuko_config_load_file` and `leuko_config_resolve_inherits` with tests.
3. Implement discovery (`leuko_config_discover_for_file`) with `root: true` semantics and tests.
4. Implement merging rules and conversion to `config_t` used by existing loader; add tests.
5. Add directory input integration: enumeration, early exclude, and cache.
6. Implement **per-file rule collection**: compute `rules_by_type` per file using merged config and integrate with `get_rules_by_type_for_file` / caching. Add unit tests verifying per-file differences (Include/Exclude/Enabled).
7. Implement parallel pipeline: YAML parse workers, per-file rule-set computation, and Ruby file processing workers; add concurrency and determinism tests.
8. Add end-to-end compatibility tests against a small RuboCop sample repo and iterate on differences.
---

*If you want, I can open a feature branch and start implementing steps 1–3 and create unit tests; tell me to proceed.*


## Implementation tasks (detailed, actionable)

To make progress reviewable and easy to land in small PRs, split each sprint step into small tasks. The following tasks are proposals for the first wave of work (each bullet is intended to be a single small PR):

- Step 1 (YAML parse + raw type)
  - Add header `include/configs/raw_config.h` with `leuko_raw_config_t` declaration and simple API stubs.
  - Implement `src/configs/raw_config.c: leuko_config_load_file()` that wraps YAML parsing and returns short error messages on parse failure.
  - Add unit tests under `tests/configs/test_raw_config.c` for valid/invalid YAML and edge cases.
  - Add CI unit job for these tests.

- Step 2 (`inherit_from` resolution)
  - Implement `leuko_config_resolve_inherits()` in `src/configs/raw_config.c`: resolve relative paths, chain, detect cycles.
  - Add tests for: relative/absolute resolution, chain resolution, cycle detection, missing files handling.
  - Add unit tests for error messages matching project policy.

- Step 3 (discovery)
  - Implement `leuko_config_discover_for_file()` that walks upward, collects candidates, and respects `root: true`.
  - Tests: hierarchy discovery, `--config` override semantics, `root: true` stops upward walk.

- Step 4 (merge semantics)
  - Implement `leuko_config_merge()` with scalar overwrite, map recursion, array concat semantics.
  - Add comprehensive unit tests (AllCops → Category → Cop; Include/Exclude order; type mismatch fallback).

- Step 5 (directory integration)
  - Implement directory enumeration helper (Ruby file matching rules), add CLI integration shims to pass directories to workflow.
  - Implement early Include/Exclude evaluation to skip files early.
  - Add tests and a small synthetic repo for integration testing.

- Step 6 (per-file rule collection & caching)
  - Reuse `build_rules_by_type_for_file` (or factor shared code) to build per-file `rules_by_type` from the merged config.
  - Add a per-file rules cache keyed by (config_id, directory/file path); add tests verifying Include/Exclude impacts rule sets.

- Step 7 (pipeline + parallelization)
  - Implement a single-process serial pipeline first (discovery → raw parse → merge → per-file rules → analyze file) and add E2E tests.
  - Add worker pool abstraction and implement YAML worker pool for parsing and a file-analysis worker pool. Make caches thread-safe.
  - Add deterministic output ordering (e.g., collect results and sort by path before printing) to ensure stable tests.
  - Add concurrency tests (small synthetic repo with many files) and measure speedups.

- Step 8 (compatibility & CI)
  - Add an end-to-end compatibility test that runs RuboCop and Leuko on a small sample repo and produces a diff report.
  - Add a CI workflow to run compatibility tests on pull requests (optionally gated/expensive job).
  - Add benchmarking job to CI matrix (optional, can be run manually on demand for PRs that affect performance).

- Misc / infra
  - Create feature branch naming convention: `feature/config-discovery/*` and PR checklist.
  - Add documentation to `docs/CONFIG_DISCOVERY.md` summarizing the rules and limitations.
  - Add README section explaining how to run the compatibility and benchmark jobs locally.


---

If you want, I can open the feature branch `feature/config-discovery` and start with **Step 1** and the corresponding unit tests and CI job. Reply "start step1" to proceed or ask which step you prefer first.
