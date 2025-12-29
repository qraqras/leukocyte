# Leukocyte

## お願い
- [MUST]特に指定がない限り日本語で回答してください
- [MUST]コードの修正を実行する前に必ず実装方針を示して了承を得てください
- [MUST]RuboCopとの互換性を最優先してください
- [MUST]このプロジェクトは未リリースなので後方互換は考慮しなくていいです
- [MUST]`vendor/`のコードは修正しないでください
- [SHOULD]RuboCopとの比較ベンチマークはキャッシュOFFかつlayout/indentation_consistencyのみで実行してください

## コーディング規約
- [MUST]プロジェクトの接頭辞として`leuko_`を使用すること(内部関数/変数も含む)
- [MUST]stdbool.hの`bool`型を使用すること
- [MUST]`if`や`for`などの制御構文のブロックは必ず`{}` で囲むこと
- [MUST]コメントは必ず`/* */`で囲むこと
- [SHOULD]テストは仕様が固まるまで作成しないでください

## タスク
- configファイルの読み取り


## 用語対応表
このプロジェクトではRuboCopの用語を以下のように置き換えています:
| RuboCop     | Leukocyte  |
| ----------- | ---------- |
| Cop         | Rule       |
| Department  | Category   |
| Offense     | Diagnostic |
| AutoCorrect | QuickFix   |

## 設定ファイルについての仕様
このプロジェクトは現時点でRuboCopの`.rubocop.yml`を直接解釈することはしません(将来的にはC99での直接解釈を目指します)。
なぜなら、`.rubocop.yml`の仕様が複雑で対応するには時間がかかり過ぎるからです。
そのため、現時点では.rubocop.ymlの解釈にRuboCop本体を使用し、RuboCop本体が解決した結果を独自のJSON形式に変換してから読み込むことにします。
あらかじめ`leuko --init`を実行して、`.leukocyte`ディレクトリを作成します。
`.leukocyte`ディレクトリが存在するディレクトリはLeukocyteのプロジェクトルートです。
また、RuboCopと設定を同期させるために`leuko --sync`を実行して、各ディレクトリの`.rubocop.yml`に対応する`.leukocyte.resolved.json`を`.leukocyte`ディレクトリに生成します。

## プロジェクトの全体像
### プロジェクト概要
このプロジェクトの概要は以下の通りです:
- このプロジェクトはRuboCopのLayout系CopをC99で再実装することを目指します
- このプロジェクトはRuboCop互換でありながら100倍の高速化を目指します(.rubocop.ymlは.leukocyte.jsonに変換して読み込みます)
- このプロジェクトはRuboCopのすべての機能を再現することが目的ではありません
- このプロジェクトはRuboCopへ追従する必要があるので必要最小限の機能に限定します
- このプロジェクトの関心の中心はLayout系のCopです(その他のCopも将来的に提供可能な設計にします)
- このプロジェクトはCLIツールとして開発されます(将来的にRubyGemsとして提供します)
- このプロジェクトはPOSIX互換システムで動作します(Windowsネイティブは将来的な対応を検討します)
### 開発方針
このプロジェクトの開発における方針は以下の通りです:
- RubyのパーサはPrismを使用します
- 優先順位は以下の通りです
  1. RuboCopとの互換性(既存のRuboCopユーザを取り込みたい)
  2. 性能(速度、メモリ使用量)(FormatOnSaveでの使用を想定しているため非常に重要です)
  3. LSP実装(Diagnostics、Formatting、CodeActionsのみで、残りはRubyLSPに任せます)(Ruffのような開発体験を提供するために非常に重要です)
  4. RuboCopの設計や実装の踏襲(まったく重要ではありません)
