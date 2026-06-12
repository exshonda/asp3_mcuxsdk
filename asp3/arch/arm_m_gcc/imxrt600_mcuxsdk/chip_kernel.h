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
 *  kernel.hのチップ依存部（I.MX RT600 + MCUXpresso SDK用）
 *
 *  このインクルードファイルは，kernel.hでインクルードされる．他のファ
 *  イルから直接インクルードすることはない．このファイルをインクルード
 *  する前に，t_stddef.hがインクルードされるので，それらに依存してもよ
 *  い．
 */

#ifndef TOPPERS_CHIP_KERNEL_H
#define TOPPERS_CHIP_KERNEL_H

/*
 *  カーネル管理の割込み優先度の範囲
 *
 *  I.MX RT600（Cortex-M33）のNVIC優先度は3ビット（TBITW_IPRI=3）．
 */
#define TMIN_INTPRI		(-7)		/* 割込み優先度の最小値（最高値）*/

/*
 *  サポートする機能の定義
 */
#if 0
#define TOPPERS_TARGET_SUPPORT_OVRHDR		/* オーバランハンドラ */
#endif

/*
 *  コア依存で共通な定義
 */
#include "core_kernel.h"

#endif /* TOPPERS_CHIP_KERNEL_H */
