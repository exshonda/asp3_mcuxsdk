# 実機検証ホストPCの初期セットアップ（一度きり）

新しい Linux PC を EVK-MIMXRT685 の実機検証ホストにする際の準備手順。
2026-06-12 に deskmini（Ubuntu）で実際に踏んだ問題と解決をそのまま記録したもの。
日常の書込み・テスト・デバッグ操作は本リポジトリ README と asp3_core の
`target/mimxrt685evk_gcc/target_user.md` を参照。

## 1. 必要ツールと所在

| ツール | 入手 | 備考 |
|---|---|---|
| arm-none-eabi-gcc / cmake / ninja | ディストリのパッケージ | 検証済み: gcc 13.2.1 |
| **LinkServer**（標準の書込み・デバッグCLI） | MCUXpresso Installer が `~/MCUXpressoInstaller/.cache/` に deb.bin を取得（下記） | **PATH に入らない**。実体: `/usr/local/LinkServer/LinkServer`。検証済み: v26.5.59 |
| SEGGER J-Link Software | MCUXpresso Installer（components: JLink）または segger.com | **任意**（プローブをJ-Link化した場合のみ）。検証済み: V9.30 |
| LPCScrypt | **LinkServer の deb.bin に同梱**（`LPCScrypt.deb`。導入後 `/usr/local/LinkServer/lpcscrypt`） | プローブのファームウェア切替に使用。NXPサイト単体配布はログインが必要 |
| gdb-multiarch | ディストリのパッケージ | OS Awareness（osdebug）に必要 |

## 2. LinkServer のインストール

MCUXpresso Installer の CLI はヘッドレスでは GUI 部の制約（`xvfb-run` が必要）と
`pkexec` での root 昇格で止まるため、**deb のダウンロードだけ**させて手動で入れるのが確実：

```bash
# ダウンロードのみ（pkexec で止まったら Ctrl-C / kill してよい）
xvfb-run -a ~/MCUXpressoInstaller/MCUXpressoInstallerCLI install -c LinkServer

# deb.bin（makeself 自己展開形式）を root なしで展開
cd /tmp && mkdir lsdeb && cd lsdeb
bash ~/MCUXpressoInstaller/.cache/LinkServer_*.x86_64.deb.bin --noexec --keep --target extracted
ls extracted   # → LinkServer_*.deb / LPCScrypt.deb / MCU-Link.deb

sudo dpkg -i extracted/LinkServer_*.x86_64.deb
/usr/local/LinkServer/LinkServer probes   # プローブ認識の確認
```

CMSIS-DAP（VID:PID `1fc9:0090`）の udev ルールは LinkServer の deb が入れる。
認識しないときは抜き差し、または `sudo udevadm control --reload-rules && sudo udevadm trigger`。

## 3. プローブのファームウェア確認と切替

EVK-MIMXRT685 のオンボードデバッグプローブは **LPC-Link2（LPC4322）**。
ファームウェアは `lsusb` で判別できる：

| lsusb 表示 | ファームウェア | ホストツール |
|---|---|---|
| `1fc9:0090 NXP LPC-LINK2 CMSIS-DAP Vx.xxx` | **CMSIS-DAP（出荷時標準）** | LinkServer |
| `1366:0105 SEGGER J-Link` | J-Link 化済み | SEGGER J-Link Software |

切替（どちらの方向も）は **LPCScrypt** で行う：

1. J5（デバッグUSB）を抜く
2. **JP1（Link2 の DFU ブートジャンパ）を装着**
3. J5 を接続（Link2 が DFU モードで起動）
4. `program_CMSIS`（標準へ）または `program_JLINK`（J-Link化）を実行
   - Linux: `/usr/local/LinkServer/lpcscrypt/scripts/program_CMSIS`
   - Windows: LPCScrypt インストール先の `scripts\program_CMSIS.cmd`
5. J5 を抜き、**JP1 を外して**再接続

> 2026-06-12 の実績：J-Link 化されていた本ボードを Windows＋LPCScrypt で
> CMSIS-DAP に書き戻し、以後 LinkServer を標準とした。
> 各ツールの J-Link 切替オプション（`-DMIMXRT685_DEBUGGER=jlink` 等）は
> README／target_user.md を参照。

## 4. VCOM（シリアル）の権限と注意 — 最初に必ず

- VCOM は `/dev/ttyACM0`（115200bps）。`dialout` グループに入っていれば開ける
  （`sudo usermod -aG dialout $USER` → 再ログイン）。
- **VCOM を読むプロセスは同時に 1 つだけ**にすること。複数のリーダ（`cat`・
  `picocom`・キャプチャスクリプトの残骸）が同じ tty を開くとバイトを奪い合い、
  出力が「欠落・文字化け・無出力」に見える。**実機検証時にこれで原因究明に
  最も時間を浪費した**。確認は `fuser -v /dev/ttyACM0`、掃除は
  `fuser -k /dev/ttyACM0`。
- テストランナ（`scripts/testexec_mcuxsdk.py`・asp3_core の
  `run_board_mimxrt685evk.sh`）はボードと tty を占有するため**並行実行禁止**。

## 5. ボードのジャンパ

| ジャンパ | 設定 | 意味 |
|---|---|---|
| **JP22** | **2-3 短絡を推奨** | 中央ピンが SoC の LDO_ENABLE。オープンだとフローティング（データシートの禁止事項）。外部 PMIC からコア電圧を供給する設定 |
| JP1 | 通常オープン | Link2 の DFU ブート（ファームウェア書換え時のみ装着） |
