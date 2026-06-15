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
 *  FRDM-MCXN947（MCUXpresso SDK統合）サポートモジュール
 */

#ifndef TOPPERS_FRDMMCXN947_MCUXSDK_H
#define TOPPERS_FRDMMCXN947_MCUXSDK_H

/*
 *  コアのクロック周波数
 *
 *  SDK の BOARD_InitBootClocks（clock_config.c）は BOARD_BootClockPLL150M
 *  を選択する．CPU コアクロック＝150MHz
 *  （clock_config.h の BOARD_BOOTCLOCKPLL150M_CORE_CLOCK）．
 */
#define CPU_CLOCK_HZ 150000000

/*
 *  割込み数
 *
 *  MCXN947（cm33_core0）の最大割込み番号は CTI0_IRQn = 155．
 *  ベクタ番号は IRQn + 16．
 */
#define TMAX_INTNO (155 + 16)

/*
 *  微少時間待ちのための定義（本来はSILのターゲット依存部）
 *
 *  Phase A（300MHz）の実機較正値（呼出7サイクル・ループ5サイクル）を
 *  150MHz に換算した理論下限：
 *  呼出 7cyc＝46.67ns→46，ループ 5cyc＝33.33ns→33．
 *  実機の test_dlynse で確認・必要なら再較正すること．
 */
#define SIL_DLY_TIM1    46
#define SIL_DLY_TIM2    33

#endif /* TOPPERS_FRDMMCXN947_MCUXSDK_H */
