# CLAUDE.md — asp3_mcuxsdk

TOPPERS/ASP3 Core を NXP MCUXpresso SDK と協調動作させる **SDK統合リポジトリ**
（EVK-MIMXRT685 / Cortex-M33）。純カーネル（`asp3_core`）を submodule 参照し、
NXP 固有部だけを本リポジトリに持つ。

> 設計・経緯の正本は submodule 側 `asp3/asp3_core/docs/dev/nxp-integration.md`。
> カーネル本体の規約は `asp3/asp3_core/AGENTS.md`。

---

## 0. リポジトリ構成

```
asp3_mcuxsdk/
├── asp3/
│   ├── asp3_mcuxsdk.cmake           ← 協調ヘルパ（ASP3_TARGET_DIR 等の解決）
│   ├── asp3_core/                   ← submodule（純カーネル＋全アーキ/チップ依存部）※public
│   ├── arch/arm_m_gcc/{imxrt600,mcxn947}_mcuxsdk/ ← NXP チップ依存部
│   └── target/{evkmimxrt685,frdmmcxn947}_mcuxsdk/ ← NXP ターゲット依存部
├── sdk/                             ← MCUXpresso SDK（submodule 直参照：core/devices-rt/devices-mcx/cmsis/components）
├── evkmimxrt685/sample1/            ← EVK-MIMXRT685 ボード/アプリ（CMakePresets：Debug 等）
├── frdm_mcxn947/sample1/            ← FRDM-MCXN947 ボード/アプリ（同上）
├── docs/                            ← host-setup / tech-notes / verification / TODO
├── .github/workflows/ci.yml         ← build-only CI（共有 dev コンテナ）
└── .claude/skills/porting-asp3-to-nxp/  ← 移植ガイドskill
```

- ツールチェーンは **arm-none-eabi gcc**。SDK は **west を使わず submodule 直参照**
  （GUI 生成不要＝**CI ビルド可能**。CubeMX/RASC と違う利点）。

## 1. ⚠️ 禁則（作業前に必読）

1. **`asp3/asp3_core/`（submodule）配下を直接編集しない**。カーネル本体は上流 ASP3 追従領域。
   変更が必要なら asp3_core リポジトリ側で行い、その `AGENTS.md` の規約（`kernel/`・`include/`・
   `library/` 編集禁止、変更は `target/`・`syssvc/`・新規ファイルに限定）に従う。
   本リポジトリの作業は **NXP 側ファイル（`asp3/arch/`・`asp3/target/`・`asp3/asp3_mcuxsdk.cmake`・`evkmimxrt685/`・`frdm_mcxn947/`）** に閉じる。
2. **カーネル内で動的メモリ確保を使わない**（`malloc`/`new` 等禁止。静的生成のみ）。

## 2. 取得・ビルド・実機確認

```bash
git clone --recurse-submodules https://github.com/toppers/asp3_mcuxsdk.git
# 既存clone: git submodule update --init --recursive

cd evkmimxrt685/sample1
cmake --preset Debug
cmake --build build/Debug          # → build/Debug/asp.elf
```

- SDK は submodule 直参照のため GUI 生成不要。詳細は README.md・`docs/host-setup.md` を参照。
- 実機検証は **EVK-MIMXRT685**（XIP 実行・書込みは MCU-Link/LinkServer）。
  `test_porting` 6/6・testexec で確認済み。シリアル出力で基本動作を確認する。
- CI（`.github/workflows/ci.yml`）は共有 dev コンテナで build-only（テストは実機要）。

## 検証の鉄則

- コードを変更したら **必ずビルドを通してから報告**。「動くはず」で報告しない。
- 実機確認はシリアル出力を根拠とする。
- asp3_core 側に変更が要る場合は別リポジトリで行い、push 権限が無ければ差分を提示して依頼。

## 参考

| 参照 | 用途 |
|---|---|
| `asp3/asp3_core/docs/dev/nxp-integration.md` | NXP 統合の正本（Phase A/B・経緯・設計） |
| `asp3/asp3_core/AGENTS.md` | カーネル本体の規約 |
| `docs/host-setup.md` | ホスト環境構築手順 |
| `.claude/skills/porting-asp3-to-nxp/` | NXP への移植ガイド skill |
