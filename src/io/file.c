#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "io/file.h"

bool read_file_to_buffer(const char *path, uint8_t **out_buf, size_t *out_size, char **err)
{
    if (!path || !out_buf || !out_size)
    {
        if (err)
        {
            *err = strdup("invalid args");
        }
        return false;
    }

    FILE *file = fopen(path, "rb");
    if (!file)
    {
        if (err)
        {
            size_t len = 64 + strlen(path);
            *err = malloc(len);
            if (*err)
            {
                snprintf(*err, len, "failed to open '%s': %s", path, strerror(errno));
            }
        }
        return false;
    }

    struct stat st;
    uint8_t *source = NULL;
    size_t file_size = 0;
    size_t read_size = 0;

    if (fstat(fileno(file), &st) == 0 && S_ISREG(st.st_mode))
    {
        /* Ensure we can allocate file_size + 1 without overflow */
        if (st.st_size < 0 || (unsigned long long)st.st_size > (unsigned long long)(SIZE_MAX - 1))
        {
            if (err)
            {
                *err = strdup("file too large");
            }
            fclose(file);
            return false;
        }
        file_size = (size_t)st.st_size;
        source = malloc(file_size + 1);
        if (!source)
        {
            if (err)
            {
                *err = strdup("out of memory");
            }
            fclose(file);
            return false;
        }
        read_size = fread(source, 1, file_size, file);
        if (read_size != file_size || ferror(file))
        {
            if (err)
            {
                *err = strdup("failed to read file");
            }
            fclose(file);
            free(source);
            return false;
        }
        fclose(file);
        source[file_size] = '\0';
    }
    else
    {
        size_t cap = 4096;
        source = malloc(cap + 1);
        if (!source)
        {
            if (err)
            {
                *err = strdup("out of memory");
            }
            fclose(file);
            return false;
        }
        read_size = 0;
        size_t n;
        size_t max_store = SIZE_MAX - 1;
        while ((n = fread(source + read_size, 1, cap - read_size, file)) > 0)
        {
            read_size += n;
            if (read_size == cap)
            {
                if (cap >= max_store)
                {
                    if (err)
                    {
                        *err = strdup("input too large");
                    }
                    free(source);
                    fclose(file);
                    return false;
                }
                size_t newcap = (cap <= max_store / 2) ? (cap * 2) : max_store;
                uint8_t *tmp = realloc(source, newcap + 1);
                if (!tmp)
                {
                    if (err)
                    {
                        *err = strdup("out of memory");
                    }
                    free(source);
                    fclose(file);
                    return false;
                }
                source = tmp;
                cap = newcap;
            }
        }
        if (ferror(file))
        {
            if (err)
            {
                *err = strdup("failed to read stream");
            }
            free(source);
            fclose(file);
            return false;
        }
        fclose(file);
        file_size = read_size;
        source[file_size] = '\0';
    }

    *out_buf = source;
    *out_size = file_size;
    if (err)
    {
        *err = NULL;
    }
    return true;
}
