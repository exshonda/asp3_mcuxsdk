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
 *  タイマドライバ（EVK-MIMXRT685 + MCUXpresso SDK用）
 *
 *  CTIMER0 を高分解能タイマ（HRT）として使用する（TIM方式）．
 *  fsl_ctimer で初期化し，高頻度に呼ばれる現在値読出し・イベント設定は
 *  CMSIS デバイスヘッダのレジスタ定義で行う（STM32Cube 統合の LL マクロ
 *  使用と同じ位置付け）．
 *
 *  カウンタクロックは main_clk（500.21MHz）をプリスケーラで1/500にした
 *  1.000421MHz（+421ppm）．SDK 既定クロック構成（BOARD_BootClockRUN）の
 *  main_clk は整数分周で正確に 1MHz にできないため，この誤差は許容する．
 */

#ifndef TOPPERS_TARGET_TIMER_H
#define TOPPERS_TARGET_TIMER_H

#include "evkmimxrt685_mcuxsdk.h"

#ifdef USE_SYSTICK_AS_TIMETICK

/*
 *  プロセッサ依存部で定義する
 */
#include "core_timer.h"

#else /* USE_SYSTICK_AS_TIMETICK */
#ifdef USE_TIM_AS_HRT

#include "kernel/kernel_impl.h"
#include <sil.h>

/*
 *  MCUXpresso SDK（CMSISデバイスヘッダ・fsl_ctimer）
 *
 *  EXC_RETURN_PREFIX は ASP3 の arm_m.h と CMSIS の core_cm33.h が同値
 *  （0xff000000）を二重定義するため，再定義警告を push/pop で回避する．
 */
#pragma push_macro("EXC_RETURN_PREFIX")
#undef EXC_RETURN_PREFIX
#include "fsl_ctimer.h"
#pragma pop_macro("EXC_RETURN_PREFIX")

/*
 *  タイマ割込みハンドラ登録のための定数（CTIMER0_IRQn = 10）
 */
#define INTNO_TIMER		(10 + 16)			/* 割込み番号 */
#define INHNO_TIMER		INTNO_TIMER			/* 割込みハンドラ番号 */
#define INTPRI_TIMER	(TMAX_INTPRI - 1)	/* 割込み優先度 */
#define INTATR_TIMER	TA_NULL				/* 割込み属性 */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  高分解能タイマの起動処理
 */
extern void	target_hrt_initialize(intptr_t exinf);

/*
 *  高分解能タイマの停止処理
 */
extern void	target_hrt_terminate(intptr_t exinf);

/*
 *  高分解能タイマの現在のカウント値の読出し
 */
Inline HRTCNT
target_hrt_get_current(void)
{
	return((HRTCNT) CTIMER0->TC);
}

/*
 *  高分解能タイマ割込みの要求
 */
Inline void
target_hrt_raise_event(void)
{
	raise_int(INTNO_TIMER);
}

/*
 *  高分解能タイマへの割込みタイミングの設定
 *
 *  高分解能タイマを，hrtcntで指定した値カウントアップしたら割込みを発
 *  生させるように設定する．
 */
Inline void
target_hrt_set_event(HRTCNT hrtcnt)
{
	/*
	 *  現在のカウント値を読み，hrtcnt後にマッチ割込みが発生するように
	 *  設定する．
	 */
	const uint32_t current = (uint32_t) target_hrt_get_current();

	CTIMER0->MR[0] = current + hrtcnt;
	CTIMER0->MCR = CTIMER_MCR_MR0I_MASK;

	/*
	 *  上で現在のカウント値を読んで以降に，hrtcnt以上カウントアップして
	 *  いた場合には，割込みを発生させる．
	 */
	if ((uint32_t) target_hrt_get_current() - current >= hrtcnt) {
		target_hrt_raise_event();
	}
}

/*
 *  割込みタイミングに指定する最大値
 */
#define HRTCNT_BOUND 4000000002U

/*
 *  高分解能タイマ割込みハンドラ
 */
extern void	target_hrt_handler(void);

#endif /* TOPPERS_MACRO_ONLY */
#endif /* USE_TIM_AS_HRT */
#endif /* USE_SYSTICK_AS_TIMETICK */

#endif /* TOPPERS_TARGET_TIMER_H */
