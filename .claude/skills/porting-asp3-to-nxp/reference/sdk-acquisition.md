# MCUXpresso SDK の取得・更新（west 非使用の submodule 直参照）

本リポジトリは MCUXpresso SDK の west / Kconfig / SDK CMake ビルドシステムを**使わない**。
`mcuxsdk-manifests` の release タグが指す各リポジトリの SHA を **submodule で直接
ピン留め**し、必要な `fsl_*` ソースだけをボードプロジェクトの CMakeLists で直接
コンパイルする（STM32Cube の HAL と同じ扱い）。

## submodule 構成

`.gitmodules` に4本（asp3_core を除く）。manifests release が指すリビジョンに合わせる。

| submodule | 実体（GitHub: nxp-mcuxpresso/...） | 用途 |
|---|---|---|
| `sdk/core` | mcuxsdk-core | 共通 fsl ドライバ（flexcomm/usart・lpflexcomm/lpuart・ctimer・common 等） |
| `sdk/devices-rt` | mcux-devices-rt | `RT600/MIMXRT685S/`（ヘッダ・system・gcc startup/ld・fsl_clock/power/reset） |
| `sdk/devices-mcx` | mcux-devices-mcx | `MCXN/MCXN947/`（同上。FRDM-MCXN947 用。release/26.03.00＝SHA 5cd233f） |
| `sdk/cmsis` | mcu-sdk-cmsis | CMSIS Core ヘッダ |
| `sdk/components` | mcux-component | fsl_debug_console ヘッダ等 |

> SoC ファミリごとに devices リポジトリが分かれる（i.MX RT＝`devices-rt`、
> MCX N＝`devices-mcx`）。新ファミリのボードを足すときは、対応する
> `mcux-devices-<family>` を `mcuxsdk-manifests` の release タグが指す SHA で
> submodule 追加する（例：`git submodule add -b release/26.03.00
> https://github.com/nxp-mcuxpresso/mcux-devices-mcx sdk/devices-mcx`）。

examples リポジトリ（巨大）は submodule にしない。

## クローン

```bash
git clone --recurse-submodules https://github.com/exshonda/asp3_mcuxsdk.git
# 既存クローンで後から：
git submodule update --init --recursive
```

## ボード初期化ファイルはコピーする

examples の `_boards/<board>/` にある初期化ファイル（いずれも BSD-3-Clause）を
ボードプロジェクトの `board/` へ**コピー**してコミットする（CubeMX 生成コードの
コミットと同じ運用）：

- `board.c` / `board.h` — `BOARD_InitDebugConsole` 等
- `clock_config.c/h` — `BOARD_BootClockRUN`（クロックツリー）
- `pin_mux.c/h` — `BOARD_InitPins`
- `flash_config.c/h` — FlexSPI 設定ブロック（FCFB・XIP ブート用）

`main()` は `BOARD_InitBootPins` → `BOARD_InitBootClocks` の後に `sta_ker()` を呼ぶ
（既存 `evkmimxrt685/sample1/main.c` を雛形にする）。

## 新しい SoC/ボードを足すときに SDK から拾うもの

| 必要物 | 所在（submodule 内） |
|---|---|
| デバイスヘッダ・`system_*.c/h` | `sdk/devices-rt/<family>/<device>/` |
| gcc startup（`startup_*.S`）・リンカ ld | `sdk/devices-rt/<family>/<device>/gcc/` |
| fsl ドライバ（clock/power/reset/usart/ctimer …） | `sdk/devices-rt/.../drivers/` と `sdk/core/drivers/` |
| CMSIS Core | `sdk/cmsis/CMSIS/Core/Include/` |

ボードプロジェクトの CMakeLists で、これらのうち**使うものだけ**を
`target_sources` / `include_directories` に列挙する（SDK の CMake は使わない）。

## SDK の更新（bump）

1. `mcuxsdk-manifests` の新しい release タグの `west.yml`（または各 project の revision）を見る
2. 4本の submodule をその SHA へ `git -C sdk/<x> checkout <sha>` で合わせる
3. ボードプロジェクトをビルドして回帰確認（警告ゼロ・asp.elf 生成）
4. `board/` にコピーしたファイルに API 変更が及んでいれば追従（差分は examples を再確認）
5. superproject で submodule ポインタをコミット

> SDK の CMake/Kconfig を使わない方針のため、bump で増減する fsl ソースは
> ボードプロジェクトの CMakeLists 側で手動調整する（自動解決はされない）。
