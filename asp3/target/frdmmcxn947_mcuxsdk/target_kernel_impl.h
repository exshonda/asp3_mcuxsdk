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

#ifndef TOPPERS_TARGET_KERNEL_IMPL_H
#define TOPPERS_TARGET_KERNEL_IMPL_H

/*
 *  ターゲット依存部モジュール（FRDM-MCXN947 + MCUXpresso SDK用）
 *
 *  カーネルのターゲット依存部のインクルードファイル．kernel_impl.hのター
 *  ゲット依存部の位置付けとなす．
 */
#include "frdmmcxn947_mcuxsdk.h"

/*
 *  TBITW_IPRI の定義のため読み込み
 */
#include <sil.h>

/*
 *  デフォルトの非タスクコンテキスト用のスタック領域の定義
 */
#define DEFAULT_ISTKSZ    (0x1000) /* 4KByte */

#ifndef TOPPERS_MACRO_ONLY

/*
 *  SDK リンカスクリプト（MCXN947_cm33_core0_flash.ld）が定義する
 *  スタックトップ（ベクタテーブル生成で参照する）
 */
extern int __StackTop;

/*
 *  ターゲットシステム依存の初期化
 */
extern void	target_initialize(void);

/*
 *  ターゲットシステムの終了
 *
 *  システムを終了する時に使う．
 */
extern void	target_exit(void) NoReturn;

#endif /* TOPPERS_MACRO_ONLY */

/*
 *  チップ依存モジュール
 */
#include <chip_kernel_impl.h>

#endif /* TOPPERS_TARGET_KERNEL_IMPL_H */
