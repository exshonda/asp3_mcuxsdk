# 新規 NXP ボードを ASP3 に追加するチェックリスト

EVK-MIMXRT685（`evkmimxrt685/sample1` ＋ `asp3/target/evkmimxrt685_mcuxsdk` ＋
`asp3/arch/arm_m_gcc/imxrt600_mcuxsdk`）を雛形に、新しい NXP ボード／SoC を追加する手順。
SDK 固有部は本リポジトリ管理、asp3_core は `ASP3_TARGET_DIR` で受け入れる。

## Step 0. ブート経路を最初に確定する（最重要）

移植の成否を分ける一次判断。[reference/boot-vector-pitfalls.md](../reference/boot-vector-pitfalls.md) §1 参照。

- [ ] ブートが **SDK startup 経由**（`startup_<device>.S`→`SystemInit`→`main()`）か、
      自前プレーンイメージか。RT6xx の EVK は通常 SDK startup＝**Secure ブート**。
- [ ] Secure ブートなら `TOPPERS_ENABLE_TRUSTZONE` を**定義する**（未定義だと
      最初のディスパッチで INVPC 即死）。非Secure（自前プレーンイメージ）なら未定義。
- [ ] ベクタ9（オフセット 0x24）のイメージタイプとブートROM の扱いを確認。

## Step 1. SDK サブモジュールの準備

[reference/sdk-acquisition.md](../reference/sdk-acquisition.md) 参照。

- [ ] 対象 SoC のデバイスファイルが `sdk/devices-rt/<family>/<device>/` にあるか確認
      （無ければ manifests が指す devices リポジトリのリビジョンを確認）。
- [ ] startup（`gcc/startup_*.S`）・リンカ ld・`system_*.c/h`・fsl ドライバの所在を把握。

## Step 2. チップ層 `asp3/arch/arm_m_gcc/<chip>_mcuxsdk/`

`imxrt600_mcuxsdk` をコピーして作る。

- [ ] `arch.cmake`：`TOPPERS_ENABLE_TRUSTZONE`（Step 0 の判断）・`TOPPERS_CORTEX_M33`・
      `__TARGET_ARCH_THUMB`・FPU 定義・`core_*` ソースの取り込み。
- [ ] `chip_sil.h`：`TBITW_IPRI`（割込み優先度ビット幅）・エンディアン。
- [ ] `chip_kernel.h`：`TMIN_INTPRI`。
- [ ] `chip_stddef.h` / `chip_syssvc.h` / `chip_kernel_impl.h` / rename 一式
      （`genrename.py` で `chip_rename.h`/`chip_unrename.h` を生成）。

## Step 3. ターゲット層 `asp3/target/<board>_mcuxsdk/`

`evkmimxrt685_mcuxsdk` をコピーして作る。

- [ ] `<board>_mcuxsdk.h`：`CPU_CLOCK_HZ`（SDK 既定クロックに合わせる・Phase A 値を
      流用しない）・`TMAX_INTNO`・`SIL_DLY_TIM*`（実機 dlynse で後較正）。
- [ ] `target_timer.c/h`：HRT。CTIMER を fsl_ctimer で。クロック換算は整数分周で
      1MHz に最も近い値（ppm 誤差は許容・文書化）。
- [ ] `target_serial.c`：SIO を fsl_usart の transactional API＋コールバックで実装。
      `target_fput_log` はレジスタ直/フォールバックでポーリング出力。
- [ ] `target_kernel_impl.c`：`target_initialize`/`target_exit`・`software_term_hook`
      の weak 定義。クロック/PMIC は SDK の `BOARD_BootClockRUN` に委ねる。
- [ ] `target_kernel.py`：ベクタテーブルの整列属性（VTOR 要件）。
- [ ] `target_stddef.h`：`<stdint.h>`・`tool_stddef.h`・`TOPPERS_assert_abort`（必須）。
- [ ] `target.cmake`：`ASP3_LDSCRIPT`・各ソース・`-Wl,--undefined=<FCFBシンボル>`。
- [ ] rename 一式を `genrename.py` で生成。
- [ ] `target_os_awareness.py`：チップ層（NVIC）API の再エクスポート。

## Step 4. ボードプロジェクト `<board>/<app>/`

`evkmimxrt685/sample1` をコピー（build/ は除く）。

- [ ] `board/`：SDK examples から `board.c/h`・`clock_config`・`pin_mux`・`flash_config`
      をコピー（BSD-3）。
- [ ] `main.c`：`BOARD_InitBootPins`→`BOARD_InitBootClocks`→`sta_ker()`。
      `DbgConsole_Init` スタブ（GC されない参照対策）。
- [ ] リンカ ld：SDK の ld をコピーし `.kernel_vector`（`ALIGN(512)`・`> m_text`）を追加
      （[snippets/kernel_vector-section.ld](../snippets/kernel_vector-section.ld)）。
- [ ] `CMakeLists.txt`：`ASP3_TARGET_DIR`＝新ターゲット・`ASP3_APPLDIR`/`ASP3_APPLNAME`・
      使う fsl ソースと include を列挙（SDK の CMake は使わない）。
- [ ] `CMakePresets.json`：**version 7**（VS Code 拡張対応）。
- [ ] fsl ヘッダ include を `#pragma push_macro/pop_macro` で挟む（マクロ二重定義警告対策）。

## Step 5. ビルド → 実機検証

- [ ] `cmake --preset Debug && cmake --build build/Debug` が**警告ゼロ**で asp.elf 生成。
- [ ] sample1：バナー →`task1 is running`→ `r` 送信で task1→2→3（ディスパッチ OK）。
- [ ] test_porting（`-DASP3_APPLDIR`/`APPLNAME`/`EXTRA_APP_C_FILES`）→ `# 6/6 passed`。
- [ ] testexec：`scripts/testexec_mcuxsdk.py`（cpuexc1/4 は arm_m 共通の既知 FAIL・
      cpuexc10 は SKIP＝正常）。
- [ ] dlynse で `SIL_DLY_TIM*` を較正し NG=0 にして `<board>_mcuxsdk.h` を更新。
- [ ] OS Awareness（`ninja ... osdebug`）。

## Step 6. 記録

- [ ] `docs/verification.md` に検証スナップショット（較正値・testexec 内訳）。
- [ ] README の構成・検証状況表を更新。
- [ ] 経緯の正本 `asp3/asp3_core/docs/dev/nxp-integration.md` に追記。
- [ ] CI（`.github/workflows/ci.yml`）に新ボードのビルドステップを追加（任意）。
