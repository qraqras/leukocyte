# Leukocyte ルール呼び出し設計

## 概要
RuboCopのCopをC言語で再現。Ruleを構造体で定義し、AST走査時にノードタイプごとにチェックを実行。Layout/Lint Categoryをサポートし、動的に有効/無効を設定可能。

## ディレクトリ構成
```
src/
├── main.c
├── rules/
│   ├── layout/
│   │   ├── indentation.c      # Layout/Indentation ルール
│   │   ├── line_length.c      # Layout/LineLength ルール
│   │   └── ... (他のLayoutルール)
│   ├── lint/
│   │   ├── syntax_error.c     # Lint/SyntaxError ルール
│   │   └── ... (他のLintルール)
│   ├── rule_manager.c         # ルール管理（初期化、収集）
│   └── rule.h                 # 共通ヘッダー
include/
├── leukocyte.h
└── rules/
    └── rule.h                 # ルール構造体定義
```

## Rule構造体
```c
typedef enum {
    CATEGORY_LAYOUT,
    CATEGORY_LINT,
} rule_category_t;

typedef struct {
    const char *name;  // Rule名（例: "Layout/Indentation"）
    rule_category_t category;  // Category
    bool enabled;  // 有効フラグ
    bool (*handlers[PM_NODE_TYPE_COUNT])(pm_node_t *node, pm_parser_t *parser, pm_diagnostic_list_t *diagnostics);  // ノードタイプごとのハンドラ
} rule_t;
```

## ビルド時の収集
- Ruleを定義し、ノードタイプごとにリスト化（rules_by_type[PM_DEF_NODE] = [有効Rule]）。
- 最適化: 無効Ruleは除外。

## 設定読み込み
- 設定ファイル（JSON/INI相当）からCategory単位と個別Ruleの有効/無効を読み込み。
- 例: `Layout: true` で全Layout Rule有効、個別設定で上書き。

## AST走査
- `visit_node` 関数で再帰走査。
- ノードタイプを取得し、rules_by_type[type] の有効Ruleを呼び出し。
- 違反があればDiagnosticリストに追加。

## 実装ステップ
1. ディレクトリ作成: `mkdir -p src/rules/layout src/rules/lint include/rules`
2. rule.h 作成（Rule構造体定義）。
3. rule_manager.c 作成（Rule定義、初期化関数）。
4. 設定読み込み関数実装。
5. main.c 更新（パース後に走査呼び出し）。
6. Layout Rule追加（例: Indentationチェック）。
7. CMakeLists.txt 更新（rules/ 追加）。
8. テスト実行。

## 注意点
- Prismのpm_node_type_t を使用。
- パフォーマンス重視で、無効Ruleをスキップ。
- 拡張性: 新Rule追加でhandlers配列を更新。
