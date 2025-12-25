#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "sources/json/parse.h"
#include "sources/node.h"

static char *write_temp(const char *name, const char *content)
{
    FILE *f = fopen(name, "w");
    if (!f) return NULL;
    if (content && content[0])
        fputs(content, f);
    fclose(f);
    return strdup(name);
}

static void rm(const char *name)
{
    if (name)
        remove(name);
}

int main(void)
{
    leuko_node_t *n = NULL;
    char *f;

    /* Empty file => EINVAL expected */
    f = write_temp("tests/tmp_empty.json", "");
    assert(f);
    errno = 0;
    int ok = leuko_json_parse(f, &n);
    assert(!ok);
    assert(errno == EINVAL);
    rm(f);
    free(f);

    /* Invalid JSON => EILSEQ expected */
    f = write_temp("tests/tmp_invalid.json", "{ invalid json");
    assert(f);
    errno = 0;
    ok = leuko_json_parse(f, &n);
    assert(!ok);
    assert(errno == EILSEQ);
    rm(f);
    free(f);

    /* Valid JSON => node produced */
    f = write_temp("tests/tmp_valid.json", "{\"general\":{\"Include\":[\"**/*.rb\"]}}\n");
    assert(f);
    errno = 0;
    ok = leuko_json_parse(f, &n);
    assert(ok);
    assert(n != NULL);
    leuko_node_free(n);
    rm(f);
    free(f);

    printf("OK\n");
    return 0;
}
