# 推奨ディレクトリ構成 (高レベル)

/src
  /cli/                ← CLI 実装・コマンドライン処理
  /common/             ← 共有コンポーネント（ルールレジストリ等）
  /configs/            ← 設定
    /common/           ← 設定共通ヘルパ
    /layout/
    /lint/
  /rules/              ← ルール実装（カテゴリ別フォルダ）
    /common/           ← ルール共通ヘルパ
    /layout/
    /lint/
  /sources/            ← ソース読み取り・収集の責務 (旧 `files`)
    /ruby/             ← Ruby ソースの収集・スキャン
    /yaml/             ← YAML の読み込み / merge / inherit 解決
  /util/               ← 汎用ユーティリティ
    /allocator/

include/
  (モジュール毎の公開ヘッダを整理: include/ruby, include/yaml, include/configs, ...)
vendor/
test/
  /unit/
  /integration/
bench/
cmake/                 ← CMake helper, モジュール用 CMake 設定
