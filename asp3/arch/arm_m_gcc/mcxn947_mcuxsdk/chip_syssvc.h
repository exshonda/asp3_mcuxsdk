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
 *  システムサービスのチップ依存部（MCX N947 + MCUXpresso SDK用）
 */

#ifndef TOPPERS_CHIP_SYSSVC_H
#define TOPPERS_CHIP_SYSSVC_H

/*
 *  トレースログに関する設定
 */
#ifdef TOPPERS_TRACE_ENABLE
#include "logtrace/trace_config.h"
#endif /* TOPPERS_TRACE_ENABLE */

/*
 *  起動メッセージにターゲット依存部の著作権表示を追加するためのマクロ
 */
#ifdef PRC_COPYRIGHT
#define TARGET_COPYRIGHT	PRC_COPYRIGHT
#endif /* PRC_COPYRIGHT */

#endif /* TOPPERS_CHIP_SYSSVC_H */
