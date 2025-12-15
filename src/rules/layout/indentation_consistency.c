/*
 * Layout/IndentationConsistency
 * https://docs.rubocop.org/rubocop/cops_layout.html#layoutindentationconsistency
 */

#include "rules/rule.h"
#include <string.h>
#include <stdlib.h>

// Forward declaration of handler
bool check_def_indentation(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg);

// Layout/IndentationConsistency rule definition.
#include "rules/layout/indentation_consistency.h"
#include "configs/config.h"
#include <yaml.h>

rule_t layout_indentation_consistency_rule = {
    .name = RULE_NAME_LAYOUT_INDENTATION_CONSISTENCY,
    .category = RULE_CATEGORY_LAYOUT,
    .enabled = true,
    .handlers = {[PM_DEF_NODE] = check_def_indentation}};

rule_config_t *layout_indentation_consistency_create_default(void)
{
    rule_config_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;
    cfg->rule_name = strdup(RULE_NAME_LAYOUT_INDENTATION_CONSISTENCY);
    cfg->enabled = true;
    cfg->severity_level = SEVERITY_WARNING;
    cfg->include = NULL;
    cfg->include_count = 0;
    cfg->exclude = NULL;
    cfg->exclude_count = 0;
    cfg->specific_config = NULL;
    cfg->specific_config_free = NULL;
    return cfg;
}

bool layout_indentation_consistency_apply_yaml(rule_config_t *config, const yaml_event_t *event, pm_parser_t *parser)
{
    /* Not implemented yet — return false to indicate event not handled */
    (void)config; (void)event; (void)parser;
    return false;
}

// Helper to get line and column from position
void get_line_column(const pm_parser_t *parser, const uint8_t *pos, size_t *line, size_t *column)
{
    pm_line_column_t lc = pm_newline_list_line_column(&parser->newline_list, pos, 1);
    *line = lc.line;
    *column = lc.column;
}

// Handler for DefNode: check indentation of body statements
bool check_def_indentation(pm_node_t *node, pm_parser_t *parser, pm_list_t *diagnostics, config_t *cfg)
{
    pm_def_node_t *def_node = (pm_def_node_t *)node;

    if (def_node->body && def_node->body->type == PM_STATEMENTS_NODE)
    {
        pm_statements_node_t *statements = (pm_statements_node_t *)def_node->body;
        for (size_t i = 0; i < statements->body.size; i++)
        {
            pm_node_t *stmt = statements->body.nodes[i];
            pm_location_t stmt_loc = stmt->location;
            size_t line, column;
            get_line_column(parser, stmt_loc.start, &line, &column);
            if (column != 2)
            {
                // Add diagnostic manually
                pm_diagnostic_t *diagnostic = (pm_diagnostic_t *)calloc(1, sizeof(pm_diagnostic_t));
                if (diagnostic)
                {
                    // Ensure node.next is NULL to prevent traversing uninitialized memory
                    diagnostic->node.next = NULL;
                    diagnostic->location.start = stmt_loc.start;
                    diagnostic->location.end = stmt_loc.end;
                    diagnostic->diag_id = PM_ERR_CANNOT_PARSE_EXPRESSION; // dummy
                    diagnostic->message = strdup("Incorrect indentation");
                    diagnostic->owned = true;
                    diagnostic->level = PM_ERROR_LEVEL_SYNTAX;
                    pm_list_append(diagnostics, (pm_list_node_t *)diagnostic);
                }
            }
        }
    }

    return true;
}
