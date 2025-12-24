#ifndef LEUKO_UTILS_GLOB_TO_REGEX_H
#define LEUKO_UTILS_GLOB_TO_REGEX_H

/* Return malloc'd regex string corresponding to glob; caller frees */
char *leuko_glob_to_regex(const char *glob);

#endif /* LEUKO_UTILS_GLOB_TO_REGEX_H */
