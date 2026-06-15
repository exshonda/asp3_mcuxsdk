# ASP3 MCUXpresso SDK

TOPPERS/ASP3 と NXP MCUXpresso SDK を組み合わせたサンプルプロジェクトです。

TOPPERS/ASP3 RTOS 上で動く `sample1` タスクが UART にバナーとメッセージを出力するサンプルが動作します。
対象ボードは **EVK-MIMXRT685**（i.MX RT685・Cortex-M33）です。
このリポジトリを通じて以下を学べます。

- NXP MCUXpresso SDK（fsl ドライバ）と TOPPERS/ASP3 RTOS の連携方法
- VS Code + CMake + MCUXpresso 拡張によるクロスコンパイル／デバッグ環境の構築
- ASP3 のタスク・セマフォ・サービスコールの基本的な使い方

純カーネル [asp3_core](https://github.com/exshonda/asp3_core) を submodule（`asp3/asp3_core`）として参照し、
SDK 固有の target 依存部・アプリ・移植ノウハウを本リポジトリで管理します
（`ASP3_TARGET_DIR`／`ASP3_LIBRARY_ONLY` 方式。asp3_pico_sdk / asp3_fsp / asp3_stm32cube と同じ構成）。

## フォルダ構成

- `asp3/asp3_core/`: 共通の TOPPERS/ASP3 RTOS 本体（submodule・純カーネル）
- `asp3/asp3_mcuxsdk.cmake`: 協調ヘルパ（`ASP3_TARGET_DIR` などの解決）
- `asp3/arch/arm_m_gcc/imxrt600_mcuxsdk/`: チップ依存部（i.MX RT685・SDK 版）
- `asp3/arch/arm_m_gcc/mcxn947_mcuxsdk/`: チップ依存部（MCX N947・SDK 版）
- `asp3/target/evkmimxrt685_mcuxsdk/`: ターゲット依存部（EVK-MIMXRT685・SDK 版）
- `asp3/target/frdmmcxn947_mcuxsdk/`: ターゲット依存部（FRDM-MCXN947・SDK 版）
- `sdk/`: MCUXpresso SDK（submodule・`mcuxsdk-manifests` release/26.03.00 直参照。
  i.MX RT は `devices-rt`、MCX N は `devices-mcx`）
- `evkmimxrt685/sample1/`: EVK-MIMXRT685 サンプル（CMake・VS Code のプロジェクトルート）
- `frdm_mcxn947/sample1/`: FRDM-MCXN947 サンプル（同上）
- `docs/`: ホスト構築・検証記録・技術ノート

`evkmimxrt685/sample1/` を CMake・VS Code（MCUXpresso 拡張）のプロジェクトルートとして扱います。
SDK は west を使わず、`mcuxsdk-manifests` release/26.03.00 が指すリビジョンを submodule で
直接ピン留めしているため、**SDK の GUI 生成ツール（Smart Configurator 相当）は不要**です
（`git clone --recurse-submodules` 直後にそのままビルドでき、CI でもビルド可能）。

## コードのダウンロード

submodule（asp3_core・MCUXpresso SDK）を含めて取得します。

```bash
git clone --recurse-submodules https://github.com/exshonda/asp3_mcuxsdk.git
# 既存clone後に submodule を取得する場合:
# git submodule update --init --recursive
```

## 拡張機能とツールチェインのインストール

VS Code でビルド・書込み・デバッグするには、NXP 公式拡張と、拡張が使うツール群
（ツールチェイン・LinkServer・CMake・Ninja など）が必要です。

### 1. MCUXpresso for VS Code 拡張

左領域の「拡張機能」アイコンを選び、Marketplace で「**MCUXpresso for VS Code**」を検索して
インストールします（拡張 ID: `NXPSemiconductors.mcuxpresso`）。
[Marketplace のページ](https://marketplace.visualstudio.com/items?itemName=NXPSemiconductors.mcuxpresso)。

### 2. ツール群（MCUXpresso Installer）

拡張のデバッグ機能は、システムに入っているツールではなく **MCUXpresso Installer が管理・登録した
ツール**を使います。拡張の「Open MCUXpresso Installer」または `MCUXpressoInstaller` を起動し、
次のコンポーネントをインストールします。

| コンポーネント | 役割 |
|---|---|
| **MCUXpresso SDK Developer** | （任意）本リポジトリは SDK を submodule 直参照するため**不要** |
| **LinkServer** | オンボード CMSIS-DAP プローブの書込み・デバッグCLI（標準の書込み手段） |
| **Additional NXP debugger support（debugCommon）** | 拡張のデバッグ基盤 |
| **CMake / Ninja / Git** | ビルドシステム（拡張が登録版を要求する） |
| **Arm GNU Toolchain（arm-none-eabi-gcc）** | クロスコンパイラ。本プロジェクトは **13.2.1（13.2 Rel1）** で作成 |

### 3. ツールチェインをプロジェクトに関連付ける

初回デバッグ時に「There is no toolchain associated to current project」と出る場合は、
ツールチェインの関連付けが必要です。

1. `Ctrl+Shift+P` →「**MCUXpresso: Associate Toolchain**」
2. 一覧に出ないときは先に「**MCUXpresso: Path to toolchain**」で、Arm GNU Toolchain の
   **ルートフォルダ**（例 `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\13.2 Rel1`）を指定
   - **`bin` フォルダではなくその親（ルート）**を選ぶこと。拡張は `<選んだフォルダ>\bin\arm-none-eabi-gcc.exe`
     を検証するため、`bin` を選ぶと「not a valid toolchain directory」になる

> **Windows でのハマりどころ**（実際に踏んだもの）はボード手順の
> [evkmimxrt685/sample1/README.md](evkmimxrt685/sample1/README.md) にまとめています
> （「No probe」＝LinkServer 未導入、「ninja のバージョンが古い」＝PATH 上で別の ninja が
> 優先されている、環境変数変更が VS Code に反映されない、など）。

## 動作確認環境

| 項目 | バージョン |
|------|-----------|
| MCUXpresso SDK | `mcuxsdk-manifests` release/26.03.00 |
| MCUXpresso for VS Code 拡張 | 26.5.49 |
| LinkServer | 26.5.59 |
| Arm GNU Toolchain（arm-none-eabi-gcc） | 13.2.1（13.2 Rel1） |
| Ninja | 1.12.1 |
| プローブ | オンボード LPC-Link2（CMSIS-DAP・出荷時ファームウェア） |

Linux ホストでの CLI 書込み・テスト実行の構築手順は [docs/host-setup.md](docs/host-setup.md) を参照してください。

## サンプルの開き方

VS Code で `evkmimxrt685/sample1` フォルダを開きます。詳細手順（ビルド・書込み・デバッグ）は
ボード別の README を参照してください。

| ボード | 詳細手順 |
|--------|----------|
| EVK-MIMXRT685 | [evkmimxrt685/sample1/README.md](evkmimxrt685/sample1/README.md) |
| FRDM-MCXN947 | [frdm_mcxn947/sample1/README.md](frdm_mcxn947/sample1/README.md) |

コマンドラインでのビルドは次のとおりです。

```bash
cd evkmimxrt685/sample1
cmake --preset Debug
cmake --build build/Debug          # → build/Debug/asp.elf
```

## 新規プロジェクト作成ガイド

既存の `evkmimxrt685/sample1/` ディレクトリを雛形に、新しいアプリケーションを作成できます。

### 最小複製対象

`evkmimxrt685/sample1/` を丸ごとコピーし、以下を編集します。

```
sample1/
├── CMakeLists.txt                   ← ターゲット・アプリのソースを変更
├── CMakePresets.json                ← そのまま流用可
├── MIMXRT685Sxxxx_cm33_flash.ld     ← そのまま流用可（ASP3 ベクタ配置入り）
├── main.c                           ← sta_ker() 起動・低レベル出力など。流用可
├── board/                           ← SDK のボード初期化（クロック/ピン/XIP）。流用可
└── .vscode/                         ← VS Code 構成（そのまま流用可）
```

別ボードに対応する場合は、`asp3/target/<新ターゲット>_mcuxsdk/` を別途用意します
（移植手順は `.claude/skills/porting-asp3-to-nxp/` を参照）。

### CMakeLists.txt の変更点

主に変更するのは次の箇所です（[evkmimxrt685/sample1/CMakeLists.txt](evkmimxrt685/sample1/CMakeLists.txt) 参照）。

```cmake
# (1) ターゲット依存部 — asp3/target/ 以下のディレクトリ名に合わせる
set(ASP3_TARGET evkmimxrt685_mcuxsdk)

# (2) アプリ（タスク定義）のディレクトリと名前 — 自作アプリに置き換える
#     既定は asp3_core 同梱の sample/sample1（.c と .cfg）
set(ASP3_APPLDIR  ${ASP3_CORE_DIR}/sample)
set(ASP3_APPLNAME sample1)

# (3) アプリのソースファイル — target_sources に自分の .c を追加
target_sources(asp PRIVATE
    main.c
    # ... SDK・board のソース ...
    ${ASP3_APPLDIR}/${ASP3_APPLNAME}.c
    ${ASP3_EXTRA_APP_C_FILES}
)
```

`ASP3_APPLDIR`／`ASP3_APPLNAME`／`ASP3_EXTRA_APP_C_FILES` は CMake の `-D` でも上書きできます。
例えば移植検証テスト（test_porting）に差し替える場合：

```bash
cmake --preset Debug -B build/Debug-porting \
    -DASP3_APPLDIR=test/porting -DASP3_APPLNAME=test_porting \
    -DASP3_EXTRA_APP_C_FILES=test/porting/tap.c
cmake --build build/Debug-porting
```

## 検証状況（実機・2026-06-12）

| 項目 | 結果 |
|---|---|
| sample1 | バナー・task1 周期実行・`r`（rot_rdq）で task1→2→3 切替 |
| test_porting（6項目） | **6/6 passed** |
| testexec 全件（36テスト） | **33 PASS／1 SKIP（cpuexc10）／2 FAIL（cpuexc1・cpuexc4＝arm_m 共通の上流由来の既知問題）** |
| VS Code（MCUXpresso 拡張） | Build／Debug（フラッシュ→`main` 停止→実行）・OS 観測 Attach 構成を確認 |

testexec の実行はボードと VCOM を占有するため並行実行は禁止です（詳細は
[docs/verification.md](docs/verification.md)）。

## ドキュメント

| ファイル | 内容 |
|---|---|
| [evkmimxrt685/sample1/README.md](evkmimxrt685/sample1/README.md) | VS Code でのビルド・書込み・デバッグ手順（Windows のハマりどころ含む） |
| [docs/host-setup.md](docs/host-setup.md) | Linux 実機検証ホストの構築（LinkServer/LPCScrypt・プローブのファームウェア切替・VCOM・ジャンパ） |
| [docs/verification.md](docs/verification.md) | 実機検証のスナップショットと再実行手順 |
| [docs/tech-notes.md](docs/tech-notes.md) | SDK 統合の技術ノート（TrustZone/EXC_RETURN・ベクタテーブル配置・HRT 精度・LinkServer の注意ほか） |
| [docs/vscode-support-plan.md](docs/vscode-support-plan.md) | MCUXpresso for VS Code 対応の経緯・検証結果 |
| [docs/TODO.md](docs/TODO.md) | 残課題とアプリプロジェクト追加手順 |

経緯（計画・実施結果）の正本は asp3_core 側の
[docs/dev/nxp-integration.md](https://github.com/exshonda/asp3_core/blob/main/docs/dev/nxp-integration.md) です。
そこに **EXC_RETURN／TrustZone・ASP3 ベクタテーブル配置・HRT 精度** など、本統合特有の
技術ポイントの root cause 解析がまとまっています。
