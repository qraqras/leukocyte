# Leukocyte

## お願い
- [MUST]特に指定がない限り日本語で回答してください
- [MUST]コードの修正を実行する前に必ず実装方針を示して了承を得てください
- [MUST]vendorのコードは修正しないでください
- [MUST]勝手にgit操作をしないでください
- [MUST]RuboCopとのAPI互換性を最優先してください(特にRuboCopに存在しない機能は追加しないでください)
- [SHOULD]処理の流れはなるべく図示してください
- [SHOULD]RuboCopとの比較ベンチマークはキャッシュOFFかつLayout/IndentationConsistencyのみで実行してください

## コーディング規約
- [MUST]`if`や`for`などの制御構文のブロックは必ずブレース `{}` で囲むこと
- [MUST]コメントは必ず`/* */`で囲むこと

## タスク
- configファイルの読み取り
  - [MUST]YAMLファイルの読み取りエラー時はRuboCop同様、診断は出力しません。単一のエラーメッセージのみを出力します。

## 用語対応表
このプロジェクトではRuboCopの用語を以下のように置き換えています:
| RuboCop     | Leukocyte  |
| ----------- | ---------- |
| Cop         | Rule       |
| Department  | Category   |
| Offense     | Diagnostic |
| AutoCorrect | QuickFix   |

## プロジェクトの全体像
### プロジェクト概要
このプロジェクトの概要は以下の通りです:
- RuboCopのすべてのLayoutといくつかの主要なLintをC99で再実装します
- RuboCopの小規模互換としてユーザへのフィードバックを高速化することが最大の目的です
- RuboCopのすべての機能を再実装することが目的ではありません
- RuboCopへの継続的な追従のために、機能はできるだけ少ないほうが良いと考えています
- 関心の中心はLayoutであり、Lintは主要なものに限定して提供します
- RuboCopとのAPI互換は必須です(.rubocop.ymlの設定や出力フォーマットなど)
- RuboCopの設計や実装について互換性を保つ必要はありません(Copのロジックは踏襲します)
- Ruffのような開発体験を提供するためにLSPの一部の機能を実装します(Diagnostics、Formatting、CodeActionsのみ)
- CLIツールとして開発します(gemでの提供は将来的に検討します)
### 数値目標
- 5000行までのファイルについてRuboCopのサーバモードと比較して少なくとも40倍高速であることを目指します
- LayoutはRuboCopの実行結果と100%同じ結果になることを期待します
### 開発方針
このプロジェクトの開発における方針は以下の通りです:
- 優先順位は以下の通りです
  1. RuboCopとのAPI互換性(既存のRuboCopユーザを取り込みたい)
  2. RuboCop独特の競合解決のロジック(パフォーマンスを犠牲にしても再現する必要があります)
  3. パフォーマンス(パフォーマンスは最大の優位性であるので非常に重要です)
  4. LSP実装(Diagnostics、Formatting、CodeActionsのみで、残りはRubyLSPに任せます)(Ruffのような開発体験を提供するために非常に重要です)
  5. RuboCopの設計や実装の踏襲(まったく重要ではありません)
- 基本的な設計はRuffを参考にします(RuboCopは局所的に参照する程度にとどめます)
- パーサはRuby純正のPrismを使用します
