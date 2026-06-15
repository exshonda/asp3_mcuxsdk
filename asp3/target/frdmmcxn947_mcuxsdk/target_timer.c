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
 *  タイマドライバ（FRDM-MCXN947 + MCUXpresso SDK用・CTIMER0/TIM方式）
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
 *  CTIMER0 のクロックに FRO_HF（PLL150M ブートで 48MHz 起動済み）を選択し，
 *  プリスケーラで 48分周＝正確に 1MHz の 32bit アップカウンタとして起動する
 *  （48MHz/48＝1.000000MHz・ppm 誤差なし）．
 *
 *  注：CLK_1M（kCLK_1M_to_CTIMER0）は CLOCK_GetClk1MFreq() が固定 1000000 を
 *  返すものの，SDK 既定クロック構成では CLK_1M 自体が有効化されておらず，
 *  機能クロックの無い CTIMER レジスタへ書込むと CTIMER_Init 内でバスストール
 *  する（実機で確認）．このため確実に走っている FRO_HF を用いる．
 */
void
target_hrt_initialize(intptr_t exinf)
{
	ctimer_config_t config;

	/*
	 *  CTIMER0 のクロックディバイダを有効化（HALT 解除）してから
	 *  クロック源を接続する．これを行わないと機能クロックが供給されず，
	 *  CTIMER_Init 内の最初のレジスタ書込みでバスストールする（実機確認）．
	 *  順序も NXP の ctimer サンプルに合わせる（SetClkDiv → AttachClk）．
	 */
	CLOCK_SetClkDiv(kCLOCK_DivCtimer0Clk, 1U);
	CLOCK_AttachClk(kFRO_HF_to_CTIMER0);

	CTIMER_GetDefaultConfig(&config);
	config.prescale = CLOCK_GetCTimerClkFreq(0U) / 1000000U - 1U;
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
