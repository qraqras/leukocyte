# leukocyte

- This project is RuboCop-compatible C99 reimplementation focused on Layout cops.

**Note (Migration):** The old flat rule registry API has been removed in favor of a
category-indexed generated registry. Use `include/common/generated_rules.h` and the
accessors `leuko_get_rule_categories()` / `leuko_get_rule_category_count()` and
`leuko_rule_find_index()` to access the rules. The legacy flat registry header
`include/common/registry/registry.h` is deprecated and will be removed in a
future major release.
