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
 *  sil.hのチップ依存部（MCX N947 + MCUXpresso SDK用）
 *
 *  このインクルードファイルは，sil.hの先頭でインクルードされる．他のファ
 *  イルからは直接インクルードすることはない．このファイルをインクルー
 *  ドする前に，t_stddef.hがインクルードされるので，それらに依存しても
 *  よい．
 */

#ifndef TOPPERS_CHIP_SIL_H
#define TOPPERS_CHIP_SIL_H

/*
 *  プロセッサのエンディアン定義
 *    MCX N947 はリトルエンディアン
 */
#define SIL_ENDIAN_LITTLE

/*
 *  割込み優先度のビット幅（core_sil.hで参照するためここで定義）
 */
#define TBITW_IPRI     3

/*
 *  プロセッサで共通な定義
 */
#include <core_sil.h>

/*
 *  一般共通レジスタ操作関数
 */
#define sil_orb( mem, val )		sil_wrb_mem( mem, sil_reb_mem( mem ) | val )
#define sil_andb( mem, val )	sil_wrb_mem( mem, sil_reb_mem( mem ) & val )
#define sil_orh( mem, val )		sil_wrh_mem( mem, sil_reh_mem( mem ) | val )
#define sil_andh( mem, val )	sil_wrh_mem( mem, sil_reh_mem( mem ) & val )
#define sil_orw( mem, val )		sil_wrw_mem( mem, sil_rew_mem( mem ) | val )
#define sil_andw( mem, val )	sil_wrw_mem( mem, sil_rew_mem( mem ) & val )

#endif /* TOPPERS_CHIP_SIL_H */
