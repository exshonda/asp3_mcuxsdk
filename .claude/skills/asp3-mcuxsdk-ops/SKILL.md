---
name: asp3-mcuxsdk-ops
description: >
  asp3_mcuxsdk リポジトリでの日常操作（ビルド／書込み／テスト／性能計測／デバッグ）を、
  対応2ボード（EVK-MIMXRT685・FRDM-MCXN947）それぞれの正しいプリセット名・LinkServer
  デバイス名・プロジェクトパス・ASP3ターゲット名で叩くための早見表。
  「sample1 をビルドして書き込みたい」「test_porting / testexec を回したい」「どのプリセット・
  どの LinkServer デバイス名」「2ボードのどちらの手順か」「DWT で性能計測したい」「デバッガを
  J-Link に切替えたい」「Windows ホストで叩きたい（LinkServer.exe のパス・COM ポート・pyserial）」
  というとき使う（build / flash / LinkServer / testexec / preset / perf / DWT / Windows / COM）。
  新規ボードの追加や実機が動かない root cause 解析は porting-asp3-to-nxp、禁則・全体像は
  CLAUDE.md / asp3_core AGENTS.md、ホスト構築・ツール詳細は docs/ を参照（そちらへ委譲する）。
---

# asp3_mcuxsdk オペレーション早見表

このリポでの **build / flash / test / perf / debug の「叩き方」** に徹したスキル。
概念・規約・root cause は他に委譲する。

## 使い分け（最初に確認）

| やりたいこと | 参照先 |
|---|---|
| **既存ボード（RT685 / MCXN947）でビルド・書込み・テスト・計測** | ← **このスキル** |
| 新規 NXP ボードを追加する／実機が動かない root cause | `.claude/skills/porting-asp3-to-nxp/`（移植・ブリングアップ） |
| 書込み・デバッガ・シリアルのツール詳細（ファームウェア切替・OS Awareness） | `porting-asp3-to-nxp/reference/flash-debug-tools.md` |
| 禁則・リポ全体像 | `CLAUDE.md` / `asp3/asp3_core/AGENTS.md` |
| ホストPC構築・概念（TrustZone/ベクタ/HRT精度・LinkServer の罠） | `docs/host-setup.md` / `docs/tech-notes.md` |

> このリポの作業は **NXP 側ファイルに閉じる**（`asp3/asp3_core/` submodule は無変更が原則）。

## 1. ボード対応表（最重要）

ボードごとに「プロジェクトパス・ASP3ターゲット・SDKデバイス・LinkServer名」が違う。
ここを取り違えるとビルド／書込みが失敗する。

| 項目 | EVK-MIMXRT685 | FRDM-MCXN947 |
|---|---|---|
| プロジェクト（cd 先） | `evkmimxrt685/sample1` | `frdm_mcxn947/sample1` |
| CMake プリセット | `Debug`（/ `Release`） | `Debug`（/ `Release`） |
| ASP3 ターゲット | `evkmimxrt685_mcuxsdk` | `frdmmcxn947_mcuxsdk` |
| SDK デバイス submodule | `sdk/devices-rt`（RT600/MIMXRT685S） | `sdk/devices-mcx`（MCXN/MCXN947） |
| LinkServer `device:board` | `MIMXRT685S:EVK-MIMXRT685` | `MCXN947:FRDM-MCXN947` |
| デバッグ USB / VCOM | J5 / `/dev/ttyACM0` 115200 | J17 / `/dev/ttyACM0` 115200 |
| VCOM（Windows） | `COM<n>`（MCU-Link VCom Port）115200 | 同左（§2.5 参照） |
| ブート | FlexSPI XIP（FCFB/flash_config 要） | 内蔵フラッシュ（XIP/FCFB 不要） |
| シリアル | Flexcomm0 USART（fsl_usart） | LPUART4（fsl_lpuart） |
| コアクロック / HRT | 250.105MHz / CTIMER0 1.000421MHz（+421ppm） | 150MHz / CTIMER0 正確に 1MHz |

両ボードとも生成物は `build/<preset>/asp.elf`。チップ層は両方とも
`TOPPERS_ENABLE_TRUSTZONE` 定義（SDK startup が Secure ブートのため。詳細は porting skill）。

## 2. ビルド

```bash
cd evkmimxrt685/sample1      # または frdm_mcxn947/sample1
cmake --preset Debug
cmake --build build/Debug    # → build/Debug/asp.elf
```

アプリ差し替え（asp3_core 標準の `-D`。パスは asp3_core ルート基準）：

```bash
CORE=$PWD/../../asp3/asp3_core
cmake --preset Debug -B build/Debug-porting \
  -DASP3_APPLDIR=$CORE/test/porting -DASP3_APPLNAME=test_porting \
  -DASP3_EXTRA_APP_C_FILES=$CORE/test/porting/tap.c
cmake --build build/Debug-porting
```

> `CMakePresets.json` は **version 7**（MCUXpresso for VS Code 拡張が環境ブロックを注入する）。
> 注入されるマシン固有パスはビルドで未参照のため、CI や別環境でもビルドは通る。

### 性能計測（DWT CYCCNT 時間源）

`histogram` サービスの時間源を μs 分解能の `fch_hrt()` から **DWT CYCCNT（サイクル精度）** に
切り替えるには、ビルド時に `USE_ARM_DWT_PMCNT` を定義する（裏取り済み＝ビルド通る）：

```bash
cmake --preset Debug -B build/DbgPerf -DCMAKE_C_FLAGS=-DUSE_ARM_DWT_PMCNT
cmake --build build/DbgPerf
```

- ns 変換係数は `target_syssvc.h` の `HIST_CONV_TIM`（`CPU_CLOCK_HZ` 依存・ボード毎）。
- 仕組み・他アーキ（arm_gcc=PMCCNTR / riscv=cycle CSR）との関係は
  `asp3/asp3_core/arch/arm_m_gcc/common/PERF_DWT.md`（概念の正本）。
- perf0..5 ベンチアプリは asp3_core の `test/` 側にあり `histogram` syssvc を要する。
  このリポでの turnkey なビルド線は未整備（必要なら asp3_core `test/testexec.py` の
  perf 結線＝`SYSOBJ=histogram` を参照）。

## 2.5 ホスト OS 別の読み替え（Linux / Windows）

コマンド本体は同じで、**ツールのパスとシリアルポートの指し方だけが違う**。

| 項目 | Linux | Windows |
|---|---|---|
| LinkServer | `/usr/local/LinkServer/LinkServer` | `C:\NXP\LinkServer_<version>\LinkServer.exe`（例 `LinkServer_26.5.59`）。**PATH 外なのでフルパス** |
| J-Link CLI | `JLinkExe` | `JLink.exe`（`C:\Program Files\SEGGER\JLink*\`） |
| VCOM | `/dev/ttyACM0` | `COM<n>`。`MCU-Link VCom Port` を選ぶ（下記で確認） |
| VCOM の占有確認 | `fuser /dev/ttyACM0` | 不要（COM は排他オープン＝二重に開けば例外） |
| testexec の依存 | 標準 Python のみ | **pyserial 必須**（`pip install pyserial`） |
| シリアル端末 | `stty` + `cat` | PowerShell の `System.IO.Ports.SerialPort`、または TeraTerm 等 |

COM 番号の確認（Bluetooth の COM が複数並ぶので名前で選ぶ）：

```powershell
Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match 'COM\d+' } | Select-Object Name
# → "MCU-Link VCom Port (COM8)"
# レジストリでも可: Get-ItemProperty 'HKLM:\HARDWARE\DEVICEMAP\SERIALCOMM' の USBSER000
```

PowerShell でのシリアル読み出し（`cat /dev/ttyACM0` 相当。**読むプロセスは1つだけ**）：

```powershell
$sp = New-Object System.IO.Ports.SerialPort 'COM8',115200,'None',8,'One'
$sp.Open(); $sp.ReadExisting()      # ループで回して読む
$sp.Write('r')                      # sample1: rot_rdq で task1→task2→task3 切替
$sp.Close()
```

> `git-bash` から叩く場合も LinkServer はフルパス。COM ポートは `/dev/ttyACM0` 相当が無いので
> シリアルは PowerShell か testexec ラッパ（pyserial）を使う。

## 3. 書込み・シリアル（標準＝LinkServer）

```bash
cd <board>/sample1/build/Debug
/usr/local/LinkServer/LinkServer flash MIMXRT685S:EVK-MIMXRT685 load asp.elf   # RT685
/usr/local/LinkServer/LinkServer flash MCXN947:FRDM-MCXN947     load asp.elf   # MCXN947
```

Windows は同じサブコマンドで実行ファイルだけ読み替える：

```powershell
& 'C:\NXP\LinkServer_26.5.59\LinkServer.exe' flash MCXN947:FRDM-MCXN947 load asp.elf
```

`load` は書込み後に**自動でリセット実行**。LinkServer は PATH に入らない（フルパス）。

```bash
stty -F /dev/ttyACM0 115200 raw -echo
cat /dev/ttyACM0            # ★読むプロセスは同時に1つだけ（fuser -k /dev/ttyACM0 で掃除）
printf 'r' > /dev/ttyACM0  # sample1: rot_rdq で task1→task2→task3 切替
```

> **VCOM 単独リーダ厳守**。複数リーダがバイトを奪い合い「文字化け・無出力」に見える
> （最も時間を溶かす罠）。ツール詳細（J-Link 切替・OS Awareness・ファームウェア確認）は
> `porting-asp3-to-nxp/reference/flash-debug-tools.md`。

最小チェック：バナー → `task1 is running (NNN)` → `r` で切替 → 仕上げに **test_porting** で `# 6/6 passed`。

## 4. 機能テスト（testexec ラッパ・ボード占有/並行禁止）

`scripts/testexec_mcuxsdk.py` が「1テストずつ configure→build→書込み→シリアル判定」を回す
（既定は **RT685**）。**Linux / Windows 両対応**（Windows は pyserial 必須。`--port` 省略時は
Linux `/dev/ttyACM0` ／ Windows は MCU-Link VCom を自動検出。LinkServer・J-Link のパスも
OS ごとに自動探索＝環境変数 `LINKSERVER` / `JLINK` で上書き可）。
MCXN947 は `--board` と `--ls-device` を渡す：

```bash
# EVK-MIMXRT685（既定）
scripts/testexec_mcuxsdk.py               # 全件（約30分）
scripts/testexec_mcuxsdk.py task1 sem1    # 個別
scripts/testexec_mcuxsdk.py --rejudge     # 保存ログの再判定のみ

# FRDM-MCXN947
scripts/testexec_mcuxsdk.py --board frdm_mcxn947/sample1 --ls-device MCXN947:FRDM-MCXN947

# J-Link ファームウェア時はどちらも --flash-tool jlink
```

Windows での起動例（COM 自動検出。明示するなら `--port COM8`）：

```powershell
python ..\..\scripts\testexec_mcuxsdk.py --board frdm_mcxn947/sample1 --ls-device MCXN947:FRDM-MCXN947
```

判定は完走マーカ（`All check points passed.` / hrt1・dlynse は専用マーカ）。
`perf*`・`arm_*` は対象外。既知 FAIL は cpuexc1/cpuexc4（arm_m 共通・上流由来。
asp3_core `docs/dev/issue-cpuexc-armm.md`）、cpuexc10 は SKIP。
**期待値（全36件）: PASS=33, FAIL=2（cpuexc1/cpuexc4）, SKIP=1（cpuexc10）**
— これ以外が出たら回帰を疑う。

## 5. デバッガ切替（標準 CMSIS-DAP+LinkServer ／ J-Link はオプション）

ターゲット名・手順は共通のまま、オプションだけ変える（詳細は flash-debug-tools.md）：

| ツール | J-Link に切替 | 既定 |
|---|---|---|
| CMake（`ninja flash`/`gdbserver`/`debug`/`osdebug`） | `-DMIMXRT685_DEBUGGER=jlink` | linkserver |
| asp3_core ボードランナ `run_board_mimxrt685evk.sh` | `MIMXRT685_FLASH=jlink` | linkserver |
| testexec ラッパ | `--flash-tool jlink` | linkserver |

## 6. オペレーション上の地雷（このリポ固有）

- **MCXN947 のビルドに `sdk/devices-mcx` が要る**。`--recurse-submodules` クローンなら入るが、
  MCX 追加より前のクローンは未初期化で
  `Cannot find source file: .../MCXN947/gcc/startup_MCXN947_cm33_core0.S` になる。
  → `git submodule update --init sdk/devices-mcx`（裏取り済み＝これで通る）。
- **CI は EVK-MIMXRT685 のみビルド**（`.github/workflows/ci.yml`）。MCXN947 はローカル検証
  （CI 未対応）。MCXN947 を変更したらローカルで `cmake --build` を必ず確認。
- **書込みツール（LinkServer）は PATH 外**。スクリプトは環境変数 `LINKSERVER` で上書き可。
- **submodule が親リポの記録より古いコミットに居ると、無い前提のヘッダで死ぬ**。実例：
  `asp3/asp3_core` が旧コミットのままだと `arch/arm_m_gcc/common/core_syssvc.h` が存在せず
  cfg1_out.c のコンパイルで `fatal error: core_syssvc.h: No such file or directory`。
  → `git submodule update --init --recursive`（`git submodule status` の先頭 `+` が食い違いの印）。
  「無いはずのファイルが無い」系のエラーは、まず submodule のコミット一致を疑う。
- **（Windows・シリアル読み出しコードを触るとき）pyserial の `timeout` に代入すると、同値でも
  `_reconfigure_port()` → `SetCommState`（CDC SET_LINE_CODING）が走り、受信中のバイトが化ける**。
  読み取りループ内で毎回代入すると、出力が密なテスト（dlynse）だけ文字化けして TIMEOUT に見える。
  → timeout は開いたとき1回だけ設定する（`testexec_mcuxsdk.py` は対処済み）。
- MCXN947 固有のブリングアップ地雷（CTIMER クロックディバイダ HALT・`-mcmse`）は
  `docs/verification.md` と porting skill に記録済み（運用では既に解決済み・再現不要）。

## 裏取り状況（2026-06-20 実行）

- 両ボードとも `cmake --preset Debug && cmake --build build/Debug` → `asp.elf` 生成を確認。
- `sdk/devices-mcx` 未初期化での MCXN947 ビルド失敗→`git submodule update --init` で解消を確認。
- LinkServer デバイス名は `/usr/local/LinkServer/devices/{EVK-MIMXRT685,FRDM-MCXN947}.json` で確認。
- `-DCMAKE_C_FLAGS=-DUSE_ARM_DWT_PMCNT` のビルド成功を確認。
- testexec ラッパの `--board`/`--ls-device`/`--flash-tool` を実コードで確認。

## 裏取り状況（2026-08-05 実行・Windows 11 ホスト / FRDM-MCXN947 実機）

- `cmake --preset Debug && cmake --build build/Debug` → `asp.elf` 生成、LinkServer 26.5.59 で
  `flash MCXN947:FRDM-MCXN947 load` 成功、COM8（MCU-Link VCom）でバナー→`task1 is running`→
  `r` 送信で `task2` 切替まで確認。
- `scripts/testexec_mcuxsdk.py` を Windows 対応（pyserial 経路・`cwd` 実行・CMake パスの `/` 化・
  LinkServer/J-Link パス自動探索）。全36件を実行し
  **PASS=33, FAIL=2（cpuexc1/cpuexc4＝既知）, SKIP=1（cpuexc10）** を確認
  （当初 PASS=32 と記載していたが合計が 36 に合わない転記誤り。2026-08-06 に訂正）。
- Linux 経路（termios+select・`/dev/ttyACM0`・`fuser`・`/usr/local/LinkServer`）は分岐内に温存。
  ただし**この日の Linux 実機での再確認は未実施**。

## 裏取り状況（2026-08-06 実行・Windows 11 ホスト / EVK-MIMXRT685 実機・J-Link FW）

- MCU-Link が **J-Link ファームウェア**の状態で一連の作業を確認（CMSIS-DAP に戻す必要なし）。
- `cmake --preset Debug && cmake --build build/Debug` → `asp.elf` 生成（m_flash_config・m_interrupts は
  100% 使用が正常）。`JLink.exe -device MIMXRT685S_M33 -if SWD -speed 4000 -CommandFile ...` で
  FlexSPI bank0 @0x08000000 へ書込み成功、バナー→`task1 is running`→`r` で `task2` 切替を確認。
- `scripts/testexec_mcuxsdk.py --flash-tool jlink --port COM19` で全36件を実行し
  **PASS=33, FAIL=2（cpuexc1/cpuexc4）, SKIP=1（cpuexc10）** を確認。
- **J-Link FW 時は `--port` の明示が必要**：ラッパの自動検出は `MCU-Link VCom` を探すが、
  J-Link FW の VCOM は `JLink CDC UART Port (COMn)` という名前で出るため引っかからない。
