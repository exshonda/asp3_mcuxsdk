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
 *  EVK-MIMXRT685（MCUXpresso SDK統合）サポートモジュール
 */

#ifndef TOPPERS_EVKMIMXRT685_MCUXSDK_H
#define TOPPERS_EVKMIMXRT685_MCUXSDK_H

/*
 *  コアのクロック周波数
 *
 *  SDK の BOARD_BootClockRUN 構成（clock_config.h の
 *  BOARD_BOOTCLOCKRUN_CORE_CLOCK）：main_pll＝528MHz×18/19＝500.21MHz，
 *  CPU＝main_clk/2＝250.105MHz．
 */
#define CPU_CLOCK_HZ 250105263

/*
 *  割込み数
 */
#define TMAX_INTNO (59 + 16)

/*
 *  微少時間待ちのための定義（本来はSILのターゲット依存部）
 *
 *  Phase A（mimxrt685evk_gcc・300MHz）の実機較正値（呼出7サイクル・
 *  ループ5サイクル）を 250.105MHz に換算した理論下限：
 *  呼出 7cyc＝27.99ns→27，ループ 5cyc＝19.99ns→19．
 *  実機の test_dlynse で確認・必要なら再較正すること．
 */
#define SIL_DLY_TIM1    27
#define SIL_DLY_TIM2    19

#endif /* TOPPERS_EVKMIMXRT685_MCUXSDK_H */
