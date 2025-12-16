
# TODO: RULES_LIST マクロ拡張 (カテゴリ / 短縮名 の導入) ✅

## 概要
- 現状の `fullname` を実行時に '/' で分割する実装をやめ、**ルール登録時点で** `category` / `rule_name` (短名) / `full_name` を保持する。
- 目的: 安全性向上、パフォーマンス改善、可読性・テスト性の向上。

## 高レベル設計
1. RULES_LIST の X-macro を拡張して、各エントリに `category` と `rule_name` (短名) を含める（例: X(field, category, rule_name, full_name, rule_ptr, ops_ptr)）。
2. `rule_registry_entry_t` に `const char *category_name` と `const char *short_name` を追加する。
3. レジストリ生成コードで静的文字列として埋める（malloc不要）。
4. `loader.c` 等の callsite では `entry->category_name` / `entry->short_name` を参照して探索する（`strchr` / `strndup` を削除）。

## 移行手順（段階的）
1. マクロ定義の拡張（設計完了） ✅
2. `rule_registry.c` の生成ロジックを更新して新フィールドを埋める
3. `include/rules` / `rules` の RULES_LIST を順次更新
4. `loader.c` の分割ロジックを削除して新フィールド参照に置換
5. ユニットテスト（レジストリ検証・loader 統合テスト）を追加
6. フルテストスイートを実行し回帰を修正
7. ドキュメントと CHANGELOG を更新
8. 互換ラッパーが必要なら追加して段階的に削除

## テストケース（必須）
- レジストリの各エントリが `category` / `short_name` / `fullname` を持つことを確認する単体テスト
- Loader の統合テスト（カテゴリノード内の rule ノードを見つけられること、トップレベル fullname による指定が動作すること）
- 既存の edge-case テストの再実行（回帰がないこと）

## 注意点
- RuboCop の互換は「1階層のみ」を前提とする（`Category/Rule`）。
- マクロ変更は一斉にビルドエラーを引き起こすため、段階的かつ注意深く適用する。

---

作業を進めてよければ、次に `RULES_LIST` の具体的なマクロ行形式と `rule_registry_entry_t` の差分を用意します。

## 実装状況（更新）
- 実装済み ✅
  - `RULES_LIST` の拡張（カテゴリ文字列と短縮名を含めるように変更）
  - `rule_registry_entry_t` に `category_name` / `short_name` を追加
  - `rule_registry` の静的生成を更新（`rule_name` は `FULLNAME(category, short)` で構築）
  - `loader.c` の runtime split ロジックを削除し `entry->category_name` / `entry->short_name` を参照するように変更
  - ユニットテスト: レジストリ検証 `tests/test_rule_registry.c` を追加
  - 統合テスト: `tests/test_config_loader.c` / `tests/test_loader_fullname.c` によりカテゴリ指定と fullname 指定の双方を確認

- 残タスク（次）
  - ドキュメントと CHANGELOG を更新する
  - 互換ラッパーを検討（必要に応じて追加）
