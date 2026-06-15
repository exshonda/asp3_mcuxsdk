# Target (FRDM-MCXN947 + MCUXpresso SDK) awareness helpers for gdb OS-awareness (ASP3).
#
# 役割: ターゲット（ボード）依存の知識。本ターゲットの割込みはコア内蔵の
#       NVIC で管理されるため，ボード/チップ固有の追加項目は無く，
#       asp3_core 側のコア層（arm_m common）の API を再エクスポートする。
#
# core_os_awareness.py は asp3_core サブモジュール（asp3/asp3_core）の
# arch/arm_m_gcc/common にあるため，本ファイルからの相対パスで sys.path に
# 追加してから import する。

import os
import sys

# 下位層(core)の import で .pyc を生成させない（ソースツリーに __pycache__ を残さない）。
sys.dont_write_bytecode = True

sys.path.insert(0, os.path.normpath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 "../../asp3_core/arch/arm_m_gcc/common")))

import core_os_awareness

# ターゲット固有の追加は今回なし。コア層の API をそのまま公開する。
int_enabled = core_os_awareness.int_enabled
int_pending = core_os_awareness.int_pending
inh_handler = core_os_awareness.inh_handler
primap_bit = core_os_awareness.primap_bit
