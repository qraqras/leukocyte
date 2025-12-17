#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>

#include "category.h"
#include "cli/cli.h"
#include "cli/formatter.h"
#include "version.h"
#include "rule.h"

/* forward declarations */
static int merge_string_lists(char ***destp, size_t *dest_countp, char **src, size_t src_count);
static char **split_comma_list(const char *str, size_t *out_count);
static void print_help(void);
static void print_version(void);

/**
 * @brief Parse command line arguments into cli_options_t.
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @param opts Pointer to cli_options_t struct to populate
 * @return 0 on success, 1 if help/version printed, -1 on error
 */
int parse_command_line(int argc, char *argv[], cli_options_t *opts)
{
    if (!opts)
    {
        return -1;
    }
    memset(opts, 0, sizeof(*opts));
    opts->formatter = CLI_FORMATTER_PROGRESS;

    static struct option long_options[] = {
        {"auto-correct", no_argument, 0, 'a'},
        {"auto-correct-all", no_argument, 0, 'A'},
        {"config", required_argument, 0, 'c'},
        {"except", required_argument, 0, 0},
        {"fix-layout", no_argument, 0, 'x'},
        {"format", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {"only", required_argument, 0, 0},
        {"version", no_argument, 0, 'v'},
        {"timings", no_argument, 0, 0},
        {0, 0, 0, 0}};

    for (;;)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "aAc:xf:hv", long_options, &option_index);
        if (c == -1)
        {
            break;
        }
        switch (c)
        {
        case 'a':
            opts->quick_fix_mode = QUICK_FIX_MODE_SAFE;
            break;
        case 'A':
            opts->quick_fix_mode = QUICK_FIX_MODE_UNSAFE;
            break;
        case 'x':
            opts->quick_fix_mode = QUICK_FIX_MODE_SAFE;
            // Treat `-x` as equivalent to `--only layout`.
            {
                const char *s = NULL;
                if (!rule_category_to_string(CATEGORY_LAYOUT, &s) || !s)
                    return -1;
                char *tok = strdup(s);
                if (!tok)
                    return -1;
                char **one = calloc(1, sizeof(char *));
                if (!one)
                {
                    free(tok);
                    return -1;
                }
                one[0] = tok;
                if (merge_string_lists(&opts->only, &opts->only_count, one, 1) != 0)
                    return -1;
            }
            break;
        case 'h':
            print_help();
            return 1;
        case 'v':
            print_version();
            return 1;
        case 'f':
            cli_formatter_from_string(optarg, &opts->formatter);
            break;
        case 'c':
            opts->config_path = strdup(optarg);
            break;
        case 0:
            if (strcmp(long_options[option_index].name, "except") == 0)
            {
                size_t tmp_count = 0;
                char **tmp = split_comma_list(optarg, &tmp_count);
                if (tmp == NULL && tmp_count == 0)
                {
                    if (tmp == NULL)
                        return -1;
                }
                if (merge_string_lists(&opts->except, &opts->except_count, tmp, tmp_count) != 0)
                    return -1;
            }
            if (strcmp(long_options[option_index].name, "only") == 0)
            {
                size_t tmp_count = 0;
                char **tmp = split_comma_list(optarg, &tmp_count);
                if (tmp == NULL && tmp_count == 0)
                {
                    if (tmp == NULL)
                        return -1;
                }
                if (merge_string_lists(&opts->only, &opts->only_count, tmp, tmp_count) != 0)
                    return -1;
            }
            if (strcmp(long_options[option_index].name, "timings") == 0)
            {
                opts->timings = true;
            }
            break;
        case '?':
            return -1;
        default:
            break;
        }
    }

    // Remaining args are paths, if any.
    if (optind < argc)
    {
        opts->paths_count = argc - optind;
        opts->paths = calloc(opts->paths_count, sizeof(char *));
        for (int i = 0; optind < argc; ++i, ++optind)
        {
            opts->paths[i] = strdup(argv[optind]);
        }
    }
    return 0;
}

/**
 * @brief Merge two lists of strings, avoiding duplicates.
 * @param destp Destination list pointer
 * @param dest_countp Pointer to destination list count
 * @param src Source list
 * @param src_count Source list count
 * @return 0 on success, -1 on failure
 */
static int merge_string_lists(char ***destp, size_t *dest_countp, char **src, size_t src_count)
{
    if (!src || src_count == 0)
    {
        return 0;
    }

    if (!*destp)
    {
        *destp = src;
        *dest_countp = src_count;
        return 0;
    }

    for (size_t i = 0; i < src_count; ++i)
    {
        char *s = src[i];
        int dup = 0;
        for (size_t j = 0; j < *dest_countp; ++j)
        {
            if (strcmp((*destp)[j], s) == 0)
            {
                dup = 1;
                break;
            }
        }
        if (dup)
        {
            free(s);
            continue;
        }
        char **tmp = realloc(*destp, (*dest_countp + 1) * sizeof(char *));
        if (!tmp)
        {
            // Free remaining src tokens.
            for (size_t k = i; k < src_count; ++k)
            {
                free(src[k]);
            }
            free(src);
            return -1;
        }
        *destp = tmp;
        (*destp)[(*dest_countp)++] = s;
    }
    free(src);
    return 0;
}

/**
 * @brief Split a comma-separated list into an array of strings.
 * @param str The comma-separated string.
 * @param out_count Output parameter to store the number of elements.
 * @return An array of strings. The caller is responsible for freeing the memory.
 */
static char **split_comma_list(const char *str, size_t *out_count)
{
    if (!str || *str == '\0')
    {
        *out_count = 0;
        return NULL;
    }
    // Count commas to determine number of elements.
    size_t count = 1;
    for (const char *p = str; *p; ++p)
    {
        if (*p == ',')
        {
            ++count;
        }
    }
    // Allocate array of string pointers.
    char **arr = calloc(count, sizeof(char *));
    if (!arr)
    {
        *out_count = 0;
        return NULL;
    }
    // Split the string.
    size_t idx = 0;
    const char *start = str;
    for (const char *p = str;; ++p)
    {
        if (*p == ',' || *p == '\0')
        {
            size_t len = p - start;
            char *tok = malloc(len + 1);
            if (!tok)
            {
                // Free previously allocated tokens.
                for (size_t j = 0; j < idx; ++j)
                {
                    free(arr[j]);
                }
                free(arr);
                *out_count = 0;
                return NULL;
            }
            memcpy(tok, start, len);
            tok[len] = '\0';
            arr[idx++] = tok;
            if (*p == '\0')
            {
                break;
            }
            start = p + 1;
        }
    }
    *out_count = idx;
    return arr;
}

/**
 * @brief  Print help message to stdout.
 */
static void print_help(void)
{
    printf("Usage: leuko [options] [file1, file2, ...]\n");
    printf("Options:\n");
    printf("  -a, --auto-correct          Automatically fix safe issues\n");
    printf("  -A, --auto-correct-all      Automatically fix all issues (including unsafe)\n");
    printf("  -c, --config <path>         Specify configuration file path\n");
    printf("      --except <rule1,rule2>  Exclude specific rules\n");
    printf("  -x, --fix-layout            Fix layout issues (safe only)\n");
    printf("  -f, --format <format>       Specify output format (text, json)\n");
    printf("  -h, --help                  Show this help message\n");
    printf("  -v, --version               Show version information\n");
    printf("      --timings               Print per-file phase timings (parse, build_rules, visit, handler)\n");
    printf("      --only <rule1,rule2>    Only include specific rules\n");
}

/**
 * @brief  Print version information to stdout.
 */
static void print_version(void)
{
    printf(LEUKO_VERSION);
}

/**
 * @brief Free memory allocated in cli_options_t.
 * @param opts Pointer to cli_options_t struct to free
 */
void cli_options_free(cli_options_t *opts)
{
    if (!opts)
    {
        return;
    }
    for (size_t i = 0; i < opts->paths_count; ++i)
    {
        free(opts->paths[i]);
    }
    free(opts->paths);
    for (size_t i = 0; i < opts->only_count; ++i)
    {
        free(opts->only[i]);
    }
    free(opts->only);
    for (size_t i = 0; i < opts->except_count; ++i)
    {
        free(opts->except[i]);
    }
    free(opts->except);
    free(opts->config_path);
}
