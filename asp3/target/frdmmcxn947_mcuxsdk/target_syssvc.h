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
 *  システムサービスのターゲット依存部（FRDM-MCXN947 + MCUXpresso SDK用）
 */

#ifndef TOPPERS_TARGET_SYSSVC_H
#define TOPPERS_TARGET_SYSSVC_H

#include "frdmmcxn947_mcuxsdk.h"

/*
 *  チップ共通のハードウェア資源の読み込み
 */
#include "chip_syssvc.h"

/*
 *  起動メッセージのターゲットシステム名
 */
#define TARGET_NAME    "FRDM-MCXN947(MCUXSDK)"

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

#endif /* TOPPERS_TARGET_SYSSVC_H */
