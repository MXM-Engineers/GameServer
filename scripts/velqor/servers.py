import ctypes
import socket
import os
import re
import subprocess
import time

from .common import BUILD, SERVER_READY_RE, SERVERS, ensure_dir

CFG_FILES = ["login.cfg", "hub.cfg", "game.cfg", "matchmaker.cfg"]


def _cfg_path(name):
    return os.path.join(BUILD, name)


def read_cfg_bytes(name):
    path = _cfg_path(name)
    if not os.path.exists(path):
        return None
    with open(path, "rb") as f:
        return f.read()


def read_cfg(name):
    raw = read_cfg_bytes(name)
    if raw is None:
        return None
    return raw.decode("utf-8", "surrogateescape")


def write_cfg(name, text):
    with open(_cfg_path(name), "wb") as f:
        f.write(text.encode("utf-8", "surrogateescape"))


def write_cfg_bytes(name, raw):
    with open(_cfg_path(name), "wb") as f:
        f.write(raw)


def apply_cfg_text(text, sets):
    lines = text.splitlines()
    applied = {}
    out = []
    seen = set()
    for line in lines:
        key = line.split("=", 1)[0].strip() if "=" in line else None
        if key is None or key not in sets:
            out.append(line)
            continue
        if key in seen:
            continue
        seen.add(key)
        value = sets[key]
        if value is None:
            applied[key] = None
            continue
        out.append("%s=%s" % (key, value))
        applied[key] = value
    for key, value in sets.items():
        if key not in seen:
            if value is not None:
                out.append("%s=%s" % (key, value))
            applied[key] = value
    return "\n".join(out) + "\n", applied, {}


class CfgFixture:
    def __init__(self, run_dir, log):
        self.run_dir = run_dir
        self.backup_dir = ensure_dir(os.path.join(run_dir, "cfg_backup"))
        self.log = log
        self.originals = {}
        self.applied = {}
        self.skipped = {}

    def capture(self, names=None):
        for name in names or CFG_FILES:
            raw = read_cfg_bytes(name)
            if raw is None:
                continue
            self.originals[name] = raw
            with open(os.path.join(self.backup_dir, name), "wb") as f:
                f.write(raw)
        self.log.emit("cfg_captured", files=sorted(self.originals), backup_dir=self.backup_dir)

    def apply(self, overrides):
        for name, sets in (overrides or {}).items():
            text = read_cfg(name)
            if text is None:
                raise FileNotFoundError(_cfg_path(name))
            new_text, applied, skipped = apply_cfg_text(text, sets)
            if applied and new_text != text:
                write_cfg(name, new_text)
            self.applied[name] = applied
            self.skipped[name] = skipped
        self.log.emit("cfg_applied", files=sorted(self.applied),
                      detail={k: dict(v) for k, v in self.applied.items()},
                      skipped={k: dict(v) for k, v in self.skipped.items()})

    def verify(self, expectations):
        result = {}
        for name, sets in (expectations or {}).items():
            text = read_cfg(name) or ""
            values = {}
            for line in text.splitlines():
                if "=" in line:
                    k, v = line.split("=", 1)
                    values[k.strip()] = v.strip()
            skipped = self.skipped.get(name, {})
            checked = sets
            ok = all(values.get(k) == str(v) for k, v in checked.items() if v is not None)
            absent_ok = all(k not in values for k, v in checked.items() if v is None)
            result[name] = {
                "ok": bool(ok and absent_ok),
                "values": {k: values.get(k) for k in sets},
                "checked": sorted(checked),
                "skipped": sorted(skipped),
            }
        return result

    def restore(self):
        restored = []
        for name, raw in self.originals.items():
            write_cfg_bytes(name, raw)
            restored.append(name)
        mismatches = []
        for name, raw in self.originals.items():
            if read_cfg_bytes(name) != raw:
                mismatches.append(name)
        self.log.emit("cfg_restored", files=restored, mismatches=mismatches)
        return {"restored": restored, "mismatches": mismatches}


def listen_inventory():
    get_table = ctypes.windll.iphlpapi.GetExtendedTcpTable
    get_table.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ulong),
                         ctypes.c_int, ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
    get_table.restype = ctypes.c_ulong
    size = ctypes.c_ulong()
    buffer = None
    while True:
        error = get_table(buffer, ctypes.byref(size), False, socket.AF_INET, 3, 0)
        if error == 122:
            buffer = ctypes.create_string_buffer(size.value)
            continue
        if error:
            raise ctypes.WinError(error)
        break
    found = {}
    count = ctypes.c_ulong.from_buffer(buffer).value
    for index in range(count):
        row = (ctypes.c_ulong * 6).from_buffer(buffer, 4 + index * 24)
        port = socket.ntohs(row[2] & 0xffff)
        found.setdefault(port, set()).add(row[5])
    return found


def listen_pids(port):
    return sorted(listen_inventory().get(port, set()))


class ServerProc:
    def __init__(self, name, exe, log_name, port, popen, log_offset, stdout_path):
        self.name = name
        self.exe = exe
        self.log_name = log_name
        self.port = port
        self.popen = popen
        self.log_offset = log_offset
        self.stdout_path = stdout_path
        self.ready = False
        self.ready_log = False


class ServerManager:
    def __init__(self, run_dir, log, env):
        self.run_dir = run_dir
        self.log = log
        self.env = env
        self.procs = []

    def _read_new(self, path, offset):
        if not os.path.exists(path):
            return "", offset
        size = os.path.getsize(path)
        if size < offset:
            offset = 0
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            f.seek(offset)
            data = f.read()
            return data, f.tell()

    def start(self, name, ready_timeout=60.0, extra_args=None, abort=None):
        spec = next((s for s in SERVERS if s[0] == name), None)
        if spec is None:
            raise RuntimeError("unknown server %s" % name)
        _, exe_name, log_name, port = spec
        exe = os.path.join(BUILD, exe_name)
        if not os.path.exists(exe):
            raise RuntimeError("server binary missing: %s" % exe)
        env = dict(self.env)
        missing = [key for key in ("VELQOR_RUN_DIR", "VELQOR_RUN_ID") if not env.get(key)]
        if missing:
            raise RuntimeError("server %s spawn env missing run identity %s" % (name, missing))
        holders = listen_pids(port)
        if holders:
            raise RuntimeError("server %s port %d already LISTENed by pid(s) %s before spawn"
                               % (name, port, holders))
        log_path = os.path.join(BUILD, log_name)
        offset = os.path.getsize(log_path) if os.path.exists(log_path) else 0
        stdout_path = os.path.join(self.run_dir, "server_%s_stdout.txt" % name)
        stdout = open(stdout_path, "wb")
        try:
            popen = subprocess.Popen([exe] + list(extra_args or []), cwd=BUILD, env=env,
                                     stdout=stdout, stderr=subprocess.STDOUT,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
        finally:
            stdout.close()
        proc = ServerProc(name, exe, log_name, port, popen, offset, stdout_path)
        self.procs.append(proc)
        self.log.emit("server_spawn", name=name, exe=exe, pid=popen.pid, port=port,
                      stdout=stdout_path, log=log_path)
        deadline = time.monotonic() + ready_timeout
        seen = ""
        while True:
            if abort is not None:
                reason = abort()
                if reason:
                    raise RuntimeError("server %s startup aborted: %s" % (name, reason))
            if popen.poll() is not None:
                raise RuntimeError("server %s exited early code=%s log_tail=%s" %
                                   (name, popen.returncode, seen[-400:]))
            data, offset = self._read_new(log_path, proc.log_offset)
            proc.log_offset = offset
            if data:
                seen += data
                if SERVER_READY_RE in seen:
                    proc.ready_log = True
            owners = listen_pids(port)
            if popen.pid in owners:
                proc.ready = True
                break
            if owners:
                raise RuntimeError("server %s port %d LISTENed by foreign pid(s) %s, not child %d"
                                   % (name, port, owners, popen.pid))
            if time.monotonic() >= deadline:
                raise RuntimeError(
                    "server %s not ready within %.0fs: port %d has no LISTEN owner (ready_log=%s)"
                    % (name, ready_timeout, port, proc.ready_log))
            time.sleep(0.2)
        self.log.emit("server_ready", name=name, pid=popen.pid, port=port,
                      listen_pids=[popen.pid], listen_confirmed=True,
                      ready_log_seen=proc.ready_log)
        return proc

    def stop(self, timeout_s=15.0):
        result = []
        for proc in self.procs:
            if proc.popen.poll() is not None:
                result.append({"name": proc.name, "pid": proc.popen.pid, "state": "exited",
                               "code": proc.popen.returncode})
                continue
            subprocess.run(["taskkill", "/PID", str(proc.popen.pid), "/T", "/F"],
                           capture_output=True)
            try:
                proc.popen.wait(timeout=timeout_s)
                state = "killed"
            except subprocess.TimeoutExpired:
                state = "still_running"
            result.append({"name": proc.name, "pid": proc.popen.pid, "state": state,
                           "code": proc.popen.returncode})
        self.log.emit("servers_stopped", result=result)
        return result
