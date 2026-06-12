# 実機で動かない時のデバッグ・チェックリスト（NXP MCUXpresso SDK）

症状から原因へ。詳細は [reference/boot-vector-pitfalls.md](../reference/boot-vector-pitfalls.md)・
`docs/tech-notes.md`・[reference/flash-debug-tools.md](../reference/flash-debug-tools.md)。

## まず確認（ツール起因を除外）

- [ ] **UART が複数プロセスに開かれていないか**：`fuser -v /dev/ttyACM0`。出力の
      「文字化け・欠落・無出力」の最頻原因。`fuser -k /dev/ttyACM0` で掃除。
- [ ] **プローブのファームウェア**：`lsusb`（`1fc9:0090`=CMSIS-DAP / `1366:0105`=J-Link）。
      使うツールと一致しているか。
- [ ] **書込みが本当に成功したか**：`LinkServer flash ... load asp.elf` のログ。
      書込み前接続で旧イメージの断片が UART に混ざることがある（マーカ判定は許容設計）。
- [ ] **接続が壊れて見える**（メモリ全FF・フラッシュ 0）：bootROM ストールや残留
      redlink プロセス。gdbserver を止めて `flash load` からやり直す。

## 症状 → 原因

| 症状 | 最有力の原因 | 対策 |
|---|---|---|
| バナーは出るが**最初のタスク切替で即死** | TrustZone 未定義（EXC_RETURN 不正・INVPC/CFSR=0x40000） | `arch.cmake` で `TOPPERS_ENABLE_TRUSTZONE` を定義（SDK startup は Secure ブート） |
| **リンクが通らない**（`.vector will not fit` / LMA overlap） | ASP3 ベクタが orphan で `m_interrupts` に溢れる | ld に `.kernel_vector`（ALIGN(512)・> m_text）を追加 |
| バナーも出ない・全く無反応 | クロック未初期化／`sta_ker()` 前で停止／ベクタ整列違反 | `main()` の `BOARD_InitBootClocks` を確認・VTOR 整列（ALIGN 512）・gdb でリセット直後を追う |
| **低レベルログ（バナー）が出ない**が動いてはいる | `target_fput_log` が stdio/DbgConsole 経由 | USART レジスタ直/ポーリング実装に |
| `get_tim` が少しずつズレる | HRT が +421ppm（SDK 既定クロックの整数分周） | 仕様内・許容（厳密 1MHz は PLL 変更が要る） |
| ビルド時 `EXC_RETURN_PREFIX` 等の**二重定義警告** | fsl ヘッダと arm_m.h が同マクロ定義 | fsl include を `#pragma push_macro/pop_macro` で挟む |
| `DbgConsole_Init` 未解決参照 | board.c が関数アドレスを XIP 判定に使い GC されない | `main.c` にスタブを置く |
| `uint32_t`/`Inline` 未定義の大量エラー | `target_stddef.h` の必須 include 欠落 | `<stdint.h>`・`tool_stddef.h`・`TOPPERS_assert_abort` を入れる |
| `core_terminate` からのリンクエラー | `software_term_hook` 未定義 | weak 定義を `target_kernel_impl.c` に |

## gdb で追うとき

- [ ] **`--attach` で接続**（通常接続はリセット＋bootROM ストールで観測できない）。
      `ninja ... osdebug` が attach 構成。
- [ ] gdb に **`set mem inaccessible-by-default off`**（LinkServer のデバイス定義は
      SRAM データバスエイリアスを持たず、無いとカーネルオブジェクト読出しが
      `Cannot access memory` で拒否される）。
- [ ] フォルト解析は **CFSR/HFSR・スタックフレームのレジスタを一次情報**にする
      （ICF でバックトレースが無関係な関数を指すことがある）。INVPC は CFSR bit18。
- [ ] バッチ自動化で gdbserver（非 attach）に `continue` すると停止を待たない問題あり
      → attach＋`interrupt`＋`shell sleep`、またはシリアルマーカ判定
      （`testexec_mcuxsdk.py`）を使う。

## 既知の非PASS（正常・追わなくてよい）

- `cpuexc1` / `cpuexc4`：arm_m 共通の上流由来の既知 FAIL
  （`asp3/asp3_core/docs/dev/issue-cpuexc-armm.md`）。
- `cpuexc10`：SKIP（not necessary）。
