// Tests for file scanning
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "io/scan.h"

static int test_scan_dir(void)
{
    char tmpl[] = "/tmp/leuko_scan_XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (!dir)
        return 1;
    char path_a[512];
    char path_b[512];
    char subdir[512];
    snprintf(path_a, sizeof(path_a), "%s/a.rb", dir);
    snprintf(path_b, sizeof(path_b), "%s/b.txt", dir);
    snprintf(subdir, sizeof(subdir), "%s/sub", dir);
    mkdir(subdir, 0755);
    char path_c[512];
    snprintf(path_c, sizeof(path_c), "%s/sub/c.rb", dir);
    FILE *f = fopen(path_a, "w");
    if (!f)
        return 1;
    fclose(f);
    f = fopen(path_b, "w");
    if (!f)
        return 1;
    fclose(f);
    f = fopen(path_c, "w");
    if (!f)
        return 1;
    fclose(f);

    char *in[] = {dir};
    char **out = NULL;
    size_t out_count = 0;
    char *err = NULL;
    int ok = leuko_collect_ruby_files(in, 1, &out, &out_count, &err);
    int rv = 0;
    if (!ok)
    {
        fprintf(stderr, "collect_ruby_files failed: %s\n", err ? err : "unknown");
        free(err);
        rv = 1;
        goto cleanup;
    }
    /* Expect a.rb and sub/c.rb, order not guaranteed but both present */
    int found_a = 0, found_c = 0;
    for (size_t i = 0; i < out_count; ++i)
    {
        if (strcmp(out[i], path_a) == 0)
            found_a = 1;
        if (strcmp(out[i], path_c) == 0)
            found_c = 1;
    }
    if (!found_a || !found_c)
    {
        fprintf(stderr, "unexpected scan_dir results (out_count=%zu):\n", out_count);
        for (size_t i = 0; i < out_count; ++i)
            fprintf(stderr, "  %s\n", out[i]);
        rv = 1;
    }

    /* Also test glob expansion for *.rb */
    {
        char pattern[512];
        snprintf(pattern, sizeof(pattern), "%s/*.rb", dir);
        char *in_glob[] = {pattern};
        char **out2 = NULL;
        size_t out2_count = 0;
        char *err2 = NULL;
        int ok2 = leuko_collect_ruby_files(in_glob, 1, &out2, &out2_count, &err2);
        if (!ok2)
        {
            fprintf(stderr, "collect_ruby_files(glob) failed: %s\n", err2 ? err2 : "unknown");
            free(err2);
            rv = 1;
        }
        else
        {
            int found_a2 = 0;
            for (size_t i = 0; i < out2_count; ++i)
            {
                if (strcmp(out2[i], path_a) == 0)
                    found_a2 = 1;
            }
            if (!found_a2)
            {
                fprintf(stderr, "unexpected glob results (out2_count=%zu):\n", out2_count);
                for (size_t i = 0; i < out2_count; ++i)
                    fprintf(stderr, "  %s\n", out2[i]);
                rv = 1;
            }
        }
        if (out2)
        {
            for (size_t i = 0; i < out2_count; ++i)
                free(out2[i]);
            free(out2);
        }
    }

cleanup:
    if (out)
    {
        for (size_t i = 0; i < out_count; ++i)
            free(out[i]);
        free(out);
    }
    unlink(path_a);
    unlink(path_b);
    unlink(path_c);
    rmdir(subdir);
    rmdir(dir);
    return rv;
}

static int test_dash_token(void)
{
    char *in[] = {"-"};
    char **out = NULL;
    size_t out_count = 0;
    char *err = NULL;
    int ok = leuko_collect_ruby_files(in, 1, &out, &out_count, &err);
    if (!ok)
    {
        fprintf(stderr, "collect_ruby_files failed: %s\n", err ? err : "unknown");
        free(err);
        return 1;
    }
    if (out_count != 1 || strcmp(out[0], "-") != 0)
    {
        if (out)
        {
            for (size_t i = 0; i < out_count; ++i)
                free(out[i]);
            free(out);
        }
        return 1;
    }
    free(out[0]);
    free(out);
    return 0;
}

static int test_glob_star(void)
{
    char tmpl[] = "/tmp/leuko_scan_XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (!dir)
        return 1;
    char path_a[512];
    char path_b[512];
    char subdir[512];
    snprintf(path_a, sizeof(path_a), "%s/a.rb", dir);
    snprintf(path_b, sizeof(path_b), "%s/b.txt", dir);
    snprintf(subdir, sizeof(subdir), "%s/sub", dir);
    mkdir(subdir, 0755);
    char path_c[512];
    snprintf(path_c, sizeof(path_c), "%s/sub/c.rb", dir);
    FILE *f = fopen(path_a, "w");
    if (!f)
        return 1;
    fclose(f);
    f = fopen(path_b, "w");
    if (!f)
        return 1;
    fclose(f);
    f = fopen(path_c, "w");
    if (!f)
        return 1;
    fclose(f);

    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%s/*", dir);
    char *in_glob[] = {pattern};
    char **out = NULL;
    size_t out_count = 0;
    char *err = NULL;
    int ok = leuko_collect_ruby_files(in_glob, 1, &out, &out_count, &err);
    int rv = 0;
    if (!ok)
    {
        fprintf(stderr, "collect_ruby_files(glob star) failed: %s\n", err ? err : "unknown");
        free(err);
        rv = 1;
        goto cleanup;
    }
    int found_a = 0, found_c = 0, found_b = 0;
    for (size_t i = 0; i < out_count; ++i)
    {
        if (strcmp(out[i], path_a) == 0)
            found_a = 1;
        if (strcmp(out[i], path_c) == 0)
            found_c = 1;
        if (strcmp(out[i], path_b) == 0)
            found_b = 1;
    }
    if (!found_a || !found_c || found_b)
        rv = 1;

cleanup:
    if (out)
    {
        for (size_t i = 0; i < out_count; ++i)
            free(out[i]);
        free(out);
    }
    unlink(path_a);
    unlink(path_b);
    unlink(path_c);
    rmdir(subdir);
    rmdir(dir);
    return rv;
}

int main(void)
{
    int failures = 0;
    int r1 = test_scan_dir();
    if (r1 != 0)
        fprintf(stderr, "test_scan_dir failed\n");
    failures += r1;
    int r2 = test_dash_token();
    if (r2 != 0)
        fprintf(stderr, "test_dash_token failed\n");
    failures += r2;
    int r3 = test_glob_star();
    if (r3 != 0)
        fprintf(stderr, "test_glob_star failed\n");
    failures += r3;
    /* New test: directory excludes should skip files under matching directories */
    {
        char tmpl[] = "/tmp/leuko_scan_excl_XXXXXX";
        char *dir = mkdtemp(tmpl);
        char sub_a[512];
        char sub_b[512];
        char excl_dir[512];
        snprintf(sub_a, sizeof(sub_a), "%s/included/a.rb", dir);
        snprintf(excl_dir, sizeof(excl_dir), "%s/excluded", dir);
        snprintf(sub_b, sizeof(sub_b), "%s/excluded/b.rb", dir);
        /* Create included/excluded directories */
        char included_dir[512];
        snprintf(included_dir, sizeof(included_dir), "%s/included", dir);
        char excluded_dir[512];
        snprintf(excluded_dir, sizeof(excluded_dir), "%s/excluded", dir);
        mkdir(included_dir, 0755);
        mkdir(excluded_dir, 0755);
        /* Create files */
        FILE *f = fopen(sub_a, "w");
        if (!f)
            return 1;
        fclose(f);
        f = fopen(sub_b, "w");
        if (!f)
            return 1;
        fclose(f);
        char *in[] = {dir};
        char *excludes[] = {"*/excluded/*"};
        char **out = NULL;
        size_t out_count = 0;
        char *err = NULL;
        int ok = leuko_collect_ruby_files_with_exclude(in, 1, &out, &out_count, excludes, 1, &err);
        if (!ok)
        {
            fprintf(stderr, "collect_with_exclude failed: %s\n", err ? err : "unknown");
            failures += 1;
        }
        else
        {
            int found_a = 0, found_b = 0;
            for (size_t i = 0; i < out_count; ++i)
            {
                if (strcmp(out[i], sub_a) == 0)
                    found_a = 1;
                if (strcmp(out[i], sub_b) == 0)
                    found_b = 1;
            }
            if (!found_a || found_b)
            {
                fprintf(stderr, "exclude test failed: found_a=%d found_b=%d out_count=%zu\n", found_a, found_b, out_count);
                for (size_t i = 0; i < out_count; ++i)
                    fprintf(stderr, "  %s\n", out[i]);
                failures += 1;
            }
        }
        if (out)
        {
            for (size_t i = 0; i < out_count; ++i)
                free(out[i]);
            free(out);
        }
        unlink(sub_a);
        unlink(sub_b);
        rmdir(excl_dir);
        rmdir("/tmp/leuko_scan_excl_XXXXXX/included");
        rmdir(dir);
    }
    fprintf(stderr, "r1=%d r2=%d r3=%d failures=%d\n", r1, r2, r3, failures);
    if (failures)
    {
        fprintf(stderr, "%d tests failed\n", failures);
        return 1;
    }
    printf("All IO scan tests passed\n");
    return 0;
}
