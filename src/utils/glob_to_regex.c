#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Convert a simple glob pattern to a POSIX extended regex string.
 * This is a best-effort conversion for prototype purposes.
 * Caller must free returned string.
 */
char *leuko_glob_to_regex(const char *glob)
{
    if (!glob)
        return NULL;
    size_t len = strlen(glob);
    /* allocate conservative buffer: each char may expand, add anchors */
    size_t bufcap = len * 4 + 16;
    char *out = malloc(bufcap);
    if (!out)
        return NULL;
    char *p = out;
    *p++ = '^';
    for (size_t i = 0; i < len; i++)
    {
        char c = glob[i];
        if (c == '*')
        {
            /* handle ** specially (match across slashes) */
            if (i + 1 < len && glob[i + 1] == '*')
            {
                /* skip second */
                i++;
                /* convert to .* */
                memcpy(p, ".*", 2);
                p += 2;
            }
            else
            {
                /* single * => match any except slash */
                const char *s = "[^/]*";
                size_t sl = strlen(s);
                memcpy(p, s, sl);
                p += sl;
            }
        }
        else if (c == '?')
        {
            const char *s = "[^/]";
            memcpy(p, s, 4);
            p += 4;
        }
        else if (c == '.')
        {
            *p++ = '\\';
            *p++ = '.';
        }
        else if (c == '+' || c == '(' || c == ')' || c == '|' || c == '{' || c == '}' || c == '^' || c == '$')
        {
            *p++ = '\\';
            *p++ = c;
        }
        else if (c == '[')
        {
            /* copy until closing bracket or end */
            *p++ = '[';
            i++;
            while (i < len && glob[i] != ']')
            {
                *p++ = glob[i++];
            }
            if (i < len && glob[i] == ']')
                *p++ = ']';
        }
        else if (c == '/')
        {
            *p++ = '/';
        }
        else
        {
            *p++ = c;
        }
    }
    *p++ = '$';
    *p = '\0';
    return out;
}
