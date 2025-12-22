# Changelog

## Unreleased

### Changed

- Refactor: Public config API parameter order unified to be `cfg` first. Deprecated docs-first wrappers removed.
  - New functions: `leuko_config_apply_file`, `leuko_config_apply_docs`, `config_apply_file`, `config_apply_docs`.
  - Rationale: clearer API and consistent naming across modules.

- Refactor: Config discovery and conversion modules reorganized:
  - `src/configs/discovery/` now contains discovery and caching logic.
  - `src/configs/conversion/` (was loader) now contains YAML -> `leuko_config_t` conversion logic.

- Feature: Warm cache and read-only lookup semantics for worker threads.
  - `leuko_config_warm_cache_for_files` added for preloading configs.
  - `leuko_config_get_cached_config_for_file_ro` provides read-only lookups for worker threads.

- Refactor: YAML helpers standardized to multi-document out-param style and single-doc helpers removed.

- Docs: Added migration notes and guidance for updating callsites.

### Removed

- Removed legacy header `include/configs/generated_config.h` and deprecated docs-first wrapper APIs.

### Notes

- Tests and build updated to reflect API renames and include path changes.
- Please update downstream code to call the new cfg-first APIs.
