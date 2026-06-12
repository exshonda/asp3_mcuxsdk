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
 *  タイマドライバ（EVK-MIMXRT685 + MCUXpresso SDK用・CTIMER0/TIM方式）
 */

#include "kernel_impl.h"
#include "time_event.h"
#include "target_timer.h"
#include <sil.h>

#include "fsl_clock.h"
#include "fsl_ctimer.h"

/*
 *  タイマの起動処理
 *
 *  CTIMER0 のクロックに main_clk を選択し，プリスケーラで約1MHz
 *  （main_clk/500＝1.000421MHz）を生成する 32bit アップカウンタとして
 *  起動する．
 */
void
target_hrt_initialize(intptr_t exinf)
{
	ctimer_config_t config;

	CLOCK_AttachClk(kMAIN_CLK_to_CTIMER0);

	CTIMER_GetDefaultConfig(&config);
	config.prescale = CLOCK_GetCtimerClkFreq(0U) / 1000000U - 1U;
	CTIMER_Init(CTIMER0, &config);
	CTIMER_StartTimer(CTIMER0);
}

/*
 *  タイマの停止処理
 */
void
target_hrt_terminate(intptr_t exinf)
{
	CTIMER_StopTimer(CTIMER0);
	CTIMER_Deinit(CTIMER0);
}

/*
 *  タイマ割込みハンドラ
 */
void
target_hrt_handler(void)
{
	/*
	 *  マッチ0の割込みフラグをクリアし，マッチ割込みを禁止する．
	 */
	CTIMER0->IR = CTIMER_IR_MR0INT_MASK;
	CTIMER0->MCR = 0U;

	/*
	 *  高分解能タイマ割込みを処理する．
	 */
	signal_time();
}
