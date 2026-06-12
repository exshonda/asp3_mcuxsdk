cmake_minimum_required(VERSION 3.22)

#
#  TOPPERS/ASP3 Core ＋ MCUXpresso SDK 統合プロジェクト（EVK-MIMXRT685）
#
#  SDK のスタートアップ・リンカスクリプト・ボード初期化（クロック/ピン/
#  FlexSPI=XIP）でブートし，main() が sta_ker() を呼んで ASP3 を起動する．
#  ASP3 カーネルは asp3_core サブモジュールをライブラリ専用モード
#  （ASP3_LIBRARY_ONLY）でビルドし，NXP 固有のチップ/ターゲット依存部は
#  本リポジトリ asp3/ 側から ASP3_TARGET_DIR で供給する
#  （asp3_stm32cube/asp3_fsp と同型．経緯は asp3_core の
#  docs/dev/nxp-integration.md）．
#

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

#  実行ファイル名は asp（→ asp.elf．テストランナ・既存手順との互換のため）
project(asp C ASM)
message("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#  MCUXpresso SDK（サブモジュール）のパス
get_filename_component(MCUX_SDK_DIR ${CMAKE_SOURCE_DIR}/../sdk ABSOLUTE)
set(MCUX_DEVICE_DIR ${MCUX_SDK_DIR}/devices-rt/RT600/MIMXRT685S)

add_executable(asp)

# ===== TOPPERS/ASP3 Core 取り込み（submodule＋ライブラリ専用モード） =====

#  ターゲット依存部（別ボードは asp3/target/ のディレクトリ名に合わせる）
set(ASP3_TARGET evkmimxrt685_mcuxsdk)

#  MCUXSDK 協調ヘルパ（ASP3_TARGET_DIR / ASP3_CORE_DIR / ASP3_ROOT_DIR を解決）
include(../asp3/asp3_mcuxsdk.cmake)

#  アプリ（タスク定義）は asp3_core 同梱 sample（自作アプリに差し替え可）
#  -D ASP3_APPLDIR/ASP3_APPLNAME 指定時はそれを尊重する（test_porting 等の差し替え用）
if(NOT DEFINED ASP3_APPLDIR)
    set(ASP3_APPLDIR  ${ASP3_CORE_DIR}/sample)
    set(ASP3_APPLNAME sample1)
endif()
#  相対パス指定（例：-DASP3_APPLDIR=test/porting）は asp3_core 基準で解決する
if(NOT IS_ABSOLUTE ${ASP3_APPLDIR})
    set(ASP3_APPLDIR ${ASP3_CORE_DIR}/${ASP3_APPLDIR})
endif()
if(DEFINED ASP3_EXTRA_APP_C_FILES)
    set(_extra_app_c_files "")
    foreach(path IN LISTS ASP3_EXTRA_APP_C_FILES)
        if(NOT IS_ABSOLUTE ${path})
            set(path ${ASP3_CORE_DIR}/${path})
        endif()
        list(APPEND _extra_app_c_files ${path})
    endforeach()
    set(ASP3_EXTRA_APP_C_FILES ${_extra_app_c_files})
endif()

#  asp3 カーネルライブラリをライブラリ専用モードでビルド
set(ASP3_LIBRARY_ONLY ON CACHE BOOL "build asp3 as library only" FORCE)
add_subdirectory(${ASP3_CORE_DIR} asp3)

# ===== MCUXpresso SDK・ボード初期化・アプリのソース =====

target_sources(asp PRIVATE
    main.c

    #  ボード初期化（SDK examples の _boards/evkmimxrt685 からコピー＝BSD-3）
    board/board.c
    board/clock_config.c
    board/pin_mux.c
    board/flash_config.c

    #  SDK デバイス（startup・system・デバイス固有ドライバ）
    ${MCUX_DEVICE_DIR}/gcc/startup_MIMXRT685S_cm33.S
    ${MCUX_DEVICE_DIR}/system_MIMXRT685S_cm33.c
    ${MCUX_DEVICE_DIR}/drivers/fsl_clock.c
    ${MCUX_DEVICE_DIR}/drivers/fsl_power.c
    ${MCUX_DEVICE_DIR}/drivers/fsl_reset.c

    #  SDK 共通 fsl ドライバ
    ${MCUX_SDK_DIR}/core/drivers/common/fsl_common.c
    ${MCUX_SDK_DIR}/core/drivers/common/fsl_common_arm.c
    ${MCUX_SDK_DIR}/core/drivers/flexcomm/fsl_flexcomm.c
    ${MCUX_SDK_DIR}/core/drivers/flexcomm/usart/fsl_usart.c
    ${MCUX_SDK_DIR}/core/drivers/ctimer/fsl_ctimer.c
    ${MCUX_SDK_DIR}/core/drivers/flexspi/fsl_flexspi.c
    ${MCUX_SDK_DIR}/core/drivers/cache/cache64/fsl_cache.c
    ${MCUX_SDK_DIR}/core/drivers/lpc_gpio/fsl_gpio.c

    #  ASP3 アプリ（タスク本体）
    ${ASP3_APPLDIR}/${ASP3_APPLNAME}.c
    ${ASP3_EXTRA_APP_C_FILES}
)

target_include_directories(asp PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/board
    ${MCUX_SDK_DIR}/cmsis/Core/Include
    ${MCUX_DEVICE_DIR}
    ${MCUX_DEVICE_DIR}/drivers
    ${MCUX_SDK_DIR}/devices-rt/RT600/periph
    ${MCUX_SDK_DIR}/core/drivers/common
    ${MCUX_SDK_DIR}/core/drivers/flexcomm
    ${MCUX_SDK_DIR}/core/drivers/flexcomm/usart
    ${MCUX_SDK_DIR}/core/drivers/flexcomm/i2c
    ${MCUX_SDK_DIR}/core/drivers/i3c
    ${MCUX_SDK_DIR}/core/drivers/ctimer
    ${MCUX_SDK_DIR}/core/drivers/flexspi
    ${MCUX_SDK_DIR}/core/drivers/cache/cache64
    ${MCUX_SDK_DIR}/core/drivers/lpc_gpio
    ${MCUX_SDK_DIR}/core/drivers/lpc_iopctl
    ${MCUX_SDK_DIR}/components/debug_console_lite
    ${ASP3_APPLDIR}
)

target_compile_definitions(asp PRIVATE
    CPU_MIMXRT685SFVKB_cm33
    BOOT_HEADER_ENABLE=1
)

target_compile_options(asp PRIVATE
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -ffunction-sections
    -fdata-sections
    -Wall
)

target_link_options(asp PRIVATE
    -mcpu=cortex-m33
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    --specs=nano.specs
    --specs=nosys.specs
    -T${CMAKE_SOURCE_DIR}/MIMXRT685Sxxxx_cm33_flash.ld
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=asp.map
)
set_target_properties(asp PROPERTIES LINK_DEPENDS
    ${CMAKE_SOURCE_DIR}/MIMXRT685Sxxxx_cm33_flash.ld)

target_link_libraries(asp asp3)

#  システムサービス（banner / syslog / serial / logtask 等）を追加
asp3_add_syssvc(asp)

#  TOPPERS/ASP3 を使うための MCUXSDK の設定（現状なし）
asp3_set_mcuxsdk_options(asp)
