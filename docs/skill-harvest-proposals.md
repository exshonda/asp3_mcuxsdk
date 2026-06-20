# 汎用skill 提案メモ（toppers-skills キュレーター宛・push しない）

asp3_mcuxsdk の棚卸し（`toppers-skills/docs/harvest-prompt.md`）で抽出した**汎用候補**。
分担の正本は `toppers-skills/AGENTS.md`。本リポジトリの固有知見は
`.claude/skills/asp3-mcuxsdk-ops/` と `.claude/skills/porting-asp3-to-nxp/` に置いた。

> 取り込み時は本文を実装非依存に保ち、注記として
> `〔分類: … ｜ プロセッサ: … ｜ 出所: asp3_mcuxsdk〕` を付ける（既存skillの書式に合わせる）。

## B. 追記候補（未収載と確認したもの・2件）

### B1. HRT の計数クロックは正確な 1MHz/1µs にならないことがある

- **対象skill**: `toppers-kernel-dev`（§4 移植のタイマ設定の注意） … または `toppers-kernel-debug` §5
- **追記案の要旨**: HRT の計数クロックをベンダ固定のクロックツリーの整数分周で作る場合、
  正確な 1µs（1MHz）ティックが得られず小さな単調ドリフト（ppm オーダ）が残ることがある。
  実用上は許容できることが多いが「タイマは正確に 1MHz」を前提にせず、誤差を実測して
  文書化する。整数分周で 1MHz を作れる固定発振器を選べれば誤差ゼロにできる。
- **分類**: 時間分割/タイマ・制約/アーキ構造
- **プロセッサ**: 非依存（timer-as-HRT 全般）
- **出所リポ**: asp3_mcuxsdk（同SoC系の2ボードで +約400ppm と 誤差0 の対比が出た）
- **実装非依存の根拠**: クロック分周の一般的性質。固有レジスタ名・周波数値・API を本文に書かない。

### B2. 機能クロック未供給のペリフェラルは初期化時に無音ハングする

- **対象skill**: `toppers-kernel-debug` … `references/debugging-playbook.md` §2（起動が進まない症状の原因リストへ1項目）
- **追記案の要旨**: 機能クロックやそのディバイダが既定で停止/HALT のペリフェラルでは、
  ドライバが最初のレジスタ書込みをした瞬間にバスがストールしてハングし、カーネル起動が
  無音のまま止まることがある。症状「ごく初期で無出力のまま停止」の原因候補に
  「初期化対象ペリフェラルの機能クロックが供給されていない（クロックゲート/ディバイダ停止）」
  を加える。ドライバ初期化の前にクロック供給を有効化する。
- **分類**: 観測/デバッガ・制約/アーキ構造
- **プロセッサ**: 非依存（多くの組込みSoC。近年の ARM-M SoC で顕在化しやすい）
- **出所リポ**: asp3_mcuxsdk
- **実装非依存の根拠**: クロックゲーティング一般の症状。固有API・レジスタ名・SoC名を本文に書かない。

### B3.（任意・スコープ要判断）GUI生成非依存のSDKベンダリングでCIビルド可能化

- **対象skill**: 既存3skillに明確な該当なし（`toppers-kernel-dev` の移植/統合補足、または
  新規「SDK統合パターン」メモの候補＝キュレーター判断）
- **追記案の要旨**: ベンダSDKが manifest/west 等の取得方式でも、必要部分だけをコミット/
  ピン留めして直参照すれば、GUI生成ツール非依存で素のクローン直後にビルドでき、CIでも
  ビルドできる。生成系のボード初期化コードはコミットして再生成不要にする。
- **分類**: 統合/CI
- **プロセッサ**: 非依存
- **出所リポ**: asp3_mcuxsdk（pico/fsp/stm32 統合と同方針＝複数リポで再現する整合パターン）
- **実装非依存の根拠**: SDK統合の方針。固有のSDK名・コマンド・パスを本文に書かない。

## C. 重複のため提案しなかったもの（既に toppers-skills に収載済み）

棚卸しで grep 確認した結果、以下は**既に汎用skillに入っている**（多くは出所 asp3_mcuxsdk と
明記済み＝過去の棚卸しで取り込み済み）。再提案しない。

| 知見 | 既存の収載先 |
|---|---|
| ブートのSecure状態とEXC_RETURN不整合→最初のディスパッチで即フォルト（移植時はブート経路を先に確認） | `toppers-kernel-debug` debugging-playbook.md（項6） |
| cfg生成ベクタがベンダldで orphan→配置エラー／VTOR 整列要件 | `toppers-kernel-dev` SKILL.md（出所 asp3_mcuxsdk 明記） |
| 走行中システムはリセットなしの attach で観測する | `toppers-kernel-debug` observing-execution.md |
| シリアルの単独リーダ規律（複数リーダはバイト奪い合い→欠落/文字化け/無音） | `toppers-kernel-debug` observing-execution.md（出所 asp3_mcuxsdk 明記） |
| サイクルカウンタ時間源（Cortex-M=DWT／A=PMU／RISC-V=cycle CSR）でns精度計測 | `toppers-kernel-debug` performance-measurement.md |
| 終了処理が送信FIFOドレインを待たずシリアル停止→最後の出力が落ちる | `toppers-kernel-debug` observing-execution.md |
| 二大禁則・静的生成のみ・ビルド検証の鉄則 | `toppers-kernel-dev` SKILL.md／各リポ CLAUDE.md |

## 固有（自リポに置いた・汎用化しない）

- 2ボードの build/flash/test/perf の具体手順・プリセット名・LinkServer device名・
  ASP3ターゲット名・testexec ラッパ起動・デバッガ切替オプション・devices-mcx 初期化・
  CI範囲 → `.claude/skills/asp3-mcuxsdk-ops/`
- 新規NXPボード移植手順・ブート/ベクタ落とし穴の具体・SDK取得手順・MCXN947固有地雷
  （CTIMERディバイダHALT・-mcmse） → `.claude/skills/porting-asp3-to-nxp/` ＋ `docs/`
