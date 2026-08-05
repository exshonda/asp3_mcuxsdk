#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  EVK-MIMXRT685（MCUXpresso SDK統合）実機向け testexec ランナー
#
#  asp3_core の test/testexec.py は asp3_core 単体を configure する前提のため、
#  外側プロジェクト（evkmimxrt685）では使えない。本スクリプトは外側プロジェクト
#  を 1 テストずつ configure（-DASP3_APPLDIR/APPLNAME/APPCFGNAME 差し替え）→
#  build → LinkServer（標準）または JLinkExe（--flash-tool jlink）で書込み →
#  シリアル出力を判定する（asp3_stm32cube の scripts/testexec_stm32.py の移植）。
#
#  判定は CI ランナー（asp3_core scripts/ci/run_testexec.py）互換:
#    PASS    "All check points passed."（hrt1/dlynse は SPECIAL_SPEC の完走マーカ）
#    SKIP    "This test program is not necessary."
#    FAIL    "## "・"not ok " 行・"Unregistered Exception"・SPECIAL_SPEC の fail
#    TIMEOUT 期限内にいずれも出ない
#
#  --rejudge で（実行せず）保存済み serial ログのみ再判定する。
#
#  前提: オンボードプローブ（LPC-Link2）が標準のCMSIS-DAPファームウェアで、
#  LinkServer が導入済みであること（J-Linkファームウェア化した場合は
#  --flash-tool jlink。asp3_core target/mimxrt685evk_gcc/target_user.md 参照）。
#  VCOM（--port）を読むプロセスは本スクリプトのみにすること（複数リーダは
#  バイトを奪い合い誤判定の原因になる。並行実行も禁止）。
#
#  Linux / Windows 両対応:
#    シリアルは POSIX では termios+select、Windows では pyserial を使う
#    （Windows は `pip install pyserial` が必要）。--port 省略時は
#    Linux=/dev/ttyACM0、Windows=MCU-Link VCom を自動検出。LinkServer /
#    J-Link の実行ファイルも OS ごとの既定パスを探す（環境変数 LINKSERVER・
#    JLINK で上書き可）。
#
#  使い方:
#    scripts/testexec_mcuxsdk.py [--board evkmimxrt685/sample1] [--port PORT] [test ...]
#    テスト名省略時は標準機能テスト一式（拡張パッケージ・perf・arm_* は対象外）
#
import argparse
import glob
import os
import re
import subprocess
import sys
import time

WINDOWS = (os.name == "nt")

if not WINDOWS:
    import select
    import termios

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
CORE = os.path.join(REPO, "asp3", "asp3_core")


def cm(path):
    """CMake 引数用のパス（Windows のバックスラッシュはエスケープ扱いされる）"""
    return path.replace("\\", "/")


def find_linkserver():
    if os.environ.get("LINKSERVER"):
        return os.environ["LINKSERVER"]
    if WINDOWS:
        #  既定インストール先 C:\NXP\LinkServer_<version>\LinkServer.exe（最新を選ぶ）
        cands = sorted(glob.glob(r"C:\NXP\LinkServer_*\LinkServer.exe"))
        if cands:
            return cands[-1]
        return "LinkServer.exe"
    if os.path.exists("/usr/local/LinkServer/LinkServer"):
        return "/usr/local/LinkServer/LinkServer"
    return "LinkServer"


def find_jlink():
    if os.environ.get("JLINK"):
        return os.environ["JLINK"]
    if WINDOWS:
        cands = sorted(glob.glob(r"C:\Program Files*\SEGGER\JLink*\JLink.exe"))
        if cands:
            return cands[-1]
        return "JLink.exe"
    return "JLinkExe"


JLINK = find_jlink()
LINKSERVER = find_linkserver()

#  標準機能テスト（asp3_core test/testexec.py の TEST_SPEC から、
#  拡張パッケージ（messagebuf/ovrhdr/rstr/subprio/inherit）・perf・arm_* を除く）
TEST_SPEC = {
    **{f"cpuexc{i}": {"SRC": f"test_cpuexc{i}", "CFG": "test_cpuexc"}
       for i in range(1, 11)},
    "dlynse":   {"SRC": "test_dlynse"},
    "dtq1":     {"SRC": "test_dtq1"},
    "exttsk":   {"SRC": "test_exttsk"},
    "flg1":     {"SRC": "test_flg1"},
    "hrt1":     {"SRC": "test_hrt1"},
    "int1":     {"SRC": "test_int1"},
    "mpf1":     {"SRC": "test_mpf1"},
    **{f"mutex{i}": {"SRC": f"test_mutex{i}"} for i in range(1, 9)},
    "notify1":  {"SRC": "test_notify1"},
    "pdq1":     {"SRC": "test_pdq1"},
    "raster1":  {"SRC": "test_raster1"},
    "raster2":  {"SRC": "test_raster2"},
    "sem1":     {"SRC": "test_sem1"},
    "sem2":     {"SRC": "test_sem2"},
    "suspend1": {"SRC": "test_suspend1"},
    "sysman1":  {"SRC": "test_sysman1"},
    "sysstat1": {"SRC": "test_sysstat1"},
    "task1":    {"SRC": "test_task1"},
    "tmevt1":   {"SRC": "test_tmevt1"},
}


def run(cmd, log_path, cwd=None, timeout=300):
    """shell 経由でコマンド実行（cd は使わず cwd で渡す＝Windows のドライブ跨ぎ対策）"""
    with open(log_path, "w") as out:
        return subprocess.call(cmd, shell=True, cwd=cwd, timeout=timeout,
                               stdout=out, stderr=subprocess.STDOUT) == 0


def check_port_free(port):
    """tty を読む他プロセスがいれば警告する（バイト奪い合い防止）"""
    if WINDOWS:
        #  Windows の COM ポートは排他オープンなので、他プロセスが掴んでいれば
        #  open 時に例外になる（事前チェック不要）
        return
    try:
        out = subprocess.run(["fuser", port], capture_output=True, text=True)
        pids = out.stdout.split()
        if pids:
            print(f"WARNING: {port} is in use by PID(s) {' '.join(pids)} -- "
                  f"output may be lost (kill them first)", file=sys.stderr)
    except FileNotFoundError:
        pass


def default_port():
    if not WINDOWS:
        return "/dev/ttyACM0"
    #  MCU-Link / J-Link の VCOM を自動検出（pyserial → 失敗時はレジストリ）
    try:
        from serial.tools import list_ports
        for p in list_ports.comports():
            text = f"{p.description} {p.manufacturer or ''} {p.hwid or ''}"
            if re.search(r"MCU-?Link|J-?Link|VCom|USB Serial", text, re.I):
                return p.device
    except ImportError:
        pass
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                            r"HARDWARE\DEVICEMAP\SERIALCOMM") as key:
            for i in range(winreg.QueryInfoKey(key)[1]):
                name, value, _ = winreg.EnumValue(key, i)
                if "USBSER" in name.upper():
                    return value
    except OSError:
        pass
    return None


class PosixSerialPort:
    """termios + select（従来の Linux 動作をそのまま維持）"""

    def __init__(self, port, baud=115200):
        self.fd = os.open(port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        attrs = termios.tcgetattr(self.fd)
        attrs[0] = attrs[1] = attrs[3] = 0                       # raw
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL  # 8N1
        attrs[4] = attrs[5] = getattr(termios, f"B{baud}")
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        termios.tcflush(self.fd, termios.TCIFLUSH)

    def read(self, timeout):
        r, _, _ = select.select([self.fd], [], [], timeout)
        if not r:
            return b""
        try:
            return os.read(self.fd, 4096)
        except BlockingIOError:
            return b""

    def close(self):
        os.close(self.fd)


class PySerialPort:
    """Windows 用（pyserial）"""

    def __init__(self, port, baud=115200):
        try:
            import serial
        except ImportError:
            sys.exit("ERROR: pyserial is required on Windows "
                     "(pip install pyserial)")
        self.ser = serial.Serial(port, baud, timeout=0.5)
        self.ser.reset_input_buffer()

    def read(self, timeout):
        #  ser.timeout への代入は（同値でも）_reconfigure_port() を呼び、win32 では
        #  SetCommState＝CDC SET_LINE_CODING になる。受信中に毎回やるとバイトが
        #  化けるので、値が変わるときだけ設定する（dlynse で実際に化けた）
        if self.ser.timeout != timeout:
            self.ser.timeout = timeout
        data = self.ser.read(1)          # timeout まで待つ
        if data:
            pending = self.ser.in_waiting
            if pending:
                data += self.ser.read(pending)
        return data

    def close(self):
        self.ser.close()


def open_serial(port):
    return PySerialPort(port) if WINDOWS else PosixSerialPort(port)


#  判定マーカー（asp3_core scripts/ci/run_testexec.py と同一仕様）
PASS_MARK = "All check points passed."
SKIP_MARK = "This test program is not necessary."
FAIL_PATTERNS = (
    re.compile(r"^not ok "),
    re.compile(r"^## "),
    re.compile(r"^Unregistered (Exception|Interrupt)"),
)
SPECIAL_SPEC = {
    "hrt1": {
        "pass": "high resolution timer count test finishes.",
        "fail": (re.compile(r"goes back"),),
    },
    "dlynse": {
        "pass": "-- for checking boundary conditions --",
        "fail": (re.compile(r"sil_dly_nse\(\d+\): \d+ NG"),),
    },
}


def judge_text(test, text):
    special = SPECIAL_SPEC.get(test, {})
    pass_mark = special.get("pass", PASS_MARK)
    fail_patterns = FAIL_PATTERNS + special.get("fail", ())
    for line in text.splitlines():
        for pat in fail_patterns:
            if pat.search(line):
                return "FAIL", line.strip()
    if SKIP_MARK in text:
        return "SKIP", "not necessary on this target"
    if pass_mark in text:
        return "PASS", ""
    return None, ""


def judge_serial(test, sp, deadline, log_file):
    buf = b""
    while time.time() < deadline:
        chunk = sp.read(0.5)
        if not chunk:
            continue
        buf += chunk
        log_file.write(chunk)
        verdict, detail = judge_text(test, buf.decode("ascii", errors="replace"))
        if verdict:
            return verdict, detail
    return "TIMEOUT", ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--board", default="evkmimxrt685/sample1",
                    help="ボードプロジェクトのパス（REPO相対）")
    ap.add_argument("--port", default=None,
                    help="シリアルポート（省略時: Linux=/dev/ttyACM0、"
                         "Windows=MCU-Link VCom を自動検出）")
    ap.add_argument("--flash-tool", choices=["linkserver", "jlink"],
                    default="linkserver")
    ap.add_argument("--ls-device", default="MIMXRT685S:EVK-MIMXRT685")
    ap.add_argument("--jlink-device", default="MIMXRT685S_M33")
    ap.add_argument("--run-timeout", type=int, default=90)
    ap.add_argument("--rejudge", action="store_true",
                    help="実行せず保存済み serial ログのみ再判定")
    ap.add_argument("tests", nargs="*", default=list(TEST_SPEC))
    args = ap.parse_args()

    board_dir = os.path.join(REPO, args.board)
    build_dir = os.path.join(board_dir, "build", "TestExec")
    logs = os.path.join(build_dir, "logs")
    os.makedirs(logs, exist_ok=True)

    results = {}
    if args.rejudge:
        for test in args.tests:
            slog = os.path.join(logs, f"{test}.serial.log")
            if not os.path.isfile(slog):
                results[test] = ("NO_LOG", "")
                continue
            with open(slog, "rb") as f:
                text = f.read().decode("ascii", errors="replace")
            verdict, detail = judge_text(test, text)
            results[test] = (verdict or "TIMEOUT", detail)
        return summary(results)

    port = args.port or default_port()
    if port is None:
        sys.exit("ERROR: serial port not found -- specify it with --port")
    print(f"serial port: {port}", flush=True)
    check_port_free(port)

    #  書込みコマンド（LinkServer＝load後にリセット実行／J-Link＝loadfile→r→g）
    #  いずれも cwd=build_dir で実行する（相対 asp.elf 前提）
    if args.flash_tool == "jlink":
        jlink_cmd = os.path.join(build_dir, "flash.jlink")
        with open(jlink_cmd, "w") as f:
            f.write("loadfile asp.elf\nr\ng\nqc\n")
        flash_cmd = (
            f"\"{JLINK}\" -device {args.jlink_device} -if SWD "
            f"-speed 4000 -autoconnect 1 -NoGui 1 -CommandFile \"{jlink_cmd}\""
        )
    else:
        flash_cmd = f"\"{LINKSERVER}\" flash {args.ls_device} load asp.elf"

    for test in args.tests:
        spec = TEST_SPEC.get(test)
        if spec is None:
            results[test] = ("SKIP", "unknown test name")
            continue
        applname = spec["SRC"]
        cfgname = spec.get("CFG", applname)
        print(f"== {test} ==", flush=True)

        cfg_cmd = (
            f"cmake --preset Debug -B \"{cm(build_dir)}\" "
            f"\"-DASP3_APPLDIR={cm(CORE)}/test\" "
            f"-DASP3_APPLNAME={applname} "
            f"-DASP3_APPCFGNAME={cfgname} "
            f"\"-DASP3_EXTRA_APP_C_FILES={cm(CORE)}/syssvc/test_svc.c\""
        )
        log = os.path.join(logs, f"{test}.build.log")
        if not (run(cfg_cmd, log, cwd=board_dir) and
                run(f"cmake --build \"{cm(build_dir)}\"",
                    log.replace(".build.", ".ninja."), cwd=board_dir)):
            results[test] = ("BUILD_FAIL", f"see {log}")
            print("   BUILD_FAIL", flush=True)
            continue

        #  シリアルは書込みより先に開く（起動直後のバナー取りこぼし防止）
        sp = open_serial(port)
        try:
            if not run(flash_cmd, os.path.join(logs, f"{test}.flash.log"),
                       cwd=build_dir):
                results[test] = ("FLASH_FAIL", "")
                print("   FLASH_FAIL", flush=True)
                continue
            with open(os.path.join(logs, f"{test}.serial.log"), "wb") as slog:
                verdict, detail = judge_serial(test, sp,
                                               time.time() + args.run_timeout, slog)
        finally:
            sp.close()
        results[test] = (verdict, detail)
        print(f"   {verdict} {detail}", flush=True)
    return summary(results)


def summary(results):
    print("\n==== summary ====")
    counts = {}
    for test, (verdict, detail) in results.items():
        counts[verdict] = counts.get(verdict, 0) + 1
        print(f"{verdict:10s} {test:10s} {detail}")
    print(", ".join(f"{k}={v}" for k, v in sorted(counts.items())),
          f"/ total={len(results)}")
    return 0 if set(counts) <= {"PASS", "SKIP"} else 1


if __name__ == "__main__":
    sys.exit(main())
