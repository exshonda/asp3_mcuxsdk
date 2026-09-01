# TOPPERS/ASP3 の VS Code 向け MCUXpresso 拡張機能対応（EVK-MIMXRT685）

`evkmimxrt685/sample1` を **MCUXpresso for VS Code** 拡張でビルド・書込み・デバッグする手順です。
事前に、トップ [README.md](../../README.md) の「拡張機能とツールチェインのインストール」を済ませてください。

## ターゲット

- [EVK-MIMXRT685](https://www.nxp.com/design/design-center/development-boards-and-designs/EVK-MIMXRT685)
  （i.MX RT685・Cortex-M33。HiFi4 DSP は停止のまま CM33 のみ使用）

## コードのダウンロード

submodule（asp3_core・MCUXpresso SDK）を含めて取得します。VS Code のターミナル
（`Ctrl + Shift + @`）で実行できます。

```bash
git clone --recurse-submodules https://github.com/toppers/asp3_mcuxsdk.git
```

## プロジェクトを開く

VS Code で **`evkmimxrt685/sample1` フォルダ**を開きます。`.vscode/`・`CMakePresets.json`・
`mcuxpresso-tools.json` が同梱されているため、PROJECTS ビューに `asp3_sample1` として認識され、
CMake の構成が行われます。

初回に「**There is no toolchain associated to current project**」と表示された場合は、
トップ README の「ツールチェインをプロジェクトに関連付ける」に従って Arm GNU Toolchain
（13.2 Rel1 の**ルートフォルダ**）を関連付けてください。

> SDK は submodule 直参照のため、Smart Configurator などによるコード再生成は不要です。

## ビルド

コマンドラインの場合：

```bash
cmake --preset Debug
cmake --build build/Debug          # → build/Debug/asp.elf
```

VS Code の場合は、メニュー「ターミナル」→「タスクの実行」→「**CMake: build**」、または
ステータスバー／PROJECTS ビューの Build を実行します（Debug プリセット）。

## EVK-MIMXRT685 に接続してデバッグ

1. EVK-MIMXRT685 のデバッグ USB（J5）と PC を接続します。オンボード LPC-Link2 が
   CMSIS-DAP ファームウェアであることを前提とします（出荷時標準。確認・切替は
   [docs/host-setup.md](../../docs/host-setup.md) §3）。
2. シリアル出力は同 USB の **VCOM（115200bps）** に出ます。VS Code の Serial Monitor 拡張や
   TeraTerm 等で開きます（**VCOM を読むプロセスは同時に 1 つだけ**）。
3. 左領域の「実行とデバッグ」アイコンで「**Debug asp.elf**」を選び、デバッグを開始します。
   フラッシュ書込み後 `main` で停止するので、「続行」で実行します。
4. シリアルに TOPPERS のバナーと `sample1` の出力（`task1 is running (NNN)` など）が表示されます。
   `r` を送ると `rot_rdq` により task1→task2→task3 が切り替わります。

実行中システムを観測したい場合は「**Attach asp.elf (OS observation)**」構成を使います
（通常接続はリセット＋bootROM ストールで観測できないため attach を使う。詳細は
[docs/tech-notes.md](../../docs/tech-notes.md)）。

コマンドラインで書き込む場合（LinkServer 標準）：

```bash
cd build/Debug
LinkServer flash MIMXRT685S:EVK-MIMXRT685 load asp.elf   # 書込み後に自動リセット実行
```

## Windows でのハマりどころ（実際に踏んだもの）

| 症状 | 原因 | 対処 |
|---|---|---|
| **No probe** | LinkServer 未導入（CMSIS-DAP の列挙に必須） | MCUXpresso Installer で LinkServer をインストール |
| **There is no toolchain associated** | プロジェクトにツールチェインが関連付けられていない | 「MCUXpresso: Associate Toolchain」で関連付け |
| **not a valid toolchain directory** | ツールチェイン選択で `bin` を選んだ | `bin` の**親（ルート）**フォルダを選ぶ（`<root>\bin\arm-none-eabi-gcc.exe` を検証するため） |
| **ninja のバージョンが古い** | PATH 上で別製品同梱の古い ninja（例: STM32CubeCLT の 1.11.1）が、MCUXpresso 版（1.12.1）より先に見つかる | システム環境変数 PATH で MCUXpresso 版 ninja を優先（または古い方を後ろへ）。Windows はシステム PATH→ユーザ PATH の順で結合される点に注意 |
| 環境変数を変えても拡張に反映されない | 既存の `explorer.exe` / VS Code プロセスが変更前の環境を保持している | サインアウト→サインイン、または explorer 再起動後に **VS Code を完全終了**して起動し直す（ウィンドウの再読み込みでは不十分。同一プロセスに新ウィンドウが付くだけ） |

> プローブが認識されているかは LinkServer で確認できます（`LinkServer probes`）。
> CMSIS-DAP が一覧に出れば書込み・デバッグの準備は整っています。
