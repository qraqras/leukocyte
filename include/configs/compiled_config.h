#ifndef LEUKO_CONFIGS_COMPILED_CONFIG_H
#define LEUKO_CONFIGS_COMPILED_CONFIG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* forward declare leuko_config_t to avoid heavy includes in header */
typedef struct leuko_config_s leuko_config_t;
/* forward declare leuko_node_t to avoid heavy includes in header */
typedef struct leuko_node_s leuko_node_t;
#include "utils/allocator/arena.h" /* leuko_arena */

typedef struct leuko_compiled_config_s
{
    /* ディレクトリ情報 */
    char *dir;            /* 所有: 絶対ディレクトリパス */
    uint64_t fingerprint; /* mtime+content+parent の簡易ハッシュ */

    /* マージ済み in-memory node（所有） */
    leuko_node_t *merged_node; /* 所有（ヒープ割当て） */

    /* 各ルールのマージ済み設定（子優先でフルマージ済み） */
    leuko_config_t *effective_config; /* 所有（配下の rule 設定を含む） */
    bool effective_config_from_arena; /* true if effective_config allocated in arena (do not free individually) */

    /* 使用された設定ファイルの一覧（解析履歴／診断用） */
    char **source_files;
    size_t source_files_count;

    /* メモリ管理 / キャッシュ用 */
    struct leuko_arena *arena; /* 短寿命割当用アリーナ（所有） */
    int refcount;              /* 参照カウント */
} leuko_compiled_config_t;

/* API */
leuko_compiled_config_t *leuko_compiled_config_build(const char *dir,
                                                     const leuko_compiled_config_t *parent);
void leuko_compiled_config_ref(leuko_compiled_config_t *cfg);
void leuko_compiled_config_unref(leuko_compiled_config_t *cfg);
const leuko_node_t *leuko_compiled_config_merged_node(const leuko_compiled_config_t *cfg);
const leuko_config_t *leuko_compiled_config_rules(const leuko_compiled_config_t *cfg);

/* Accessors to avoid pulling large deps in tests */
size_t leuko_compiled_config_general_include_count(const leuko_compiled_config_t *cfg);
const char *leuko_compiled_config_general_include_at(const leuko_compiled_config_t *cfg, size_t idx);

/* Provide typed access to general config without pulling heavy headers */
#include "configs/common/general_config.h"
#include "configs/common/category_config.h"

const leuko_config_general_t *leuko_compiled_config_general(const leuko_compiled_config_t *cfg);
const leuko_config_category_base_t *leuko_compiled_config_get_category(const leuko_compiled_config_t *cfg, const char *name);

/* Access a rule view pointer (base+specific) from the generated static view for testing or convenience. */
void *leuko_compiled_config_view_rule(const leuko_compiled_config_t *cfg, const char *category, const char *rule_name);

/* Category accessors */
size_t leuko_compiled_config_category_include_count(const leuko_compiled_config_t *cfg, const char *category);
const char *leuko_compiled_config_category_include_at(const leuko_compiled_config_t *cfg, const char *category, size_t idx);

bool leuko_compiled_config_matches_dir(const leuko_compiled_config_t *cfg, const char *path);

#endif /* LEUKO_CONFIGS_COMPILED_CONFIG_H */
