import ctypes
import ctypes.wintypes as wt
import json
import os
import threading
import time

from .common import qpc_ns

user32 = ctypes.windll.user32
user32.GetForegroundWindow.restype = wt.HWND
user32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]

kernel32 = ctypes.windll.kernel32
kernel32.OpenProcess.restype = wt.HANDLE
kernel32.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
PROCESS_QUERY_LIMITED_INFORMATION = 0x1000


def process_creation_ns(pid):
    if not pid:
        return None
    handle = kernel32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, int(pid))
    if not handle:
        return None
    try:
        code = wt.DWORD()
        if not kernel32.GetExitCodeProcess(handle, ctypes.byref(code)) or code.value != 259:
            return None
        creation = wt.FILETIME()
        exit_time = wt.FILETIME()
        kernel_time = wt.FILETIME()
        user_time = wt.FILETIME()
        if not kernel32.GetProcessTimes(handle, ctypes.byref(creation), ctypes.byref(exit_time),
                                        ctypes.byref(kernel_time), ctypes.byref(user_time)):
            return None
        return ((creation.dwHighDateTime << 32) | creation.dwLowDateTime) * 100
    finally:
        kernel32.CloseHandle(handle)


def owned_map(owned):
    result = {}
    for item in owned or ():
        if isinstance(item, (tuple, list)):
            if len(item) != 2:
                continue
            pid, creation = item
        else:
            pid, creation = item, None
        if not pid:
            continue
        result[int(pid)] = int(creation) if creation else None
    return result


def is_owned(owned, fg_pid):
    recorded = owned.get(fg_pid)
    if not recorded:
        return False
    return process_creation_ns(fg_pid) == recorded


def foreground():
    hwnd = user32.GetForegroundWindow()
    pid = wt.DWORD(0)
    if hwnd:
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    return int(hwnd or 0), int(pid.value)


def cursor_pos():
    pt = wt.POINT()
    if not user32.GetCursorPos(ctypes.byref(pt)):
        return None
    return int(pt.x), int(pt.y)


def guard_mutation(rec):
    if rec.get("blocked") in (True, 1):
        return None
    return "blocked_false"


class GuardWatcher(threading.Thread):
    def __init__(self, log, run_dir, client_pid_getter, interval_s=0.1):
        super().__init__(daemon=True)
        self.log = log
        self.run_dir = run_dir
        self.client_pid_getter = client_pid_getter
        self.interval_s = interval_s
        self.stop_event = threading.Event()
        self.abort_event = threading.Event()
        self.abort_reason = None
        self.offset = 0
        self.path = None
        self.counts = {"events": 0, "blocked": 0, "mutations": 0, "errors": 0}

    def snapshot(self):
        return dict(self.counts, path=self.path, abort=self.abort_reason)

    def run(self):
        while not self.stop_event.wait(self.interval_s):
            pid = self.client_pid_getter()
            if not pid:
                continue
            path = os.path.join(self.run_dir, "client_%d.ndjson" % pid)
            if path != self.path:
                self.path = path
                self.offset = 0
            if not os.path.exists(path):
                continue
            try:
                size = os.path.getsize(path)
                if size < self.offset:
                    self.offset = 0
                if size == self.offset:
                    continue
                with open(path, "r", encoding="utf-8", errors="replace") as f:
                    f.seek(self.offset)
                    data = f.read()
                    self.offset = f.tell()
            except OSError:
                continue
            for line in data.splitlines():
                line = line.strip()
                if not line:
                    continue
                try:
                    rec = json.loads(line)
                except ValueError:
                    continue
                event = rec.get("event")
                if event == "error":
                    self.counts["errors"] += 1
                    self.log.emit("client_error", detail=rec)
                if event != "os_guard":
                    continue
                self.counts["events"] += 1
                if rec.get("blocked") in (True, 1):
                    self.counts["blocked"] += 1
                mutation = guard_mutation(rec)
                if mutation:
                    self.counts["mutations"] += 1
                    self.log.emit("guard_mutation", key=mutation, detail=rec)
                    if not self.abort_event.is_set():
                        self.abort_reason = "native_guard_unblocked:%s" % mutation
                        self.abort_event.set()
        self.log.emit("guard_watch_stop", **self.counts)

    def stop(self):
        self.stop_event.set()
        self.join(timeout=2.0)


class DesktopMonitor(threading.Thread):
    def __init__(self, log, client_pid_getter, owned_pid_getter=None, interval_s=0.05,
                 heartbeat_s=1.0, cursor_tolerance=1):
        super().__init__(daemon=True)
        self.log = log
        self.client_pid_getter = client_pid_getter
        self.owned_pid_getter = owned_pid_getter or (lambda: [])
        self.interval_s = interval_s
        self.heartbeat_s = heartbeat_s
        self.cursor_tolerance = cursor_tolerance
        self.stop_event = threading.Event()
        self.abort_event = threading.Event()
        self.started = threading.Event()
        self.abort_reason = None
        self.aim_marks = []
        self.samples = 0
        self.owned_focus_hits = 0
        self.client_focus_hits = 0
        self.cursor_samples = []
        self.foreground_timeline = []
        self.lock = threading.Lock()
        self.last_heartbeat = 0.0
        self.base_cursor = None

    def note_aim(self):
        with self.lock:
            self.aim_marks.append(qpc_ns())

    def snapshot(self):
        with self.lock:
            return {
                "client_pid": self.client_pid_getter(),
                "foreground": foreground(),
                "cursor": list(cursor_pos()) if cursor_pos() else None,
                "base_cursor": list(self.base_cursor) if self.base_cursor else None,
                "samples": self.samples,
                "client_focus_hits": self.client_focus_hits,
                "owned_focus_hits": self.owned_focus_hits,
                "foreground_transitions": len(self.foreground_timeline),
                "cursor_samples": len(self.cursor_samples),
                "abort": self.abort_reason,
            }

    def run(self):
        base = cursor_pos()
        self.base_cursor = base
        last_cursor = base
        last_fg = None
        self.log.emit("monitor_start", baseline_cursor=list(base) if base else None,
                      foreground=foreground()[1], interval_ms=int(self.interval_s * 1000))
        try:
            while not self.stop_event.wait(self.interval_s):
                self.samples += 1
                client_id = self.client_pid_getter()
                owned = owned_map(self.owned_pid_getter())
                hwnd, fg_pid = foreground()
                if fg_pid != last_fg:
                    owned_hit = is_owned(owned, fg_pid)
                    entry = {"pid": fg_pid, "hwnd": hwnd, "client": fg_pid == client_id,
                             "owned": owned_hit, "samples": self.samples}
                    self.foreground_timeline.append(entry)
                    self.log.emit("foreground_change", **entry)
                    last_fg = fg_pid
                    if fg_pid == client_id:
                        self.client_focus_hits += 1
                        if not self.abort_event.is_set():
                            self.abort_reason = "client_took_foreground"
                            self.abort_event.set()
                    elif owned_hit:
                        self.owned_focus_hits += 1
                        if not self.abort_event.is_set():
                            self.abort_reason = "owned_gui_took_foreground"
                            self.abort_event.set()
                cur = cursor_pos()
                if cur is not None and last_cursor is not None and cur != last_cursor:
                    dx, dy = cur[0] - last_cursor[0], cur[1] - last_cursor[1]
                    if abs(dx) > self.cursor_tolerance or abs(dy) > self.cursor_tolerance:
                        entry = {"from": list(last_cursor), "to": list(cur), "dx": dx, "dy": dy,
                                 "at_ns": qpc_ns()}
                        self.cursor_samples.append(entry)
                        self.log.emit("cursor_motion", **entry)
                    last_cursor = cur
                now = time.monotonic()
                if now - self.last_heartbeat >= self.heartbeat_s:
                    self.last_heartbeat = now
                    self.log.emit("monitor_heartbeat", foreground_pid=fg_pid, hwnd=hwnd,
                                  cursor=list(cur) if cur else None,
                                  client_focus_hits=self.client_focus_hits,
                                  owned_focus_hits=self.owned_focus_hits, samples=self.samples)
                if self.samples == 1:
                    self.started.set()
        finally:
            self.log.emit("monitor_stop", samples=self.samples, client_focus_hits=self.client_focus_hits,
                          owned_focus_hits=self.owned_focus_hits,
                          cursor_motions=len(self.cursor_samples),
                          foreground_transitions=len(self.foreground_timeline),
                          base_cursor=list(self.base_cursor) if self.base_cursor else None,
                          final_cursor=list(cursor_pos()) if cursor_pos() else None,
                          abort=self.abort_reason)

    def stop(self):
        self.stop_event.set()
        self.join(timeout=2.0)
