
# 現在の設計と残タスク

このファイルは現在の設計要点と、今後着手すべきタスクを簡潔にまとめた TODO です。

## 概要
- ルール一覧は X-macro(`RULES_LIST`, `include/rules_list.h`) で一元管理。
- 各ルールは `rule_t`（`rules/rule.h`）で定義され、`rule_registry` は X-macro から生成される。
- ルールごとの設定（`rule_config_t`）は `config_ops`（`initialize` と `apply_yaml`）で生成/適用される。
- `config_t`（集約設定構造体）は RULES_LIST の第1引数（フィールド識別子）から自動生成され、`config_init_defaults()` / `config_free()` を通じて初期化・解放される。

## 設計の重要点
- X-macro を単一の真実源にして、ルール・設定・レジストリを自動生成している。
- `config_ops` の責務は ①デフォルト初期化（initialize） ②YAML 適用（apply_yaml） の二つに限定。
- YAML 適用は Document モード（`yaml_document_t` / `yaml_node_t`）方式を採用予定。理由: 実装容易性、テスト性、RuboCop 構成の複雑さへの対応。
- 中央（loader）で**共通キー（enabled/severity/include/exclude）をマージして適用**し、ルール固有の複雑なマージのみルールに委譲する方針。

## 実装済み（主な変更）
- `include/configs/config_ops.h`: `initialize` / `apply` シグネチャ設計
- `include/configs/rules_list.h` / `src/rule_registry.c`：RULES_LIST による rule_registry 生成
- `include/configs/generated_config.h`, `src/configs/generated_config.c`: `config_t` の自動生成、`config_init_defaults`/`config_free` 実装
- `src/configs/config_registry.c`: レジストリ経由でデフォルト config を生成する実装
- ルール `layout/indentation_consistency` の config ops 実装とテスト

## 優先残タスク（推奨順）
1. YAML ローダ実装 (`config_load_file` / `config_apply_document`) — Document モードで実装
2. 中央での共通キーのマージ機能実装とユーティリティ（`yaml_get_merged_*`）追加
3. `config_ops.apply_yaml` シグネチャを確定し、ルール側の `apply_yaml` を実装（例: `layout_indentation_consistency_apply_yaml`）
4. YAML 関連の単体テスト追加（AllCops / Category / Rule パターン、エラーケース）
5. `rule_registry` と `config_registry` の振る舞いを検証するユニットテスト（ops が正しく登録され動作することを検証）
6. ドキュメント整備（README に設定の書き方・apply_yaml の実装ガイドを追加）

## 仕様 Decisions / 方針まとめ
- YAML は当面 Document モード（`yaml_document_t` / `yaml_node_t`）を用いる。
- 優先順位: Rule > Category > AllCops（下位が上書き）
- 中央で共通キーを適用し、ルールは固有キーの解釈のみ行う。
- エラー: 構文エラーは loader が失敗、ルール固有のパースエラーは Diagnostic を追加して部分反映を許容。

---

もし上記で優先すべき項目や追記したい内容があれば指示ください。実装を開始する場合は該当タスクを着手します。
