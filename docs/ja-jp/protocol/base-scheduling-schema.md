---
order: 6
icon: material-symbols:view-quilt-rounded
---

# インフラスケジュール設定

この文書は機械翻訳です。もし可能であれば、中国語の文書を読んでください。もし誤りや修正の提案があれば、大変ありがたく思います。

`resource/custom_infrast/*.json` 各フィールドの設定方法と説明

::: tip
JSONファイルはコメントをサポートしていません。テキスト内のコメントはプレゼンテーション用にのみ使用されます。直接コピーして使用しないでください。
:::

[ビジュアルスケジューリング生成ツール](https://ark.yituliu.cn/tools/schedule)

## 完全なフィールドの一覧

```json
{
    "title": "サブ垢スケジュール",       // タイトル，オプション
    "description": "イロハニホヘト",      // 説明，オプション
    "plans": [
        {
            "name": "早朝組",        // プランタイトル，オプション
            "description": "lol",   // 説明，オプション
            "period": [             // シフト時間の間隔，オプション
                                    // 現在時刻がこの間隔内にある場合、プランが自動的に選択されます (jsonファイル全体が複数のプランを含む場合もあります)
                                    // このフィールドが存在しない場合は、実行のたびに自動的に次のプランに切り替わります。
                                    // core ではこのフィールドを扱わないので、maa を統合するためにインターフェイスを使用する場合は、このロジックを自分で実装してください。
                [
                    "22:00",        // 必須　記述のフォーマットは hh:mm， 日をまたぐ場合はこの例のように記述してください。
                    "23:59"
                ],
                [
                    "00:00",
                    "06:00"
                ]
            ],
            "duration": 360,        // 作業時間(分) 予約項目、未実装 将来的には、シフトを変更時間を告知させるポップアップ時間になるかも？ 自動的に変更される機能になる可能性もあり
            "Fiammetta": {          // “フィアメッタ” がどのオペレーターを配属するか、オプション、記入しない場合は配属しない。
                "enable": true,     // “フィアメッタ” を使うかな、オプション、デフォルト true
                "target": "シャマレ", // ターゲットオペレーター。ここではOCRを使用して行い、対応するクライアント言語のオペレーター名を入力する必要があります
                "order": "pre",     // 全シフト前またはシフト後に使用、オプション、引数 "pre" / "post", デフォルト "pre"
            },
            "drones": {             // ドローン使用、任意、未記入の場合はドローンを使用しない
                "enable": true,     // ドローンを使用するかどうか、オプション、デフォルト true
                "room": "trading",  // 使用する施設、引数 "trading" / "manufacturing"
                "index": 1,         // 施設番号、tab の番号に対応、引数 [1 - 5]
                "rule": "all",      // ルール、予約フィールド、未実装。後で使用する可能性がある。
                "order": "pre"      // オペレーターの交換前後の使用設定、オプション、引数 "pre" / "post", デフォルト "pre" (テキーラなど)
            },
            "groups":[              // "control" / "manufacture" / "trading" の場合、オペレーターグループを設定できます
                {
                    "name":"A",
                    "operators":[
                        "グム",
                        "シルバーアッシュ",
                        "メイ"
                    ]
                },
                {
                    "name":"B",
                    "operators":[
                        "セイリュウ",
                        "ユーネクテス",
                        "ウィーディ"
                    ]
                }
            ],
            "rooms": {              // 部屋情報，必須
                                    // 引数 "control" / "manufacture" / "trading" / "power" / "meeting" / "hire" / "dormitory" / "processing"
                                    // 1つもないということは、その施設ではシフト変更にデフォルトのアルゴリズムが使用されていることを意味します。
                                    // 部屋のシフトを変更しない場合は、skip を使用するか、タスク設定 - 基地仕事 - 基地設定 で該当施設のチェックを外すだけです。
                "control": [
                    {
                        "operators": [
                            "シー",   // OCRを使用してこれを行うには、対応するクライアント言語のオペレーター名を渡す必要があります。
                            "リィン",   // JP版なら日本語で入力しろってことですね。
                            "ケルシー",
                            "アーミヤ",
                            "ムリナール"
                        ]
                    }
                ],
                "manufacture": [
                    {
                        "operators": [
                            "フェン",
                            "シーン",
                            "クルース"
                        ],
                        "sort": false,  // ソートするかどうか（上の オペレーター の順番で）、オプション、デフォルトはfalse
                        // 例：シーン、パラス、シャマレ、などのオペレーター、そして "sort": false を使用すると、オペレーターの順序が乱れ、事前有効の効果が失われる可能性があります。
                        // "sort": true を使う、この問題を避けることができる
                    },
                    {
                        "skip": true    // 現在の施設をスキップするかどうか（配列番号に対応），オプション，デフォルトは false
                                        // true の場合、他のすべてのフィールドを空にすることができる。 オペレーター交代の操作だけが省略され、ドローンの使用や手がかりの交換など、他のことは今まで通り行われます。
                    },
                    {
                        "operators": [
                            "Castle-3"
                        ],
                        "autofill": true,   // 残った位置を自動的に埋めるためにオリジナルのアルゴリズムを使用、オプション、デフォルトは false
                                        // operators が空の場合、部屋はオリジナルのアルゴリズムで全体的にスケジュールされる
                                        // operators が空でない場合、施設全体の効率ではなく、単一のオペレータの効率のみで考慮されます。
                                        // 後でカスタムシステムと競合する可能性があることに注意してください。たとえば、後で必要な関数がここで使用される場合、注意するか、autofill を最後に置いてください
                        "product": "Battle Record"  // 現在の製造資材、オプション
                                                    // 現在の設備がジョブで設定された製品と一致しないことが確認された場合、インターフェースは赤いメッセージでポップアップ表示され、後でより便利になる可能性があります
                                                    // 使用可能引数： "Battle Record" | "Pure Gold" |  "Dualchip" | "Originium Shard" | "LMD" | "Orundum"
                    },
                    {
                        "operators": [
                            "ドロシー"
                        ],
                        "candidates": [ // サブオペレーター，オプション。埋まるまで、誰でも利用可能です
                                        // autofill=true との互換性はありません。空白でない場合、autofill は false である必要があります。
                            "アステジーニ",
                            "フィリオプシス",
                            "サイレンス"
                        ]
                    },
                    {
                        "use_operator_groups":true,  // グループ内の演算子のグループ化を使用するには true に設定します。デフォルトは false です。
                        "operators":[                // 有効にすると、演算子の名前がグループ名として解釈されます。
                            "A",                     // グループ化は気分の閾値と設定順序に従って選択されます
                            "B"                      // グループ A に気分がしきい値を下回るオペレーターがいる場合、グループ B が使用されます
                        ]
                    }
                ],
                "meeting": [
                    {
                        "autofill": true // 応接室は autofill
                    }
                ]
            }
        },
        {
            "name": "夜勤組"
            // ...
        }
    ]
}
```

## サンプル

[243 有効率が最も高い 一日三回](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/243_layout_3_times_a_day.json)

[153 有効率が最も高い 一日三回](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/153_layout_3_times_a_day.json)