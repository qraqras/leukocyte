# Leukocyte

## お願い
- [MUST]特に指定がない限り日本語で回答してください
- [MUST]コードの修正を実行する前に必ず実装方針を示して了承を得てください
- [MUST]`vendor/`のコードは修正しないでください
- [MUST]RuboCopとのAPI互換性を最優先してください(特にRuboCopに存在しない機能は追加しないでください)
- [SHOULD]RuboCopとの比較ベンチマークはキャッシュOFFかつLayout/IndentationConsistencyのみで実行してください

## コーディング規約
- [MUST]プロジェクトの接頭辞として`leuko_`を使用すること(内部関数/変数も含む)
- [MUST]`if`や`for`などの制御構文のブロックは必ず`{}` で囲むこと
- [MUST]コメントは必ず`/* */`で囲むこと
- [SHOULD]テストは仕様が固まるまで作成しないでください

## タスク
- configファイルの読み取り
  - 処理フロー
    - 全Rubyファイルをまとめて収集
    - 各Rubyファイルに対応するYAMLファイルをまとめて収集
    - YAMLファイルの親子関係を解決
    - YAMLファイルを一意にして並列パース
    - YAMLファイルをマージ
    - configに適用
  - 方針
    - キャッシュは後で


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
- このプロジェクトはRuboCopのLayout系CopをC++23で再実装することを目指します
- このプロジェクトはRuboCop互換でありながら100倍の高速化を目指します(.rubocop.ymlや出力フォーマットもRuboCop互換にします)
- このプロジェクトはRuboCopのすべての機能を再現することが目的ではありません
- このプロジェクトはRuboCopへ追従する必要があるので必要最小限の機能に限定します
- このプロジェクトの関心の中心はLayout系のCopです(その他のCopも将来的に提供可能な設計にします)
- このプロジェクトはCLIツールとして開発されます(将来的にRubyGemsとして提供します)
### 開発方針
このプロジェクトの開発における方針は以下の通りです:
- RubyのパーサはPrismを使用します
- 優先順位は以下の通りです
  1. RuboCopとの互換性(既存のRuboCopユーザを取り込みたい)
  2. 性能(速度、メモリ使用量)(FormatOnSaveでの使用を想定しているため非常に重要です)
  3. LSP実装(Diagnostics、Formatting、CodeActionsのみで、残りはRubyLSPに任せます)(Ruffのような開発体験を提供するために非常に重要です)
  4. RuboCopの設計や実装の踏襲(まったく重要ではありません)
