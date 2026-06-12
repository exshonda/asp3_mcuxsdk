# 書込み・デバッグ・シリアルのツール操作（LinkServer / J-Link）

正本は `docs/host-setup.md`（ホスト構築）と `docs/tech-notes.md` §6-7（LinkServer の注意）。
ここは移植作業で実際に打つコマンドの早見表。

## プローブとファームウェア

EVK-MIMXRT685 のオンボードプローブは **LPC-Link2**。`lsusb` で現在のファームを判別：

| lsusb | ファームウェア | ホストツール | 既定 |
|---|---|---|---|
| `1fc9:0090 ... CMSIS-DAP` | CMSIS-DAP（出荷時標準） | LinkServer | ◎ 本リポジトリの標準 |
| `1366:0105 SEGGER J-Link` | J-Link 化済み | SEGGER J-Link Software | オプション |

切替は LPCScrypt（`program_CMSIS` / `program_JLINK`・JP1 装着で DFU 起動）。手順は
`docs/host-setup.md` §3。

## 書込み（標準＝LinkServer）

```bash
cd evkmimxrt685/sample1/build/Debug
/usr/local/LinkServer/LinkServer flash MIMXRT685S:EVK-MIMXRT685 load asp.elf
```

`load` は書込み後に**自動でリセット実行**する（`--no-reset` で抑止可）。
LinkServer は PATH に入らない（フルパス）。各スクリプトは環境変数 `LINKSERVER` で上書き可。

## ツール切替（ターゲット名・手順は共通／オプションだけ変える）

| ツール | J-Link に切替えるオプション | 既定 |
|---|---|---|
| CMake ターゲット（flash/gdbserver/gdb/debug/osdebug） | `-DMIMXRT685_DEBUGGER=jlink` | linkserver |
| ボードランナ（asp3_core `run_board_mimxrt685evk.sh`） | `MIMXRT685_FLASH=jlink` | linkserver |
| testexec ラッパ（`scripts/testexec_mcuxsdk.py`） | `--flash-tool jlink` | linkserver |

J-Link 直叩きの例：

```bash
printf 'loadfile asp.elf\nr\ng\nqc\n' > flash.jlink
JLinkExe -device MIMXRT685S_M33 -if SWD -speed 4000 -autoconnect 1 -NoGui 1 -CommandFile flash.jlink
```

## シリアル（VCOM）

```bash
stty -F /dev/ttyACM0 115200 raw -echo
cat /dev/ttyACM0                 # 読むプロセスは同時に1つだけ
printf 'r' > /dev/ttyACM0        # sample1 で rot_rdq（task切替）
```

**VCOM を読むプロセスは同時に 1 つだけ**。複数リーダがバイトを奪い合い「文字化け・
無出力」に見える（最も時間を溶かした罠）。確認 `fuser -v /dev/ttyACM0`、掃除
`fuser -k /dev/ttyACM0`。

## テスト実行（ボード占有・並行禁止）

```bash
# 機能テスト全件（約30分・ボードとVCOMを占有）
scripts/testexec_mcuxsdk.py
scripts/testexec_mcuxsdk.py task1 sem1        # 個別
scripts/testexec_mcuxsdk.py --rejudge          # 保存ログの再判定のみ
scripts/testexec_mcuxsdk.py --flash-tool jlink # J-Link時

# 移植検証6項目（asp3_core 標準の test_porting）→ "# 6/6 passed"
```

## OS Awareness（実行中システムの観測）

```bash
ninja -C evkmimxrt685/sample1/build/Debug osdebug   # gdbserver(--attach)+gdb-multiarch
# (gdb) interrupt → atask / stask / sem / cyc / intr
```

**`--attach` を使う**こと（通常接続はシステムリセット＋bootROM ストールで止まり、
FlexSPI 未設定でフラッシュが 0・SRAM 読めずに見える）。また LinkServer のデバイス定義は
SRAM のデータバスエイリアスを持たないため、gdb に
**`set mem inaccessible-by-default off` が必須**（`asp3_core` の
`target/mimxrt685evk_gcc/linkserver-debug.gdb` に組込み済み）。

## LinkServer 自動化の注意（tech-notes §6 要約）

- gdbserver（非 attach）に gdb -batch で `continue` すると停止を待たず次へ進むことがある。
  スクリプト化は attach モード＋`interrupt`＋`shell sleep` が確実。
- 接続が不安定（メモリ全FF・フラッシュ 0 に見える）なときは残留 redlink プロセスや
  bootROM ストールが原因。gdbserver を止めて `flash load` からやり直すのが早い。
- `testexec_mcuxsdk.py` は gdb を使わずシリアルの完走マーカで判定する（最も安定）。
