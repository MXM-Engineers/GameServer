import json
import time

import ctypes
import ctypes.wintypes as wt

ERROR_PIPE_BUSY = 231
GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3
INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value

kernel32 = ctypes.windll.kernel32
kernel32.CreateFileW.restype = wt.HANDLE
kernel32.CreateFileW.argtypes = [
    wt.LPCWSTR, wt.DWORD, wt.DWORD, ctypes.c_void_p, wt.DWORD, wt.DWORD, wt.HANDLE,
]
kernel32.WaitNamedPipeW.restype = wt.BOOL
kernel32.WaitNamedPipeW.argtypes = [wt.LPCWSTR, wt.DWORD]
kernel32.ReadFile.restype = wt.BOOL
kernel32.WriteFile.restype = wt.BOOL
kernel32.PeekNamedPipe.restype = wt.BOOL


class PipeError(Exception):
    pass


def pipe_name(pid):
    return "\\\\.\\pipe\\velqor-%d" % pid


def _abort_requested(abort):
    if abort is None:
        return False
    is_set = getattr(abort, "is_set", None)
    if is_set is not None:
        return bool(is_set())
    return bool(abort())


class PipeClient:
    def __init__(self, pid, log=None):
        self.pid = pid
        self.name = pipe_name(pid)
        self.handle = None
        self.pending = b""
        self.log = log
        self.seq = 0

    def connect(self, timeout_s=30.0, abort=None):
        deadline = time.monotonic() + timeout_s
        last = None
        while True:
            if _abort_requested(abort):
                raise PipeError("aborted")
            if time.monotonic() >= deadline:
                break
            handle = kernel32.CreateFileW(
                self.name, GENERIC_READ | GENERIC_WRITE, 0, None, OPEN_EXISTING, 0, None
            )
            if handle and handle != INVALID_HANDLE_VALUE:
                self.handle = handle
                return True
            last = kernel32.GetLastError()
            if last == ERROR_PIPE_BUSY:
                kernel32.WaitNamedPipeW(self.name, 100)
            time.sleep(0.02)
        raise PipeError("connect failed for %s (last error %s)" % (self.name, last))

    def close(self):
        if self.handle:
            try:
                kernel32.CloseHandle(self.handle)
            except Exception:
                pass
            self.handle = None

    def _write(self, data):
        written = wt.DWORD(0)
        ok = kernel32.WriteFile(self.handle, data, len(data), ctypes.byref(written), None)
        if not ok:
            raise PipeError("WriteFile failed err=%d" % kernel32.GetLastError())
        return written.value

    def _read_line(self, timeout_s):
        deadline = time.monotonic() + timeout_s
        while True:
            idx = self.pending.find(b"\n")
            if idx >= 0:
                line, self.pending = self.pending[:idx], self.pending[idx + 1:]
                return line.decode("utf-8", "replace")
            if time.monotonic() > deadline:
                raise PipeError("timeout waiting for ack")
            avail = wt.DWORD(0)
            ok = kernel32.PeekNamedPipe(self.handle, None, 0, None, ctypes.byref(avail), None)
            if not ok:
                raise PipeError("PeekNamedPipe failed err=%d" % kernel32.GetLastError())
            if avail.value == 0:
                time.sleep(0.002)
                continue
            buf = ctypes.create_string_buffer(65536)
            read = wt.DWORD(0)
            ok = kernel32.ReadFile(self.handle, buf, 65536, ctypes.byref(read), None)
            if not ok:
                raise PipeError("ReadFile failed err=%d" % kernel32.GetLastError())
            if read.value == 0:
                raise PipeError("pipe closed")
            self.pending += buf.raw[: read.value]

    def request(self, op, args=(), ack_timeout_s=5.0):
        self.seq += 1
        line = " ".join([op, str(self.seq)] + [str(a) for a in args])
        self._write(("%s\n" % line).encode("ascii"))
        raw = self._read_line(ack_timeout_s)
        try:
            ack = json.loads(raw)
        except ValueError:
            raise PipeError("malformed ack for %s: %r" % (op, raw))
        if not isinstance(ack, dict) or "status" not in ack:
            raise PipeError("ack missing status for %s: %r" % (op, raw))
        try:
            ack_seq = int(ack["seq"])
        except (KeyError, TypeError, ValueError):
            raise PipeError("ack missing seq for %s: %r" % (op, raw))
        if ack_seq != self.seq:
            raise PipeError("ack seq mismatch for %s: sent %d got %s" % (op, self.seq, ack_seq))
        if self.log is not None:
            self.log(line, ack)
        return ack

    def status(self, timeout_s=5.0):
        return self.request("STATUS", (), timeout_s)

    def begin(self, timeout_s=5.0):
        return self.request("BEGIN", (), timeout_s)

    def key(self, at_ms, vk, down, timeout_s=5.0):
        return self.request("KEY", (at_ms, vk, 1 if down else 0), timeout_s)

    def cursor(self, at_ms, x, y, timeout_s=5.0):
        return self.request("CURSOR", (at_ms, x, y), timeout_s)

    def mark(self, at_ms, mark_id, timeout_s=5.0):
        return self.request("MARK", (at_ms, mark_id), timeout_s)

    def play(self, timeout_s=10.0):
        return self.request("PLAY", (), timeout_s)

    def stop(self, timeout_s=10.0):
        return self.request("STOP", (), timeout_s)

    def wait_ready(self, timeout_s, poll_s=0.2, predicate=None, abort=None):
        deadline = time.monotonic() + timeout_s
        last = None
        while True:
            if _abort_requested(abort):
                raise PipeError("aborted")
            if time.monotonic() >= deadline:
                return last
            last = self.status()
            if predicate is None:
                if str(last.get("ready")).lower() in ("1", "true") or last.get("ready") is True:
                    return last
            elif predicate(last):
                return last
            time.sleep(poll_s)
