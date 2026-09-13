import argparse
import glob
import json
import os
import re
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from velqor import compare, elevate, monitor, pipeclient, servers, winproc
from velqor.common import (A_TAIL_MIN_MS, BUILD, CLIENT_ARGS, CLIENT_EXE, DEFAULT_DLL,
                           DEFAULT_INJECTOR, DEFAULT_SCENARIO, EventLog, NdjsonTail, RUN_ROOT,
                           SNIPER_NORMAL_A_SKILL_ID, VK_FRONT, VK_QUICKSLOT_A, VK_ULTIMATE_R,
                           ensure_dir, iter_ndjson, iter_ndjson_all, qpc_ns, read_json, write_json)

INJECT_RE = re.compile(r"VELQOR_INJECT\s+pid=(\d+)\s+created=(\d+)")
INJECT_ERROR_RE = re.compile(r"VELQOR_ERROR\s+(\S+)")
INJECTOR_TEMPLATE_DEFAULT = [
    "--exe", "{exe}", "--dll", "{dll}", "--cwd", "{cwd}", "--", "{client_args}",
]


class AbortRun(Exception):
    pass


def expand_template(template, mapping):
    argv = []
    for item in template:
        if item == "{client_args}":
            argv.extend(mapping["client_args"])
        else:
            argv.append(item.format(**mapping))
    return argv


class RunContext:
    def __init__(self, run_dir, run_id, scenario, scenario_path, log, env, args):
        self.run_dir = run_dir
        self.run_id = run_id
        self.scenario = scenario
        self.scenario_path = scenario_path
        self.log = log
        self.env = env
        self.args = args
        self.client_pid = None
        self.client_info = None
        self.injector_pid = None
        self.injector_handle = None
        self.injector_proc = None
        self.injector_mode = None
        self.injector_stdout = None
        self.injector_created = None
        self.guard = None
        self.pipe = None
        self.mon = None
        self.server_mgr = None
        self.fixture = None
        self.owned = {}
        self.hub_pids = []
        self.epoch_ns = None
        self.epoch_source = None
        self.viewport_w = None
        self.viewport_h = None
        self.stopped = False
        self.steps = []
        self.aborted = None
        self.trial_results = []
        self.exec_gaps = []
        self.stop_release = {"verified": "not_attempted"}
        self.shutdown = {}

    def meta(self):
        return {
            "run_id": self.run_id,
            "run_dir": self.run_dir,
            "scenario": self.scenario.get("name"),
            "scenario_path": os.path.abspath(self.scenario_path),
            "started_ns": self.started_ns,
            "started_wall_ns": self.started_wall_ns,
            "client_pid": self.client_pid,
            "client_creation_ns": (self.client_info or {}).get("creation_ns"),
            "client_exe": (self.client_info or {}).get("exe"),
            "injector_pid": self.injector_pid,
            "injector_mode": self.injector_mode,
            "injector_created": self.injector_created,
            "guard": self.guard.snapshot() if self.guard else None,
            "server_pids": {p.name: p.popen.pid for p in (self.server_mgr.procs if self.server_mgr else [])},
            "owned_pids": {str(pid): created for pid, created in owned_identities(self)},
            "aborted": self.aborted,
            "epoch_ns": self.epoch_ns,
            "epoch_source": self.epoch_source,
            "viewport": {"w": self.viewport_w, "h": self.viewport_h},
            "exec_gaps": self.exec_gaps,
            "steps": self.steps,
        }

    def persist_meta(self):
        write_json(os.path.join(self.run_dir, "run_meta.json"), self.meta())


def progress(ctx, stage, **fields):
    line = "VELQOR stage=%s %s" % (stage, json.dumps(fields, sort_keys=True, default=str))
    print(line, flush=True)
    try:
        ctx.log.emit("controller_stage", stage=stage, **fields)
    except Exception:
        pass


def load_scenario(path):
    with open(path, "r", encoding="utf-8") as f:
        scenario = json.load(f)
    if scenario.get("schema") != 1:
        raise RuntimeError("unsupported scenario schema")
    if not scenario.get("trials") and not scenario.get("steps"):
        raise RuntimeError("scenario has no steps")
    return scenario


def abort_reason(ctx):
    for watcher in (ctx.mon, ctx.guard):
        if watcher is not None and watcher.abort_event.is_set():
            return watcher.abort_reason or "monitor_abort"
    return None


def check_abort(ctx):
    reason = abort_reason(ctx)
    if reason:
        raise AbortRun(reason)


def hub_server_pids(ctx):
    raw = (ctx.env or {}).get("VELQOR_HUB_SERVER_PIDS") or ""
    pids = []
    for token in raw.split(","):
        token = token.strip()
        if token.isdigit() and int(token) > 0:
            pid = int(token)
            pids.append((pid, monitor.process_creation_ns(pid)))
    return pids


def note_owned(ctx, pid):
    if not pid:
        return None
    pid = int(pid)
    if pid not in ctx.owned:
        ctx.owned[pid] = monitor.process_creation_ns(pid)
    return ctx.owned[pid]


def owned_identities(ctx):
    pairs = []
    seen = set()
    pids = [proc.popen.pid for proc in (ctx.server_mgr.procs if ctx.server_mgr else [])]
    pids.extend([ctx.injector_pid, ctx.client_pid])
    pids.extend([pid for pid, _ in ctx.hub_pids])
    for pid in pids:
        if not pid:
            continue
        pid = int(pid)
        if pid in seen:
            continue
        seen.add(pid)
        pairs.append((pid, ctx.owned.get(pid)))
    return pairs


def owned_survivors(ctx):
    names = {}
    if ctx.server_mgr is not None:
        for proc in ctx.server_mgr.procs:
            names[int(proc.popen.pid)] = proc.name
    if ctx.injector_pid:
        names.setdefault(int(ctx.injector_pid), "injector")
    if ctx.client_pid:
        names.setdefault(int(ctx.client_pid), "client")
    survivors = []
    for pid, created in sorted(ctx.owned.items()):
        if not created:
            continue
        if monitor.process_creation_ns(pid) == created:
            survivors.append({"pid": pid, "name": names.get(pid), "creation_ns": created})
    return survivors


def wait_owned_exit(ctx, pid, timeout_s=10.0):
    if not pid:
        return None
    pid = int(pid)
    created = ctx.owned.get(pid) or monitor.process_creation_ns(pid)
    if not created:
        return None
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        if monitor.process_creation_ns(pid) != created:
            return True
        time.sleep(0.1)
    return False


def cleanup_clean(result):
    if result.get("survivors") or result.get("leftover_clients"):
        return False
    if result.get("client_gone") is False or result.get("injector_gone") is False:
        return False
    for key in ("cfg_error", "servers_error", "client_kill_error", "injector_kill_error",
                "pipe_error", "monitor_error", "guard_error"):
        if result.get(key):
            return False
    if (result.get("cfg") or {}).get("mismatches"):
        return False
    for entry in (result.get("servers") or []):
        if entry.get("state") == "still_running":
            return False
    return True


def pipe_call(ctx, fn, *args, **kwargs):
    try:
        return fn(*args, **kwargs)
    except pipeclient.PipeError:
        check_abort(ctx)
        raise


def wait_until(ctx, target_qpc_ns, label):
    while True:
        check_abort(ctx)
        now = qpc_ns()
        if now >= target_qpc_ns:
            return
        time.sleep(min(0.05, max(0.005, (target_qpc_ns - now) / 1e9)))


def set_viewport(ctx, status):
    if not isinstance(status, dict):
        raise RuntimeError("STATUS unavailable; cannot resolve normalized cursor")
    try:
        w = int(status.get("viewport_w"))
        h = int(status.get("viewport_h"))
    except (TypeError, ValueError):
        raise RuntimeError("STATUS viewport missing: %s" % json.dumps(status))
    if w <= 0 or h <= 0:
        raise RuntimeError("STATUS viewport invalid: %s" % json.dumps(status))
    ctx.viewport_w = w
    ctx.viewport_h = h
    ctx.persist_meta()


def normalize_px(norm, size):
    if size <= 0:
        raise RuntimeError("STATUS viewport unavailable for normalized cursor")
    return max(0, min(size - 1, int(round(float(norm) * size))))


def cursor_px(ctx, step):
    if "norm_x" not in step or "norm_y" not in step:
        raise RuntimeError("cursor step requires norm_x/norm_y")
    if not ctx.viewport_w or not ctx.viewport_h:
        raise RuntimeError("STATUS viewport not captured before cursor step")
    return (normalize_px(step["norm_x"], ctx.viewport_w),
            normalize_px(step["norm_y"], ctx.viewport_h))


def schedule_steps(ctx, steps, base_ms, trial_name, epoch_ns=None):
    epoch = ctx.epoch_ns if epoch_ns is None else epoch_ns
    records = []
    for step in steps:
        check_abort(ctx)
        at_ms = base_ms + int(step["at_ms"])
        op = step["op"]
        record = dict(step)
        if op == "key":
            vk = int(step["vk"])
            if vk == VK_ULTIMATE_R:
                raise RuntimeError("ultimate R is prohibited; A must be driven or inspected")
            ack = ctx.pipe.key(at_ms, vk, int(step["down"]))
        elif op == "cursor":
            x, y = cursor_px(ctx, step)
            record["x"] = x
            record["y"] = y
            ctx.mon.note_aim()
            ack = ctx.pipe.cursor(at_ms, x, y)
        elif op == "mark":
            ack = ctx.pipe.mark(at_ms, str(step["id"]))
        else:
            raise RuntimeError("unknown step op %s" % op)
        record["trial"] = trial_name
        record["at_ms"] = at_ms
        record["scheduled_ns"] = epoch + at_ms * 1_000_000
        record["ack"] = ack
        record["ack_status"] = ack.get("status") if isinstance(ack, dict) else None
        if record["ack_status"] not in ("queued", "ok"):
            raise RuntimeError("pipe rejected %s: %s" % (op, json.dumps(ack)))
        raw_seq = ack.get("cmd_seq") if isinstance(ack, dict) else None
        if raw_seq is None:
            raise RuntimeError("pipe ack missing cmd_seq for %s: %s" % (op, json.dumps(ack)))
        try:
            record["cmd_seq"] = int(raw_seq)
        except (TypeError, ValueError):
            raise RuntimeError("pipe ack cmd_seq malformed for %s: %s" % (op, json.dumps(ack)))
        ctx.steps.append(record)
        records.append(record)
        ctx.log.emit("step_scheduled", **{k: record.get(k) for k in
                                          ("op", "label", "id", "vk", "down", "x", "y", "at_ms",
                                           "trial", "ack_status", "cmd_seq")},
                     scheduled_ns=record["scheduled_ns"])
    ctx.persist_meta()
    return records


def write_inputs(ctx):
    write_json(os.path.join(ctx.run_dir, "inputs.json"), {
        "run_id": ctx.run_id,
        "scenario": ctx.scenario.get("name"),
        "epoch_ns": ctx.epoch_ns,
        "epoch_source": ctx.epoch_source,
        "steps": ctx.steps,
        "probe": ctx.scenario.get("probe", {}),
        "disconnect_release_check": ctx.scenario.get("disconnect_release_check", {}),
    })


def wait_for_cast(ctx, skill_id, since_ns, timeout_s):
    tail = NdjsonTail(os.path.join(ctx.run_dir, "server_*.ndjson"))
    deadline = time.monotonic() + timeout_s
    while True:
        check_abort(ctx)
        for rec in tail.poll():
            if rec.get("event") == "cast_accept" and rec.get("skill_id") == skill_id \
                    and (rec.get("qpc_ns") or 0) >= since_ns:
                return rec
        if time.monotonic() >= deadline:
            return None
        time.sleep(0.1)


def wait_cmd_exec(ctx, cmd_seqs, timeout_s):
    wanted = {seq for seq in cmd_seqs if seq is not None}
    found = {}
    if not wanted:
        return found
    tail = NdjsonTail(os.path.join(ctx.run_dir, "client_*.ndjson"))
    deadline = time.monotonic() + timeout_s
    while True:
        check_abort(ctx)
        for rec in tail.poll():
            if rec.get("event") != "cmd_exec":
                continue
            seq = rec.get("cmd_seq")
            if seq in wanted and seq not in found:
                found[seq] = rec
        if len(found) >= len(wanted) or time.monotonic() >= deadline:
            return found
        time.sleep(0.05)


def legality_snapshot(ctx):
    out = {"client_not_ready": [], "client_errors": [], "server_skill_cancel": [],
           "server_skill_done": []}
    for rec in client_records(ctx.run_dir):
        if rec.get("event") == "not_ready":
            out["client_not_ready"].append(rec.get("reason"))
        elif rec.get("event") == "error":
            out["client_errors"].append(rec.get("message"))
    for rec in iter_ndjson_all(os.path.join(ctx.run_dir, "server_*.ndjson")):
        if rec.get("event") == "skill_cancel":
            out["server_skill_cancel"].append({k: rec.get(k) for k in
                                              ("skill_id", "reason", "player_index", "master_slot")})
        elif rec.get("event") == "skill_done":
            out["server_skill_done"].append({k: rec.get(k) for k in
                                             ("skill_id", "reason", "player_index", "master_slot")})
    return out


def client_records(run_dir):
    for path in sorted(glob.glob(os.path.join(run_dir, "client_*.ndjson"))):
        for rec in iter_ndjson(path):
            yield rec


def verify_stop_release(ctx, stop_ns):
    result = {"verified": False, "client": None, "server": None, "server_stopped": None,
              "released": None}
    for rec in client_records(ctx.run_dir):
        if (rec.get("qpc_ns") or 0) < stop_ns:
            continue
        if rec.get("event") == "stop_release" and isinstance(rec.get("released"), list):
            result["released"] = rec["released"]
            if VK_FRONT in rec["released"]:
                result["client"] = {"event": "stop_release", "released": rec["released"],
                                    "qpc_ns": rec.get("qpc_ns"), "frame": rec.get("frame")}
        if rec.get("event") == "cmd_exec" and rec.get("op") == "key" \
                and rec.get("vk") == VK_FRONT and rec.get("down") == 0:
            result["client"] = {"event": "cmd_exec", "op": "key", "vk": VK_FRONT, "down": 0,
                                "qpc_ns": rec.get("qpc_ns"), "frame": rec.get("frame"),
                                "cmd_seq": rec.get("cmd_seq")}
    posts = [r for r in iter_ndjson_all(os.path.join(ctx.run_dir, "server_*.ndjson"))
             if r.get("event") == "post_sim" and (r.get("qpc_ns") or 0) >= stop_ns]
    posts.sort(key=lambda r: r.get("qpc_ns") or 0)
    if posts:
        window = posts[0]["qpc_ns"] + 2_000_000_000
        in_window = [p for p in posts if (p.get("qpc_ns") or 0) <= window]
        speeds = [p.get("move_speed") for p in in_window if p.get("move_speed") is not None]
        if speeds:
            result["server"] = {"samples": len(in_window), "max_move_speed_after_stop": max(speeds),
                                "final_move_speed": speeds[-1], "first_qpc_ns": in_window[0]["qpc_ns"]}
            result["server_stopped"] = max(speeds) <= 0.5
    result["verified"] = bool(result["client"]) or bool(result["server_stopped"])
    return result


def launch_injector(ctx):
    sc = ctx.scenario["client"]
    injector = sc.get("injector") or DEFAULT_INJECTOR
    dll = sc.get("dll") or DEFAULT_DLL
    client_exe = sc.get("exe") or CLIENT_EXE
    cwd = sc.get("cwd") or os.path.dirname(client_exe)
    client_args = list(sc.get("args") or CLIENT_ARGS)
    if not os.path.exists(injector):
        raise RuntimeError("injector binary missing: %s" % injector)
    if not os.path.exists(client_exe):
        raise RuntimeError("client exe missing: %s" % client_exe)
    mapping = {
        "run_dir": ctx.run_dir,
        "run_id": ctx.run_id,
        "dll": dll,
        "exe": client_exe,
        "cwd": cwd,
        "client_args": client_args,
    }
    template = sc.get("injector_args") or INJECTOR_TEMPLATE_DEFAULT
    argv = expand_template(template, mapping)
    stdout_path = os.path.join(ctx.run_dir, "injector_stdout.txt")
    ctx.injector_stdout = stdout_path
    for stale in (os.path.join(ctx.run_dir, "injector.json"), stdout_path):
        if os.path.exists(stale):
            os.remove(stale)
    mode = ctx.args.injector_mode
    if mode == "auto":
        mode = "direct" if elevate.is_admin() else "elevated"
    ctx.injector_mode = mode
    if mode == "elevated":
        if " " in ctx.run_dir:
            raise RuntimeError("elevated injector launch requires a space-free run_dir: %s" % ctx.run_dir)
        batch_path = os.path.join(ctx.run_dir, "launch_inject.cmd")
        for name, value in (("VELQOR_RUN_DIR", ctx.run_dir), ("VELQOR_RUN_ID", ctx.run_id)):
            if not value or any(ch.isspace() for ch in value):
                raise RuntimeError("elevated injector launch requires a whitespace-free %s: %r"
                                   % (name, value))
        batch_lines = [
            "@echo off",
            "set VELQOR_RUN_DIR=%s" % ctx.run_dir,
            "set VELQOR_RUN_ID=%s" % ctx.run_id,
            "\"%s\" %s > \"%s\" 2>&1" % (injector, subprocess.list2cmdline(argv), stdout_path),
        ]
        with open(batch_path, "w", encoding="utf-8", newline="") as f:
            f.write("".join(line + "\r\n" for line in batch_lines))
        cmd_exe = os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), "System32", "cmd.exe")
        pid, handle, err = elevate.runas(cmd_exe, ["/c " + batch_path], cwd=os.path.dirname(injector))
        if err:
            raise RuntimeError(err)
        ctx.injector_pid = pid
        ctx.injector_handle = handle
        ctx.log.emit("injector_launch", mode=mode, pid=pid, argv=[injector] + argv,
                     stdout=stdout_path, cmd=batch_path,
                     env={"VELQOR_RUN_DIR": ctx.run_dir, "VELQOR_RUN_ID": ctx.run_id})
    else:
        stream = open(stdout_path, "wb")
        try:
            ctx.injector_proc = subprocess.Popen([injector] + argv, cwd=os.path.dirname(injector),
                                                 env=ctx.env, stdout=stream,
                                                 stderr=subprocess.STDOUT,
                                                 creationflags=subprocess.CREATE_NO_WINDOW)
        finally:
            stream.close()
        ctx.injector_pid = ctx.injector_proc.pid
        ctx.log.emit("injector_launch", mode=mode, pid=ctx.injector_proc.pid,
                     argv=[injector] + argv, stdout=stdout_path)
    ctx.persist_meta()


def wait_client_pid(ctx, timeout_s=120.0):
    stdout_path = ctx.injector_stdout
    deadline = time.monotonic() + timeout_s
    tail = ""
    while time.monotonic() < deadline:
        check_abort(ctx)
        if stdout_path and os.path.exists(stdout_path):
            with open(stdout_path, "r", encoding="utf-8", errors="replace") as f:
                tail = f.read()
            match = INJECT_RE.search(tail)
            if match:
                ctx.client_pid = int(match.group(1))
                ctx.injector_created = int(match.group(2))
                break
            error = INJECT_ERROR_RE.search(tail)
            if error:
                raise RuntimeError("injector error %s; stdout_tail=%s" % (error.group(1), tail[-500:]))
        injector_json = read_json(os.path.join(ctx.run_dir, "injector.json"))
        if isinstance(injector_json, dict) and injector_json.get("pid"):
            ctx.client_pid = int(injector_json["pid"])
            ctx.injector_created = int(injector_json.get("created") or 0) or None
            break
        if ctx.injector_proc is not None and ctx.injector_proc.poll() is not None:
            raise RuntimeError("injector exited code=%s without ready line; stdout_tail=%s" %
                               (ctx.injector_proc.returncode, tail[-500:]))
        time.sleep(0.1)
    if ctx.client_pid is None:
        raise RuntimeError("injector ready line not seen within %.0fs; stdout_tail=%s" %
                           (timeout_s, tail[-500:]))
    ctx.client_info = winproc.process_info(ctx.client_pid)
    ctx.log.emit("client_process", pid=ctx.client_pid, injector_created=ctx.injector_created,
                 info=ctx.client_info)
    ctx.persist_meta()
    return ctx.client_pid


def status_ready(status):
    if not isinstance(status, dict):
        return False
    return status.get("ready") is True or str(status.get("ready")).lower() in ("1", "true")


def status_frontend(status):
    if not isinstance(status, dict):
        return False
    return str(status.get("phase") or "").strip().lower() == "frontend"


def wait_for_ready(ctx):
    check_abort(ctx)
    sc = ctx.scenario["client"]
    timeout_s = float(sc.get("ready_timeout_s", 180))
    deadline = time.monotonic() + timeout_s

    def remaining():
        return max(1.0, deadline - time.monotonic())

    def ready_or_frontend(status):
        return status_ready(status) or status_frontend(status)

    status = pipe_call(ctx, ctx.pipe.wait_ready, remaining(), predicate=ready_or_frontend,
                       abort=lambda: abort_reason(ctx))
    ctx.log.emit("pipe_status", stage="initial", status=status)
    if not status_ready(status) and status_frontend(status) and sc.get("play_if_not_ready", True):
        ack = ctx.pipe.play()
        ctx.log.emit("pipe_play", ack=ack)
        if isinstance(ack, dict) and ack.get("status") == "error":
            raise RuntimeError("PLAY rejected in frontend: %s" % json.dumps(ack))
        status = pipe_call(ctx, ctx.pipe.wait_ready, remaining(), predicate=status_ready,
                           abort=lambda: abort_reason(ctx))
        ctx.log.emit("pipe_status", stage="after_play", status=status)
    if not status_ready(status):
        raise RuntimeError("bridge not ready (STATUS.ready false): %s" % json.dumps(status))
    set_viewport(ctx, status)
    return status


def client_trace_path(status):
    path = status.get("tracepath") if isinstance(status, dict) else None
    if not isinstance(path, str) or not path.strip():
        raise RuntimeError("STATUS.tracepath absent; no client trace to verify: %s"
                           % json.dumps(status))
    path = path.strip()
    if not os.path.isfile(path):
        raise RuntimeError("STATUS.tracepath is not an existing file: %s" % path)
    return path


def wait_trace_event(ctx, pattern, event, timeout_s):
    tail = NdjsonTail(pattern)
    deadline = time.monotonic() + timeout_s
    observed = {}
    foreign = set()
    while True:
        check_abort(ctx)
        for rec in tail.poll():
            name = rec.get("event")
            if isinstance(name, str):
                observed[name] = observed.get(name, 0) + 1
            if name != event:
                continue
            if rec.get("run_id") != ctx.run_id:
                foreign.add(str(rec.get("run_id")))
                continue
            return rec
        if time.monotonic() >= deadline:
            counts = ", ".join("%s=%d" % (k, observed[k]) for k in sorted(observed))
            raise RuntimeError(
                "trace evidence missing: event %s not in %s within %.1fs "
                "(run_id=%s observed=[%s] foreign_run_ids=%s)"
                % (event, pattern, timeout_s, ctx.run_id, counts, sorted(foreign)))
        time.sleep(0.1)


def wait_server_playable(ctx, pattern, timeout_s):
    tail = NdjsonTail(pattern)
    deadline = time.monotonic() + timeout_s
    observed = {}
    foreign = set()
    owner = None
    posts = {}
    while True:
        check_abort(ctx)
        for rec in tail.poll():
            name = rec.get("event")
            if isinstance(name, str):
                observed[name] = observed.get(name, 0) + 1
            if rec.get("run_id") != ctx.run_id:
                if name in ("server_start", "player_join", "post_sim"):
                    foreign.add(str(rec.get("run_id")))
                continue
            if name == "player_join":
                if owner is None and not rec.get("is_bot"):
                    owner = rec
            elif name == "post_sim":
                game_id = rec.get("game_id")
                player_index = rec.get("player_index")
                if isinstance(game_id, int) and isinstance(player_index, int):
                    key = (game_id, player_index)
                    if key not in posts:
                        posts[key] = rec
        sim = None
        if owner is not None:
            game_id = owner.get("game_id")
            player_index = owner.get("player_index")
            if isinstance(game_id, int) and isinstance(player_index, int):
                sim = posts.get((game_id, player_index))
        if owner is not None and sim is not None:
            return owner, sim
        if time.monotonic() >= deadline:
            missing = []
            if owner is None:
                missing.append("player_join")
            if sim is None:
                missing.append("post_sim")
            counts = ", ".join("%s=%d" % (k, observed[k]) for k in sorted(observed))
            raise RuntimeError(
                "trace evidence missing: server events %s not in %s within %.1fs "
                "(run_id=%s observed=[%s] foreign_run_ids=%s)"
                % (", ".join(missing), pattern, timeout_s, ctx.run_id, counts, sorted(foreign)))
        time.sleep(0.1)


def verify_pre_begin_trace(ctx, status):
    timeout_s = float(ctx.scenario["client"].get("ready_timeout_s", 180))
    if not isinstance(ctx.run_id, str) or not ctx.run_id or ctx.run_id != ctx.run_id.strip():
        raise RuntimeError("run_id must be a nonempty whitespace-free string: %r" % (ctx.run_id,))
    tracepath = client_trace_path(status)
    server_glob = os.path.join(ctx.run_dir, "server_*.ndjson")
    if not glob.glob(server_glob):
        raise RuntimeError("no server trace matches %s; aborting before BEGIN" % server_glob)
    hello = wait_trace_event(ctx, tracepath, "hello", timeout_s)
    ctx.log.emit("trace_evidence", source="client", evidence_event="hello", path=tracepath,
                 run_id=ctx.run_id, record_qpc_ns=hello.get("qpc_ns"))
    sample = wait_trace_event(ctx, tracepath, "sample", timeout_s)
    ctx.log.emit("trace_evidence", source="client", evidence_event="sample", path=tracepath,
                 run_id=ctx.run_id, record_qpc_ns=sample.get("qpc_ns"),
                 frame=sample.get("frame"), phase=sample.get("phase"))
    player_join, post_sim = wait_server_playable(ctx, server_glob, timeout_s)
    ctx.log.emit("trace_evidence", source="server", evidence_event="player_join", path=server_glob,
                 run_id=ctx.run_id, record_qpc_ns=player_join.get("qpc_ns"),
                 player_index=player_join.get("player_index"), game_id=player_join.get("game_id"))
    ctx.log.emit("trace_evidence", source="server", evidence_event="post_sim", path=server_glob,
                 run_id=ctx.run_id, record_qpc_ns=post_sim.get("qpc_ns"),
                 player_index=post_sim.get("player_index"), game_id=post_sim.get("game_id"))
    progress(ctx, "trace_evidence", tracepath=tracepath, server_glob=server_glob,
             sample_frame=sample.get("frame"), owner_player_index=player_join.get("player_index"))


def execute(ctx):
    sc = ctx.scenario
    ctx.fixture = servers.CfgFixture(ctx.run_dir, ctx.log)
    ctx.fixture.capture()
    ctx.fixture.apply(sc["server"]["cfg"])
    verify = ctx.fixture.verify(sc["server"]["cfg"])
    ctx.log.emit("cfg_verify", result=verify)
    bad = [name for name, res in verify.items() if not res["ok"]]
    if bad:
        raise RuntimeError("cfg fixture not applied: %s" % bad)
    progress(ctx, "cfg_ready", files=sorted(verify))

    preexisting = winproc.list_named(os.path.basename(sc["client"].get("exe") or CLIENT_EXE))
    ctx.log.emit("preexisting_clients", pids=preexisting)
    if preexisting:
        raise RuntimeError("client already running before injector launch: %s" % preexisting)

    ctx.hub_pids = hub_server_pids(ctx)
    for pid, created in ctx.hub_pids:
        if pid and created:
            ctx.owned.setdefault(pid, created)
    ctx.guard = monitor.GuardWatcher(ctx.log, ctx.run_dir, lambda: ctx.client_pid)
    ctx.guard.start()
    ctx.mon = monitor.DesktopMonitor(
        ctx.log, lambda: ctx.client_pid, owned_pid_getter=lambda: owned_identities(ctx),
        cursor_tolerance=sc.get("focus_policy", {}).get("cursor_tolerance", 1))
    ctx.mon.start()
    ctx.mon.started.wait(2.0)
    check_abort(ctx)
    progress(ctx, "monitors_started", owned_pids=[pid for pid, _ in owned_identities(ctx)])

    ctx.server_mgr = servers.ServerManager(ctx.run_dir, ctx.log, ctx.env)
    ready_timeout_s = sc["server"].get("ready_timeout_s", 90)
    for name in sc["server"]["order"]:
        check_abort(ctx)
        try:
            proc = ctx.server_mgr.start(name, ready_timeout_s, abort=lambda: abort_reason(ctx))
        except RuntimeError:
            check_abort(ctx)
            raise
        note_owned(ctx, proc.popen.pid)
    ctx.persist_meta()
    progress(ctx, "servers_ready", pids={p.name: p.popen.pid for p in ctx.server_mgr.procs})
    check_abort(ctx)

    launch_injector(ctx)
    note_owned(ctx, ctx.injector_pid)
    progress(ctx, "injector_launched", pid=ctx.injector_pid, mode=ctx.injector_mode)
    wait_client_pid(ctx)
    note_owned(ctx, ctx.client_pid)
    progress(ctx, "client_started", pid=ctx.client_pid,
             created=(ctx.client_info or {}).get("creation_ns"),
             exe=(ctx.client_info or {}).get("exe"))
    ctx.pipe = pipeclient.PipeClient(ctx.client_pid)
    pipe_call(ctx, ctx.pipe.connect, timeout_s=60, abort=lambda: abort_reason(ctx))
    ctx.log.emit("pipe_connected", client_pid=ctx.client_pid, pipe=ctx.pipe.name)
    progress(ctx, "pipe_connected", pipe=ctx.pipe.name)

    status = wait_for_ready(ctx)
    progress(ctx, "bridge_ready", phase=status.get("phase"), ready=status.get("ready"),
             unfocused=status.get("unfocused"), tracepath=status.get("tracepath"))
    verify_pre_begin_trace(ctx, status)

    pre_ms = int(sc.get("timing", {}).get("pre_begin_settle_ms", 1200))
    time.sleep(pre_ms / 1000.0)
    check_abort(ctx)
    ack = ctx.pipe.begin()
    ctx.log.emit("pipe_begin", ack=ack)
    if isinstance(ack, dict) and ack.get("epoch_ns"):
        ctx.epoch_ns = int(ack["epoch_ns"])
        ctx.epoch_source = "ack"
    elif isinstance(ack, dict) and ack.get("epoch_qpc_ns"):
        ctx.epoch_ns = int(ack["epoch_qpc_ns"])
        ctx.epoch_source = "ack"
    else:
        ctx.epoch_ns = qpc_ns()
        ctx.epoch_source = "controller_after_ack"
    ctx.persist_meta()
    progress(ctx, "begin", epoch_ns=ctx.epoch_ns, epoch_source=ctx.epoch_source, ack=ack)

    fresh = ctx.pipe.status()
    ctx.log.emit("pipe_status", stage="after_begin", status=fresh)
    if not status_ready(fresh):
        raise RuntimeError("bridge not ready after BEGIN: %s" % json.dumps(fresh))
    set_viewport(ctx, fresh)

    base_ms = 0
    proven_skills = set()
    timing = sc.get("timing", {})
    gap_ms = int(timing.get("trial_gap_ms", 800))
    exec_timeout_s = float(timing.get("exec_timeout_ms", 8000)) / 1000.0
    for trial in sc.get("trials", []):
        name = trial["name"]
        if trial.get("only_if_no_cast") and proven_skills:
            ctx.trial_results.append({"name": name, "skipped": "cast_already_proven"})
            ctx.log.emit("trial_skipped", trial=name, reason="cast_already_proven")
            continue
        steps = trial["steps"]
        trial_base_ms = base_ms
        ctx.log.emit("trial_start", trial=name, base_ms=trial_base_ms, skill_vk=trial.get("skill_vk"))
        progress(ctx, "trial_start", trial=name, base_ms=trial_base_ms)
        records = schedule_steps(ctx, steps, trial_base_ms, name)
        max_at = max(int(s["at_ms"]) for s in steps)
        phase_end_ns = ctx.epoch_ns + (trial_base_ms + max_at) * 1_000_000
        wait_until(ctx, phase_end_ns, name)
        exec_map = wait_cmd_exec(ctx, [r["cmd_seq"] for r in records], exec_timeout_s)
        missing = [r.get("label") or r.get("id") or r.get("op")
                   for r in records if r["cmd_seq"] not in exec_map]
        if missing:
            ctx.exec_gaps.append({"phase": name, "missing": missing})
        cast = None
        entry = None
        ev = trial.get("evidence") or {}
        if ev:
            entry = next((r for r in records if r.get("label") == ev.get("entry_label")), None)
            if entry is None:
                raise RuntimeError("trial %s has no evidence entry %s" % (name, ev.get("entry_label")))
            if int(entry.get("vk", -1)) != VK_QUICKSLOT_A:
                raise RuntimeError("trial %s evidence entry must be quickslot A" % name)
            skill_id = int(ev["skill_id"])
            entry_exec = exec_map.get(entry["cmd_seq"])
            if entry_exec is not None:
                cast = wait_for_cast(ctx, skill_id, entry_exec.get("qpc_ns") or 0,
                                     float(ev.get("cast_timeout_ms", 15000)) / 1000.0)
            if cast is not None:
                proven_skills.add(int(cast.get("skill_id")))
                tail_ms = max(int(ev.get("tail_ms", A_TAIL_MIN_MS)), A_TAIL_MIN_MS)
                tail_end_ns = (cast.get("qpc_ns") or 0) + tail_ms * 1_000_000
                ctx.log.emit("a_evidence", trial=name, entry_label=entry.get("label"),
                             entry_cmd_seq=entry["cmd_seq"],
                             entry_qpc_ns=entry_exec.get("qpc_ns"), cast_qpc_ns=cast.get("qpc_ns"),
                             skill_id=cast.get("skill_id"), player_index=cast.get("player_index"),
                             actor_uid=cast.get("actor_uid"), master_slot=cast.get("master_slot"),
                             game_id=cast.get("game_id"), tail_ms=tail_ms, tail_end_ns=tail_end_ns)
                progress(ctx, "a_evidence", trial=name, skill_id=cast.get("skill_id"),
                         tail_end_ns=tail_end_ns)
                wait_until(ctx, tail_end_ns, "%s_tail" % name)
                phase_end_ns = max(phase_end_ns, tail_end_ns)
            else:
                ctx.log.emit("a_evidence_missing", trial=name, skill_id=skill_id,
                             entry_exec_found=entry_exec is not None, cmd_exec_missing=missing,
                             legality=legality_snapshot(ctx))
                progress(ctx, "a_evidence_missing", trial=name,
                         entry_exec_found=entry_exec is not None)
        base_ms = int((phase_end_ns - ctx.epoch_ns) / 1_000_000) + gap_ms
        ctx.trial_results.append({
            "name": name, "base_ms": trial_base_ms, "skill_vk": trial.get("skill_vk"),
            "trial_start_ns": ctx.epoch_ns + trial_base_ms * 1_000_000,
            "cast_observed": cast is not None,
            "cast_skill_id": cast.get("skill_id") if cast else None,
            "evidence_entry": entry.get("label") if entry else None,
            "cmd_exec_missing": missing,
            "exec_verified": not missing,
        })
        ctx.log.emit("trial_end", trial=name, cast_observed=cast is not None,
                     skill_id=cast.get("skill_id") if cast else None, cmd_exec_missing=missing)
        progress(ctx, "trial_end", trial=name, cast_observed=cast is not None, base_ms=base_ms)

    probe = sc.get("probe") or {}
    probe_end_ms = base_ms
    if probe.get("enabled"):
        probe_base = base_ms + int(probe.get("base_at_ms", 0))
        ctx.log.emit("probe_start", base_ms=probe_base)
        progress(ctx, "probe_start", base_ms=probe_base)
        probe_records = schedule_steps(ctx, probe["steps"], probe_base, "probe")
        probe_end_ms = probe_base + max(int(s["at_ms"]) for s in probe["steps"])
        wait_until(ctx, ctx.epoch_ns + (probe_end_ms + gap_ms) * 1_000_000, "probe")
        probe_exec = wait_cmd_exec(ctx, [r["cmd_seq"] for r in probe_records], exec_timeout_s)
        probe_missing = [r.get("label") or r.get("id") or r.get("op")
                         for r in probe_records if r["cmd_seq"] not in probe_exec]
        if probe_missing:
            ctx.exec_gaps.append({"phase": "probe", "missing": probe_missing})
        ctx.log.emit("probe_exec", executed=len(probe_exec), missing=probe_missing)
        progress(ctx, "probe_end", probe_end_ms=probe_end_ms, cmd_exec_missing=probe_missing)

    stop_check = sc.get("stop_release_check") or {}
    hold_step = stop_check.get("hold_step")
    if stop_check.get("enabled") and hold_step:
        hold_base = probe_end_ms + int(stop_check.get("hold_offset_ms", 3200))
        hold_records = schedule_steps(ctx, [hold_step], hold_base, "stop_release")
        mark = stop_check.get("mark")
        if mark:
            hold_records += schedule_steps(ctx, [mark], hold_base, "stop_release")
            wait_until(ctx, ctx.epoch_ns + (hold_base + int(mark["at_ms"]) + gap_ms) * 1_000_000,
                       "hold")
        else:
            wait_until(ctx, ctx.epoch_ns + (hold_base + gap_ms) * 1_000_000, "hold")
        hold_exec = wait_cmd_exec(ctx, [r["cmd_seq"] for r in hold_records], exec_timeout_s)
        hold_missing = [r.get("label") or r.get("id") or r.get("op")
                        for r in hold_records if r["cmd_seq"] not in hold_exec]
        if hold_missing:
            ctx.exec_gaps.append({"phase": "stop_release", "missing": hold_missing})
        ctx.log.emit("hold_exec", executed=len(hold_exec), missing=hold_missing)

    write_inputs(ctx)
    stop_ns = qpc_ns()
    ctx.log.emit("stop_request", qpc_ns=stop_ns)
    progress(ctx, "stop_request", qpc_ns=stop_ns)
    stop_ack = ctx.pipe.stop()
    ctx.stopped = True
    ctx.log.emit("stop_ack", ack=stop_ack)
    time.sleep(int(timing.get("stop_settle_ms", 1500)) / 1000.0)
    ctx.stop_release = verify_stop_release(ctx, stop_ns)
    ctx.log.emit("stop_release", result=ctx.stop_release)
    progress(ctx, "stop_release", result=ctx.stop_release)
    disconnect_check = sc.get("disconnect_release_check") or {}
    if disconnect_check.get("enabled"):
        verify_disconnect_release(ctx, disconnect_check, exec_timeout_s)
    write_inputs(ctx)
    return ctx


def verify_disconnect_release(ctx, spec, timeout_s):
    ack = ctx.pipe.begin()
    epoch = int(ack["epoch_ns"])
    ctx.stopped = False
    holds = schedule_steps(ctx, spec["steps"], 0, "disconnect_release", epoch_ns=epoch)
    pending = schedule_steps(ctx, [spec["pending_step"]], 0, "disconnect_release", epoch_ns=epoch)[0]
    executed = wait_cmd_exec(ctx, [r["cmd_seq"] for r in holds], timeout_s)
    if len(executed) != len(holds):
        raise RuntimeError("disconnect probe held keys did not execute")
    close_ns = qpc_ns()
    ctx.log.emit("disconnect_request", pending_cmd_seq=pending["cmd_seq"])
    ctx.pipe.close()
    ctx.pipe = None
    tail = NdjsonTail(os.path.join(ctx.run_dir, "client_*.ndjson"))
    released = None
    release_ns = None
    cancelled = True
    input_released = False
    last_sample_ns = 0
    deadline = epoch + (int(spec["pending_step"]["at_ms"]) + 1500) * 1_000_000
    while qpc_ns() < deadline:
        check_abort(ctx)
        for rec in tail.poll():
            if rec.get("qpc_ns", 0) < close_ns:
                continue
            if rec.get("event") == "stop_release":
                released = rec.get("released")
                release_ns = rec.get("qpc_ns")
            elif rec.get("event") == "cmd_exec" and rec.get("cmd_seq") == pending["cmd_seq"]:
                cancelled = False
            elif rec.get("event") == "input_state" and rec.get("vk") == 17:
                input_released = rec.get("modifier_vk") == -1 and 123 not in rec.get("held_types", [])
            elif rec.get("event") == "sample" and rec.get("phase") == "post":
                last_sample_ns = rec["qpc_ns"]
        time.sleep(0.05)
    continued = last_sample_ns >= pending["scheduled_ns"]
    verified = released == [65, 17] and cancelled and input_released and continued
    ctx.stopped = verified
    result = {"verified": verified, "released": released, "release_qpc_ns": release_ns,
              "cancelled_pending": cancelled, "input_released": input_released,
              "continued_after_pending": continued}
    ctx.log.emit("disconnect_release", **result)
    progress(ctx, "disconnect_release", **result)
    if not verified:
        raise RuntimeError("disconnect release verification failed: %s" % json.dumps(result))


def cleanup(ctx, reason="normal"):
    result = {"reason": reason}
    try:
        if ctx.pipe is not None:
            if not ctx.stopped:
                try:
                    ctx.pipe.stop()
                    ctx.stopped = True
                except Exception as exc:
                    result["pipe_stop_error"] = str(exc)
            ctx.pipe.close()
    except Exception as exc:
        result["pipe_error"] = str(exc)
    try:
        if ctx.steps:
            write_inputs(ctx)
    except Exception as exc:
        result["inputs_write_error"] = str(exc)
    try:
        if ctx.client_pid:
            result["client_kill"] = winproc.kill_tree(ctx.client_pid)
            result["client_gone"] = wait_owned_exit(ctx, ctx.client_pid)
    except Exception as exc:
        result["client_kill_error"] = str(exc)
    try:
        if ctx.injector_proc is not None and ctx.injector_proc.poll() is None:
            result["injector_kill"] = winproc.kill_tree(ctx.injector_proc.pid, elevated=False)
        elif ctx.injector_handle is not None:
            elevate.close(ctx.injector_handle)
        if ctx.injector_pid:
            result["injector_gone"] = wait_owned_exit(ctx, ctx.injector_pid)
    except Exception as exc:
        result["injector_kill_error"] = str(exc)
    try:
        if ctx.server_mgr is not None:
            result["servers"] = ctx.server_mgr.stop()
    except Exception as exc:
        result["servers_error"] = str(exc)
    try:
        if ctx.mon is not None:
            ctx.mon.stop()
            result["monitor"] = ctx.mon.snapshot()
    except Exception as exc:
        result["monitor_error"] = str(exc)
    try:
        if ctx.guard is not None:
            ctx.guard.stop()
            result["guard"] = ctx.guard.snapshot()
    except Exception as exc:
        result["guard_error"] = str(exc)
    try:
        if ctx.fixture is not None:
            result["cfg"] = ctx.fixture.restore()
    except Exception as exc:
        result["cfg_error"] = str(exc)
    client_exe = (ctx.scenario.get("client") or {}).get("exe") or CLIENT_EXE
    result["leftover_clients"] = winproc.list_named(os.path.basename(client_exe))
    result["survivors"] = owned_survivors(ctx)
    result["clean"] = cleanup_clean(result)
    ctx.shutdown = result
    try:
        ctx.log.emit("run_cleanup", **{k: v for k, v in result.items() if k != "monitor"})
    except Exception:
        pass
    ctx.persist_meta()
    return result


def cmd_run(args):
    scenario = load_scenario(args.scenario)
    run_root = args.run_root or RUN_ROOT
    run_id = args.run_id or ("%s_%s" % (time.strftime("%Y%m%d_%H%M%S", time.localtime()),
                                       scenario["name"]))
    run_dir = args.run_dir or os.path.join(run_root, run_id)
    ensure_dir(run_dir)
    log = EventLog(os.path.join(run_dir, "controller.ndjson"), "controller", run_id)
    env = dict(os.environ)
    env["VELQOR_RUN_DIR"] = run_dir
    env["VELQOR_RUN_ID"] = run_id
    ctx = RunContext(run_dir, run_id, scenario, args.scenario, log, env, args)
    ctx.started_ns = qpc_ns()
    ctx.started_wall_ns = time.time_ns()
    ctx.log.emit("run_start", scenario=scenario.get("name"), scenario_path=os.path.abspath(args.scenario),
                 run_dir=run_dir, argv=sys.argv)
    ctx.persist_meta()
    progress(ctx, "run_start", run_dir=run_dir, run_id=run_id, scenario=scenario.get("name"))
    failure = None
    try:
        execute(ctx)
    except AbortRun as exc:
        ctx.aborted = "abort:%s" % exc
        log.emit("abort", reason=str(exc))
        failure = "abort:%s" % exc
    except Exception as exc:
        ctx.aborted = "error:%s" % exc
        log.emit("run_error", error="%s: %s" % (type(exc).__name__, exc))
        failure = "%s: %s" % (type(exc).__name__, exc)
    cleanup(ctx, "failure" if failure else "normal")
    report = compare.build_report(run_dir, tolerance=float(scenario.get("compare", {}).get(
        "tolerance_units", 25.0)))
    verdict = report.get("verdict") or {}
    report["run"] = ctx.meta()
    report["shutdown"] = ctx.shutdown
    report["trials"] = ctx.trial_results
    report["exec_gaps"] = ctx.exec_gaps
    report["stop_release"] = ctx.stop_release
    if not failure and ctx.shutdown.get("clean") is not True:
        failure = "cleanup_incomplete"
    if not failure and verdict.get("complete") is not True:
        failure = "incomplete_verdict"
    report["failure"] = failure
    write_json(os.path.join(run_dir, "report.json"), report)
    log.emit("run_end", failure=failure, verdict=verdict, clean=ctx.shutdown.get("clean"),
             shutdown={k: v for k, v in ctx.shutdown.items() if k != "monitor"})
    progress(ctx, "done", failure=failure, verdict=verdict)
    log.close()
    print(json.dumps({
        "run_dir": run_dir,
        "run_id": run_id,
        "failure": failure,
        "verdict": verdict,
        "counts": report["counts"],
        "casts": report["casts"],
        "focus": report["focus"],
        "guard": report.get("guard"),
        "trials": ctx.trial_results,
        "exec_gaps": ctx.exec_gaps,
        "stop_release": ctx.stop_release,
        "shutdown": {k: v for k, v in ctx.shutdown.items() if k != "monitor"},
    }, indent=2))
    print(compare.summarize(report))
    return 0 if not failure else 1


def cmd_prepare(args):
    scenario = load_scenario(args.scenario)
    run_root = args.run_root or RUN_ROOT
    run_id = args.run_id or ("%s_%s" % (time.strftime("%Y%m%d_%H%M%S", time.localtime()),
                                       scenario["name"]))
    run_dir = args.run_dir or os.path.join(run_root, run_id)
    ensure_dir(run_dir)
    log = EventLog(os.path.join(run_dir, "controller.ndjson"), "controller", run_id)
    fixture = servers.CfgFixture(run_dir, log)
    fixture.capture()
    fixture.apply(scenario["server"]["cfg"])
    verify = fixture.verify(scenario["server"]["cfg"])
    log.emit("cfg_verify", result=verify)
    write_json(os.path.join(run_dir, "run_meta.json"), {
        "run_id": run_id, "run_dir": run_dir, "scenario": scenario.get("name"),
        "scenario_path": os.path.abspath(args.scenario), "prepared": True,
    })
    log.close()
    print(json.dumps({"run_dir": run_dir, "run_id": run_id, "verify": verify}, indent=2))
    return 0


def cmd_cleanup(args):
    run_dir = os.path.abspath(args.run_dir)
    meta = read_json(os.path.join(run_dir, "run_meta.json"), {}) or {}
    killed = []
    pids = []
    if meta.get("client_pid"):
        pids.append(("client", meta["client_pid"]))
    for name, pid in (meta.get("server_pids") or {}).items():
        pids.append((name, pid))
    for name, pid in pids:
        killed.append(dict(name=name, **winproc.kill_tree(pid)))
    backup = os.path.join(run_dir, "cfg_backup")
    restored = []
    if os.path.isdir(backup):
        for name in os.listdir(backup):
            src = os.path.join(backup, name)
            dst = os.path.join(BUILD, name)
            if os.path.isfile(src):
                with open(src, "r", encoding="utf-8", errors="replace") as f:
                    text = f.read()
                with open(dst, "w", encoding="utf-8", newline="") as f:
                    f.write(text)
                restored.append(name)
    print(json.dumps({"run_dir": run_dir, "killed": killed, "cfg_restored": restored,
                      "leftover_clients": winproc.list_named("MXMClient_DP_p3.exe")}, indent=2))
    return 0


def compare_acceptance_ok(report, require_cast):
    verdict = report.get("verdict") or {}
    if verdict.get("exec_evidence") is not True:
        return False
    if verdict.get("unfocused") is False:
        return False
    if verdict.get("guard_mutations"):
        return False
    if verdict.get("run_id_consistent") is False:
        return False
    if require_cast:
        if verdict.get("cast_proven") is not True:
            return False
        if SNIPER_NORMAL_A_SKILL_ID not in (verdict.get("cast_skills") or []):
            return False
    return True


def cmd_compare(args):
    report = compare.build_report(args.run_dir, tolerance=args.tolerance)
    out = args.out or os.path.join(args.run_dir, "compare.json")
    write_json(out, report)
    print(compare.summarize(report))
    print("compare_json=%s" % out)
    ok = compare_acceptance_ok(report, args.require_cast)
    print("acceptance=%s" % ("ok" if ok else "not_ok"))
    return 0 if ok else 1


def cmd_status(args):
    run_dir = os.path.abspath(args.run_dir)
    meta = read_json(os.path.join(run_dir, "run_meta.json"), {}) or {}
    info = {}
    for pid in [meta.get("client_pid")] + list((meta.get("server_pids") or {}).values()):
        if pid:
            info[str(pid)] = winproc.process_info(pid)
    print(json.dumps({"run_dir": run_dir, "meta": meta, "processes": info,
                      "leftover_clients": winproc.list_named("MXMClient_DP_p3.exe")}, indent=2))
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(prog="velqor-control")
    parser.add_argument("--run-root", default=None)
    sub = parser.add_subparsers(dest="command", required=True)

    p_run = sub.add_parser("run")
    p_run.add_argument("--scenario", default=DEFAULT_SCENARIO)
    p_run.add_argument("--run-dir", default=None)
    p_run.add_argument("--run-id", default=None)
    p_run.add_argument("--injector-mode", choices=["auto", "direct", "elevated"], default="auto")
    p_run.set_defaults(func=cmd_run)

    p_prep = sub.add_parser("prepare")
    p_prep.add_argument("--scenario", default=DEFAULT_SCENARIO)
    p_prep.add_argument("--run-dir", default=None)
    p_prep.add_argument("--run-id", default=None)
    p_prep.set_defaults(func=cmd_prepare)

    p_cmp = sub.add_parser("compare")
    p_cmp.add_argument("--run-dir", required=True)
    p_cmp.add_argument("--out", default=None)
    p_cmp.add_argument("--tolerance", type=float, default=25.0)
    p_cmp.add_argument("--require-cast", action="store_true")
    p_cmp.set_defaults(func=cmd_compare)

    p_clean = sub.add_parser("cleanup")
    p_clean.add_argument("--run-dir", required=True)
    p_clean.set_defaults(func=cmd_cleanup)

    p_status = sub.add_parser("status")
    p_status.add_argument("--run-dir", required=True)
    p_status.set_defaults(func=cmd_status)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
