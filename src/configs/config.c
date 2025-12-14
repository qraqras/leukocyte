/* Config implementation: pattern list and glob matching, without Options.
 * 'Options' have been removed by project policy; typed rule config structs
 * are used for rule-specific settings.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <fnmatch.h>

#include "configs/config.h"
#include "configs/severity.h"

// Pattern list (dynamic array) structure
struct pattern_list_s
{
    char **patterns;
    size_t count;
    size_t capacity;
};

// Rule config structure (no Options)
struct rule_config_s
{
    char *rule_name;
    bool enabled;
    int severity;
    struct pattern_list_s *include;
    struct pattern_list_s *exclude;
    void *typed;
    void (*typed_free)(void *);
    struct rule_config_s *next;
};

// Config structure
struct config_s
{
    struct pattern_list_s *global_exclude;
    struct rule_config_s *rules;
};

// --- Rule config functions --------------------------------------------
rule_config_t *rule_config_create(const char *name)
{
    if (!name)
        return NULL;
    struct rule_config_s *r = calloc(1, sizeof(struct rule_config_s));
    if (!r)
        return NULL;
    r->rule_name = strdup(name);
    r->enabled = false;
    r->severity = SEVERITY_CONVENTION;
    r->include = NULL;
    r->exclude = NULL;
    return r;
}

void rule_config_set_enabled(rule_config_t *r, bool enabled)
{
    if (!r)
        return;
    r->enabled = enabled;
}

void rule_config_set_severity(rule_config_t *r, int severity)
{
    if (!r)
        return;
    r->severity = severity;
}

void rule_config_set_typed(rule_config_t *r, void *typed, void (*free_fn)(void *))
{
    if (!r)
        return;
    r->typed = typed;
    r->typed_free = free_fn;
}

// --- Pattern list helpers --------------------------------------------
pattern_list_t *pattern_list_add(pattern_list_t **head, const char *pattern)
{
    if (!head || !pattern)
        return NULL;
    struct pattern_list_s *pl = (struct pattern_list_s *)*head;
    if (!pl)
    {
        pl = calloc(1, sizeof(struct pattern_list_s));
        if (!pl)
            return NULL;
        pl->capacity = 4;
        pl->patterns = calloc(pl->capacity, sizeof(char *));
        if (!pl->patterns)
        {
            free(pl);
            return NULL;
        }
        pl->count = 0;
        *head = (pattern_list_t *)pl;
    }
    if (pl->count + 1 > pl->capacity)
    {
        size_t newcap = pl->capacity * 2;
        char **tmp = realloc(pl->patterns, newcap * sizeof(char *));
        if (!tmp)
            return (pattern_list_t *)pl;
        pl->patterns = tmp;
        pl->capacity = newcap;
    }
    pl->patterns[pl->count] = strdup(pattern);
    if (!pl->patterns[pl->count])
        return (pattern_list_t *)pl;
    pl->count++;
    return (pattern_list_t *)pl;
}

void pattern_list_free(pattern_list_t *list)
{
    struct pattern_list_s *p = (struct pattern_list_s *)list;
    if (!p)
        return;
    for (size_t i = 0; i < p->count; ++i)
    {
        free(p->patterns[i]);
    }
    free(p->patterns);
    free(p);
}

// --- Glob match --------------------------------------------------------
bool config_matches_path(pattern_list_t *patterns, const char *path)
{
    if (!patterns || !path)
        return false;
    struct pattern_list_s *pl = (struct pattern_list_s *)patterns;
    for (size_t i = 0; i < pl->count; ++i)
    {
        if (fnmatch(pl->patterns[i], path, 0) == 0)
            return true;
    }
    return false;
}

// --- Config lifecycle stubs -----------------------------------------
config_t *config_load_from_file(const char *path)
{
    (void)path;
    config_t *cfg = calloc(1, sizeof(config_t));
    return cfg;
}

void config_free(config_t *cfg)
{
    if (!cfg)
        return;
    if (cfg->global_exclude)
        pattern_list_free(cfg->global_exclude);
    struct rule_config_s *r = cfg->rules;
    while (r)
    {
        struct rule_config_s *rn = r->next;
        if (r->rule_name)
            free(r->rule_name);
        if (r->include)
            pattern_list_free(r->include);
        if (r->exclude)
            pattern_list_free(r->exclude);
        if (r->typed && r->typed_free)
            r->typed_free(r->typed);
        free(r);
        r = rn;
    }
    free(cfg);
}
