---
name: porting-asp3-to-nxp
description: TOPPERS/ASP3 を NXP i.MX RT6xx/RT5xx（MCUXpresso SDK + arm-none-eabi-gcc）の新しいボードに移植する手順と、SDK統合特有の落とし穴（SDK startup でのSecureブートとEXC_RETURN・ASP3ベクタテーブル配置・SDK既定クロックとHRT精度・west非使用のsubmodule直参照・LinkServer/J-Link 操作）をまとめたガイド。新規ボード対応、実機ブリングアップのデバッグ、ビルド・書込み・検証の作業で参照する。
---

# TOPPERS/ASP3 を NXP MCUXpresso SDK ボードに移植する

このリポジトリ（`asp3_mcuxsdk`）は TOPPERS/ASP3 Core を NXP MCUXpresso SDK 環境で
動かすための移植層。**EVK-MIMXRT685**（i.MX RT685・Cortex-M33）を実例として、
新規 NXP ボード対応や実機ブリングアップに必要な作業をまとめる。

ASP3 はシングルプロセッサカーネルで、RT685 では **CM33 のみ**使用する（HiFi4 DSP は停止のまま）。

## このスキルが扱う作業

1. **新規ボード（例: EVK-MIMXRT595 / LPCXpresso55S69）を ASP3 に追加する** → [checklists/new-board.md](checklists/new-board.md)
2. **実機で動かない時のデバッグ** → [checklists/bringup-debug.md](checklists/bringup-debug.md)
3. **SDK の取得・更新（west 非使用の submodule 直参照）** → [reference/sdk-acquisition.md](reference/sdk-acquisition.md)
4. **書込み・デバッグ・シリアルのツール操作（LinkServer / J-Link）** → [reference/flash-debug-tools.md](reference/flash-debug-tools.md)
5. **ブート経路と EXC_RETURN・ベクタ配置の落とし穴** → [reference/boot-vector-pitfalls.md](reference/boot-vector-pitfalls.md)

## 重要な前提知識（最初に読む）

### 1. ディレクトリ構成

```
asp3_mcuxsdk/
├── asp3/
│   ├── asp3_core/                          # カーネル本体（git submodule・無変更が原則）
│   ├── arch/arm_m_gcc/imxrt600_mcuxsdk/    # チップ層（SDK版・外側リポジトリ管理）
│   ├── target/evkmimxrt685_mcuxsdk/        # ターゲット依存部（SDK版）
│   └── asp3_mcuxsdk.cmake                  # glue（ASP3_TARGET_DIR / ASP3_CORE_DIR 解決）
├── sdk/                                    # MCUXpresso SDK（submodule・release タグでピン留め）
│   ├── core/ devices-rt/ cmsis/ components/
├── evkmimxrt685/sample1/                   # ボードプロジェクト（asp3_fsp と同構成）
│   ├── CMakeLists.txt / CMakePresets.json / main.c
│   ├── MIMXRT685Sxxxx_cm33_flash.ld        # SDK の ld に ASP3 ベクタ配置を追加
│   └── board/                              # SDK examples のボード初期化（BSD-3・コピー）
└── docs/                                   # host-setup / verification / tech-notes / TODO
```

新規ボード追加では `asp3/target/<新ターゲット>_mcuxsdk` と `<board>/<app>/` の
2 箇所をセットで用意する。SDK 固有部は外側リポジトリ管理、asp3_core は
`ASP3_TARGET_DIR` で受け入れる（Pico SDK / FSP / STM32 統合と同方針）。

### 2. SDK との付き合い方（west を使わない）

MCUXpresso SDK は本来 west マニフェスト方式だが、本リポジトリは
`mcuxsdk-manifests` の release タグが指す SHA を **submodule で直接ピン留め**する
（core / devices-rt / cmsis / components の4本）。**west・Kconfig・SDK の CMake
ビルドシステムは使わない**。必要な `fsl_*` ソースだけをボードプロジェクトの
CMakeLists で直接コンパイルする（STM32Cube の HAL と同じ扱い）。

- これにより **GUI 生成ツールが不要**になり、`git clone --recurse-submodules`
  直後にそのままビルドできる（CI でもビルド可能＝CubeMX/RASC と違う利点）。
- ボード初期化ファイル（`board.c` / `clock_config` / `pin_mux` / `flash_config`）は
  examples リポジトリ（巨大）から**コピー**する（CubeMX 生成コードのコミットと同じ扱い）。
- 取得・更新の詳細は [reference/sdk-acquisition.md](reference/sdk-acquisition.md)。

### 3. 実機ブリングアップで踏んだ地雷（必読）

詳細は [reference/boot-vector-pitfalls.md](reference/boot-vector-pitfalls.md)。要点:

| # | 落とし穴 | 症状 | 対策（実装済みの形） |
|---|---|---|---|
| 1 | **SDK startup は Secure ブート→ TrustZone 定義が必要** | 最初のディスパッチで INVPC（UsageFault・CFSR=0x40000）即死 | `imxrt600_mcuxsdk/arch.cmake` で `TOPPERS_ENABLE_TRUSTZONE` を**定義する**（mps2_an521 と同じ。Phase A の自前プレーンイメージブートとは**逆**） |
| 2 | **ASP3 ベクタテーブルが orphan→ リンク失敗** | `.vector will not fit` / LMA overlap | SDK の ld をコピーし `.kernel_vector`（`ALIGN(512)`・`> m_text`）を追加して明示配置 |
| 3 | **VTOR 整列違反** | バナーは出るがタスク切替で即死 | `target_kernel.py` がテーブルに整列属性を生成＋ld の ALIGN(512) |
| 4 | **HRT が正確に 1MHz にならない** | `get_tim` がわずかにドリフト（実用上問題なし） | SDK 既定 main_clk の整数分周＝1.000421MHz（+421ppm）を許容・文書化 |
| 5 | **fsl ヘッダと arm_m.h のマクロ二重定義** | `EXC_RETURN_PREFIX` 等の警告 | fsl include を `#pragma push_macro/pop_macro` で挟む |

> **最重要の教訓**：ブートイメージのタイプ（ベクタ9＝オフセット 0x24 の値とブート経路）が
> EXC_RETURN 構成を決める。新ボード移植では**まずブート経路を確認**すること。

### 4. ビルド

```bash
cd evkmimxrt685/sample1
cmake --preset Debug
cmake --build build/Debug        # → build/Debug/asp.elf
```

アプリ（既定 sample1）の差し替えは asp3_core 標準の `-D` 機構が使える:

```bash
CORE=$PWD/../../asp3/asp3_core
cmake --preset Debug -B build/Debug-porting \
  -DASP3_APPLDIR=$CORE/test/porting -DASP3_APPLNAME=test_porting \
  -DASP3_EXTRA_APP_C_FILES=$CORE/test/porting/tap.c
cmake --build build/Debug-porting
```

> `CMakePresets.json` は **version 7 必須**（MCUXpresso for VS Code 拡張が環境ブロックを
> 注入する。詳細は tech-notes §8）。注入されるマシン固有パスはビルドでは未参照のため、
> CI やそのパスを持たない環境でもビルドは通る。

### 5. 書込みと動作確認

標準はオンボード LPC-Link2 の **CMSIS-DAP（出荷時ファームウェア）＋ LinkServer**：

```bash
cd evkmimxrt685/sample1/build/Debug
/usr/local/LinkServer/LinkServer flash MIMXRT685S:EVK-MIMXRT685 load asp.elf
stty -F /dev/ttyACM0 115200 raw -echo
cat /dev/ttyACM0          # VCOM を読むプロセスは同時に1つだけ
```

最小チェック（sample1）:
1. バナー → `Sample program starts (exinf = 0).` → `task1 is running (NNN)`
2. `printf 'r' > /dev/ttyACM0` で task1→task2→task3 が切り替わればディスパッチ OK
3. 仕上げは **test_porting（TAP 6項目）** を書込み、`# 6/6 passed` を確認

ツールのパス・J-Link 切替・OS Awareness は
[reference/flash-debug-tools.md](reference/flash-debug-tools.md)。

---

## 参考

- 経緯・root cause 解析の正本: `asp3/asp3_core/docs/dev/nxp-integration.md`
- 技術ノート（再び踏みそうな知見）: `docs/tech-notes.md`
- ホストPC構築（LinkServer/LPCScrypt・ファームウェア切替・VCOM・ジャンパ）: `docs/host-setup.md`
- 実機検証スナップショットと再実行手順: `docs/verification.md`
- 動作確認済み: EVK-MIMXRT685、MCUXpresso SDK `mcuxsdk-manifests` release/26.03.00、
  arm-none-eabi-gcc 13.2.1、LinkServer（CMSIS-DAP）・J-Link 切替可
- 同型の移植スキル: [asp3_stm32cube](https://github.com/toppers/asp3_stm32cube)（STM32Cube HAL）、
  [asp3_fsp](https://github.com/toppers/asp3_fsp)（Renesas RA / FSP）、
  asp3_pico_sdk（Raspberry Pi Pico）
- ベアメタル版（SDK不使用・自前ブート）: asp3_core 本体の `target/mimxrt685evk_gcc`
  （Phase A。SDK版とは TrustZone/クロック/ブートが**逆または別**なので値を流用しないこと）
