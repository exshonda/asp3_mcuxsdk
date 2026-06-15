#
#  アーキ依存部のCMake定義（MCX N947 / Cortex-M33 + MCUXpresso SDK）
#
#  外部（SDK）ターゲットのパス解決規約（asp3_core PORTING_GUIDE「外部ターゲット」）：
#   - 共通arch（arch/arm_m_gcc/common）は asp3_core サブモジュール側＝ARCHDIR
#   - チップ依存部（imxrt600_mcuxsdk）は本リポジトリ側＝CHIPDIR
#  ARCHDIR/CHIPDIR/TARGETDIR は target.cmake で設定済み．
#
#  start.S（_kernel_start）は含めない：リセットは SDK の startup_MCXN947_cm33_core0.S
#  （Reset_Handler）が握り，main() が sta_ker() を呼ぶ（STM32Cube/FSP と同方針）．
#  ベクタテーブルのリセットエントリも Reset_Handler を指す（target_kernel.py 参照）．
#  SIO は target_serial.c（fsl_usart）が供給する．
#

list(APPEND ASP3_SYMVAL_TABLES
    ${ARCHDIR}/common/core_sym.def
)

list(APPEND ASP3_OFFSET_TRB_FILES
    ${ARCHDIR}/common/core_offset.py
)

list(APPEND ASP3_INCLUDE_DIRS
    ${CHIPDIR}
    ${ARCHDIR}/common
    ${ASP3_ROOT_DIR}/arch/gcc
)

#  TOPPERS_ENABLE_TRUSTZONE を定義する：SDK の startup（イメージタイプ＝0）で
#  ブートした CPU は Secure 状態で実行されるため，EXC_RETURN は Secure 用
#  （ES/S=1）を用いる（mps2_an521＝Secure 実行と同じ構成）．
#  ※ Phase A（asp3_core 本体の mcxn947 (Phase A 相当の自前ブート)）はベクタ9＝bit14 の
#  「プレーンイメージ」ブートのため未定義（EXC_RETURN 0xFFFFFFBC）＝逆．
#  未定義のままだと最初のディスパッチで INVPC（UsageFault）になる（実機確認）．
list(APPEND ASP3_COMPILE_DEFS
    TOPPERS_CORTEX_M33
    TOPPERS_ENABLE_TRUSTZONE
    __TARGET_ARCH_THUMB=5
    __TARGET_FPU_FPV4_SP
)

list(APPEND ASP3_ARCH_C_FILES
    ${ARCHDIR}/common/core_kernel_impl.c
    ${ARCHDIR}/common/core_support.S
)
