# TODO・残課題（2026-06-12 時点）

実施済みの経緯は `asp3/asp3_core/docs/dev/nxp-integration.md`（正本）と
`docs/vscode-support-plan.md` を参照。

## 1. CI（GitHub Actions） — 完了（2026-06-12）

`.github/workflows/ci.yml` を新設。asp3_core と同じ開発コンテナイメージ
（`ghcr.io/toppers/asp3_core-dev:20260606`）で `actions/checkout`
（`submodules: recursive`）→ `cmake --preset Debug && cmake --build` の
build-only ジョブ。CMakePresets の machine 固有 env（`/home/honda/...`）は
ビルドに不要（PATH 上の arm-none-eabi-gcc を使う＝当該パス不在のローカルでも
ビルドが通ることを確認済み）。テスト実行は実機が要るため CI ではビルドのみ。

## 2. 移植 skill（porting-asp3-to-nxp） — 完了（2026-06-12）

`.claude/skills/porting-asp3-to-nxp/` を asp3_stm32cube（porting-asp3-to-stm32）と
同構成で作成：
- `SKILL.md`（エントリ・前提知識・地雷サマリ・ビルド/書込み）
- `reference/boot-vector-pitfalls.md`（Secureブート/EXC_RETURN・ベクタ配置・VTOR整列・
  HRT精度・マクロ二重定義）・`sdk-acquisition.md`（west非使用のsubmodule直参照と更新）・
  `flash-debug-tools.md`（LinkServer/J-Link・VCOM・OS Awareness）
- `checklists/new-board.md`（新ボード追加・Step0=ブート経路の確定が最重要）・
  `bringup-debug.md`（症状→原因）
- `snippets/kernel_vector-section.ld`（.kernel_vector 配置・実物から抽出）

素材＝`docs/host-setup.md`・`docs/tech-notes.md`・nxp-integration.md（tech-notes §1
ブートイメージタイプとEXC_RETURN・§2ベクタ配置・§3クロック換算を反映）。

## 3. SVD（Peripherals ビュー）

VS Code デバッグの Peripherals 表示には SVD が必要。SVD は別リポジトリ
（`mcux-soc-svd`・manifests の base.yml にエントリあり）。必要になったら
submodule 追加（または該当 SVD 1ファイルのコピー）→ launch.json に `svdPath`。

## 4. アプリプロジェクトの追加（必要に応じて）

`evkmimxrt685/<アプリ名>/` として sample1 を雛形にコピーすれば増やせる
（asp3_fsp の ek_ra6m5/sample と同構成）。手順：
1. `evkmimxrt685/sample1` を丸ごとコピー（build/ は除く）
2. CMakeLists.txt の `ASP3_APPLDIR`/`ASP3_APPLNAME` 既定値とプロジェクト説明を変更
3. `.vscode/mcuxpresso-tools.json` の `projectName` を変更
4. board/・リンカスクリプトは共通のため原則そのまま
（テストは testexec ラッパが -D 差し替えで回すため、テスト用に別プロジェクトを
作る必要はない）

## 5. その他（小粒）

- `LinkServer` の PATH 化（/usr/local/bin へのシンボリックリンク）を host-setup に
  含めるか検討（現状は各スクリプトがフルパス既定＋`LINKSERVER` 環境変数で対応）
- ~~feat/mimxrt685evk（asp3_core）の main マージ後に submodule を main 追従へ戻す~~
  → 完了（2026-06-12）：asp3_core PR #1（squash）で main に取り込まれ、submodule を
  main の `32f9749` へ bump 済み
- VS Code 拡張が CMakePresets.json / .vscode に注入する差分の扱い
  （現状：コミットして共有。マシン固有パスはビルド未参照のため実害なし。
  気になる場合は拡張側のアップデートで方式が変わる可能性に注意）
