/*
 * Layout/IndentationConsistency
 * https://docs.rubocop.org/rubocop/cops_layout.html#layoutindentationconsistency
 */

#include <stdlib.h>
#include <string.h>

#include "configs/config.h"
#include "configs/config_file.h"
#include "configs/severity.h"
#include "configs/layout/indentation_consistency.h"
#include "rules/layout/indentation_consistency.h"

#include <stdio.h>
#include <yaml.h>

rule_config_t *rule_config_create_default_layout_indentation_consistency(void)
{
    rule_config_t *r = rule_config_create(RULE_NAME_LAYOUT_INDENTATION_CONSISTENCY);
    if (!r)
    {
        return NULL;
    }
    rule_config_set_enabled(r, true);
    rule_config_set_severity(r, SEVERITY_CONVENTION);

    indentation_consistency_t *typed = calloc(1, sizeof(indentation_consistency_t));
    if (typed)
    {
        typed->enforced_style = INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;
        rule_config_set_typed(r, typed, free);
    }
    return r;
}

// Map scalar to enforced_style enum
static indentation_consistency_enforced_style_t parse_enforced_style(const char *s)
{
    if (!s)
        return INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;
    if (strcmp(s, "indented_internal_methods") == 0)
        return INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS;
    // default
    return INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;
}

// Helper to read scalar value as C-string from event
static char *event_scalar_strdup(const yaml_event_t *e)
{
    if (!e || e->type != YAML_SCALAR_EVENT)
        return NULL;
    return strndup((const char *)e->data.scalar.value, e->data.scalar.length);
}

// Parse a YAML sequence of scalars into newly allocated array of strings
// `value_event` should be the YAML_SEQUENCE_START_EVENT already parsed by the caller.
static bool parse_sequence_of_strings(yaml_parser_t *parser, yaml_event_t *value_event, char ***out_array, size_t *out_count)
{
    if (!value_event || value_event->type != YAML_SEQUENCE_START_EVENT)
        return false;

    yaml_event_delete(value_event); // consume start

    size_t capacity = 4;
    size_t count = 0;
    char **arr = calloc(capacity, sizeof(char *));
    if (!arr)
        return false;

    yaml_event_t event;
    while (1)
    {
        if (!yaml_parser_parse(parser, &event))
            goto fail;
        if (event.type == YAML_SEQUENCE_END_EVENT)
        {
            yaml_event_delete(&event);
            break;
        }
        if (event.type == YAML_SCALAR_EVENT)
        {
            if (count >= capacity)
            {
                capacity *= 2;
                char **tmp = realloc(arr, capacity * sizeof(char *));
                if (!tmp)
                    goto fail;
                arr = tmp;
            }
            arr[count++] = event_scalar_strdup(&event);
        }
        yaml_event_delete(&event);
    }

    *out_array = arr;
    *out_count = count;
    return true;

fail:
    // free partial
    for (size_t i = 0; i < count; i++)
        free(arr[i]);
    free(arr);
    yaml_event_delete(&event);
    return false;
}

// Apply YAML mapping for Layout/IndentationConsistency
void apply_yaml_to_IndentationConsistency(const char *yaml_path, common_config_t *common, indentation_consistency_t **typed_out)
{
    if (!yaml_path || !common)
        return;


    FILE *f = fopen(yaml_path, "rb");
    if (!f)
        return;

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser))
    {
        fclose(f);
        return;
    }
    yaml_parser_set_input_file(&parser, f);

    yaml_event_t event;
    bool done = false;
    while (!done && yaml_parser_parse(&parser, &event))
    {
        if (event.type == YAML_SCALAR_EVENT)
        {
            char *key = event_scalar_strdup(&event);
            if (key && strcmp(key, RULE_NAME_LAYOUT_INDENTATION_CONSISTENCY) == 0)
            {
                // next event should be mapping start
                yaml_event_delete(&event);
                if (!yaml_parser_parse(&parser, &event))
                {
                    free(key);
                    break;
                }
                if (event.type != YAML_MAPPING_START_EVENT)
                {
                    free(key);
                    yaml_event_delete(&event);
                    break;
                }
                yaml_event_delete(&event);

                // parse mapping for this rule
                while (yaml_parser_parse(&parser, &event))
                {
                    if (event.type == YAML_MAPPING_END_EVENT)
                    {
                        yaml_event_delete(&event);
                        break;
                    }
                    if (event.type != YAML_SCALAR_EVENT)
                    {
                        yaml_event_delete(&event);
                        continue;
                    }
                    char *map_key = event_scalar_strdup(&event);
                    yaml_event_delete(&event);
                    if (!yaml_parser_parse(&parser, &event))
                    {
                        free(map_key);
                        break;
                    }

                    // Enabled: boolean scalar
                    if (strcmp(map_key, "Enabled") == 0)
                    {
                        if (event.type == YAML_SCALAR_EVENT)
                        {
                            char *v = event_scalar_strdup(&event);
                            if (v)
                            {
                                if (strcmp(v, "true") == 0 || strcmp(v, "yes") == 0)
                                    common->enabled = true;
                                else if (strcmp(v, "false") == 0 || strcmp(v, "no") == 0)
                                    common->enabled = false;
                                free(v);
                            }
                        }
                        yaml_event_delete(&event);
                    }
                    else if (strcmp(map_key, "Severity") == 0)
                    {
                        if (event.type == YAML_SCALAR_EVENT)
                        {
                            char *v = event_scalar_strdup(&event);
                            if (v)
                            {
                                // map a few known values
                                if (strcmp(v, "convention") == 0)
                                    common->severity = SEVERITY_CONVENTION;
                                else if (strcmp(v, "warning") == 0)
                                    common->severity = SEVERITY_WARNING;
                                else if (strcmp(v, "error") == 0)
                                    common->severity = SEVERITY_ERROR;
                                else
                                    common->severity = atoi(v);
                                free(v);
                            }
                        }
                        yaml_event_delete(&event);
                    }
                    else if (strcmp(map_key, "EnforcedStyle") == 0)
                    {
                        if (event.type == YAML_SCALAR_EVENT)
                        {
                            char *v = event_scalar_strdup(&event);
                            if (v)
                            {
                                if (!*typed_out)
                                {
                                    *typed_out = calloc(1, sizeof(indentation_consistency_t));
                                    if (!*typed_out)
                                    {
                                        free(v);
                                        free(map_key);
                                        yaml_event_delete(&event);
                                        break;
                                    }
                                }
                                (*typed_out)->enforced_style = parse_enforced_style(v);
                                free(v);
                            }
                        }
                        yaml_event_delete(&event);
                    }
                    else if (strcmp(map_key, "Include") == 0)
                    {
                        if (!parse_sequence_of_strings(&parser, &event, (char ***)&common->include, &common->include_count))
                        {
                            // skip
                            yaml_event_delete(&event);
                        }
                    }
                    else if (strcmp(map_key, "Exclude") == 0)
                    {
                        if (!parse_sequence_of_strings(&parser, &event, (char ***)&common->exclude, &common->exclude_count))
                        {
                            // skip
                            yaml_event_delete(&event);
                        }
                    }
                    else
                    {
                        // Unknown key: skip value
                        yaml_event_delete(&event);
                    }

                    free(map_key);
                }
                free(key);
                // done searching for this rule - there could be others but we only apply this one
                break;
            }
            free(key);
        }
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(f);
}
