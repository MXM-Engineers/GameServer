import ctypes
import ctypes.wintypes as wt
import os
import subprocess

from .elevate import close, runas

kernel32 = ctypes.windll.kernel32
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
STILL_ACTIVE = 259
FILETIME_UNIX_EPOCH_100NS = 116444736000000000
QPC_FREQ = ctypes.c_longlong(0)


class FILETIME(ctypes.Structure):
    _fields_ = [("dwLowDateTime", wt.DWORD), ("dwHighDateTime", wt.DWORD)]


def _to_ns(ft):
    value = (ft.dwHighDateTime << 32) | ft.dwLowDateTime
    return value * 100


def _tasklist_entry(pid):
    try:
        out = subprocess.run(["tasklist", "/FO", "CSV", "/NH", "/FI", "PID eq %d" % pid],
                             capture_output=True, encoding="mbcs", errors="replace", timeout=10)
    except Exception:
        return None
    if out.stdout is None:
        return None
    for line in out.stdout.splitlines():
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) < 2:
            continue
        try:
            entry_pid = int(parts[1])
        except ValueError:
            continue
        if entry_pid == pid:
            entry = {"exe": parts[0]}
            if len(parts) >= 5:
                entry["session"] = parts[2]
                entry["session_number"] = parts[3]
                entry["mem_kb"] = parts[4]
            return entry
    return None


def process_info(pid):
    info = {"pid": pid, "alive": None, "creation_ns": None, "exe": None, "open_error": None}
    handle = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, pid)
    if not handle:
        info["open_error"] = kernel32.GetLastError()
        if info["open_error"] == 87:
            info["alive"] = False
    else:
        try:
            code = wt.DWORD(0)
            if kernel32.GetExitCodeProcess(handle, ctypes.byref(code)):
                info["alive"] = code.value == STILL_ACTIVE
            creation, exit_t, kernel_t, user_t = FILETIME(), FILETIME(), FILETIME(), FILETIME()
            if kernel32.GetProcessTimes(handle, ctypes.byref(creation), ctypes.byref(exit_t),
                                        ctypes.byref(kernel_t), ctypes.byref(user_t)):
                info["creation_ns"] = int((((creation.dwHighDateTime << 32) | creation.dwLowDateTime)
                                           - FILETIME_UNIX_EPOCH_100NS) * 100)
            buf = ctypes.create_unicode_buffer(32768)
            size = wt.DWORD(len(buf))
            if kernel32.QueryFullProcessImageNameW(handle, 0, buf, ctypes.byref(size)):
                info["exe"] = buf.value
        finally:
            kernel32.CloseHandle(handle)
    if info["exe"] is None or info["alive"] is None:
        entry = _tasklist_entry(pid)
        if entry:
            info["tasklist"] = entry
            if info["exe"] is None:
                info["exe"] = entry.get("exe")
            if info["alive"] is None:
                info["alive"] = True
    return info


def qpc_frequency():
    if not QPC_FREQ.value:
        kernel32.QueryPerformanceFrequency(ctypes.byref(QPC_FREQ))
    return QPC_FREQ.value


def list_named(exe_basename):
    try:
        out = subprocess.run(["tasklist", "/FO", "CSV", "/NH", "/FI",
                              "IMAGENAME eq %s" % exe_basename],
                             capture_output=True, encoding="mbcs", errors="replace")
    except Exception:
        return []
    if out.stdout is None:
        return []
    pids = []
    for line in out.stdout.splitlines():
        parts = [p.strip('"') for p in line.split('","')]
        if len(parts) >= 2 and parts[0].lower() == exe_basename.lower():
            try:
                pids.append(int(parts[1]))
            except ValueError:
                pass
    return pids


def kill_tree(pid, force=True, elevated=None):
    if pid is None:
        return {"pid": pid, "action": "none"}
    args = ["/PID", str(pid), "/T"] + (["/F"] if force else [])
    taskkill = os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32",
                            "taskkill.exe")
    if elevated is not True:
        proc = subprocess.run([taskkill] + args, capture_output=True, encoding="oem")
        if proc.returncode == 0 or elevated is False:
            return {"pid": pid, "action": "taskkill", "args": args, "rc": proc.returncode,
                    "stdout": proc.stdout.strip(), "stderr": proc.stderr.strip()}
    runas_pid, handle, err = runas(taskkill, args)
    if handle:
        close(handle)
    return {"pid": pid, "action": "taskkill_runas", "args": args, "runas_pid": runas_pid,
            "error": err}
