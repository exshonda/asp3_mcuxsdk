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
 *  シリアルインタフェースドライバのターゲット依存部
 *  （FRDM-MCXN947 + MCUXpresso SDK用・非TECS版専用）
 *
 *  LP_FLEXCOMM4 上の LPUART4（オンボードデバッグプローブ MCU-Link の
 *  VCOM）を MCUXpresso SDK の fsl_lpuart ドライバで駆動する．構造は
 *  RT685 版（target_serial.c・fsl_usart）と同形で，FIFO 割込みステータス
 *  ではなく LPUART のステータスフラグと許可割込みで送受信を判定する．
 *
 *  MCX N の LPUART は LP_FLEXCOMM の一機能のため，LPUART_Init の前に
 *  LP_FLEXCOMM_Init で LPUART モードを設定する必要がある（board.c の
 *  BOARD_InitDebugConsole と同じ手順）．割込みベクタは ASP3 が握る
 *  （SDK の LP_FLEXCOMM4_IRQHandler は使わず，INTNO_SIO→sio_isr へ）．
 */

#include <kernel.h>
#include <t_syslog.h>
#include <sil.h>
#include "target_syssvc.h"
#include "target_serial.h"

/*
 *  EXC_RETURN_PREFIX は ASP3 の arm_m.h と CMSIS の core_cm33.h が同値
 *  （0xff000000）を二重定義するため，再定義警告を push/pop で回避する．
 */
#pragma push_macro("EXC_RETURN_PREFIX")
#undef EXC_RETURN_PREFIX
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "fsl_lpflexcomm.h"
#include "fsl_lpuart.h"
#include "board.h"
#pragma pop_macro("EXC_RETURN_PREFIX")

/*
 *  ボーレート
 */
#define TARGET_LPUART_BAUDRATE	115200U

/*
 *  SIOポート管理ブロックの定義
 */
struct sio_port_control_block {
	LPUART_Type	*base;			/* LPUARTレジスタのベースアドレス */
	EXINF		exinf;			/* 拡張情報 */
	bool_t		opened;			/* オープン済み */
};

/*
 *  SIOポート管理ブロックのエリア
 */
static SIOPCB	siopcb_table[TNUM_PORT] = {
	{ (LPUART_Type *) BOARD_DEBUG_UART_BASEADDR, 0, false }	/* LPUART4＝VCOM */
};

/*
 *  SIOポートIDから管理ブロックを取り出すためのマクロ
 */
#define INDEX_SIOP(siopid)	((uint_t)((siopid) - 1))
#define get_siopcb(siopid)	(&(siopcb_table[INDEX_SIOP(siopid)]))

/*
 *  LPUARTハードウェアの初期化（クロック設定込み・二重初期化防止付き）
 */
static void
target_lpuart_hw_initialize(SIOPCB *p_siopcb)
{
	lpuart_config_t config;

	if (p_siopcb->opened) {
		return;
	}

	/*
	 *  LPUART4 のクロック源（FRO12M）を接続し，ペリフェラルのリセットを
	 *  解除した上で LP_FLEXCOMM を LPUART モードに設定する
	 *  （SDK の BOARD_DEBUG_UART_* 定義＝board.h を使用）
	 */
	CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);
	RESET_ClearPeripheralReset(BOARD_DEBUG_UART_RST);
	LP_FLEXCOMM_Init(BOARD_DEBUG_UART_INSTANCE, LP_FLEXCOMM_PERIPH_LPUART);

	/*
	 *  LPUART の初期化（115200bps・8bit・パリティなし・ストップ1bit）
	 */
	LPUART_GetDefaultConfig(&config);
	config.baudRate_Bps = TARGET_LPUART_BAUDRATE;
	config.enableTx = true;
	config.enableRx = true;
	(void) LPUART_Init(p_siopcb->base, &config, BOARD_DEBUG_UART_CLK_FREQ);

	p_siopcb->opened = true;
}

/*
 *  SIOドライバの初期化
 */
void
sio_initialize(EXINF exinf)
{
	/* 静的初期化済み（siopcb_table）のため処理なし */
}

/*
 *  SIOドライバの終了処理
 */
void
sio_terminate(EXINF exinf)
{
	uint_t	i;

	for (i = 0; i < TNUM_PORT; i++) {
		if (siopcb_table[i].opened) {
			sio_cls_por(&(siopcb_table[i]));
		}
	}
}

/*
 *  SIOの割込みサービスルーチン
 */
void
sio_isr(EXINF exinf)
{
	SIOPCB		*p_siopcb = get_siopcb((ID) exinf);
	uint32_t	flags = LPUART_GetStatusFlags(p_siopcb->base);
	uint32_t	enabled = LPUART_GetEnabledInterrupts(p_siopcb->base);

	/*
	 *  LPUART のステータスフラグは割込みの許可と無関係に立つため，
	 *  許可されている割込みについてのみコールバックを呼び出す．
	 */
	if ((enabled & (uint32_t) kLPUART_TxDataRegEmptyInterruptEnable) != 0U
			&& (flags & (uint32_t) kLPUART_TxDataRegEmptyFlag) != 0U) {
		/*
		 *  送信可能コールバックルーチンを呼び出す．
		 */
		sio_irdy_snd(p_siopcb->exinf);
	}
	if ((enabled & (uint32_t) kLPUART_RxDataRegFullInterruptEnable) != 0U
			&& (flags & (uint32_t) kLPUART_RxDataRegFullFlag) != 0U) {
		/*
		 *  受信通知コールバックルーチンを呼び出す．
		 */
		sio_irdy_rcv(p_siopcb->exinf);
	}
}

/*
 *  SIOポートのオープン
 */
SIOPCB *
sio_opn_por(ID siopid, EXINF exinf)
{
	SIOPCB	*p_siopcb = get_siopcb(siopid);

	/*
	 *  ハードウェアの初期化（既にオープンしている場合は二重に行わない）
	 */
	target_lpuart_hw_initialize(p_siopcb);
	p_siopcb->exinf = exinf;

	/*
	 *  SIOの割込みマスクを解除する．
	 */
	(void) ena_int(INTNO_SIO);
	return(p_siopcb);
}

/*
 *  SIOポートのクローズ
 */
void
sio_cls_por(SIOPCB *p_siopcb)
{
	if (p_siopcb->opened) {
		/*
		 *  送信が完了するのを待つ（待たずにディスエーブルすると
		 *  カーネル終了時の最後の出力が失われる）
		 */
		while ((LPUART_GetStatusFlags(p_siopcb->base)
				& (uint32_t) kLPUART_TransmissionCompleteFlag) == 0U) ;

		LPUART_Deinit(p_siopcb->base);
		p_siopcb->opened = false;
	}

	/*
	 *  SIOの割込みをマスクする．
	 */
	(void) dis_int(INTNO_SIO);
}

/*
 *  SIOポートへの文字送信
 */
bool_t
sio_snd_chr(SIOPCB *p_siopcb, char c)
{
	if ((LPUART_GetStatusFlags(p_siopcb->base)
			& (uint32_t) kLPUART_TxDataRegEmptyFlag) != 0U) {
		LPUART_WriteByte(p_siopcb->base, (uint8_t) c);
		return(true);
	}
	return(false);
}

/*
 *  SIOポートからの文字受信
 */
int_t
sio_rcv_chr(SIOPCB *p_siopcb)
{
	if ((LPUART_GetStatusFlags(p_siopcb->base)
			& (uint32_t) kLPUART_RxDataRegFullFlag) != 0U) {
		return((int_t) LPUART_ReadByte(p_siopcb->base));
	}
	return(-1);
}

/*
 *  SIOポートからのコールバックの許可
 */
void
sio_ena_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	switch (cbrtn) {
	case SIO_RDY_SND:
		LPUART_EnableInterrupts(p_siopcb->base,
							(uint32_t) kLPUART_TxDataRegEmptyInterruptEnable);
		break;
	case SIO_RDY_RCV:
		LPUART_EnableInterrupts(p_siopcb->base,
							(uint32_t) kLPUART_RxDataRegFullInterruptEnable);
		break;
	default:
		break;
	}
}

/*
 *  SIOポートからのコールバックの禁止
 */
void
sio_dis_cbr(SIOPCB *p_siopcb, uint_t cbrtn)
{
	switch (cbrtn) {
	case SIO_RDY_SND:
		LPUART_DisableInterrupts(p_siopcb->base,
							(uint32_t) kLPUART_TxDataRegEmptyInterruptEnable);
		break;
	case SIO_RDY_RCV:
		LPUART_DisableInterrupts(p_siopcb->base,
							(uint32_t) kLPUART_RxDataRegFullInterruptEnable);
		break;
	default:
		break;
	}
}

/*
 *		システムログの低レベル出力
 */

/*
 *  低レベル出力用SIOポートの初期化（target_initializeから呼ばれる．
 *  割込みは使わないため ena_int は行わない）
 */
void
target_fput_initialize(void)
{
	target_lpuart_hw_initialize(get_siopcb(SIOPID_FPUT));
}

/*
 *  SIOポートへのポーリング出力
 */
static void
target_lpuart_fput(char c)
{
	SIOPCB	*p_siopcb = get_siopcb(SIOPID_FPUT);

	/*
	 *  未初期化の間は捨てる（カーネル起動前の出力対策）
	 */
	if (!(p_siopcb->opened)) {
		return;
	}

	/*
	 *  送信できるまでポーリング
	 */
	while (!(sio_snd_chr(p_siopcb, c))) {
		sil_dly_nse(100);
	}
}

/*
 *  SIOポートへの文字出力
 */
void
target_fput_log(char c)
{
	if (c == '\n') {
		target_lpuart_fput('\r');
	}
	target_lpuart_fput(c);
}
