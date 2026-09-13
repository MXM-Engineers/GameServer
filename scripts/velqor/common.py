import glob
import json
import os
import sys
import threading
import time

SCHEMA = 1
CLIENT_EXE = "D:/Projets/MxM/MxM_12147/Client.Win32/MXMClient_DP_p3.exe"
CLIENT_ARGS = [
    "/LogEncryption",
    "/AuthMethod:local",
    "/Network:dev",
    "/PacketEncryption:0",
    "/AutoJoinGame",
    "/AutoLoginID:LordSk",
]

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BUILD = os.path.join(REPO, "build")
SCRIPTS = os.path.join(REPO, "scripts")
RUN_ROOT = os.path.join(BUILD, "velqor", "runs")
NATIVE_DIR = os.path.join(BUILD, "velqor", "native")
SCENARIO_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "scenarios")
DEFAULT_SCENARIO = os.path.join(SCENARIO_DIR, "owner_sniper_forward_then_a.json")
DEFAULT_INJECTOR = os.path.join(NATIVE_DIR, "velqor_inject.exe")
DEFAULT_DLL = os.path.join(NATIVE_DIR, "velqor.dll")

VK_FRONT = 0x5A
VK_QUICKSLOT_A = 0x41
VK_ULTIMATE_R = 0x52
SNIPER_NORMAL_A_SKILL_ID = 180060040
A_TAIL_MIN_MS = 4000

SERVERS = [
    ("mm", "mm_srv.exe", "matchmaker.log", 13900),
    ("hub", "hub_srv.exe", "hub_server.log", 11900),
    ("game", "game_srv.exe", "game_server.log", 12900),
    ("login", "login_srv.exe", "login_server.log", 10900),
]
SERVER_READY_RE = "Waiting for a connection"


def qpc_ns():
    return time.perf_counter_ns()


def wall_ms():
    return time.time_ns() // 1000000


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)
    return path


def new_run_dir(root, scenario_name):
    stamp = time.strftime("%Y%m%d_%H%M%S", time.localtime())
    base = "%s_%s" % (stamp, scenario_name)
    path = os.path.join(root, base)
    suffix = 1
    while os.path.exists(path):
        path = os.path.join(root, "%s_%d" % (base, suffix))
        suffix += 1
    ensure_dir(path)
    return path


class EventLog:
    def __init__(self, path, source, run_id, pid=None):
        self.path = path
        self.source = source
        self.run_id = run_id
        self.pid = os.getpid() if pid is None else pid
        self.lock = threading.Lock()
        self.file = open(path, "a", encoding="utf-8", newline="")
        self.count = 0

    def emit(self, event, **fields):
        record = {
            "schema": SCHEMA,
            "source": self.source,
            "run_id": self.run_id,
            "pid": self.pid,
            "qpc_ns": qpc_ns(),
            "event": event,
        }
        record.update(fields)
        line = json.dumps(record, separators=(",", ":"), sort_keys=False)
        with self.lock:
            self.file.write(line + "\n")
            self.file.flush()
            self.count += 1
        return record

    def close(self):
        with self.lock:
            try:
                self.file.flush()
                self.file.close()
            except Exception:
                pass


def read_ndjson(path):
    records = []
    if not os.path.exists(path):
        return records
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                records.append(json.loads(line))
            except ValueError:
                records.append({"event": "__malformed__", "raw": line})
    return records


def iter_ndjson(path):
    if not os.path.exists(path):
        return
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                yield json.loads(line)
            except ValueError:
                yield {"event": "__malformed__", "raw": line}


def iter_ndjson_all(pattern):
    for path in sorted(glob.glob(pattern)):
        for rec in iter_ndjson(path):
            yield rec


class NdjsonTail:
    def __init__(self, pattern):
        self.pattern = pattern
        self.offsets = {}
        self.pending = {}

    def poll(self):
        out = []
        for path in sorted(glob.glob(self.pattern)):
            offset = self.offsets.get(path, 0)
            pending = self.pending.get(path, b"")
            with open(path, "rb") as f:
                f.seek(offset)
                chunk = pending + f.read()
                self.offsets[path] = f.tell()
            end = chunk.rfind(b"\n")
            if end < 0:
                self.pending[path] = chunk
                continue
            self.pending[path] = chunk[end + 1:]
            for line in chunk[:end].split(b"\n"):
                line = line.strip()
                if not line:
                    continue
                try:
                    out.append(json.loads(line.decode("utf-8", "replace")))
                except ValueError:
                    out.append({"event": "__malformed__", "raw": line.decode("utf-8", "replace")})
        return out


def write_json(path, payload):
    ensure_dir(os.path.dirname(path))
    with open(path, "w", encoding="utf-8", newline="") as f:
        json.dump(payload, f, indent=2, sort_keys=False)
        f.write("\n")


def read_json(path, default=None):
    if not os.path.exists(path):
        return default
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def die(message, code=2):
    sys.stderr.write("VELQOR_ERROR: %s\n" % message)
    sys.stderr.flush()
    raise SystemExit(code)
