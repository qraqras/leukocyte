#ifndef LEUKO_CONFIGS_COMPILED_CONFIG_H
#define LEUKO_CONFIGS_COMPILED_CONFIG_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <yaml.h>
/* forward declare leuko_config_t to avoid heavy includes in header */
typedef struct leuko_config_s leuko_config_t;
#include "utils/allocator/arena.h" /* leuko_arena */

typedef struct leuko_compiled_config_s
{
    char *dir;            /* 所有: 絶対ディレクトリパス */
    uint64_t fingerprint; /* mtime+content+parent の簡易ハッシュ */

    /* AllCops レベルのマージ結果 */
    char **all_include; /* fnmatch 用パターン配列（所有） */
    size_t all_include_count;
    char **all_exclude;
    size_t all_exclude_count;

    /* ディレクトリ単位で有効な include/exclude（compiled） */
    char **include;
    size_t include_count;
    char **exclude;
    size_t exclude_count;

    /* 各ルールのマージ済み設定（子優先でフルマージ済み） */
    leuko_config_t *rules_config; /* 所有（配下の rule 設定を含む） */
    bool rules_config_from_arena; /* true if rules_config allocated in arena (do not free individually) */

    /* マージ済み libyaml ドキュメント */
    yaml_document_t *merged_doc; /* 所有（arena で割当） */

    /* 使用された設定ファイルの一覧（解析履歴／診断用） */
    char **source_files;
    size_t source_files_count;

    /* メモリ管理 / キャッシュ用 */
    struct leuko_arena *arena; /* 短寿命割当用アリーナ（所有） */
    int refcount;              /* 参照カウント */

    unsigned int flags; /* 将来の拡張用 */
} leuko_compiled_config_t;

/* API */
leuko_compiled_config_t *leuko_compiled_config_build(const char *dir,
                                                     const leuko_compiled_config_t *parent);
void leuko_compiled_config_ref(leuko_compiled_config_t *cfg);
void leuko_compiled_config_unref(leuko_compiled_config_t *cfg);
const yaml_document_t *leuko_compiled_config_merged_doc(const leuko_compiled_config_t *cfg);
const leuko_config_t *leuko_compiled_config_rules(const leuko_compiled_config_t *cfg);

/* Accessors to avoid pulling large deps in tests */
size_t leuko_compiled_config_all_include_count(const leuko_compiled_config_t *cfg);
const char *leuko_compiled_config_all_include_at(const leuko_compiled_config_t *cfg, size_t idx);

bool leuko_compiled_config_matches_dir(const leuko_compiled_config_t *cfg, const char *path);

#endif /* LEUKO_CONFIGS_COMPILED_CONFIG_H */
