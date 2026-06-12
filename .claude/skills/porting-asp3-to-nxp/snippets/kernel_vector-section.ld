/*
 * ASP3 ベクタテーブルの明示配置（SDK の ld へ追加するセクション）
 *
 * SDK の <device>_cm33_flash.ld をボードプロジェクトへコピーし、ブートベクタ用の
 * m_interrupts 領域（SDK の .isr_vector）はそのまま残したうえで、ASP3 cfg が生成する
 * _kernel_vector_table（セクション .vector）を専用セクションで m_text に配置する。
 *
 * これを入れないと .vector が orphan セクションとして m_interrupts へ引き寄せられ、
 * 「.vector will not fit」「LMA overlap」でリンクが失敗する。
 *
 * 置き場所：SECTIONS の中、ブートベクタ（.isr_vector / .interrupts）の直後・.text の前。
 *
 * ALIGN(512)：VTOR の整列要件（テーブルのエントリ数×4 以上の 2 のべき乗）。
 *   割込み源の多いボードでベクタ数が増える場合は必要整列も増える
 *   （エントリ数 N に対し 2^⌈log2(N×4)⌉）。RT685（INTNO 上限 59+16）では 512 で足りる。
 *
 * 実行時は arm_m 共通の core_initialize() が VTOR をこのテーブルへ切り替える
 *   （＝ASP3 が NVIC を掌握。ブート時のベクタは SDK の .isr_vector のまま）。
 */
  .kernel_vector :
  {
    . = ALIGN(512);
    KEEP(*(.vector))
    . = ALIGN(4);
  } > m_text
