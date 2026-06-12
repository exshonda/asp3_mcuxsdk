# ブート経路・EXC_RETURN・ベクタ配置の落とし穴（NXP MCUXpresso SDK）

SDK統合で実機ブリングアップ時に踏んだ地雷の root cause と対策。正本は
`asp3/asp3_core/docs/dev/nxp-integration.md` と本リポジトリ `docs/tech-notes.md`。

## 1. SDK startup は Secure ブート → TrustZone を定義する（INVPC の原因）

**症状**：バナーまでは出る（または出ないこともある）が、**最初のタスクディスパッチで
即死**。CFSR=0x40000（INVPC = invalid PC load on EXC_RETURN）。

**root cause**：SDK の `startup_MIMXRT685S_cm33.S` でブートした CPU は **Secure 状態**で
実行される。Secure では例外復帰の `EXC_RETURN` に S/ES ビットが立った値
（0xFFFFFFFD 系）が要る。ASP3 が `TOPPERS_ENABLE_TRUSTZONE` 未定義だと
EXC_RETURN=0xFFFFFFBC（S/ES=0）を使い、復帰時に INVPC で落ちる。

**対策**：チップ層 `arch.cmake` で `TOPPERS_ENABLE_TRUSTZONE` を**定義する**
（`asp3/arch/arm_m_gcc/imxrt600_mcuxsdk/arch.cmake`。QEMU mps2_an521 と同じ構成）。

**重要な対比**：asp3_core 本体のベアメタル版（`target/mimxrt685evk_gcc`・Phase A）は
**逆**で、`TOPPERS_ENABLE_TRUSTZONE` は**未定義**。自前ブート（ベクタテーブル
index 9＝オフセット 0x24 のイメージタイプに bit14 を立てた「プレーンイメージ」）で
非Secure として起動するため。SDK startup はベクタ9=0 のイメージで、ブートROM の
扱いが異なる。

> **移植時の最初の確認**：新ボードで、ブートが
> (a) SDK startup 経由（Secure → TrustZone 定義）か、
> (b) 自前プレーンイメージ（非Secure → 未定義）か
> を決めるのは**ベクタ9の値とブート経路**。ここを取り違えると INVPC で即死する。

## 2. ASP3 ベクタテーブルが orphan セクション → リンク失敗

**症状**：`.vector will not fit in region m_interrupts` / `LMA overlap`。

**root cause**：ASP3 の cfg が生成する `_kernel_vector_table`（セクション `.vector`）は、
SDK のリンカスクリプトでは **orphan セクション**になり、SDK ブートベクタ用の
`m_interrupts` 領域（0x08001000・長さ 0x130＝SDKブートベクタでちょうど満杯）へ
引き寄せられて溢れる。

**対策**：SDK の `MIMXRT685Sxxxx_cm33_flash.ld` をボードプロジェクトへ**コピー**し、
専用セクション `.kernel_vector`（`ALIGN(512)`・`> m_text`）を追加して明示配置する
（`evkmimxrt685/sample1/MIMXRT685Sxxxx_cm33_flash.ld`）。ブート時のベクタは SDK の
`.isr_vector`（0x08001000）のまま。実行時に arm_m 共通の `core_initialize()` が
VTOR をこの `.kernel_vector` テーブルへ切り替える（＝ASP3 が NVIC を掌握。
pico の `--wrap` のような小細工は不要・SDK は素直に従う）。

snippet: [snippets/kernel_vector-section.ld](../snippets/kernel_vector-section.ld)

## 3. VTOR 整列要件

`ALIGN(512)` は VTOR の整列要件（テーブルのエントリ数×4 以上の 2 のべき乗）を満たす。
ベクタ数が増える（割込み源の多いボード）と必要整列が大きくなる：エントリ数 N に対し
`2^⌈log2(N×4)⌉`。RT685（INTNO 上限 59+16）では 512 で足りる。`target_kernel.py` が
テーブル変数に aligned 属性を生成し、ld の ALIGN と二重で担保する。

## 4. HRT は正確な 1MHz にならない（+421ppm・許容）

SDK 既定 `BOARD_BootClockRUN`：main_pll=528MHz×18/19=**500.21MHz**、
CPU=main_clk/2=**250.105MHz**。HRT（CTIMER0）は main_clk を整数プリスケーラで割るため、
500.21MHz/500=**1.000421MHz**（+421ppm）。`get_tim` は約 36ms/日 進むが実用上問題なし
（テスト全PASS）。厳密 1MHz が要れば PLL 構成変更が要るが、SDK 既定から逸脱するため不採用。

> Phase A（自前 PLL・300MHz）とはクロックが**異なる**。`CPU_CLOCK_HZ`・`SIL_DLY_TIM*` を
> Phase A から流用しないこと。SDK版の値は `target/evkmimxrt685_mcuxsdk/evkmimxrt685_mcuxsdk.h`。

## 5. fsl ヘッダと arm_m.h のマクロ二重定義

`EXC_RETURN_PREFIX`（0xff000000）等を ASP3 の `arm_m.h` と CMSIS `core_cm33.h` が同値で
定義し警告になる。fsl ヘッダの include を `#pragma push_macro("X") / pop_macro("X")` で
挟んで警告ゼロ化する（`target_serial.c`・`target_timer.h` に実装例）。

## 6. その他のリンク・ビルド時のハマり

- **`DbgConsole_Init` 未解決参照**：`board.c` が `BOARD_InitDebugConsole` の関数アドレスを
  XIP 判定マクロに使うため、未使用でも GC されず参照が残る。debug_console コンポーネント
  一式をリンクする代わりに `main.c` のスタブで充足する。
- **`target_stddef.h` の必須要素**：`<stdint.h>`・`tool_stddef.h` の include と
  `TOPPERS_assert_abort` の定義が必要。欠けると `uint32_t` 未定義・`Inline` 未定義の
  大量エラー。
- **`software_term_hook`**：weak 定義を `target_kernel_impl.c` に置く（無いと
  `core_terminate` からの参照でリンクエラー）。
