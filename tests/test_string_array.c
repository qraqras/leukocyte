/* Tests for leuko_string_array_concat */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "util/string_array.h"

int main(void)
{
    size_t dest_count = 0;
    char **dest = NULL;

    /* initial adoption */
    printf("test: initial adoption\n");
    fflush(stdout);
    char **src = malloc(2 * sizeof(char *));
    src[0] = strdup("a");
    src[1] = strdup("b");
    {
        bool ok = leuko_string_array_concat(&dest, &dest_count, src, 2);
        if (!ok)
        {
            fprintf(stderr, "concat failed (adopt)\n");
            return 1;
        }
    }
    printf("after adopt: dest_count=%zu\n", dest_count);
    fflush(stdout);
    if (!(dest != NULL && dest_count == 2))
    {
        fprintf(stderr, "unexpected dest after adopt\n");
        return 1;
    }
    if (strcmp(dest[0], "a") != 0)
    {
        fprintf(stderr, "unexpected dest[0]\n");
        return 1;
    }
    if (strcmp(dest[1], "b") != 0)
    {
        fprintf(stderr, "unexpected dest[1]\n");
        return 1;
    }

    /* append */
    printf("test: append\n");
    fflush(stdout);
    char **src2 = malloc(1 * sizeof(char *));
    src2[0] = strdup("c");
    {
        bool ok = leuko_string_array_concat(&dest, &dest_count, src2, 1);
        if (!ok)
        {
            fprintf(stderr, "concat failed (append)\n");
            return 1;
        }
    }
    printf("after append: dest_count=%zu\n", dest_count);
    fflush(stdout);
    if (!(dest_count == 3 && strcmp(dest[2], "c") == 0))
    {
        fprintf(stderr, "unexpected dest after append\n");
        return 1;
    }

    /* cleanup */
    free(dest[0]);
    free(dest[1]);
    free(dest[2]);
    free(dest);

    /* src NULL/empty is no-op */
    printf("test: src NULL no-op\n");
    fflush(stdout);
{
        bool ok = leuko_string_array_concat(&dest, &dest_count, NULL, 0);
        if (!ok)
        {
            fprintf(stderr, "concat failed (NULL no-op)\n");
            return 1;
        }
    }

    /* invalid arguments */
    printf("test: invalid args\n");
    fflush(stdout);
{
        /* All-NULL with src_count 0 is a no-op and should return true */
        bool ok = leuko_string_array_concat(NULL, NULL, NULL, 0);
        if (!ok)
        {
            fprintf(stderr, "expected true for all-NULL no-op\n");
            return 1;
        }
    }
    {
        /* dest NULL with non-empty src should return false */
        char **src3 = malloc(1 * sizeof(char *));
        src3[0] = strdup("x");
        bool ok = leuko_string_array_concat(NULL, NULL, src3, 1);
        if (ok != false)
        {
            fprintf(stderr, "expected false for dest==NULL with non-null src\n");
            return 1;
        }
        free(src3[0]);
        free(src3);
    }

    printf("test: done\n");
    fflush(stdout);
    return 0;
}

