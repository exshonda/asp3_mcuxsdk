/*
 *  TOPPERS Software
 *      Toyohashi Open Platform for Embedded Real-Time Systems
 *
 *  Copyright (C) 2026 by Embedded and Real-Time Systems Laboratory
 *              Graduate School of Informatics, Nagoya Univ., JAPAN
 *
 *  上記著作権者は，本ソフトウェアを TOPPERS ライセンス（条件は
 *  asp3_core 同梱ファイルの先頭コメントを参照）の下で利用することを
 *  許諾する．本ソフトウェアは無保証で提供される．
 */

/*
 *  t_stddef.hのチップ依存部（MCX N947 + MCUXpresso SDK用）
 *
 *  このインクルードファイルは，t_stddef.hの先頭でインクルードされる．
 *  他のファイルからは直接インクルードすることはない．他のインクルード
 *  ファイルに先立って処理されるため，他のインクルードファイルに依存し
 *  てはならない．
 */

#ifndef TOPPERS_CHIP_STDDEF_H
#define TOPPERS_CHIP_STDDEF_H

/*
 *  チップを識別するためのマクロの定義
 */
#define TOPPERS_MCXN947_MCUXSDK		/* チップ略称 */

/*
 *  プロセッサで共通な定義
 */
#include <core_stddef.h>

#endif /* TOPPERS_CHIP_STDDEF_H */
