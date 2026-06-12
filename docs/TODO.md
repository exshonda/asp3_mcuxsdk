# TODO・残課題（2026-06-12 時点）

実施済みの経緯は `asp3/asp3_core/docs/dev/nxp-integration.md`（正本）と
`docs/vscode-support-plan.md` を参照。

## 1. CI（GitHub Actions）

ビルドジョブ未整備。SDK は submodule 直参照（west・GUI生成ツール不要）のため、
`actions/checkout` の `submodules: recursive`＋arm-none-eabi-gcc 導入だけで
`cmake --preset Debug && cmake --build` がそのまま動くはず。
asp3_core の ci.yml（mimxrt685evk build-only ジョブ）が参考になる。
テスト実行は実機が要るため CI ではビルドのみ。

## 2. 移植 skill（porting-asp3-to-nxp）

asp3_fsp（porting-asp3-to-renesas-ra）・asp3_stm32cube（porting-asp3-to-stm32）と
同様の `.claude/skills/` パッケージ。素材は揃っている：
- 手順の正本：`docs/host-setup.md`・`docs/tech-notes.md`・nxp-integration.md
- 特に他の RT6xx/RT5xx ボードへ展開する際の要点＝tech-notes §1（ブートイメージ
  タイプと EXC_RETURN）・§2（ベクタテーブル配置）・§3（クロック換算）

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
- feat/mimxrt685evk（asp3_core）の main マージ後に submodule を main 追従へ戻す
- VS Code 拡張が CMakePresets.json / .vscode に注入する差分の扱い
  （現状：コミットして共有。マシン固有パスはビルド未参照のため実害なし。
  気になる場合は拡張側のアップデートで方式が変わる可能性に注意）
