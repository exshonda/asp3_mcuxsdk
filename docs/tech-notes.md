# 技術ノート（MCUXpresso SDK × TOPPERS/ASP3 統合の知見）

Phase B（SDK統合）の実装・実機検証で得た知見のうち、**本リポジトリを触る人が
再び踏みそうなもの**を集めたメモ。経緯の正本は
`asp3/asp3_core/docs/dev/nxp-integration.md`、ホストPC構築は `docs/host-setup.md`。

## 1. TrustZone／EXC_RETURN（最重要・INVPCの原因）

**SDK の startup でブートした CPU は Secure 状態で実行される**ため、ASP3 は
`TOPPERS_ENABLE_TRUSTZONE` を**定義する**（`asp3/arch/arm_m_gcc/imxrt600_mcuxsdk/arch.cmake`。
mps2_an521 と同じ構成）。

- 未定義（EXC_RETURN 0xFFFFFFBC＝S/ES=0）のままだと、**最初のディスパッチで
  INVPC（UsageFault・CFSR=0x40000）**→ HardFault 相当の即死になる。
- **Phase A（asp3_core 本体の mimxrt685evk_gcc）とは逆**であることに注意：
  Phase A は自前ブート（ベクタテーブル index 9＝イメージタイプに bit14 を
  立てた「プレーンイメージ」）で、`TOPPERS_ENABLE_TRUSTZONE` は**未定義**。
  SDK startup はベクタ9=0 のイメージで、ブートROMの扱いが異なる。
- 教訓：**ブートイメージのタイプが EXC_RETURN 構成を決める**。NXP RT6xx 系へ
  移植する際は、まずベクタ9（オフセット0x24）の値とブート経路を確認すること。

## 2. ASP3 ベクタテーブルの配置（リンクエラーの原因）

ASP3 の cfg が生成する `_kernel_vector_table`（セクション `.vector`）は、SDK の
リンカスクリプトでは **orphan セクション**になり、`m_interrupts` 領域
（0x08001000・長さ0x130＝SDKブートベクタでちょうど満杯）へ引き寄せられて
「will not fit」「LMA overlap」でリンクが失敗する。

→ SDK の `MIMXRT685Sxxxx_cm33_flash.ld` をボードプロジェクトへコピーし、
`.kernel_vector`（`ALIGN(512)`・`> m_text`）を追加して明示配置した
（`evkmimxrt685/sample1/MIMXRT685Sxxxx_cm33_flash.ld`）。
ALIGN(512) は VTOR の整列要件（エントリ数×4 以上の2のべき乗）。

実行時は arm_m 共通の `core_initialize()` が VTOR をこのテーブルへ切り替える
（＝ASP3 が NVIC を掌握。SDK の `--wrap` 等は不要）。ブート時のベクタは SDK の
`.isr_vector`（0x08001000）のまま。

## 3. クロックと HRT 精度（+421ppm）

SDK 既定の `BOARD_BootClockRUN`：main_pll=528MHz×18/19=**500.21MHz**、
CPU=main_clk/2=**250.105MHz**、frg_pll=main_pll/12=41.68MHz。
PMIC／FBB は SDK 既定のまま（250MHz は PMIC 既定電圧で動作）。

HRT（CTIMER0）のクロックは main_clk を整数プリスケーラで割るため、正確な
1MHz は作れない（500.21MHz/500=**1.000421MHz**、+421ppm）。実用上問題なし
（テスト全PASS）だが、`get_tim` は約36ms/日進む。厳密な 1MHz が要る場合は
PLL 構成の変更が必要（SDK 既定から逸脱するため不採用）。

Phase A（300MHz・自前PLL設定）とはクロックが異なるので、`CPU_CLOCK_HZ`／
`SIL_DLY_TIM*` を流用しないこと（SDK版の値は
`asp3/target/evkmimxrt685_mcuxsdk/evkmimxrt685_mcuxsdk.h`）。

## 4. SDK の取得方式（west 不使用）

現行 MCUXpresso SDK は west マニフェスト方式だが、本リポジトリは
`mcuxsdk-manifests` の release タグが指す SHA を **submodule で直接ピン留め**
（core／devices-rt／cmsis／components の4本＝計約90MB）。
- west・Kconfig・SDKのCMakeビルドシステムは使わず、必要な fsl ソースだけを
  ボードプロジェクトの CMakeLists で直接コンパイルする（STM32Cube の HAL と同じ扱い）。
- examples リポジトリ（200MB）は submodule にせず、ボード初期化ファイル
  （board.c／clock_config／pin_mux／flash_config＝BSD-3）を
  `evkmimxrt685/sample1/board/` へコピー。
- SDK 更新時は manifests の新 release が指す SHA に4本を合わせて bump する。

## 5. 細かいハマりどころ

- **DbgConsole_Init の未解決参照**：board.c が `BOARD_InitDebugConsole` の
  「関数アドレス」を XIP 判定マクロに使うため、未使用でも GC されず
  `DbgConsole_Init` への参照が残る。debug_console コンポーネント一式を
  リンクする代わりに `main.c` のスタブで充足している。
- **EXC_RETURN_PREFIX の二重定義警告**：ASP3 の `arm_m.h` と CMSIS の
  `core_cm33.h` が同値（0xff000000）を定義。fsl ヘッダの include を
  `#pragma push_macro/pop_macro` で挟んで警告ゼロ化（`target_serial.c`・
  `target_timer.h`）。
- **target_stddef.h の必須要素**：`<stdint.h>`・`tool_stddef.h` の include と
  `TOPPERS_assert_abort` の定義が必要（stm32cubemx 版の写しだけでは不足。
  欠けると `uint32_t` 未定義や `Inline` 未定義の大量エラー）。
- **`software_term_hook`**：weak 定義を target_kernel_impl.c に置く（無いと
  core_terminate からの参照でリンクエラー）。

## 6. LinkServer の注意（デバッグ・自動化）

- **接続時動作**：通常接続（`gdbserver`／`flash`）は「システムリセット＋
  bootROM ストール」で止まる。**実行中システムの観測（OS Awareness）には
  `--attach`** を使う（リセットなし）。ストール状態のまま放置すると
  FlexSPI 未設定でフラッシュが 0 に見える・SRAM が読めない等、一見壊れた
  ような状態に見えるので注意（continue すれば普通にブートする）。
- **`set mem inaccessible-by-default off` が必須**：LinkServer のデバイス定義
  （`/usr/local/LinkServer/devices/EVK-MIMXRT685.json`）のメモリマップには
  SRAM の**データバスエイリアス（0x20000000〜）が無い**ため、既定設定の gdb は
  カーネルオブジェクトの読出しを `Cannot access memory` で拒否する。
  `linkserver-debug.gdb`（asp3_core target/mimxrt685evk_gcc/）に組込み済み。
- `LinkServer flash <dev> load asp.elf` は書込み後に**自動でリセット実行**する
  （`--no-reset` で抑止可）。書込み前の接続でも一度リセットが走るため、
  旧イメージの出力断片が UART キャプチャに混ざることがある（テストの
  マーカ判定はこれを許容する設計）。
- LinkServer は PATH に入らない（`/usr/local/LinkServer/LinkServer`）。
  各スクリプトは環境変数 `LINKSERVER` で上書き可能。

## 7. デバッガの切替（LinkServer ⇔ J-Link）

プローブのファームウェア切替手順は `docs/host-setup.md` §3。ツール側は
**ターゲット名・手順共通のままオプションで切替**できる：

| ツール | オプション | 既定 |
|---|---|---|
| CMake（flash/gdbserver/gdb/debug/osdebug） | `-DMIMXRT685_DEBUGGER=linkserver\|jlink` | linkserver |
| ボードランナ（asp3_core run_board_mimxrt685evk.sh） | `MIMXRT685_FLASH=jlink` | linkserver |
| testexec ラッパ（scripts/testexec_mcuxsdk.py） | `--flash-tool jlink` | linkserver |
