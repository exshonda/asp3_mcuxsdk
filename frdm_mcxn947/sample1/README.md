# TOPPERS/ASP3 の VS Code 向け MCUXpresso 拡張機能対応（FRDM-MCXN947）

`frdm_mcxn947/sample1` を **MCUXpresso for VS Code** 拡張でビルド・書込み・デバッグする手順です。
事前に、トップ [README.md](../../README.md) の「拡張機能とツールチェインのインストール」を済ませてください。

## ターゲット

- [FRDM-MCXN947](https://www.nxp.com/design/design-center/development-boards-and-designs/FRDM-MCXN947)
  （MCX N947・デュアル Cortex-M33。ASP3 はシングルプロセッサカーネルのため **cm33_core0 のみ**使用）

EVK-MIMXRT685 版との主な違い：

| 項目 | EVK-MIMXRT685 | FRDM-MCXN947 |
|---|---|---|
| ブート | FlexSPI XIP（要 FCFB／flash_config） | **内蔵フラッシュ**（XIP/FCFB 不要） |
| シリアル | Flexcomm0 USART（fsl_usart） | **LPUART4**（LP_FLEXCOMM4・fsl_lpuart） |
| コアクロック | 250.105MHz（main_pll） | **150MHz**（PLL150M） |
| HRT（CTIMER0） | main_clk/500＝1.000421MHz（+421ppm） | **FRO_HF(48MHz)/48＝正確に 1MHz**（ppm 誤差なし） |
| SDK デバイス | `sdk/devices-rt` | `sdk/devices-mcx`（本対応で submodule 追加） |

> ブートは SDK startup（Secure 実行）のため、チップ層は RT685 と同じく
> `TOPPERS_ENABLE_TRUSTZONE` を定義します（未定義だと最初のディスパッチで
> INVPC 即死。詳細は `.claude/skills/porting-asp3-to-nxp/reference/boot-vector-pitfalls.md`）。

## コードのダウンロード

submodule（asp3_core・MCUXpresso SDK＝core/devices-rt/**devices-mcx**/cmsis/components）を
含めて取得します。

```bash
git clone --recurse-submodules https://github.com/exshonda/asp3_mcuxsdk.git
# 既存clone: git submodule update --init --recursive
```

## プロジェクトを開く

VS Code で **`frdm_mcxn947/sample1` フォルダ**を開きます。`.vscode/`・`CMakePresets.json`・
`mcuxpresso-tools.json` が同梱されているため、PROJECTS ビューに認識され CMake の構成が行われます。

初回に「**There is no toolchain associated to current project**」と表示された場合は、
トップ README の「ツールチェインをプロジェクトに関連付ける」に従って Arm GNU Toolchain
（13.2 Rel1 の**ルートフォルダ**）を関連付けてください。

> SDK は submodule 直参照のため、Smart Configurator などによるコード再生成は不要です。

## ビルド

```bash
cd frdm_mcxn947/sample1
cmake --preset Debug
cmake --build build/Debug          # → build/Debug/asp.elf
```

VS Code の場合は「ターミナル」→「タスクの実行」→「**CMake: build**」、またはステータスバー／
PROJECTS ビューの Build を実行します（Debug プリセット）。

アプリの差し替え（test_porting 等）は asp3_core 標準の `-D` 機構が使えます：

```bash
CORE=$PWD/../../asp3/asp3_core
cmake --preset Debug -B build/Debug-porting \
  -DASP3_APPLDIR=$CORE/test/porting -DASP3_APPLNAME=test_porting \
  -DASP3_EXTRA_APP_C_FILES=$CORE/test/porting/tap.c
cmake --build build/Debug-porting
```

## FRDM-MCXN947 に接続してデバッグ

1. FRDM-MCXN947 の MCU-Link USB（J17）と PC を接続します。オンボード MCU-Link が
   CMSIS-DAP ファームウェアであることを前提とします（確認・切替は
   [docs/host-setup.md](../../docs/host-setup.md)）。
2. シリアル出力は同 USB の **VCOM（115200bps）** に出ます（LPUART4＝P1_8/P1_9）。
   VS Code の Serial Monitor 拡張や TeraTerm 等で開きます
   （**VCOM を読むプロセスは同時に 1 つだけ**）。
3. 「実行とデバッグ」で「**Debug asp.elf**」を選び、デバッグを開始します。
   フラッシュ書込み後 `main` で停止するので、「続行」で実行します。
4. シリアルに TOPPERS のバナーと `sample1` の出力（`task1 is running (NNN)` など）が表示されます。
   `r` を送ると `rot_rdq` により task1→task2→task3 が切り替わります。

コマンドラインで書き込む場合（LinkServer 標準）：

```bash
cd build/Debug
LinkServer flash MCXN947:FRDM-MCXN947 load asp.elf   # 書込み後に自動リセット実行
```

## 実機検証状況（2026-06-15・FRDM-MCXN947 実機で確認済み）

MCU-Link CMSIS-DAP + LinkServer 26.5.59 で検証済み：

1. **sample1**：バナー →`task1 is running (NNN)`→ `r` 送信で `#rot_rdq` → task2 へ切替（ディスパッチ OK）。
2. **test_porting**：`# 6/6 passed`（syslog / tick_timer / task / semaphore / eventflag / alarm）。
3. （任意）**dlynse**：`SIL_DLY_TIM1=46 / SIL_DLY_TIM2=33`（150MHz 理論値）の実機較正。
   test_porting のタイマ系は通っているため必須ではない。

### ブリングアップで踏んだ MCXN947 固有の落とし穴（解決済み）

- **CTIMER クロックディバイダの HALT**：`CLOCK_AttachClk` の前に
  `CLOCK_SetClkDiv(kCLOCK_DivCtimer0Clk, 1U)` が必要（無いと `CTIMER_Init` の
  レジスタ書込みでバスストール＝起動が無音のまま止まる）。
- **Secure 実行とペリフェラルエイリアス**：Secure ブートに合わせ `-mcmse` を付与し、
  CMSIS が Secure エイリアス（0x5000_xxxx）を選ぶようにする。
  詳細は [docs/verification.md](../../docs/verification.md)。
