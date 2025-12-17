# TODO — メモリ・性能調査結果と次のアクション ✅

## 調査の背景
- 目的: Leuko のスケーラビリティ問題（ファイルサイズに比例した線形スケール）の原因を特定するためにベンチとプロファイリングを実行。
- 実施した計測: ベンチ (tests/bench/bench_*.rb)、gprof（CPU）、LD_PRELOAD ベースの簡易 malloc プロファイラ、環境変数有効化によるモジュール別割当計測（`PRISM_ALLOC_STATS=1`）。

---

## 主要な発見 🔍
- **字句解析 (parser_lex など) が大きな割当を生成**: `prism` 名義の計測で `callocs=519,994`、`bytes_alloc ≈ 48.64 MB` が観測され、字句解析側で多数の短命な割当が発生している。
- **pm_constant_pool が大量の挿入を処理**: `allocs = 40,001`、`bytes_alloc ≈ 2.68 MB`。トークン／名前の挿入と関係あり。
- **pm_newline_list が大きなメモリ塊を確保**: `allocs = 4`（少回数）だが `bytes_alloc ≈ 5.76 MB` と、行数に依存した大きな確保が発生。
- **全体概観（簡易 malloc プロファイラ）**: `total_alloc_calls ≈ 240k`, `total_calloc_calls ≈ 680k`, `peak ≈ 47.6 MB` — 小さな割当が大量に発生している。

---

## 解釈（短く）
- ファイル長に線形スケールする主因は「字句解析がバイトごと／トークンごとに短命なメモリ割当を大量に行っていること」と「行数に比例して大きな配列を確保する newline list の挙動」の組合せです。

---

## 優先度付き推奨アクション（次のステップ） 🔧
1. **アリーナ（region allocator）を導入する PoC（優先度: 高）**
   - 目的: トークンや一時オブジェクトの割当をまとめて行い、個別割当（calloc/malloc）回数を大幅削減する。字句解析部分に限定した軽量アリーナを先に作る。
   - 検証: PoC を導入して `tests/bench/bench_200000.rb` の `PRISM_ALLOC_STATS` と malloc_profiler の結果で割当回数とメモリピークの改善を確認。

2. **pm_newline_list の容量戦略改善（優先度: 中）**
   - 目的: 行数に応じた初期容量推定（または成長率の見直し）で大きな再確保を削減する。
   - 検証: 初期容量推定を入れたベンチで bytes_alloc / realloc 回数を比較。

3. **字句解析の fast-path とコピー削減（優先度: 中）**
   - 目的: ASCII fast-path、名前コピーの参照化（可能な場合）などで1割〜数十％の改善を目指す。
   - 検証: `lex_identifier` や `parser_lex` の最もホットなコードを微最適化した上でベンチ実行。

4. **pm_constant_pool のコスト改善（優先度: 中）**
   - 目的: ハッシュテーブルの衝突処理や resize のコスト、頻繁な所有権切り替えに伴うコピー/解放コストを減らす。
   - 検証: 挿入のホットパスを最適化し、alloc/bytes を再測定。

5. **継続的なベンチ & プロファイル**
   - ベンチ (1KB–1MB) を継続的に実行し、各改修の効果を数値で比較する。プロファイラ（gprof / malloc_profiler / PRISM_ALLOC_STATS）を回して差分を確認。

---

## 小さな短期タスク（実装しやすい）
- [ ] `pm_newline_list` の初期 capacity をファイル長（概算行数）から推定するロジックを追加して効果確認。
- [ ] `PRISM_ALLOC_STATS` の出力を CI のベンチジョブに組み込む（回帰検出）
- [ ] `malloc_profiler.so` をリポジトリ内 tools に整理し、再利用できるようにドキュメント化する。

---

## メモ（実験時のコマンド）
- ベンチ実行: `python3 tests/bench/run_scaling.py`（既存スクリプト）
- 単体ベンチ: `PRISM_ALLOC_STATS=1 LD_PRELOAD=build/malloc_profiler.so build_gprof/leuko --timings tests/bench/bench_200000.rb`
- gprof ビルド: `cmake -S . -B build_gprof -DENABLE_GPROF=ON && cmake --build build_gprof -j` → 実行で `gmon.out` を生成 → `gprof build_gprof/leuko gmon.out > gprof.txt`

---

必要であれば、上記タスク（特に PoC のアリーナ導入）について、実装方針（コードの差分）を提示してから実装に移ります。どれを優先して着手しましょうか？
