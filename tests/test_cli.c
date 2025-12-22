// Tests for CLI comma-splitting via parse_command_line
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cli/cli.h"
#include "worker/jobs.h"

static int test_only_basic(void)
{
    char *argv[] = {"prog", "--only", "a,b,c", NULL};
    cli_options_t opts;
    optind = 1;
    int rc = parse_command_line(3, argv, &opts);
    if (rc != 0)
    {
        fprintf(stderr, "parse_command_line rc=%d\n", rc);
        return 1;
    }
    int rv = 0;
    if (opts.only_count != 3)
    {
        fprintf(stderr, "unexpected only_count=%zu\n", opts.only_count);
        rv = 1;
    }
    else if (strcmp(opts.only[0], "a") != 0 || strcmp(opts.only[1], "b") != 0 || strcmp(opts.only[2], "c") != 0)
    {
        fprintf(stderr, "unexpected tokens: %s,%s,%s\n", opts.only[0], opts.only[1], opts.only[2]);
        rv = 1;
    }
    cli_options_free(&opts);
    return rv;
}

static int test_except_leading_empty(void)
{
    char *argv[] = {"prog", "--except", ",a", NULL};
    cli_options_t opts;
    optind = 1;
    int rc = parse_command_line(3, argv, &opts);
    if (rc != 0)
    {
        fprintf(stderr, "parse_command_line rc=%d\n", rc);
        return 1;
    }
    int rv = 0;
    if (opts.except_count != 2)
    {
        fprintf(stderr, "unexpected except_count=%zu\n", opts.except_count);
        rv = 1;
    }
    else if (strcmp(opts.except[0], "") != 0 || strcmp(opts.except[1], "a") != 0)
    {
        fprintf(stderr, "unexpected tokens: '%s','%s'\n", opts.except[0], opts.except[1]);
        rv = 1;
    }
    cli_options_free(&opts);
    return rv;
}

static int test_x_basic(void)
{
    char *argv[] = {"prog", "-x", NULL};
    cli_options_t opts;
    optind = 1;
    int rc = parse_command_line(2, argv, &opts);
    if (rc != 0)
    {
        fprintf(stderr, "parse_command_line rc=%d\n", rc);
        return 1;
    }
    int rv = 0;
    if (opts.only_count != 1)
    {
        fprintf(stderr, "unexpected only_count=%zu\n", opts.only_count);
        rv = 1;
    }
    else if (strcmp(opts.only[0], "layout") != 0)
    {
        fprintf(stderr, "unexpected token: %s\n", opts.only[0]);
        rv = 1;
    }
    cli_options_free(&opts);
    return rv;
}

static int test_x_and_only_merge(void)
{
    char *argv[] = {"prog", "-x", "--only", "a,b", NULL};
    cli_options_t opts;
    optind = 1;
    int rc = parse_command_line(4, argv, &opts);
    if (rc != 0)
    {
        fprintf(stderr, "parse_command_line rc=%d\n", rc);
        return 1;
    }
    int rv = 0;
    if (opts.only_count != 3)
    {
        fprintf(stderr, "unexpected only_count=%zu\n", opts.only_count);
        rv = 1;
    }
    else
    {
        int found_layout = 0, found_a = 0, found_b = 0;
        for (size_t i = 0; i < opts.only_count; ++i)
        {
            if (strcmp(opts.only[i], "layout") == 0)
                found_layout = 1;
            if (strcmp(opts.only[i], "a") == 0)
                found_a = 1;
            if (strcmp(opts.only[i], "b") == 0)
                found_b = 1;
        }
        if (!found_layout || !found_a || !found_b)
        {
            fprintf(stderr, "missing tokens: layout=%d a=%d b=%d\n", found_layout, found_a, found_b);
            rv = 1;
        }
    }
    cli_options_free(&opts);
    return rv;
}

static int test_parallel_sets_jobs(void)
{
    char *argv[] = {"prog", "--parallel", NULL};
    cli_options_t opts;
    optind = 1;
    int rc = parse_command_line(2, argv, &opts);
    if (rc != 0)
    {
        fprintf(stderr, "parse_command_line rc=%d\n", rc);
        return 1;
    }
    int rv = 0;
    if (!opts.parallel)
    {
        fprintf(stderr, "expected opts.parallel to be true\n");
        rv = 1;
    }
    else
    {
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        if (n < 1)
            n = 1;
        if (leuko_num_workers() != (size_t)n)
        {
            fprintf(stderr, "leuko_num_workers mismatch: got=%zu expected=%ld\n", leuko_num_workers(), n);
            rv = 1;
        }
    }
    cli_options_free(&opts);
    return rv;
}

int main(void)
{
    int failures = 0;
    failures += test_only_basic();
    failures += test_except_leading_empty();
    failures += test_parallel_sets_jobs();
    if (failures)
    {
        fprintf(stderr, "%d tests failed\n", failures);
        return 1;
    }
    printf("All CLI tests passed\n");
    return 0;
}
