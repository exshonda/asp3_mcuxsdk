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
 *  システムサービスのターゲット依存部（EVK-MIMXRT685 + MCUXpresso SDK用）
 */

#ifndef TOPPERS_TARGET_SYSSVC_H
#define TOPPERS_TARGET_SYSSVC_H

#include "evkmimxrt685_mcuxsdk.h"

/*
 *  チップ共通のハードウェア資源の読み込み
 */
#include "chip_syssvc.h"

/*
 *  起動メッセージのターゲットシステム名
 */
#define TARGET_NAME    "EVK-MIMXRT685(MCUXSDK)"

/*
 *  シリアルポート数の定義
 */
#define TNUM_PORT		1		/* サポートするシリアルポートの数 */

/*
 *  低レベル出力で使用するSIOポート
 */
#define SIOPID_FPUT		1

/*
 *  システムログの低レベル出力のための文字出力
 *
 *  ターゲット依存の方法で，文字cを表示/出力/保存する．
 */
#ifndef TOPPERS_MACRO_ONLY
extern void	target_fput_log(char c);
#endif /* TOPPERS_MACRO_ONLY */

/*
 *  システムサービスのコア依存部の読み込み（性能評価の時間源等）
 *
 *  i.MX RT685（Cortex-M33）は DWT を実装するので，USE_ARM_DWT_PMCNT 定義時は
 *  histogram の時間源が DWT CYCCNT（サイクル精度）になる．未定義なら fch_hrt．
 *  （core_syssvc.h は asp3_core 提供＝arch/arm_m_gcc/common）
 */
#include "core_syssvc.h"

#ifdef USE_ARM_DWT_PMCNT
/*
 *  DWT CYCCNT のサイクル数をナノ秒へ変換する（HIST_CONV_TIM）．
 *
 *  変換係数はコアクロック依存なのでターゲット依存部で与える（CYCCNT は
 *  コアクロック＝CPU_CLOCK_HZ で計数．上の evkmimxrt685_mcuxsdk.h が定義）．
 *  ns = cycles * 1000 / (CPU_CLOCK_HZ[MHz])．乗算は uint64 でオーバフロー回避．
 */
#define HIST_CONV_TIM(time)	\
			((uint_t)((uint64_t)(time) * 1000U / (CPU_CLOCK_HZ / 1000000U)))
#endif /* USE_ARM_DWT_PMCNT */

#endif /* TOPPERS_TARGET_SYSSVC_H */
