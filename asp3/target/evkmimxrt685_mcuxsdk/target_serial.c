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
 *  （EVK-MIMXRT685 + MCUXpresso SDK用・非TECS版専用）
 *
 *  Flexcomm0 USART（オンボードデバッグプローブの VCOM）を MCUXpresso SDK
 *  の fsl_usart ドライバで駆動する．構造は asp3_core 本体の
 *  arch/arm_m_gcc/imxrt600/imxrt600_usart.c（Phase A・レジスタ直叩き）と
 *  同形で，レジスタ操作を fsl_usart/fsl_clock の API に置き換えたもの．
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
#include "fsl_usart.h"
#include "board.h"
#pragma pop_macro("EXC_RETURN_PREFIX")

/*
 *  ボーレート
 */
#define TARGET_USART_BAUDRATE	115200U

/*
 *  SIOポート管理ブロックの定義
 */
struct sio_port_control_block {
	USART_Type	*base;			/* USARTレジスタのベースアドレス */
	EXINF		exinf;			/* 拡張情報 */
	bool_t		opened;			/* オープン済み */
};

/*
 *  SIOポート管理ブロックのエリア
 */
static SIOPCB	siopcb_table[TNUM_PORT] = {
	{ USART0, 0, false }		/* Flexcomm0＝デバッグコンソール */
};

/*
 *  SIOポートIDから管理ブロックを取り出すためのマクロ
 */
#define INDEX_SIOP(siopid)	((uint_t)((siopid) - 1))
#define get_siopcb(siopid)	(&(siopcb_table[INDEX_SIOP(siopid)]))

/*
 *  USARTハードウェアの初期化（クロック設定込み・二重初期化防止付き）
 */
static void
target_usart_hw_initialize(SIOPCB *p_siopcb)
{
	usart_config_t config;

	if (p_siopcb->opened) {
		return;
	}

	/*
	 *  FRG0 のクロック源を frg_pll に設定し，Flexcomm0 へ FRG クロックを
	 *  接続する（SDK の BOARD_DEBUG_UART_* 定義＝board.h を使用）
	 */
	CLOCK_SetFRGClock(BOARD_DEBUG_UART_FRG_CLK);
	CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

	/*
	 *  USART の初期化（115200bps・8bit・パリティなし・ストップ1bit・
	 *  FIFO有効．送信は FIFO 空きで割込み，受信は FIFO 非空で割込み）
	 */
	USART_GetDefaultConfig(&config);
	config.baudRate_Bps = TARGET_USART_BAUDRATE;
	config.enableTx = true;
	config.enableRx = true;
	config.txWatermark = kUSART_TxFifo0;
	config.rxWatermark = kUSART_RxFifo1;
	(void) USART_Init(p_siopcb->base, &config, BOARD_DEBUG_UART_CLK_FREQ);

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
	uint32_t	stat = p_siopcb->base->FIFOINTSTAT;

	if ((stat & USART_FIFOINTSTAT_TXLVL_MASK) != 0U) {
		/*
		 *  送信可能コールバックルーチンを呼び出す．
		 */
		sio_irdy_snd(p_siopcb->exinf);
	}
	if ((stat & USART_FIFOINTSTAT_RXLVL_MASK) != 0U) {
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
	target_usart_hw_initialize(p_siopcb);
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
		 *  送信FIFOが掃けるのを待つ（待たずにディスエーブルすると
		 *  カーネル終了時の最後の出力が失われる）
		 */
		while ((USART_GetStatusFlags(p_siopcb->base)
				& (uint32_t) kUSART_TxFifoEmptyFlag) == 0U) ;

		USART_Deinit(p_siopcb->base);
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
	if ((USART_GetStatusFlags(p_siopcb->base)
			& (uint32_t) kUSART_TxFifoNotFullFlag) != 0U) {
		USART_WriteByte(p_siopcb->base, (uint8_t) c);
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
	if ((USART_GetStatusFlags(p_siopcb->base)
			& (uint32_t) kUSART_RxFifoNotEmptyFlag) != 0U) {
		return((int_t) USART_ReadByte(p_siopcb->base));
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
		USART_EnableInterrupts(p_siopcb->base,
							(uint32_t) kUSART_TxLevelInterruptEnable);
		break;
	case SIO_RDY_RCV:
		USART_EnableInterrupts(p_siopcb->base,
							(uint32_t) kUSART_RxLevelInterruptEnable);
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
		USART_DisableInterrupts(p_siopcb->base,
							(uint32_t) kUSART_TxLevelInterruptEnable);
		break;
	case SIO_RDY_RCV:
		USART_DisableInterrupts(p_siopcb->base,
							(uint32_t) kUSART_RxLevelInterruptEnable);
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
	target_usart_hw_initialize(get_siopcb(SIOPID_FPUT));
}

/*
 *  SIOポートへのポーリング出力
 */
static void
target_usart_fput(char c)
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
		target_usart_fput('\r');
	}
	target_usart_fput(c);
}
