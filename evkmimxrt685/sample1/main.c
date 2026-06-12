/*
 *  TOPPERS/ASP3 Core ＋ MCUXpresso SDK 統合（EVK-MIMXRT685）
 *
 *  SDK の startup_MIMXRT685S_cm33.S（Reset_Handler→SystemInit→_start）から
 *  呼ばれるエントリ．SDK 流のボード初期化（ピン・クロック＝XIP対応の
 *  FlexSPI クロック切替を含む）を行ってから ASP3 カーネルを起動する．
 *  以降の割込み管理は ASP3 が掌握する（core_initialize() が VTOR を
 *  ASP3 のベクタテーブルへ切り替える）．
 */

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"

extern void sta_ker(void);

/*
 *  DbgConsole_Init のスタブ
 *
 *  デバッグコンソールは ASP3 の syslog／シリアルドライバが代替するため
 *  使用しない（BOARD_InitDebugConsole は呼ばない）が，board.c が
 *  BOARD_InitDebugConsole の「アドレス」を XIP 判定（BOARD_IS_XIP_FLEXSPI）
 *  に使うためリンク対象に残り，DbgConsole_Init への参照が生じる．
 *  debug_console コンポーネント一式をリンクする代わりにスタブで満たす．
 */
status_t DbgConsole_Init(uint8_t instance, uint32_t baudRate, serial_port_type_t device,
						 uint32_t clkSrcFreq)
{
	(void) instance; (void) baudRate; (void) device; (void) clkSrcFreq;
	return kStatus_Fail;
}

int main(void)
{
	/* SDK 流のボード初期化（ピン・クロック） */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();

	/* ASP3 カーネルの起動（リターンしない） */
	sta_ker();

	return 0;
}
