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
- **gdb バッチ自動化の注意**：LinkServer gdbserver（非attach）に gdb -batch で
  `continue` すると停止を待たずに次コマンドへ進むことがある（"Cannot execute
  this command while the target is running"）。スクリプト化するときは
  attach モード＋`interrupt`＋`shell sleep` の組合せが確実
  （`scripts/testexec_mcuxsdk.py` はシリアルマーカ判定のため gdb 不使用）。
- 接続が不安定なとき（メモリが全FF・フラッシュが0に見える等）は、残留した
  redlink サーバプロセスや bootROM ストール状態が原因のことがある。
  gdbserver を素直に止めて `flash load` からやり直すのが早い。

## 7. デバッガの切替（LinkServer ⇔ J-Link）

プローブのファームウェア切替手順は `docs/host-setup.md` §3。ツール側は
**ターゲット名・手順共通のままオプションで切替**できる：

| ツール | オプション | 既定 |
|---|---|---|
| CMake（flash/gdbserver/gdb/debug/osdebug） | `-DMIMXRT685_DEBUGGER=linkserver\|jlink` | linkserver |
| ボードランナ（asp3_core run_board_mimxrt685evk.sh） | `MIMXRT685_FLASH=jlink` | linkserver |
| testexec ラッパ（scripts/testexec_mcuxsdk.py） | `--flash-tool jlink` | linkserver |

## 8. MCUXpresso for VS Code 拡張

対応の経緯・検証は `docs/vscode-support-plan.md`。要点：

- 拡張用メタデータは **`.vscode/mcuxpresso-tools.json`**
  （`projectType: "cmake-freestanding"`・`${userHome}` 変数可）
- launch.json はデバッグタイプ **`mcuxpresso-debug`**。
  `gdbServerConfigs: {linkserver:{}, segger:{}, pemicro:{}}`（空）で
  **接続プローブを自動検出**＝LinkServer/J-Link どちらのファームウェアでも動く。
  ASP3 観測用に `set mem inaccessible-by-default off` を gdbInitCommands に追加済み
  （§6 と同じ理由）
- **CMakePresets.json は version 7 必須**：拡張がプロジェクト認識時に
  環境ブロック（`${pathListSep}` マクロ使用・マシン固有絶対パス）を自動注入する。
  v3 のままだと CMake Tools の展開が失敗し Build できない。
  注入されたパスは本プロジェクトのビルドでは未参照のため実害なし

## 9. MCX N（FRDM-MCXN947）固有の知見

i.MX RT685（§1〜3）と**別 SoC ファミリ**。SDK デバイスは `sdk/devices-mcx`
（submodule・`mcux-devices-mcx`）。RT685 と値・周辺が違うので流用しないこと。
実機検証は `docs/verification.md`、移植手順は porting skill の `checklists/new-board.md`。

- **CTIMER クロックディバイダの HALT（最重要・無音ハングの原因）**：MCX N の CTIMER は
  per-instance のクロックディバイダ（CTIMERCLKDIV）が**既定で HALT**。`CLOCK_AttachClk`
  だけでは機能クロックが流れず、`CTIMER_Init` の最初のレジスタ書込み（`base->IR`）で
  **バスストール**する（応答待ちで永久停止＝カーネルが無音のまま起動しない）。
  対策：`CLOCK_AttachClk` の**前に** `CLOCK_SetClkDiv(kCLOCK_DivCtimer0Clk, 1U)`。
  NXP の `_boards/frdmmcxn947/driver_examples/ctimer/.../hardware_init.c` と同じ順序。
- **Secure ペリフェラルエイリアス**：MCXN947 は Secure(0x5000_xxxx)/NS(0x4000_xxxx) の
  二重エイリアス。CMSIS デバイスヘッダは `__ARM_FEATURE_CMSE` の有無でどちらの番地を
  `CTIMER0`/`LPUART4`/`SYSCON` 等に割り当てるかを切替える。Secure ブートに合わせ
  ビルドへ **`-mcmse`** を付与し Secure エイリアスを選ばせ、`TOPPERS_ENABLE_TRUSTZONE`
  （§1・EXC_RETURN）と一貫させる。`target.cmake` と `frdm_mcxn947/sample1/CMakeLists.txt`
  の両方の compile options に付ける（カーネル lib と fsl ドライバの双方に効かせる）。
- **CLK_1M は当てにしない**：`CLOCK_GetClk1MFreq()` は固定 1000000 を返すが、CLK_1M
  自体が SDK 既定構成では無効のことがある。HRT は確実に走っている **FRO_HF(48MHz)** を
  CTIMER ディバイダで 48 分周し、正確に 1MHz を得る（§3 の RT685 +421ppm とは違い誤差ゼロ）。
- **シリアルは LP_FLEXCOMM 上の LPUART**：`core/drivers/lpflexcomm/lpuart` の fsl_lpuart を
  使う（RT685 の flexcomm/usart とは別）。`LPUART_Init` の前に
  `LP_FLEXCOMM_Init(inst, LP_FLEXCOMM_PERIPH_LPUART)`、`RESET_ClearPeripheralReset`、
  `CLOCK_AttachClk` が要る。FRDM-MCXN947 の VCOM は **LPUART4（LP_FLEXCOMM4・P1_8/P1_9）**。
- **内蔵フラッシュブート**：XIP/FlexSPI/FCFB は不要。ld は `MCXN947_cm33_core0_flash.ld`
  （TEXT_START=0x0）をコピーし `.kernel_vector`（`ALIGN(1024)`＝TMAX_INTNO 171→172 ベクタ
  ×4=688B の 2 のべき乗）を `> m_text` に追加。RT685 の `--undefined=<FCFB>` は不要。

## 10. Windows での LinkServer / gdb / VCOM 実機デバッグ手順

§6・7 は Linux 前提（`/dev/ttyACM0`）。Windows（MCU-Link CMSIS-DAP）での実作業メモ：

- **プローブ確認**：`& "C:\nxp\LinkServer_<ver>\LinkServer.exe" probes`
  （FRDM-MCXN947 は `MCXN947 / FRDM-MCXN947`・Capabilities に DEBUG/VCOM/SIO）。
- **書込み**：`LinkServer.exe flash "MCXN947:FRDM-MCXN947" load asp.elf`。
  ただし **`flash load` 完了後はブートROMストールでターゲットが停止したまま**
  （`Stopped (Was Reset)`）＝そのままでは走らない。
- **走らせて VCOM を読む（確実な手順）**：gdbserver + gdb で `load` → `continue &` →
  数百 ms 待ち → **`detach`**（デバッグ接続を解放するとターゲットは走り続ける）→ gdb 終了 →
  LinkServer も終了 → **その後** COM ポートを開いて読む。
  - **VCOM の COM 番号は `[System.IO.Ports.SerialPort]::GetPortNames()` で確認**
    （FRDM-MCXN947 は「MCU-Link VCom Port (COMxx)」）。115200/8N1。
  - **デバッグ接続中は MCU-Link が USB 再列挙し VCOM が一時的に消える/不安定**。
    読むのはデバッグ解放後にし、`Open()` はリトライループで囲む。
- **無音ハングの切り分け（CTIMER バスストールを特定した方法）**：
  `arm-none-eabi-gdb -batch` で `target remote :3333` → `load` → 関数に breakpoint →
  `continue`。**`continue` が返らない＝その先で停止している**。停止位置を関数単位で
  追うには、対象関数で止めてから `while`＋**`next`** ループを回す。
  **`next` がある呼出しから戻らず `E22`（Could not read registers）になった関数が犯人**
  （本件は `CTIMER_Init`）。さらに `stepi`＋`info symbol $pc` で命令単位に追うと、
  どのレジスタ書込みでストールするか（`base->IR=0xFF`）まで特定できる。
  - **注意**：この LinkServer gdbserver では非同期 `interrupt`/`monitor halt` が
    バッチで効かない（`Selected thread is running`）。停止状態の観測は
    breakpoint か上記 `next`/`stepi` 法を使う（async halt に頼らない）。
  - gdb の `set mem inaccessible-by-default off` は §6 同様付けておく。
