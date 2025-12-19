(The file `/workspaces/leukocyte/.github/TODO.md` exists, but is empty)
# TODO: ProcessedSource 設計メモ (行配列・単一ルックアップ API)

## 概要
- 目的: RuboCop の `ProcessedSource` 相当の高速で確実な行/列/行先頭判定 API を C 側で提供する。
- 背景: 既存の実装は改行オフセット配列と二分探索を利用しているが、ルール側で同一位置に対して複数回 bsearch を行ってしまい余計なコストが発生する。

## 設計目標
- RuboCop と同等の判定（pos→(line, col, begins_its_line)）を提供する。
- ルール側の問い合わせを「1 回のルックアップ」で満たし、bsearch の重複を削減する。
- メモリ増は許容範囲（行数に比例した O(L)）に抑える。
- 既存 API との互換性を維持しつつ新 API を追加する。

## データ構造（`processed_source_t` 拡張案）
- 既存: `const pm_newline_list_t *newline_list; const uint8_t *start; const uint8_t *end; int32_t start_line; size_t lines_count;`
- 追加: `size_t *first_non_ws_offsets; /* 行ごとの最初の非空白オフセット */`
- 追加（オプション）: `size_t *line_starts; /* newline_list->offsets を直接参照しても良い */`
- キャッシュ: `size_t last_idx;`（最近使った行インデックス、初期値 = SIZE_MAX）

## API（提案）
- 新構造体:
	```c
	typedef struct {
			int32_t line;     /* 1-based */
			size_t col;       /* byte-based column */
			bool begins;      /* pos がその行の最初の非空白文字か */
	} processed_source_pos_info_t;
	```
- 新関数:
	- `void processed_source_init_from_parser(processed_source_t *ps, const pm_parser_t *parser);`
		- 既存に加え `first_non_ws_offsets` を一回計算して保持する（O(L)）。
	- `void processed_source_pos_info(const processed_source_t *ps, const uint8_t *pos, processed_source_pos_info_t *out);`
		- 1 回の bsearch（またはキャッシュヒットで O(1)）で line/col/begins を決定する。
	- `bool processed_source_begins_its_line(const processed_source_t *ps, const uint8_t *pos);`
		- 既存の互換関数は内部で新 API を呼ぶ実装にして後方互換性を保つ。

## 実装手順（優先順）
1. `processed_source_init_from_parser` に `first_non_ws_offsets` の計算を追加。まずは ASCII のスペース/タブのみを扱う。
2. `processed_source_pos_info` を実装（bsearch → line index → line_begin → col → begins = (offset == first_non_ws[line_index])）。
3. `processed_source_begins_its_line` を新 API 呼び出しで実装し置換。
4. IndentationConsistency ルールを `processed_source_pos_info` を使うよう変更（col と begins を 1 回で取得）。
5. 単体テスト追加・ビルド・既存テスト実行。
6. ベンチ実行（既存 bench スイート）で速度差を測定。

## テスト計画
- `tests/test_processed_source.c` を拡張:
	- 行先頭が空白/タブで始まるケースで `begins` の期待値を検証。
	- 中間位置や同一行で複数トークンがあるケースで `begins=false` を確認。
	- `processed_source_pos_info` の line/col の正しさを検証。

## ベンチ計画
- before/after 比較:
	- 既存ベンチ群（bench_5k..200k）を使用。
	- 着目指標: parse_ms, visit_ms, handlers_ms（特に visit/handlers の改善）
	- 比較パターン: (a) 現状、(b) first_non_ws + pos_info、(c) + last_idx キャッシュ

## 受け入れ基準
- 単体テストが全て通ること。
- IndentationConsistency の挙動が RuboCop と一致すること（テスト含む）。
- visit/handlers の合計時間が現状より有意に改善すること（目標: 2×以上、理想的にはさらに大きい改善）。

## リスクと将来対応
- Unicode 表示幅（display-width）対応は別タスク（今はバイト単位の列）。
- メモリが気になる場合は `first_non_ws_offsets` を lazy init（必要なときだけ計算）にするオプションを検討。

---

## タスク（短期）
- [ ] `processed_source_pos_info` 実装
- [ ] テスト追加・修正
- [ ] IndentationConsistency を新 API に切り替え
- [ ] ベンチを回してレポート作成

## 備考
- 実装は段階的に行い、各段階でベンチとテストを回しながら進めること。
