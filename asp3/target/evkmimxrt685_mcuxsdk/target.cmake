#
#  ターゲット依存部のCMake定義（EVK-MIMXRT685 / i.MX RT685 + MCUXpresso SDK）
#
#  外部（SDK）ターゲットのパス解決規約（asp3_core PORTING_GUIDE「外部ターゲット」）：
#   - 共通arch（arch/arm_m_gcc/common）は asp3_core サブモジュール側＝ARCHDIR
#   - チップ依存部（imxrt600_mcuxsdk）・ターゲット依存部は本リポジトリ側
#     ＝CMAKE_CURRENT_LIST_DIR 相対
#   - MCUXpresso SDK は本リポジトリの sdk/ サブモジュール群
#
set(ARCHDIR ${ASP3_ROOT_DIR}/arch/arm_m_gcc)
get_filename_component(CHIPDIR ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm_m_gcc/imxrt600_mcuxsdk ABSOLUTE)
set(TARGETDIR ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(MCUX_SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../sdk ABSOLUTE)
set(MCUX_DEVICE_DIR ${MCUX_SDK_DIR}/devices-rt/RT600/MIMXRT685S)

#
#  コンフィギュレーション関連（Python cfg）
#
list(APPEND ASP3_CFG_FILES
    ${TARGETDIR}/target_kernel.cfg
)

list(APPEND ASP3_KERNEL_CFG_TRB_FILES
    ${TARGETDIR}/target_kernel.py
)

list(APPEND ASP3_CHECK_TRB_FILES
    ${TARGETDIR}/target_check.py
)

#
#  インクルードディレクトリ（MCUXpresso SDK の CMSIS・デバイス・fslドライバ・
#  ボードファイルを含む．fslドライバのソースはボードプロジェクト側で
#  コンパイルする＝STM32CubeのHALと同じ扱い）
#
list(APPEND ASP3_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/board
    ${MCUX_SDK_DIR}/cmsis/Core/Include
    ${MCUX_DEVICE_DIR}
    ${MCUX_DEVICE_DIR}/drivers
    ${MCUX_SDK_DIR}/devices-rt/RT600/periph
    ${MCUX_SDK_DIR}/core/drivers/common
    ${MCUX_SDK_DIR}/core/drivers/flexcomm
    ${MCUX_SDK_DIR}/core/drivers/flexcomm/usart
    ${MCUX_SDK_DIR}/core/drivers/ctimer
    ${MCUX_SDK_DIR}/core/drivers/lpc_iopctl
    ${MCUX_SDK_DIR}/core/drivers/lpc_gpio
    ${TARGETDIR}
)

list(APPEND ASP3_COMPILE_DEFS
    CPU_MIMXRT685SFVKB_cm33
    USE_TIM_AS_HRT
    TOPPERS_FPU_ENABLE
    TOPPERS_FPU_LAZYSTACKING
    TOPPERS_FPU_CONTEXT
)

#
#  Cortex-M33 + FPU（fpv5-sp-d16 / hard）— MCUXpresso SDK 既定に合わせる．
#  asp3 ライブラリと cfg1_out（オフセット抽出用ELF）に適用される．
#
list(APPEND ASP3_COMPILE_OPTIONS
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -ffunction-sections
    -fdata-sections
)

#
#  cfg1_out（使い捨てELF）の最小リンク：nm でのシンボル値抽出のみのため
#  SDK のリンカスクリプトは不要．未参照シンボルがGCで消えると cfg パス2が
#  失敗するため gc-sections は使わない．
#
list(APPEND ASP3_LINK_OPTIONS
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -nostartfiles
    -nostdlib
    -Wl,--no-gc-sections
    #  cfg1_out にはスタートアップが無いため，エントリは main スタブ
    #  （target_cfg1_out.h）を指定して未解決エントリ警告を抑止する
    -Wl,--entry=main
)
list(APPEND ASP3_LINK_LIBS c gcc)

#
#  ターゲット依存部のソース（いずれも非TECS版）
#
list(APPEND ASP3_TARGET_C_FILES
    ${TARGETDIR}/target_kernel_impl.c
    ${TARGETDIR}/target_timer.c
    ${TARGETDIR}/target_serial.c
)

#
#  アーキ依存部（チップ層）のインクルード
#
include(${CHIPDIR}/arch.cmake)
