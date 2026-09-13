import ctypes
import ctypes.wintypes as wt

SEE_MASK_NOCLOSEPROCESS = 0x00000040
WAIT_OBJECT_0 = 0
WAIT_TIMEOUT = 0x102
SW_HIDE = 0
SW_SHOWNOACTIVATE = 4

kernel32 = ctypes.windll.kernel32
shell32 = ctypes.windll.shell32


class SHELLEXECUTEINFOW(ctypes.Structure):
    _fields_ = [
        ("cbSize", wt.DWORD),
        ("fMask", wt.ULONG),
        ("hwnd", wt.HWND),
        ("lpVerb", wt.LPCWSTR),
        ("lpFile", wt.LPCWSTR),
        ("lpParameters", wt.LPCWSTR),
        ("lpDirectory", wt.LPCWSTR),
        ("nShow", ctypes.c_int),
        ("hInstApp", wt.HINSTANCE),
        ("lpIDList", ctypes.c_void_p),
        ("lpClass", wt.LPCWSTR),
        ("hkeyClass", wt.HKEY),
        ("dwHotKey", wt.DWORD),
        ("hIcon", wt.HANDLE),
        ("hProcess", wt.HANDLE),
    ]


def is_admin():
    try:
        return bool(shell32.IsUserAnAdmin())
    except Exception:
        return False


def runas(exe, args=None, cwd=None, show=SW_HIDE):
    params = " ".join(args or [])
    sei = SHELLEXECUTEINFOW()
    sei.cbSize = ctypes.sizeof(SHELLEXECUTEINFOW)
    sei.fMask = SEE_MASK_NOCLOSEPROCESS
    sei.lpVerb = "runas"
    sei.lpFile = exe
    sei.lpParameters = params
    sei.lpDirectory = cwd
    sei.nShow = show
    if not shell32.ShellExecuteExW(ctypes.byref(sei)):
        return None, None, "ShellExecuteExW runas failed err=%d" % ctypes.get_last_error()
    handle = sei.hProcess
    pid = wt.DWORD(0)
    if handle:
        kernel32.GetProcessId.restype = wt.DWORD
        pid = kernel32.GetProcessId(handle)
    else:
        pid = 0
    return int(pid or 0), handle, None


def wait(handle, timeout_s):
    if not handle:
        return None
    rc = kernel32.WaitForSingleObject(handle, int(timeout_s * 1000))
    if rc == WAIT_OBJECT_0:
        code = wt.DWORD(0)
        kernel32.GetExitCodeProcess(handle, ctypes.byref(code))
        return code.value
    return None


def close(handle):
    if handle:
        try:
            kernel32.CloseHandle(handle)
        except Exception:
            pass
