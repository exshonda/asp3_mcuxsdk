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
 *  デバッグコンソール（LP_FLEXCOMM4 上の LPUART4＝オンボードデバッグ
 *  プローブ MCU-Link の VCOM）を fsl_lpuart ドライバで駆動する．
 */

#ifndef TOPPERS_TARGET_SERIAL_H
#define TOPPERS_TARGET_SERIAL_H

#include "frdmmcxn947_mcuxsdk.h"

/*
 *  SIOの割込み番号（LP_FLEXCOMM4_IRQn = 39）
 */
#define INTNO_SIO		(39 + 16)			/* 割込み番号 */
#define INHNO_SIO		INTNO_SIO			/* 割込みハンドラ番号 */
#define INTPRI_SIO		(TMAX_INTPRI - 1)	/* 割込み優先度 */
#define INTATR_SIO		TA_NULL				/* 割込み属性 */
#define ISRPRI_SIO		1					/* LPUART ISR優先度 */

/*
 *  コールバックルーチンの識別番号
 */
#define SIO_RDY_SND		1U		/* 送信可能コールバック */
#define SIO_RDY_RCV		2U		/* 受信通知コールバック */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  SIOポート管理ブロックの定義
 */
typedef struct sio_port_control_block	SIOPCB;

/*
 *  SIOドライバの初期化
 */
extern void	sio_initialize(EXINF exinf);

/*
 *  SIOドライバの終了処理
 */
extern void	sio_terminate(EXINF exinf);

/*
 *  SIOの割込みサービスルーチン
 */
extern void	sio_isr(EXINF exinf);

/*
 *  SIOポートのオープン
 */
extern SIOPCB	*sio_opn_por(ID siopid, EXINF exinf);

/*
 *  SIOポートのクローズ
 */
extern void	sio_cls_por(SIOPCB *p_siopcb);

/*
 *  SIOポートへの文字送信
 */
extern bool_t	sio_snd_chr(SIOPCB *p_siopcb, char c);

/*
 *  SIOポートからの文字受信
 */
extern int_t	sio_rcv_chr(SIOPCB *p_siopcb);

/*
 *  SIOポートからのコールバックの許可
 */
extern void	sio_ena_cbr(SIOPCB *p_siopcb, uint_t cbrtn);

/*
 *  SIOポートからのコールバックの禁止
 */
extern void	sio_dis_cbr(SIOPCB *p_siopcb, uint_t cbrtn);

/*
 *  SIOポートからの送信可能コールバック
 */
extern void	sio_irdy_snd(EXINF exinf);

/*
 *  SIOポートからの受信通知コールバック
 */
extern void	sio_irdy_rcv(EXINF exinf);

/*
 *  低レベル出力用SIOポートの初期化（target_initializeから呼ばれる）
 */
extern void	target_fput_initialize(void);

#endif /* TOPPERS_MACRO_ONLY */

#endif /* TOPPERS_TARGET_SERIAL_H */
