# leukocyte

- This project is RuboCop-compatible C99 reimplementation focused on Layout cops.

**Note (Migration):** The old flat rule registry API has been removed in favor of a
category-indexed rule registry. Use `include/common/rule_registry.h` and the
accessors `leuko_get_rule_categories()` / `leuko_get_rule_category_count()` and
`leuko_rule_find_index()` to access the rules. The legacy flat registry header `include/common/registry/registry.h` has been removed in favor of the category-indexed registry implemented in `include/common/rule_registry.h`. Use its accessors (`leuko_get_rule_categories`, `leuko_get_rule_category_count`, `leuko_rule_find_index`) instead.

**Note (Migration):** The flat fields `general_include`/`general_exclude` on `leuko_config_t` have been removed. Use `cfg->general->include` / `cfg->general->exclude` or the accessor `leuko_config_get_general_config()` (and use `leuko_config_new()` to allocate a config when needed).
