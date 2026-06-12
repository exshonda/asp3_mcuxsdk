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
 *  kernel.hのターゲット依存部（EVK-MIMXRT685 + MCUXpresso SDK用）
 *
 *  このインクルードファイルは，kernel.hでインクルードされる．他のファ
 *  イルから直接インクルードすることはない．このファイルをインクルード
 *  する前に，t_stddef.hがインクルードされるので，それらに依存してもよ
 *  い．
 */

#ifndef TOPPERS_TARGET_KERNEL_H
#define TOPPERS_TARGET_KERNEL_H

#include "evkmimxrt685_mcuxsdk.h"

#ifdef USE_TIM_AS_HRT

/*
 *  高分解能タイマのタイマ周期
 *
 *  タイマ周期が2^32の場合には，このマクロを定義しない．
 */
#undef TCYC_HRTCNT

/*
 *  高分解能タイマのカウント値の進み幅
 */
#define TSTEP_HRTCNT 1U

#endif /* USE_TIM_AS_HRT */

#ifndef TOPPERS_MACRO_ONLY

extern void	sta_ker(void);

/*
 *  SDK startup（startup_MIMXRT685S_cm33.S）のリセットハンドラ
 *  （ベクタテーブル生成で参照する．target_kernel.py 参照）
 */
extern void	Reset_Handler(void);

#endif /* TOPPERS_MACRO_ONLY */

/*
 *  チップで共通な定義
 */
#include "chip_kernel.h"

#endif /* TOPPERS_TARGET_KERNEL_H */
