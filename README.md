# asp3_mcuxsdk

TOPPERS/ASP3 Core と NXP MCUXpresso SDK の統合リポジトリ（対象：EVK-MIMXRT685／i.MX RT685・Cortex-M33）。

純カーネル [asp3_core](https://github.com/exshonda/asp3_core) を submodule（`asp3/asp3_core`）として参照し、
SDK固有の target 依存部・アプリ・移植ノウハウを本リポジトリで管理する
（`ASP3_TARGET_DIR`／`ASP3_LIBRARY_ONLY` 方式。asp3_pico_sdk / asp3_fsp / asp3_stm32cube と同じA案構成）。

## 状態

**Phase B（SDK統合）実装済み・実機検証済み**（2026-06-12）。経緯は asp3_core の
[`docs/dev/nxp-integration.md`](https://github.com/exshonda/asp3_core/blob/main/docs/dev/nxp-integration.md) を参照：

- **Phase A**（完了・asp3_core側）：ベアメタル `mimxrt685evk` ターゲットを asp3_core 本体に追加・実機検証済み
- **Phase B**（完了・本リポジトリ）：MCUXpresso SDK（fsl_ドライバ）との協調動作
  - SDK の startup／リンカスクリプト／ボード初期化（クロック・ピン・XIP）でブートし、`main()` が `sta_ker()` を呼ぶ
  - `core_initialize()` が VTOR を ASP3 のベクタテーブルへ切替＝**ASP3がNVIC掌握・SDKは素直に従う**（STM32/FSPと同方式）
  - カーネルのシリアル（SIO）＝fsl_usart、高分解能タイマ＝fsl_ctimer（CTIMER0）

## 構成

```
asp3_mcuxsdk/
├── asp3/
│   ├── asp3_core/                       ← submodule（純カーネル）
│   ├── asp3_mcuxsdk.cmake               ← 協調ヘルパ（ASP3_TARGET_DIR等の解決）
│   ├── arch/arm_m_gcc/imxrt600_mcuxsdk/ ← チップ依存部（SDK版）
│   └── target/evkmimxrt685_mcuxsdk/     ← ターゲット依存部（SDK版）
├── sdk/                                 ← MCUXpresso SDK（submodule・release/26.03.00）
│   ├── core/        … mcuxsdk-core（共通fslドライバ）
│   ├── devices-rt/  … mcux-devices-rt（RT600デバイスファイル・startup・ld）
│   ├── cmsis/       … mcu-sdk-cmsis（CMSIS Core）
│   └── components/  … mcux-component（fsl_debug_console ヘッダ等）
├── docs/                                ← 知見・検証記録（host-setup / verification / tech-notes）
└── evkmimxrt685/                        ← ボードディレクトリ（asp3_fsp と同構成）
    └── sample1/                         ← アプリプロジェクト
        ├── CMakeLists.txt / CMakePresets.json / main.c
        ├── MIMXRT685Sxxxx_cm33_flash.ld ← SDK の ld に ASP3 ベクタテーブル配置を追加
        └── board/                       ← SDK examples のボード初期化ファイル（BSD-3）
```

SDK は west を使わず、`mcuxsdk-manifests` release/26.03.00 が指すリビジョンを
submodule で直接ピン留めしている（GUI生成ツール不要＝CIでもそのままビルド可）。

## クローン

```bash
git clone --recurse-submodules https://github.com/exshonda/asp3_mcuxsdk.git
```

## ビルド

```bash
cd evkmimxrt685/sample1
cmake --preset Debug
cmake --build build/Debug          # → build/Debug/asp.elf
```

アプリの差し替え（例：移植検証テスト）：

```bash
cmake --preset Debug -B build/Debug-porting \
    -DASP3_APPLDIR=test/porting -DASP3_APPLNAME=test_porting \
    -DASP3_EXTRA_APP_C_FILES=test/porting/tap.c
cmake --build build/Debug-porting
```

## 実機実行（EVK-MIMXRT685）

書込み・VCOM・テストランナの手順は asp3_core 側の
[`target/mimxrt685evk_gcc/target_user.md`](https://github.com/exshonda/asp3_core/blob/feat/mimxrt685evk/target/mimxrt685evk_gcc/target_user.md)
と共通。**標準はオンボードLPC-Link2のCMSIS-DAP（出荷時ファームウェア）＋
LinkServer**（プローブをJ-Link化した場合は各ツールのオプションで切替可）：

```bash
cd evkmimxrt685/sample1/build/Debug
/usr/local/LinkServer/LinkServer flash MIMXRT685S:EVK-MIMXRT685 load asp.elf
```

テスト実行は asp3_core の `scripts/ci/run_board_mimxrt685evk.sh`（UARTキャプチャ→
J-Link書込み→完走マーカ待ち）をビルドディレクトリで実行する。
**VCOM（/dev/ttyACM0）を読むプロセスは同時に1つだけ**にすること。

## 検証状況（実機・2026-06-12）

| 項目 | 結果 |
|---|---|
| sample1 | バナー・task1周期実行・`r`（rot_rdq）でtask1→2→3切替 |
| test_porting（6項目） | **6/6 passed** |
| testexec 全件（36テスト） | **33 PASS／1 SKIP（cpuexc10=not necessary）／2 FAIL（cpuexc1・cpuexc4＝arm_m 共通の上流由来の既知問題．asp3_core `docs/dev/issue-cpuexc-armm.md`）**．dlynse は NG=0（較正値 SIL_DLY_TIM1/2=27/19） |

testexec の実行は専用ラッパを使う（ボードと VCOM を占有するため並行実行禁止）：

```bash
scripts/testexec_mcuxsdk.py              # 全件（約30分）
scripts/testexec_mcuxsdk.py task1 sem1   # 個別指定
scripts/testexec_mcuxsdk.py --rejudge    # 保存済みログの再判定のみ
scripts/testexec_mcuxsdk.py --flash-tool jlink ...   # J-Linkファームウェア時
```

## ドキュメント

| ファイル | 内容 |
|---|---|
| [`docs/host-setup.md`](docs/host-setup.md) | 実機検証ホストPCの構築（LinkServer/LPCScryptの導入・プローブのファームウェア確認と切替・VCOMの注意・ジャンパ） |
| [`docs/verification.md`](docs/verification.md) | 実機検証のスナップショットと再実行手順（testexec/test_porting/sample1/OS Awareness） |
| [`docs/tech-notes.md`](docs/tech-notes.md) | SDK統合の技術ノート（TrustZone/EXC_RETURN・ベクタテーブル配置・HRT精度・LinkServerの注意ほか） |

経緯（計画・実施結果）の正本は asp3_core 側の
[`docs/dev/nxp-integration.md`](https://github.com/exshonda/asp3_core/blob/main/docs/dev/nxp-integration.md)。

## 主要な技術ポイント

- **EXC_RETURN／TrustZone**：SDK startup（イメージタイプ=0）でブートした CPU は
  **Secure 状態**で実行されるため `TOPPERS_ENABLE_TRUSTZONE` を**定義する**
  （mps2_an521 と同じ）。Phase A のプレーンイメージブート（ベクタ9=bit14・未定義）
  とは**逆**なので注意。未定義のままだと最初のディスパッチで INVPC（UsageFault）になる
- **ベクタテーブル配置**：ASP3 cfg 生成の `.vector` セクションは SDK の ld では
  orphan となり `m_interrupts` 領域に収まらないため、ld のコピーに
  `.kernel_vector`（ALIGN(512)・m_text）を追加して配置している
- **HRT 精度**：CTIMER0 のクロックは SDK 既定 main_clk（500.21MHz）の整数分周で
  1.000421MHz（+421ppm）。正確な 1MHz は得られないため許容して文書化
- **クロック**：SDK 既定 `BOARD_BootClockRUN`（CPU 250.105MHz）。PMIC/FBB は SDK 既定
  （Phase A の 300MHz/FBB 設定は使わない）
