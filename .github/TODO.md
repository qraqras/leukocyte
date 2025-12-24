# TODO / 設計メモ（現行設計）

## 目的 🎯
- RuboCop 互換のファイル収集を実現するため、**ディレクトリを再帰的に降下しながら**各ディレクトリの `.rubocop.yml` をフルマージして、その場で Include/Exclude を反映して Ruby ファイルを収集する。

---

## 要旨（短く）
- 探索はディレクトリ再帰（DFS）で降下し、**親の compiled-config を基に子ディレクトリの設定を親→子で deep merge して即時確定**する。
- 確定した `Exclude/Include` に基づきファイル収集を行い、**Exclude によるサブツリー剪定（降下停止）**を行う。
- YAML の `inherit_from` は再帰的に解決し、ローカル参照は参照元を基準に解く。URL はフェッチ（タイムアウト/制限）して取り込む。

---

## 詳細フロー（ステップ）🔁
1. **入力正規化**: CLI paths を正規化（絶対化、スラッシュ統一、シンボリックの扱い方等）。
2. **ディレクトリ再帰降下（DFS）**:
   - 現在ノード（ディレクトリ）に入る前に親の `compiled_config` を取得。
   - 当該ディレクトリに `.rubocop.yml` があれば読み込み・ `inherit_from` を解決して親→子で **フルマージ**（逐次）を行い `compiled_config` を生成・キャッシュ。
   - 生成した `compiled_config` の `Exclude/Include` をプリコンパイル（glob/regex）。
   - もしディレクトリ自体が `Exclude` にマッチするなら、そのサブツリーの降下を停止（剪定）。
   - そうでなければ、当該ディレクトリ内のファイルを `compiled_config` に基づいて収集（Ruby 判定）する。
3. **キャッシュ & インクリメンタル**:
   - 各 `compiled_config` は `mtime + content-hash + inherit fingerprint` をキーにキャッシュ。
   - 変更検知時は影響ノードのみトポロジカルに再生成する。
4. **エラー処理（即時終了）**:
   - 破損 YAML / URL 取得失敗 / 循環が発生した場合は、処理を**即時終了（エラー返却）**します。RuboCop と同様に設定の診断を出力せず、失敗時は中断する方針です。
   - 将来的に寛容な動作（例: `--tolerate-config-errors`）をオプションで追加することは可能です。

---

## データモデル（簡潔）🧩
- `leuko_config_raw_t`: path, mtime, parsed-node
- `leuko_config_compiled_t`: dir, fingerprint, compiled_excludes[], arena, refcount, is_full
- キャッシュ: `map<dir, leuko_config_compiled_t*>`

---

## 実装上の注意点 / 制約⚠️
- **逐次フルマージ**はまず確実性優先で実装（単一スレッド）。必要になれば依存グラフ単位で並列化する。
- `inherit_from` の URL フェッチは並列に出来るが、タイムアウトと並列上限（デフォルト 5s / 8）を設ける。
- パターンの意味（basename vs path）や `inherit_mode` の挙動は RuboCop と同等になるよう細かく確認・テストする必要がある。

---

## テスト要件（必須）✅
- 親→子優先の正当性（子が近接で優先される）
- `inherit_from` の再帰・循環・リモート失敗時に処理を**即時終了**すること
- `inherit_mode` による配列マージ（Exclude の結合/上書き）
- ディレクトリ剪定（Exclude による降下停止）の確認
- 大規模ツリーでの性能（必要に応じてベンチ）

---

## 優先タスク（着手順）
1. 最小プロトタイプ: ディレクトリ降下 walker + 親→子での逐次フルマージ + Exclude による剪定 + 単体/統合テスト
2. 完全実装: `inherit_from` の URL と相対解決、`inherit_mode` の正確な合成、エラー処理の確立（即時終了、将来的に寛容モードのオプション）
3. 最適化: YAML 並列パース（dedupe or promise）、ネット制御、ベンチ

---

必要なら、この TODO をもとに最初のプロトタイプを実装します。実装を開始して良ければ「進めて」とだけ返してください。
