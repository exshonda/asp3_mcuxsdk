/*
 *  TOPPERS/ASP Kernel
 *      Toyohashi Open Platform for Embedded Real-Time Systems/
 *      Advanced Standard Profile Kernel
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Informatics, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は
 *  asp3_core 同梱ファイルの先頭コメントを参照）の下で利用することを
 *  許諾する．本ソフトウェアは無保証で提供される．
 */

/*
 *		テストプログラムのターゲット依存定義（EVK-MIMXRT685 + MCUXpresso SDK用）
 */

#ifndef TOPPERS_TARGET_TEST_H
#define TOPPERS_TARGET_TEST_H

#define STACK_SIZE (1024)
#define MEASURE_TWICE

/*
 *  int1テスト等で使用する割込み番号（CTIMER1＝クロック未供給の空きIRQ）．
 *  NVICのソフトpend（ISPR）で発生・ハンドラ入口で自動クリアされる
 *  ため intno1_clear() は空でよい（asp3_core 本体の mimxrt685evk_gcc と同じ）．
 */
#define INTNO1			(11 + 16)	/* CTIMER1_IRQn(11) + 16 = 27 */
#define INTNO1_INTATR	TA_ENAINT
#define INTNO1_INTPRI	(-2)
#define intno1_clear()

/*
 *  コア依存モジュール（ARM-M用）
 */
#include "core_test.h"

#endif /* TOPPERS_TARGET_TEST_H */
