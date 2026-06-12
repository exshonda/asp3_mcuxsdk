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
 *  ターゲット依存モジュール（EVK-MIMXRT685 + MCUXpresso SDK用）
 *
 *  クロック・ピン・XIP（FlexSPI）の初期化は SDK 側（startup_MIMXRT685S_cm33.S
 *  → SystemInit → main() の BOARD_InitBootPins/BOARD_InitBootClocks）が行う．
 *  ここではコア依存部の初期化（VTOR の ASP3 ベクタテーブルへの切替を含む）と，
 *  システムログ低レベル出力用 SIO ポートの初期化のみを行う．
 */
#include "kernel_impl.h"
#include <sil.h>
#include "target_serial.h"

/*
 *  ターゲット依存部 初期化処理
 */
void
target_initialize(void)
{
	/*
	 *  コア依存部の初期化（VTOR を ASP3 のベクタテーブルへ切り替える）
	 */
	core_initialize();

	/*
	 *  システムログの低レベル出力用に SIO（FC0 USART）を初期化する．
	 *  （割込みは使わないポーリング出力．ポートの割込み有効化は
	 *  シリアルドライバのオープン時に行われる）
	 */
	target_fput_initialize();
}

/*
 *  ターゲット依存部 終了処理
 */
void
target_exit(void)
{
	/* コア依存部の終了処理 */
	core_terminate();
	while (1) ;
}

/*
 *  デフォルトのsoftware_term_hook（weak定義）
 */
__attribute__((weak))
void software_term_hook(void)
{
}
