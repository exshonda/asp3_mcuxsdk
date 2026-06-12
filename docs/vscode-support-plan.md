# MCUXpresso for VS Code 対応の実施プラン（検討：2026-06-12）

asp3_fsp（Renesas拡張）・asp3_pico_sdk（Raspberry Pi Pico拡張）と同様に、
NXP 公式の **MCUXpresso for VS Code** 拡張から本リポジトリのプロジェクトを
ビルド・書込み・デバッグできるようにする。

## 前例の構成（調査結果）

| リポジトリ | 拡張 | 対応方法 |
|---|---|---|
| asp3_fsp | Renesas RA（`renesas-hardware` デバッグタイプ） | アプリプロジェクト直下に `.vscode/`（settings=CMake Tools・launch=拡張のデバッグタイプ・cmake-kits・tasks=RASC起動） |
| asp3_pico_sdk | Raspberry Pi Pico | 同じく `sample1/.vscode/`（拡張コマンド参照の launch/tasks・extensions.json 推奨） |

→ 本リポジトリも **`evkmimxrt685/sample1/` 直下に拡張用メタデータと `.vscode/` を置く**。

## MCUXpresso for VS Code の仕組み（拡張本体 26.5.49 を解析）

- **プロジェクト認識**＝プロジェクト直下の **`mcuxpresso-tools.json`**。
  `projectType` に汎用 CMake 用の **`"cmake-freestanding"`** がある（スキーマ
  `~/.vscode/extensions/NXPSemiconductors.mcuxpresso-*/schemas/mcuxpresso-tools-schema.json`）。
  フィールド：`projectName`／`projectType`／`version`（既定 "25.3"）／
  `sdk{path,boardId,deviceId,coreId}`／`toolchainPath`／`activeBuildType`
- **ビルド**＝`CMakePresets.json`（Debug/Release configurePresets）を使用。
  **本リポジトリは既に同形式**（`evkmimxrt685/sample1/CMakePresets.json`）＝そのまま適合
- **デバッグ**＝拡張独自のデバッグタイプ **`mcuxpresso-debug`**（cortex-debug 非依存）。
  主な属性：`probeType`（linkserver/jlink/pemicro）・`executable`・`svdPath`・
  `gdbServerPath`・`stopAtSymbol`・`isAttach` 等。
  **launch.json が無い場合は初回デバッグ時にウィザードが生成**する
  （Pure CMake プロジェクトの公式ドキュメントに記載）
- ツール群（armgcc・LinkServer・J-Link）は MCUXpresso Installer 管理（本PCは導入済み）

## 実施手順

### Step 1: mcuxpresso-tools.json の追加（`evkmimxrt685/sample1/`）

```json
{
    "projectName": "asp3_sample1",
    "projectType": "cmake-freestanding",
    "version": "25.3",
    "sdk": {
        "path": "${workspaceFolder}/../../sdk/core",
        "boardId": "evkmimxrt685",
        "deviceId": "MIMXRT685S",
        "coreId": "cm33"
    }
}
```

- `deviceId` は LinkServer のデバイス名（`MIMXRT685S`）と一致させる
- `sdk.path` の解釈（リポジトリ関連付けの要否）は GUI 検証で確定。
  不要なら sdk フィールドごと省略し、初回デバッグウィザードでデバイス選択でもよい

### Step 2: .vscode/ の追加（同ディレクトリ）

- `extensions.json`：`NXPSemiconductors.mcuxpresso`・`ms-vscode.cmake-tools`・
  `ms-vscode.cpptools`・`ms-vscode.vscode-serial-monitor` を推奨に
- `settings.json`：`"cmake.useCMakePresets": "always"` ほか最小限
- `launch.json`（LinkServer 標準・J-Link はコメントまたは第2エントリ）：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "mcuxpresso-debug",
            "request": "launch",
            "name": "Debug asp.elf (LinkServer)",
            "probeType": "linkserver",
            "executable": "${workspaceFolder}/build/Debug/asp.elf",
            "stopAtSymbol": "main"
        },
        {
            "type": "mcuxpresso-debug",
            "request": "launch",
            "name": "Debug asp.elf (J-Link)",
            "probeType": "jlink",
            "executable": "${workspaceFolder}/build/Debug/asp.elf",
            "stopAtSymbol": "main"
        }
    ]
}
```

### Step 3: GUI 検証（要・人手＝VS Code 操作）

1. VS Code で `evkmimxrt685/sample1` を開く（または QUICKSTART PANEL → Import Project）
2. PROJECTS ビューに認識されること・Build（Debug プリセット）が通ること
3. Debug 起動：LinkServer でフラッシュ→`main` 停止→ブレーク・ステップ・
   レジスタ表示。**初回ウィザードが launch.json を再生成した場合は本プランの
   案との差分を取り込み**（ウィザード生成物が正）
4. シリアルモニタ（/dev/ttyACM0・115200）で sample1 出力確認
   （**他のリーダと併用禁止**＝docs/host-setup.md §4）

### Step 4: 任意（後続）

- **SVD（Peripherals ビュー）**：SVD は別リポジトリ（mcux-soc-svd・manifest の
  base.yml にあり）。必要なら submodule 追加して `svdPath` を設定
- OS Awareness（gdb-multiarch スクリプト）の launch.json への組込み
  （`gdbClientExtraArgs`／`postLaunchCommands` で `source os_awareness.py`）
- test_porting 等のテストアプリ用ビルド構成（CMakePresets に
  `-DASP3_APPLDIR` 等を持つプリセットを追加）

### Step 5: 記録・コミット

- README（ドキュメント表・使い方）と `docs/tech-notes.md`（拡張対応の知見）更新
- asp3_core 側 `docs/dev/nxp-integration.md` の残課題に結果を追記
- コミットは feat（.vscode/メタデータ追加）＋docs

## 未確定事項（Step 3 で確定させる）

1. `sdk.path` の要否・正しい値（cmake-freestanding でデバイス関連付けに必要か）
2. ウィザード生成の launch.json と本プラン案の差分（probeType の表記等）
3. `mcuxpresso-tools.json` の `${workspaceFolder}` 変数が使えるか（不可なら相対パス）
4. Windows ホストでのパス（toolchainPath 未指定で Installer 管理ツールが使われるか）

## 参考

- [MCUXpresso for VS Code（Marketplace）](https://marketplace.visualstudio.com/items?itemName=NXPSemiconductors.mcuxpresso)
- [公式ドキュメント（Pure CMake Projects）](https://mcuxpresso.nxp.com/mcux-vscode/25.12/html/Working-with-Pure-CMake-Projects.html)
- [vscode-for-mcux（GitHub）](https://github.com/nxp-mcuxpresso/vscode-for-mcux)
- 拡張同梱スキーマ：`~/.vscode/extensions/nxpsemiconductors.mcuxpresso-26.5.49/schemas/mcuxpresso-tools-schema.json`
