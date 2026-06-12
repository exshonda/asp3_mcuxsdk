# asp3_mcuxsdk

TOPPERS/ASP3 Core と NXP MCUXpresso SDK の統合リポジトリ（対象：EVK-MIMXRT685／i.MX RT685・Cortex-M33）。

純カーネル [asp3_core](https://github.com/exshonda/asp3_core) を submodule（`asp3/asp3_core`）として参照し、
SDK固有の target 依存部・アプリ・移植ノウハウを本リポジトリで管理する
（`ASP3_TARGET_DIR`／`ASP3_LIBRARY_ONLY` 方式。asp3_pico_sdk / asp3_fsp / asp3_stm32cube と同じA案構成）。

## 状態

**準備中**。計画は asp3_core の
[`docs/dev/nxp-integration.md`](https://github.com/exshonda/asp3_core/blob/main/docs/dev/nxp-integration.md) を参照：

- **Phase A**（先行・asp3_core側）：ベアメタル `mimxrt685evk` ターゲットを asp3_core 本体に追加
  （genuine ASP3 3.7.0 移植を非TECS＋Python cfg＋3.7.2規約へ変換）
- **Phase B**（本リポジトリ）：MCUXpresso SDK（fsl_ドライバ）との協調動作

## 予定構成

```
asp3_mcuxsdk/
├── asp3/
│   ├── asp3_core/          ← submodule（純カーネル）
│   └── target/             ← SDK向けターゲット依存部（ASP3_TARGET_DIRで供給）
├── asp3_mcuxsdk.cmake      ← 協調ヘルパ
└── sample1/                ← サンプルアプリ
```

## クローン

```bash
git clone --recurse-submodules https://github.com/exshonda/asp3_mcuxsdk.git
```
