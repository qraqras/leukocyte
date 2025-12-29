/* gen_rule_struct.c - very small PoC generator that reads a rule schema JSON and emits
 * a C header and source for the rule (struct, defaults, from_json/free using cJSON).
 * Build with: gcc -std=c99 -Ivendor/cjson -o gen_rule_struct tools/gen_rule_struct.c vendor/cjson/cJSON.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <schemas_dir> <outdir>\n", argv[0]);
        return 2;
    }
    const char *schemas_dir = argv[1];
    const char *outdir = argv[2];

    /* Discover rule schema files - accept either a directory of .json files or a single .json path */
    typedef struct
    {
        char *category; /* snake_case */
        char *rule;     /* snake_case short name */
        char *filename; /* path */
    } rule_info_t;

    rule_info_t *rules = NULL;
    size_t rules_len = 0;

    /* ensure outdir/rules and outdir/categories exist */
    char rules_dir[1024];
    char categories_dir[1024];
    snprintf(rules_dir, sizeof(rules_dir), "%s/rules", outdir);
    snprintf(categories_dir, sizeof(categories_dir), "%s/categories", outdir);
    if (mkdir(rules_dir, 0755) != 0 && errno != EEXIST)
    {
        fprintf(stderr, "failed to create %s: ", rules_dir);
        perror("mkdir");
        return 1;
    }
    if (mkdir(categories_dir, 0755) != 0 && errno != EEXIST)
    {
        fprintf(stderr, "failed to create %s: ", categories_dir);
        perror("mkdir");
        return 1;
    }

    struct stat st;
    if (stat(schemas_dir, &st) == 0 && S_ISREG(st.st_mode))
    {
        /* single file provided */
        char *path = malloc(strlen(schemas_dir) + 1);
        strcpy(path, schemas_dir);
        char *base = strrchr(path, '/');
        base = base ? base + 1 : path;
        char *dot = strstr(base, ".json");
        if (!dot)
        {
            free(path);
            fprintf(stderr, "not a json file: %s\n", schemas_dir);
            return 1;
        }
        size_t stem_len = dot - base;
        char *stem = malloc(stem_len + 1);
        strncpy(stem, base, stem_len);
        stem[stem_len] = '\0';
        char *first_dot = strchr(stem, '.');
        if (!first_dot)
        {
            free(path);
            free(stem);
            fprintf(stderr, "file stem not in expected format: %s\n", schemas_dir);
            return 1;
        }
        *first_dot = '\0';
        char *cat = malloc(strlen(stem) + 1);
        strcpy(cat, stem);
        char *r = malloc(strlen(first_dot + 1) + 1);
        strcpy(r, first_dot + 1);
        free(stem);
        rules = realloc(rules, sizeof(rule_info_t) * (rules_len + 1));
        rules[rules_len].category = cat;
        rules[rules_len].rule = r;
        rules[rules_len].filename = path;
        rules_len++;
    }
    else
    {
        DIR *d = opendir(schemas_dir);
        if (!d)
        {
            perror("opendir");
            return 1;
        }
        struct dirent *entry;
        while ((entry = readdir(d)) != NULL)
        {
            const char *name = entry->d_name;
            size_t L = strlen(name);
            if (L < 6)
                continue; /* at least x.json */
            if (strcmp(name + L - 5, ".json") != 0)
                continue;
            /* build full path */
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", schemas_dir, name);
            char *fname = malloc(strlen(path) + 1);
            strcpy(fname, path);
            /* derive category and rule from filename: e.g., layout.indentation_consistency.json */
            char *base = strrchr(fname, '/');
            base = base ? base + 1 : fname;
            char *dot = strstr(base, ".json");
            if (!dot)
            {
                free(fname);
                continue;
            }
            size_t stem_len = dot - base;
            char *stem = malloc(stem_len + 1);
            strncpy(stem, base, stem_len);
            stem[stem_len] = '\0';
            char *first_dot = strchr(stem, '.');
            if (!first_dot)
            {
                free(fname);
                free(stem);
                continue;
            }
            *first_dot = '\0';
            char *cat = malloc(strlen(stem) + 1);
            strcpy(cat, stem);
            char *r = malloc(strlen(first_dot + 1) + 1);
            strcpy(r, first_dot + 1);
            free(stem);
            /* store */
            rules = realloc(rules, sizeof(rule_info_t) * (rules_len + 1));
            rules[rules_len].category = cat;
            rules[rules_len].rule = r;
            rules[rules_len].filename = fname;
            rules_len++;
        }
        closedir(d);
    }

    printf("found %zu rule schemas\n", rules_len);
    if (rules_len == 0)
    {
        fprintf(stderr, "no rule schemas found in %s\n", schemas_dir);
        return 1;
    }

    /* Generate aggregated header and source: leuko_config.h / leuko_config.c */

    /* Aggregated header */
    char hdrpath[1024];
    snprintf(hdrpath, sizeof(hdrpath), "%s/leuko_config.h", outdir);
    FILE *h = fopen(hdrpath, "w");
    if (!h)
    {
        perror("open hdr");
        return 1;
    }
    fprintf(h, "/* generated by gen_rule_struct.c - do not edit */\n");
    fprintf(h, "#ifndef LEUKO_CONFIG_H\n#define LEUKO_CONFIG_H\n\n");
    fprintf(h, "#include <stdbool.h>\n#include <stddef.h>\n#include \"cJSON.h\"\n\n");
    /* include general header so leuko_general_t is available */
    fprintf(h, "#include \"leuko_general.h\"\n\n");
    /* per-rule headers will be included as canonical names (leuko_<category>_<rule>.h) */

    /* For each rule: generate a per-rule canonical header/source and a short alias header/source, then include the alias header in the aggregated header */
    for (size_t i = 0; i < rules_len; ++i)
    {
        char *cat = rules[i].category;
        char *rl = rules[i].rule;

        char *content = read_file(rules[i].filename);
        if (!content)
        {
            continue;
        }
        cJSON *rs = cJSON_Parse(content);
        free(content);
        if (!rs)
            continue;
        /* find properties either at root or within allOf entries */
        cJSON *props = cJSON_GetObjectItem(rs, "properties");
        if (!props)
        {
            cJSON *all = cJSON_GetObjectItem(rs, "allOf");
            if (all && cJSON_IsArray(all))
            {
                cJSON *entry = all->child;
                while (entry)
                {
                    cJSON *pp = cJSON_GetObjectItem(entry, "properties");
                    if (pp && cJSON_IsObject(pp))
                    {
                        props = pp;
                        break;
                    }
                    entry = entry->next;
                }
            }
        }

        /* canonical header: leuko_<category>_<rule>.h (placed in rules/) */
        char canon_h[1024];
        snprintf(canon_h, sizeof(canon_h), "%s/rules/leuko_%s_%s.h", outdir, cat, rl);
        FILE *ch = fopen(canon_h, "w");
        if (ch)
        {
            fprintf(ch, "/* generated by gen_rule_struct.c - do not edit */\n");
            fprintf(ch, "#ifndef LEUKO_%s_%s_H\n#define LEUKO_%s_%s_H\n\n", cat, rl, cat, rl);
            fprintf(ch, "#include <stdbool.h>\n#include \"cJSON.h\"\n\n");
            fprintf(ch, "typedef struct {\n    bool enabled;\n    char *severity;\n    char **include;\n    size_t include_len;\n    char **exclude;\n    size_t exclude_len;\n");
            if (props && cJSON_IsObject(props))
            {
                cJSON *p = props->child;
                while (p)
                {
                    const char *pname = p->string;
                    cJSON *ptype = cJSON_GetObjectItem(p, "type");
                    if (ptype && cJSON_IsString(ptype))
                    {
                        if (strcmp(ptype->valuestring, "string") == 0)
                            fprintf(ch, "    char *%s;\n", pname);
                        else if (strcmp(ptype->valuestring, "integer") == 0)
                            fprintf(ch, "    int %s;\n", pname);
                        else if (strcmp(ptype->valuestring, "boolean") == 0)
                            fprintf(ch, "    bool %s;\n", pname);
                    }
                    p = p->next;
                }
            }
            fprintf(ch, "} leuko_%s_%s_t;\n\n", cat, rl);
            fprintf(ch, "void leuko_%s_%s_init_defaults(leuko_%s_%s_t *out);\n", cat, rl, cat, rl);
            fprintf(ch, "int leuko_%s_%s_from_json(leuko_%s_%s_t *out, const cJSON *json);\n", cat, rl, cat, rl);
            fprintf(ch, "void leuko_%s_%s_free(leuko_%s_%s_t *p);\n\n", cat, rl, cat, rl);
            fprintf(ch, "#endif\n");
            fclose(ch);
            printf("wrote header %s\n", canon_h);
        }
        else
        {
            fprintf(stderr, "failed to write %s: ", canon_h);
            perror("fopen");
        }

        /* include the canonical header (from rules/) in the aggregated header */
        fprintf(h, "#include \"rules/leuko_%s_%s.h\"\n", cat, rl);

        /* canonical .c implementation (placed in rules/) */
        char canon_c[1024];
        snprintf(canon_c, sizeof(canon_c), "%s/rules/leuko_%s_%s.c", outdir, cat, rl);
        FILE *cc = fopen(canon_c, "w");
        if (cc)
        {
            fprintf(cc, "/* generated by gen_rule_struct.c - do not edit */\n");
            fprintf(cc, "#include \"rules/leuko_%s_%s.h\"\n#include <stdlib.h>\n#include <string.h>\n\n", cat, rl);
            fprintf(cc, "static char *leuko_strdup(const char *s) { if (!s) return NULL; char *r = malloc(strlen(s)+1); if (r) strcpy(r, s); return r; }\n\n");
            /* init_defaults */
            fprintf(cc, "void leuko_%s_%s_init_defaults(leuko_%s_%s_t *out) {\n    out->enabled = true;\n    out->severity = leuko_strdup(\"convention\");\n    out->include = NULL;\n    out->include_len = 0;\n    out->exclude = NULL;\n    out->exclude_len = 0;\n", cat, rl, cat, rl);
            if (props && cJSON_IsObject(props))
            {
                cJSON *p = props->child;
                while (p)
                {
                    const char *pname = p->string;
                    cJSON *ptype = cJSON_GetObjectItem(p, "type");
                    cJSON *def = cJSON_GetObjectItem(p, "default");
                    if (ptype && cJSON_IsString(ptype))
                    {
                        if (strcmp(ptype->valuestring, "string") == 0)
                        {
                            if (def && cJSON_IsString(def))
                                fprintf(cc, "    out->%s = leuko_strdup(\"%s\");\n", pname, def->valuestring);
                            else
                                fprintf(cc, "    out->%s = NULL;\n", pname);
                        }
                        else if (strcmp(ptype->valuestring, "integer") == 0)
                        {
                            if (def && cJSON_IsNumber(def))
                                fprintf(cc, "    out->%s = %d;\n", pname, def->valueint);
                            else
                                fprintf(cc, "    out->%s = 0;\n", pname);
                        }
                        else if (strcmp(ptype->valuestring, "boolean") == 0)
                        {
                            if (def && cJSON_IsBool(def))
                                fprintf(cc, "    out->%s = cJSON_IsTrue(def);\n", pname);
                            else
                                fprintf(cc, "    out->%s = false;\n", pname);
                        }
                    }
                    p = p->next;
                }
            }
            fprintf(cc, "}\n\n");
            /* from_json */
            fprintf(cc, "int leuko_%s_%s_from_json(leuko_%s_%s_t *out, const cJSON *json) {\n    if (!cJSON_IsObject(json)) return -1;\n    cJSON *e = cJSON_GetObjectItemCaseSensitive(json, \"enabled\"); if (e) { if (cJSON_IsBool(e)) out->enabled = cJSON_IsTrue(e); else return -1; }\n    cJSON *sev = cJSON_GetObjectItemCaseSensitive(json, \"severity\"); if (sev) { if (cJSON_IsString(sev) && sev->valuestring) { free(out->severity); out->severity = leuko_strdup(sev->valuestring); } else return -1; }\n    /* include */\n    cJSON *inc = cJSON_GetObjectItemCaseSensitive(json, \"include\"); if (inc) { if (!cJSON_IsArray(inc)) return -1; size_t n = cJSON_GetArraySize(inc); if (out->include) { for (size_t i=0;i<out->include_len;++i) free(out->include[i]); free(out->include); out->include = NULL; out->include_len = 0; } out->include = malloc(sizeof(char*) * n); if (!out->include && n>0) return -1; out->include_len = n; for (size_t i=0;i<n;++i) { cJSON *it = cJSON_GetArrayItem(inc, i); if (!cJSON_IsString(it) || !it->valuestring) return -1; out->include[i] = leuko_strdup(it->valuestring); } }\n    /* exclude */\n    cJSON *exc = cJSON_GetObjectItemCaseSensitive(json, \"exclude\"); if (exc) { if (!cJSON_IsArray(exc)) return -1; size_t m = cJSON_GetArraySize(exc); if (out->exclude) { for (size_t i=0;i<out->exclude_len;++i) free(out->exclude[i]); free(out->exclude); out->exclude = NULL; out->exclude_len = 0; } out->exclude = malloc(sizeof(char*) * m); if (!out->exclude && m>0) return -1; out->exclude_len = m; for (size_t i=0;i<m;++i) { cJSON *it = cJSON_GetArrayItem(exc, i); if (!cJSON_IsString(it) || !it->valuestring) return -1; out->exclude[i] = leuko_strdup(it->valuestring); } }\n", cat, rl, cat, rl);
            if (props && cJSON_IsObject(props))
            {
                cJSON *p = props->child;
                while (p)
                {
                    cJSON *ptype = cJSON_GetObjectItem(p, "type");
                    const char *pname = p->string;
                    if (ptype && cJSON_IsString(ptype))
                    {
                        if (strcmp(ptype->valuestring, "string") == 0)
                            fprintf(cc, "    cJSON *%s_j = cJSON_GetObjectItemCaseSensitive(json, \"%s\"); if (%s_j) { if (cJSON_IsString(%s_j) && %s_j->valuestring) { free(out->%s); out->%s = leuko_strdup(%s_j->valuestring); } else return -1; }\n", pname, pname, pname, pname, pname, pname, pname, pname);
                        else if (strcmp(ptype->valuestring, "integer") == 0)
                            fprintf(cc, "    cJSON *%s_j = cJSON_GetObjectItemCaseSensitive(json, \"%s\"); if (%s_j) { if (cJSON_IsNumber(%s_j)) out->%s = %s_j->valueint; else return -1; }\n", pname, pname, pname, pname, pname, pname);
                        else if (strcmp(ptype->valuestring, "boolean") == 0)
                            fprintf(cc, "    cJSON *%s_j = cJSON_GetObjectItemCaseSensitive(json, \"%s\"); if (%s_j) { if (cJSON_IsBool(%s_j)) out->%s = cJSON_IsTrue(%s_j); else return -1; }\n", pname, pname, pname, pname, pname, pname);
                    }
                    p = p->next;
                }
            }
            fprintf(cc, "    return 0;\n}\n\n");
            /* free */
            fprintf(cc, "void leuko_%s_%s_free(leuko_%s_%s_t *p) { if (!p) return; free(p->severity); ", cat, rl, cat, rl);
            if (props && cJSON_IsObject(props))
            {
                cJSON *pp = props->child;
                while (pp)
                {
                    if (pp->type & cJSON_String)
                        fprintf(cc, "free(p->%s); ", pp->string);
                    pp = pp->next;
                }
            }
            fprintf(cc, "if (p->include) { for (size_t i=0;i<p->include_len;++i) free(p->include[i]); free(p->include); p->include = NULL; p->include_len = 0; } if (p->exclude) { for (size_t i=0;i<p->exclude_len;++i) free(p->exclude[i]); free(p->exclude); p->exclude = NULL; p->exclude_len = 0; }");
            fprintf(cc, "}\n\n");
            fclose(cc);
            printf("wrote source %s\n", canon_c);
        }
        else
        {
            fprintf(stderr, "failed to write %s: ", canon_c);
            perror("fopen");
        }

        cJSON_Delete(rs);
    }

    /* Aggregated header: include per-category headers (generated below) and declare top-level leuko_config_t */
    /* generate a separate general module: leuko_general.h / leuko_general.c */
    char general_h[1024];
    snprintf(general_h, sizeof(general_h), "%s/leuko_general.h", outdir);
    FILE *gh = fopen(general_h, "w");
    if (gh)
    {
        fprintf(gh, "/* generated by gen_rule_struct.c - general header - do not edit */\n");
        fprintf(gh, "#ifndef LEUKO_GENERAL_H\n#define LEUKO_GENERAL_H\n\n");
        fprintf(gh, "#include <stdbool.h>\n#include <stddef.h>\n#include \"cJSON.h\"\n\n");
        fprintf(gh, "typedef struct {\n    bool enabled;\n    char *severity;\n    char **include;\n    size_t include_len;\n    char **exclude;\n    size_t exclude_len;\n} leuko_general_t;\n\n");
        fprintf(gh, "void leuko_general_init_defaults(leuko_general_t *out);\n");
        fprintf(gh, "int leuko_general_from_json(leuko_general_t *out, const cJSON *json);\n");
        fprintf(gh, "void leuko_general_free(leuko_general_t *out);\n\n");
        fprintf(gh, "#endif\n");
        fclose(gh);
        printf("wrote general header %s\n", general_h);
    }
    else
    {
        fprintf(stderr, "failed to write %s: ", general_h);
        perror("fopen");
    }
    /* generate general source */
    char general_c[1024];
    snprintf(general_c, sizeof(general_c), "%s/leuko_general.c", outdir);
    FILE *gc = fopen(general_c, "w");
    if (gc)
    {
        fprintf(gc, "/* generated by gen_rule_struct.c - general source - do not edit */\n");
        fprintf(gc, "#include \"leuko_general.h\"\n#include <stdlib.h>\n#include <string.h>\n\n");
        fprintf(gc, "static char *leuko_strdup(const char *s) { if (!s) return NULL; char *r = malloc(strlen(s)+1); if (r) strcpy(r, s); return r; }\n\n");
        fprintf(gc, "void leuko_general_init_defaults(leuko_general_t *out) {\n    out->enabled = true;\n    out->severity = leuko_strdup(\"convention\");\n    out->include = NULL;\n    out->include_len = 0;\n    out->exclude = NULL;\n    out->exclude_len = 0;\n}\n\n");
        fprintf(gc, "int leuko_general_from_json(leuko_general_t *out, const cJSON *json) {\n    if (!cJSON_IsObject(json)) return -1;\n    cJSON *e = cJSON_GetObjectItemCaseSensitive(json, \"enabled\"); if (e) { if (cJSON_IsBool(e)) out->enabled = cJSON_IsTrue(e); else return -1; }\n    cJSON *sev = cJSON_GetObjectItemCaseSensitive(json, \"severity\"); if (sev) { if (cJSON_IsString(sev) && sev->valuestring) { free(out->severity); out->severity = leuko_strdup(sev->valuestring); } else return -1; }\n    /* include */\n    cJSON *inc = cJSON_GetObjectItemCaseSensitive(json, \"include\"); if (inc) { if (!cJSON_IsArray(inc)) return -1; size_t n = cJSON_GetArraySize(inc); if (out->include) { for (size_t i=0;i<out->include_len;++i) free(out->include[i]); free(out->include); out->include = NULL; out->include_len = 0; } out->include = malloc(sizeof(char*) * n); if (!out->include && n>0) return -1; out->include_len = n; for (size_t i=0;i<n;++i) { cJSON *it = cJSON_GetArrayItem(inc, i); if (!cJSON_IsString(it) || !it->valuestring) return -1; out->include[i] = leuko_strdup(it->valuestring); } }\n    /* exclude */\n    cJSON *exc = cJSON_GetObjectItemCaseSensitive(json, \"exclude\"); if (exc) { if (!cJSON_IsArray(exc)) return -1; size_t m = cJSON_GetArraySize(exc); if (out->exclude) { for (size_t i=0;i<out->exclude_len;++i) free(out->exclude[i]); free(out->exclude); out->exclude = NULL; out->exclude_len = 0; } out->exclude = malloc(sizeof(char*) * m); if (!out->exclude && m>0) return -1; out->exclude_len = m; for (size_t i=0;i<m;++i) { cJSON *it = cJSON_GetArrayItem(exc, i); if (!cJSON_IsString(it) || !it->valuestring) return -1; out->exclude[i] = leuko_strdup(it->valuestring); } }\n    return 0;\n}\n\n");
        fprintf(gc, "void leuko_general_free(leuko_general_t *out) { if (!out) return; if (out->severity) free(out->severity); if (out->include) { for (size_t i=0;i<out->include_len;++i) free(out->include[i]); free(out->include); out->include = NULL; out->include_len = 0; } if (out->exclude) { for (size_t i=0;i<out->exclude_len;++i) free(out->exclude[i]); free(out->exclude); out->exclude = NULL; out->exclude_len = 0; } }\n");
        fclose(gc);
        printf("wrote general source %s\n", general_c);
    }
    else
    {
        fprintf(stderr, "failed to write %s: ", general_c);
        perror("fopen");
    }

    /* Generate per-category headers and sources, and include the headers in the aggregated header */
    for (size_t i = 0; i < rules_len; ++i)
    {
        int seen = 0;
        for (size_t j = 0; j < i; ++j)
            if (strcmp(rules[j].category, rules[i].category) == 0)
            {
                seen = 1;
                break;
            }
        if (seen)
            continue;
        char *cat = rules[i].category;

        /* category header (placed in categories/) */
        char cat_h_path[1024];
        snprintf(cat_h_path, sizeof(cat_h_path), "%s/categories/leuko_category_%s.h", outdir, cat);
        FILE *ch = fopen(cat_h_path, "w");
        if (ch)
        {
            fprintf(ch, "/* generated by gen_rule_struct.c - category header - do not edit */\n");
            fprintf(ch, "#ifndef LEUKO_CATEGORY_%s_H\n#define LEUKO_CATEGORY_%s_H\n\n", cat, cat);
            fprintf(ch, "#include <stdbool.h>\n#include <stddef.h>\n#include \"cJSON.h\"\n\n");
            /* include per-rule canonical headers */
            for (size_t k = 0; k < rules_len; ++k)
                if (strcmp(rules[k].category, cat) == 0)
                    fprintf(ch, "#include \"rules/leuko_%s_%s.h\"\n", cat, rules[k].rule);
            fprintf(ch, "\ntypedef struct {\n    bool enabled;\n    char *severity;\n    char **include;\n    size_t include_len;\n    char **exclude;\n    size_t exclude_len;\n");
            for (size_t k = 0; k < rules_len; ++k)
                if (strcmp(rules[k].category, cat) == 0)
                    fprintf(ch, "    leuko_%s_%s_t %s;\n", cat, rules[k].rule, rules[k].rule);
            fprintf(ch, "} leuko_category_%s_t;\n\n", cat);
            fprintf(ch, "void leuko_category_%s_init_defaults(leuko_category_%s_t *out);\n", cat, cat);
            fprintf(ch, "int leuko_category_%s_from_json(leuko_category_%s_t *out, const cJSON *json);\n", cat, cat);
            fprintf(ch, "void leuko_category_%s_free(leuko_category_%s_t *out);\n\n", cat, cat);
            fprintf(ch, "#endif\n");
            fclose(ch);
            printf("wrote category header %s\n", cat_h_path);
        }
        else
        {
            fprintf(stderr, "failed to write %s: ", cat_h_path);
            perror("fopen");
        }

        /* category source (placed in categories/) */
        char cat_c_path[1024];
        snprintf(cat_c_path, sizeof(cat_c_path), "%s/categories/leuko_category_%s.c", outdir, cat);
        FILE *ccat = fopen(cat_c_path, "w");
        if (ccat)
        {
            fprintf(ccat, "/* generated by gen_rule_struct.c - category source - do not edit */\n");
            fprintf(ccat, "#include \"categories/leuko_category_%s.h\"\n#include <stdlib.h>\n#include <string.h>\n\n", cat);
            fprintf(ccat, "static char *leuko_strdup(const char *s) { if (!s) return NULL; char *r = malloc(strlen(s)+1); if (r) strcpy(r, s); return r; }\n\n");

            /* init_defaults */
            fprintf(ccat, "void leuko_category_%s_init_defaults(leuko_category_%s_t *out) {\n    out->enabled = true;\n    out->severity = leuko_strdup(\"convention\");\n    out->include = NULL;\n    out->include_len = 0;\n    out->exclude = NULL;\n    out->exclude_len = 0;\n", cat, cat);
            for (size_t k = 0; k < rules_len; ++k)
                if (strcmp(rules[k].category, cat) == 0)
                    fprintf(ccat, "    leuko_%s_%s_init_defaults(&out->%s);\n", cat, rules[k].rule, rules[k].rule);
            fprintf(ccat, "}\n\n");

            /* from_json */
            fprintf(ccat, "int leuko_category_%s_from_json(leuko_category_%s_t *out, const cJSON *json) {\n    if (!cJSON_IsObject(json)) return -1;\n    /* rules */\n    cJSON *rules = cJSON_GetObjectItemCaseSensitive(json, \"rules\");\n    if (rules && cJSON_IsObject(rules)) {\n", cat, cat);
            for (size_t k = 0; k < rules_len; ++k)
                if (strcmp(rules[k].category, cat) == 0)
                    fprintf(ccat, "        cJSON *r_%s = cJSON_GetObjectItemCaseSensitive(rules, \"%s\"); if (r_%s) { if (leuko_%s_%s_from_json(&out->%s, r_%s) != 0) return -1; }\n", rules[k].rule, rules[k].rule, rules[k].rule, cat, rules[k].rule, rules[k].rule, rules[k].rule);
            fprintf(ccat, "    }\n    /* include */\n    cJSON *inc = cJSON_GetObjectItemCaseSensitive(json, \"include\"); if (inc) { if (!cJSON_IsArray(inc)) return -1; size_t n = cJSON_GetArraySize(inc); if (out->include) { for (size_t i=0;i<out->include_len;++i) free(out->include[i]); free(out->include); out->include = NULL; out->include_len = 0; } out->include = malloc(sizeof(char*) * n); if (!out->include && n>0) return -1; out->include_len = n; for (size_t i=0;i<n;++i) { cJSON *it = cJSON_GetArrayItem(inc, i); if (!cJSON_IsString(it) || !it->valuestring) return -1; out->include[i] = leuko_strdup(it->valuestring); } }\n    /* exclude */\n    cJSON *exc = cJSON_GetObjectItemCaseSensitive(json, \"exclude\"); if (exc) { if (!cJSON_IsArray(exc)) return -1; size_t m = cJSON_GetArraySize(exc); if (out->exclude) { for (size_t i=0;i<out->exclude_len;++i) free(out->exclude[i]); free(out->exclude); out->exclude = NULL; out->exclude_len = 0; } out->exclude = malloc(sizeof(char*) * m); if (!out->exclude && m>0) return -1; out->exclude_len = m; for (size_t i=0;i<m;++i) { cJSON *it = cJSON_GetArrayItem(exc, i); if (!cJSON_IsString(it) || !it->valuestring) return -1; out->exclude[i] = leuko_strdup(it->valuestring); } }\n    return 0;\n}\n\n");

            /* free */
            fprintf(ccat, "void leuko_category_%s_free(leuko_category_%s_t *out) { if (!out) return; ", cat, cat);
            for (size_t k = 0; k < rules_len; ++k)
                if (strcmp(rules[k].category, cat) == 0)
                    fprintf(ccat, "leuko_%s_%s_free(&out->%s); ", cat, rules[k].rule, rules[k].rule);
            fprintf(ccat, "if (out->include) { for (size_t i=0;i<out->include_len;++i) free(out->include[i]); free(out->include); out->include = NULL; out->include_len = 0; } if (out->exclude) { for (size_t i=0;i<out->exclude_len;++i) free(out->exclude[i]); free(out->exclude); out->exclude = NULL; out->exclude_len = 0; } free(out->severity); }\n\n");
            fclose(ccat);
            printf("wrote category source %s\n", cat_c_path);
        }
        else
        {
            fprintf(stderr, "failed to write %s: ", cat_c_path);
            perror("fopen");
        }

        /* include the category header (from categories/) in the aggregated header */
        fprintf(h, "#include \"categories/leuko_category_%s.h\"\n", cat);
    }

    /* leuko_config_t */
    fprintf(h, "typedef struct {\n    char *schema_version;\n    leuko_general_t general;\n    struct {\n");
    for (size_t i = 0; i < rules_len; ++i)
    {
        int seen = 0;
        for (size_t j = 0; j < i; ++j)
            if (strcmp(rules[j].category, rules[i].category) == 0)
            {
                seen = 1;
                break;
            }
        if (!seen)
            fprintf(h, "        leuko_category_%s_t %s;\n", rules[i].category, rules[i].category);
    }
    fprintf(h, "    } categories;\n} leuko_config_t;\n\n");

    fprintf(h, "void leuko_config_init_defaults(leuko_config_t *out);\n");
    fprintf(h, "int leuko_config_from_json(leuko_config_t *out, const cJSON *json);\n");
    fprintf(h, "void leuko_config_free(leuko_config_t *out);\n");
    fprintf(h, "#endif\n");
    fclose(h);

    /* Aggregated source */
    char srcpath[1024];
    snprintf(srcpath, sizeof(srcpath), "%s/leuko_config.c", outdir);
    FILE *s = fopen(srcpath, "w");
    if (!s)
    {
        perror("open src");
        return 1;
    }
    fprintf(s, "/* generated by gen_rule_struct.c - do not edit */\n");
    fprintf(s, "#include \"leuko_config.h\"\n#include \"cJSON.h\"\n#include <string.h>\n#include <stdlib.h>\n\n");
    fprintf(s, "static char *leuko_strdup(const char *s) { if (!s) return NULL; char *r = malloc(strlen(s)+1); if (r) strcpy(r, s); return r; }\n\n");

    /* per-rule implementations are generated in individual per-rule .c files (so they are not emitted into the aggregated source) */
    for (size_t i = 0; i < rules_len; ++i)
    {
        /* nothing here; per-rule .c written earlier */
    }

    /* Top-level init/from/free */
    fprintf(s, "void leuko_config_init_defaults(leuko_config_t *out) {\n");
    fprintf(s, "    out->schema_version = leuko_strdup(\"1.0.0\");\n");
    fprintf(s, "    out->general.enabled = true;\n");
    fprintf(s, "    out->general.severity = leuko_strdup(\"convention\");\n");
    for (size_t i = 0; i < rules_len; ++i)
    {
        int seen = 0;
        for (size_t j = 0; j < i; ++j)
            if (strcmp(rules[j].category, rules[i].category) == 0)
            {
                seen = 1;
                break;
            }
        if (!seen)
            fprintf(s, "    leuko_category_%s_init_defaults(&out->categories.%s);\n", rules[i].category, rules[i].category);
    }
    fprintf(s, "}\n\n");

    fprintf(s, "int leuko_config_from_json(leuko_config_t *out, const cJSON *json) {\n");
    fprintf(s, "    if (!cJSON_IsObject(json)) return -1;\n");
    fprintf(s, "    cJSON *gen = cJSON_GetObjectItemCaseSensitive(json, \"general\");\n");
    fprintf(s, "    if (gen && cJSON_IsObject(gen)) { cJSON *e = cJSON_GetObjectItemCaseSensitive(gen, \"enabled\"); if (e) { if (cJSON_IsBool(e)) out->general.enabled = cJSON_IsTrue(e); else return -1; } cJSON *sev = cJSON_GetObjectItemCaseSensitive(gen, \"severity\"); if (sev) { if (cJSON_IsString(sev) && sev->valuestring) { free(out->general.severity); out->general.severity = leuko_strdup(sev->valuestring); } else return -1; } }\n");
    fprintf(s, "    cJSON *cats = cJSON_GetObjectItemCaseSensitive(json, \"categories\");\n");
    fprintf(s, "    if (cats && cJSON_IsObject(cats)) {\n");
    for (size_t i = 0; i < rules_len; ++i)
    {
        int seen = 0;
        for (size_t j = 0; j < i; ++j)
            if (strcmp(rules[j].category, rules[i].category) == 0)
            {
                seen = 1;
                break;
            }
        if (!seen)
            fprintf(s, "        cJSON *cat_%s = cJSON_GetObjectItemCaseSensitive(cats, \"%s\"); if (cat_%s) { if (leuko_category_%s_from_json(&out->categories.%s, cat_%s) != 0) return -1; }\n", rules[i].category, rules[i].category, rules[i].category, rules[i].category, rules[i].category, rules[i].category);
    }
    fprintf(s, "    }\n    return 0;\n}\n\n");

    fprintf(s, "void leuko_config_free(leuko_config_t *out) { if (!out) return; free(out->schema_version); free(out->general.severity); ");
    for (size_t i = 0; i < rules_len; ++i)
    {
        int seen = 0;
        for (size_t j = 0; j < i; ++j)
            if (strcmp(rules[j].category, rules[i].category) == 0)
            {
                seen = 1;
                break;
            }
        if (!seen)
            fprintf(s, "leuko_category_%s_free(&out->categories.%s); ", rules[i].category, rules[i].category);
    }
    fprintf(s, "}\n");
    fclose(s);

    printf("generated aggregated config to %s/leuko_config.h and %s/leuko_config.c\n", outdir, outdir);

    /* cleanup */
    for (size_t i = 0; i < rules_len; ++i)
    {
        free(rules[i].category);
        free(rules[i].rule);
        free(rules[i].filename);
    }
    free(rules);
    return 0;
}
