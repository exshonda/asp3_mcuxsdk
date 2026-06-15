/*
 *  TOPPERS/ASP3 Core ＋ MCUXpresso SDK 統合（FRDM-MCXN947）
 *
 *  SDK の startup_MCXN947_cm33_core0.S（Reset_Handler→SystemInit→_start）
 *  から呼ばれるエントリ．SDK 流のボード初期化（ピン・クロック）を行って
 *  から ASP3 カーネルを起動する．MCXN947 は内蔵フラッシュ実行のため XIP
 *  （FlexSPI）設定は不要．以降の割込み管理は ASP3 が掌握する
 *  （core_initialize() が VTOR を ASP3 のベクタテーブルへ切り替える）．
 */

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

extern void sta_ker(void);

int main(void)
{
	/* SDK 流のボード初期化（ピン・クロック） */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();

	/* ASP3 カーネルの起動（リターンしない） */
	sta_ker();

	return 0;
}
