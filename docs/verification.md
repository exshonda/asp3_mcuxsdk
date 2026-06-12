# 実機検証の状況と再実行手順（2026-06-12 時点）

EVK-MIMXRT685（Phase B＝MCUXpresso SDK 統合ビルド）の実機検証スナップショット。
経緯・root cause 解析の正本は `asp3/asp3_core/docs/dev/nxp-integration.md`。
Phase A（asp3_core 本体のベアメタル `mimxrt685evk` ターゲット）の検証状況は
asp3_core の `target/mimxrt685evk_gcc/target_user.md` を参照。

## 検証結果サマリ（SDK統合ビルド）

| 項目 | 結果 |
|---|---|
| sample1（バナー・タスク切替 `r`） | OK |
| test_porting（TAP 6項目） | **6/6 passed** |
| testexec（機能テスト36本） | **PASS=33 / SKIP=1 / FAIL=2** |
| dlynse（sil_dly_nse 較正） | NG=0（`SIL_DLY_TIM1/2`=27/19） |
| OS Awareness（atask/stask/intr） | OK（LinkServer gdbserver --attach 経由） |

環境：MCUXpresso SDK release/26.03.00／arm-none-eabi-gcc 13.2.1／
LinkServer v26.5.59（プローブ＝LPC-Link2 CMSIS-DAP V5.460）。
J-Link 構成（V9.30・切替前）でも同一結果を確認済み。

### testexec の非PASS 内訳（すべて既知・正常）

| テスト | 判定 | 理由 |
|---|---|---|
| cpuexc1・cpuexc4 | FAIL | 上流 arm_m 固有の特性（PRIMASK SIL 中の UsageFault が HardFault 昇格）。mps2／pico2_arm／Phase A と同一挙動。`asp3/asp3_core/docs/dev/issue-cpuexc-armm.md` |
| cpuexc10 | SKIP | `This test program is not necessary.`（このターゲット構成では不要＝正常） |

特記：**hrt1・dlynse は実機専用テストで PASS**。dlynse の較正値は
Phase A（300MHz）の実測サイクル数（呼出7cyc・ループ5cyc）を SDK クロック
（250.105MHz）に換算した理論下限 27/19
（`asp3/target/evkmimxrt685_mcuxsdk/evkmimxrt685_mcuxsdk.h`）。実測で NG=0 を確認。

## 再実行手順

### testexec（全件・約30分）

```bash
scripts/testexec_mcuxsdk.py                 # 全36テスト
scripts/testexec_mcuxsdk.py task1 sem1      # 個別
scripts/testexec_mcuxsdk.py --rejudge       # 保存済みログの再判定のみ
scripts/testexec_mcuxsdk.py --flash-tool jlink ...   # J-Linkファームウェア時
```

ログ：`evkmimxrt685/sample1/build/TestExec/logs/<test>.{build,ninja,flash,serial}.log`

### test_porting（移植検証6項目）

```bash
cd evkmimxrt685/sample1
cmake --preset Debug -B build/Debug-porting \
    -DASP3_APPLDIR=test/porting -DASP3_APPLNAME=test_porting \
    -DASP3_EXTRA_APP_C_FILES=test/porting/tap.c
cmake --build build/Debug-porting
cd build/Debug-porting
bash ../../../../asp3/asp3_core/scripts/ci/run_board_mimxrt685evk.sh 60
# → "# 6/6 passed"
```

### sample1（手動確認）

```bash
cd evkmimxrt685/sample1 && cmake --preset Debug && cmake --build build/Debug
cd build/Debug
/usr/local/LinkServer/LinkServer flash MIMXRT685S:EVK-MIMXRT685 load asp.elf
# 別プロセスで /dev/ttyACM0 115200 を1つだけ開いてバナー・task1 を確認
# 'r' 送信で task1→2→3 のローテーションを確認
```

### OS Awareness

```bash
# 観測対象（sample1等）を flash で実行状態にしてから：
/usr/local/LinkServer/LinkServer gdbserver --attach --gdb-port 3333 MIMXRT685S:EVK-MIMXRT685 &
gdb-multiarch build/Debug/asp.elf \
  -ex 'set mem inaccessible-by-default off' \
  -ex 'target remote :3333' \
  -ex "python import sys; sys.path.insert(0, '<repo>/asp3/target/evkmimxrt685_mcuxsdk')" \
  -ex 'source <repo>/asp3/asp3_core/scripts/gdb_os_aware/os_awareness.py'
# (gdb) interrupt → atask / stask / sem / cyc / intr
```

注意点（--attach・inaccessible-by-default）は `docs/tech-notes.md` §LinkServer を参照。
