/* Config implementation: pattern list and glob matching, without Options.
 * 'Options' have been removed by project policy; typed rule config structs
 * are used for rule-specific settings.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <fnmatch.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

// Rule config structure.
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
