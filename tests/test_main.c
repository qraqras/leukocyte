// Simple tests for read_file_to_buffer
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "io/file.h"
#include <stdint.h>

static int test_read_regular_file(void)
{
    char tmpl[] = "/tmp/leuko_test_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0)
        return 1;
    const char *data = "hello world\n";
    write(fd, data, strlen(data));
    close(fd);

    uint8_t *buf = NULL;
    size_t sz = 0;
    char *err = NULL;
    int ok = read_file_to_buffer(tmpl, &buf, &sz, &err);
    unlink(tmpl);
    if (!ok)
    {
        fprintf(stderr, "test_read_regular_file failed: %s\n", err ? err : "unknown");
        free(err);
        return 1;
    }
    int rv = 0;
    if (sz != strlen(data) || memcmp(buf, data, sz) != 0)
        rv = 1;
    free(buf);
    return rv;
}

static int test_missing_file(void)
{
    uint8_t *buf = NULL;
    size_t sz = 0;
    char *err = NULL;
    int ok = read_file_to_buffer("/no/such/file/hopefully", &buf, &sz, &err);
    if (ok)
    {
        free(buf);
        return 1;
    }
    if (err)
    {
        free(err);
        return 0;
    }
    return 0;
}

int main(void)
{
    int failures = 0;
    failures += test_read_regular_file();
    failures += test_missing_file();
    if (failures)
    {
        fprintf(stderr, "%d tests failed\n", failures);
        return 1;
    }
    printf("All tests passed\n");
    return 0;
}
