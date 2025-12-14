#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>

#include "cli.h"

static char **split_comma_list(const char *s, size_t *out_count)
{
    if (!s || *s == '\0')
    {
        *out_count = 0;
        return NULL;
    }
    // Count commas
    size_t count = 1;
    for (const char *p = s; *p; ++p)
        if (*p == ',')
            ++count;
    char **arr = calloc(count, sizeof(char *));
    size_t idx = 0;
    const char *start = s;
    for (const char *p = s;; ++p)
    {
        if (*p == ',' || *p == '\0')
        {
            size_t len = p - start;
            arr[idx] = malloc(len + 1);
            memcpy(arr[idx], start, len);
            arr[idx][len] = '\0';
            ++idx;
            if (*p == '\0')
                break;
            start = p + 1;
        }
    }
    *out_count = idx;
    return arr;
}

int cli_parse(int argc, char *argv[], cli_options_t *opts)
{
    if (!opts)
        return -1;
    memset(opts, 0, sizeof(*opts));
    opts->format = strdup("text");

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {"format", required_argument, 0, 'f'},
        {"config", required_argument, 0, 'c'},
        {"only", required_argument, 0, 0},
        {"except", required_argument, 0, 0},
        {"auto-correct", no_argument, 0, 0},
        {0, 0, 0, 0}};

    while (1)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "hVf:c:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c)
        {
        case 'h':
            printf("Usage: %s [options] [files]\n", argv[0]);
            return 1;
        case 'V':
            printf("leuko version\n");
            return 1;
        case 'f':
            free(opts->format);
            opts->format = strdup(optarg);
            break;
        case 'c':
            opts->config_path = strdup(optarg);
            break;
        case 0:
            if (strcmp(long_options[option_index].name, "only") == 0)
            {
                opts->only = split_comma_list(optarg, &opts->only_count);
            }
            else if (strcmp(long_options[option_index].name, "except") == 0)
            {
                opts->except = split_comma_list(optarg, &opts->except_count);
            }
            else if (strcmp(long_options[option_index].name, "auto-correct") == 0)
            {
                opts->auto_correct = true;
            }
            break;
        case '?':
            return -1;
        default:
            break;
        }
    }

    // Remaining args are paths
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

void cli_options_free(cli_options_t *opts)
{
    if (!opts)
        return;
    for (size_t i = 0; i < opts->paths_count; ++i)
        free(opts->paths[i]);
    free(opts->paths);
    for (size_t i = 0; i < opts->only_count; ++i)
        free(opts->only[i]);
    free(opts->only);
    for (size_t i = 0; i < opts->except_count; ++i)
        free(opts->except[i]);
    free(opts->except);
    free(opts->config_path);
    free(opts->format);
}
