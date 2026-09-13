import glob
import json
import os

from .common import (A_TAIL_MIN_MS, SNIPER_NORMAL_A_SKILL_ID, VK_FRONT, VK_QUICKSLOT_A,
                     iter_ndjson, read_json, write_json)

A_SKILL_ID = SNIPER_NORMAL_A_SKILL_ID
A_TAIL_NS = A_TAIL_MIN_MS * 1_000_000
FORWARD_VK = VK_FRONT
A_VK = VK_QUICKSLOT_A
FORBIDDEN_POSE_KEYS = ("destPos", "dest_pos", "pose", "client_pose")
CORRECTION_OBSERVATION = "post_handler"
CORRECTION_VEC_FIELDS = ("feet", "before_feet", "visual")
CORRECTION_MATCH_NS = 1_000_000_000
POST_CONTIGUITY_NS = 500_000_000
MIN_CLASS_TYPE = 1
HELD_INPUT_QUICKSLOT_1 = 14
HELD_INPUT_SKILLUP_1 = 123
VK_CONTROL = 17
LEARN_TRIAL = "learn_ctrl_a"
LEARN_LABEL = "skillup_a_down"


def guard_mutation(record):
    if record.get("blocked") in (True, 1):
        return None
    return "blocked_false"


def vec3(value):
    if isinstance(value, (list, tuple)) and len(value) >= 3:
        try:
            return [float(value[0]), float(value[1]), float(value[2])]
        except (TypeError, ValueError):
            return None
    if isinstance(value, dict):
        try:
            return [float(value["x"]), float(value["y"]), float(value["z"])]
        except (KeyError, TypeError, ValueError):
            return None
    return None


def distance(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b)) ** 0.5


def format_vec(value):
    if value is None:
        return "None"
    return "[%.2f,%.2f,%.2f]" % (value[0], value[1], value[2])


def collect_files(run_dir, pattern):
    return sorted(glob.glob(os.path.join(run_dir, pattern)))


class RunData:
    def __init__(self, run_dir):
        self.run_dir = run_dir
        self.meta = read_json(os.path.join(run_dir, "run_meta.json"), {}) or {}
        self.run_id = self.meta.get("run_id")
        self.controller = []
        self.client_files = collect_files(run_dir, "client_*.ndjson")
        self.server_files = collect_files(run_dir, "server_*.ndjson")
        self.client = []
        self.server = []
        self.controller_path = os.path.join(run_dir, "controller.ndjson")
        self.streams = {}
        self.run_id_mismatches = []
        self.forbidden_pose_fields = []
        for path in self.client_files:
            for rec in iter_ndjson(path):
                self._collect(rec, path, "client", self.client)
        for path in self.server_files:
            for rec in iter_ndjson(path):
                self._collect(rec, path, "server", self.server)
        for rec in iter_ndjson(self.controller_path):
            self._collect(rec, self.controller_path, "controller", self.controller)
        if not self.run_id:
            self.run_id = self._derive_run_id()
        self._resolve_run_id_mismatches()

    def _collect(self, rec, path, source, sink):
        sink.append(rec)
        basename = os.path.basename(path)
        stream = self.streams.setdefault((source, basename), {
            "source": source, "file": basename, "run_ids": set(), "missing_run_id": 0,
            "records": 0})
        stream["records"] += 1
        rid = rec.get("run_id")
        if rid in (None, ""):
            stream["missing_run_id"] += 1
        else:
            stream["run_ids"].add(rid)
        if rec.get("event") == "sample":
            for key in FORBIDDEN_POSE_KEYS:
                if key in rec:
                    self.forbidden_pose_fields.append(
                        {"file": basename, "qpc_ns": rec.get("qpc_ns"), "key": key})
                    break

    def _derive_run_id(self):
        for source in ("controller", "client", "server"):
            for key in sorted(self.streams):
                stream = self.streams[key]
                if stream["source"] == source and stream["run_ids"]:
                    return sorted(stream["run_ids"])[0]
        return None

    def _resolve_run_id_mismatches(self):
        for key in sorted(self.streams):
            stream = self.streams[key]
            for rid in sorted(stream["run_ids"]):
                if self.run_id is None or rid != self.run_id:
                    self.run_id_mismatches.append(
                        {"source": stream["source"], "file": stream["file"], "run_id": rid})
            if stream["missing_run_id"]:
                self.run_id_mismatches.append(
                    {"source": stream["source"], "file": stream["file"], "run_id": None,
                     "count": stream["missing_run_id"]})

    def identity_evidence(self):
        evidence = {}
        for source in ("client", "server", "controller"):
            streams = [s for s in self.streams.values() if s["source"] == source]
            run_ids = set()
            missing = 0
            records = 0
            for stream in streams:
                run_ids |= stream["run_ids"]
                missing += stream["missing_run_id"]
                records += stream["records"]
            consistent = (records > 0 and missing == 0 and len(run_ids) == 1
                          and self.run_id is not None and next(iter(run_ids)) == self.run_id)
            evidence[source] = {
                "files": sorted(s["file"] for s in streams),
                "records": records,
                "run_ids": sorted(run_ids),
                "missing_run_id": missing,
                "consistent": consistent,
            }
        return evidence


def parse_client_samples(records, phase="post"):
    out = []
    for rec in records:
        if rec.get("event") != "sample":
            continue
        if phase is not None and rec.get("phase") is not None and rec.get("phase") != phase:
            continue
        feet = vec3(rec.get("feet"))
        if feet is None:
            continue
        out.append({
            "qpc_ns": rec.get("qpc_ns"),
            "run_id": rec.get("run_id"),
            "phase": rec.get("phase"),
            "actor_id": rec.get("local_actor_id"),
            "class_type": rec.get("class_type"),
            "frame": rec.get("frame"),
            "feet": feet,
            "visual": vec3(rec.get("visual")),
        })
    out.sort(key=lambda r: r["qpc_ns"] or 0)
    return out


def client_phases(records):
    transitions = [r for r in records if r.get("event") == "phase"]
    return {
        "transitions": transitions,
        "not_ready": [r for r in records if r.get("event") == "not_ready"],
        "errors": [r for r in records if r.get("event") == "error"],
        "hello": [r for r in records if r.get("event") == "hello"],
        "final_phase": transitions[-1].get("to") if transitions else None,
    }


def guard_summary(records):
    events = [r for r in records if r.get("event") == "os_guard"]
    blocked = [r for r in events if r.get("blocked") in (True, 1)]
    mutations = [r for r in events if guard_mutation(r)]
    apis = {}
    for rec in events:
        key = rec.get("api") or "unknown"
        entry = apis.setdefault(key, {"total": 0, "blocked": 0, "unblocked": 0})
        entry["total"] += 1
        if rec.get("blocked") in (True, 1):
            entry["blocked"] += 1
        else:
            entry["unblocked"] += 1
    return {
        "events": len(events),
        "blocked": len(blocked),
        "mutations": len(mutations),
        "apis": apis,
        "blocked_api_counters_present": bool(events),
    }


def parse_server(records):
    result = {
        "start": None,
        "instances": [],
        "cleanup": [],
        "joins": [],
        "post_sim": [],
        "casts": [],
        "skill_commands": [],
        "skill_exec": [],
        "skill_done": [],
        "corrections": [],
        "input_move": [],
        "tag_applies": [],
        "trace_overflow": [],
        "other": {},
    }
    for rec in records:
        event = rec.get("event")
        game_id = rec.get("game_id")
        if event == "server_start":
            result["start"] = rec
        elif event == "instance_init":
            result["instances"].append(rec)
        elif event == "instance_cleanup":
            result["cleanup"].append(rec)
        elif event == "player_join":
            result["joins"].append(rec)
        elif event == "post_sim":
            pos = vec3(rec.get("pos_server"))
            if pos is None:
                continue
            result["post_sim"].append({
                "qpc_ns": rec.get("qpc_ns"),
                "run_id": rec.get("run_id"),
                "game_id": game_id,
                "player_index": rec.get("player_index"),
                "user_id": rec.get("user_id"),
                "actor_uid": rec.get("actor_uid"),
                "master_slot": rec.get("master_slot"),
                "pos": pos,
                "vel": vec3(rec.get("vel")),
                "grounded": rec.get("grounded"),
                "move_speed": rec.get("move_speed"),
                "sim_t": rec.get("sim_t"),
                "update_seq": rec.get("update_seq"),
            })
        elif event == "cast_accept":
            result["casts"].append(rec)
        elif event == "skill_command":
            result["skill_commands"].append(rec)
        elif event == "skill_exec":
            result["skill_exec"].append(rec)
        elif event == "skill_done":
            result["skill_done"].append(rec)
        elif event == "position_correction":
            result["corrections"].append(rec)
        elif event == "input_move":
            result["input_move"].append(rec)
        elif event == "tag_apply":
            result["tag_applies"].append(rec)
        elif event == "trace_overflow":
            result["trace_overflow"].append({"qpc_ns": rec.get("qpc_ns"),
                                             "dropped_event": rec.get("dropped_event")})
        else:
            result["other"][event] = result["other"].get(event, 0) + 1
    return result


def owner_player(server):
    humans = [join for join in server["joins"] if not join.get("is_bot")]
    if len(humans) == 1:
        return humans[0]
    return None


def owner_join_reason(server):
    joins = server["joins"]
    if not joins:
        return "owner_join_missing"
    humans = [join for join in joins if not join.get("is_bot")]
    if not humans:
        return "owner_join_no_human"
    if len(humans) > 1:
        return "owner_join_ambiguous"
    return None


def owner_game_id(server):
    owner = owner_player(server)
    if owner and owner.get("game_id") is not None:
        return owner.get("game_id")
    return None


def instance_epochs(server):
    inits = sorted([init for init in server["instances"] if init.get("qpc_ns") is not None],
                   key=lambda r: r["qpc_ns"])
    cleanups = sorted([rec for rec in server["cleanup"] if rec.get("qpc_ns") is not None],
                      key=lambda r: r["qpc_ns"])
    epochs = []
    for index, init in enumerate(inits):
        game_id = init.get("game_id")
        start = init.get("qpc_ns")
        next_start = inits[index + 1]["qpc_ns"] if index + 1 < len(inits) else None
        end = None
        for cleanup in cleanups:
            if cleanup.get("game_id") != game_id:
                continue
            candidate = cleanup["qpc_ns"]
            if candidate < start:
                continue
            if next_start is not None and candidate > next_start:
                continue
            if end is None or candidate < end:
                end = candidate
        epochs.append({
            "game_id": game_id,
            "start_ns": start,
            "end_ns": end,
            "sortie_uid": init.get("sortie_uid"),
            "index": index,
        })
    return epochs


def epoch_indices(epochs, qpc):
    if not epochs or qpc is None:
        return []
    return [index for index, epoch in enumerate(epochs)
            if (epoch.get("start_ns") is None or qpc >= epoch["start_ns"])
            and (epoch.get("end_ns") is None or qpc <= epoch["end_ns"])]


def run_id_matches(rec, run_id):
    if run_id is None:
        return True
    return rec.get("run_id") == run_id


def valid_class_type(value):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return False
    return int(value) >= MIN_CLASS_TYPE and int(value) == value


def same_id(left, right):
    if left is None or right is None:
        return False
    if isinstance(left, bool) or isinstance(right, bool):
        return False
    if not isinstance(left, (int, float)) or not isinstance(right, (int, float)):
        return False
    return int(left) == int(right)


def active_class(owner, slot):
    if owner is None or slot is None:
        return None
    slot = int(slot)
    if slot == 0:
        return owner.get("main_class")
    if slot == 1:
        return owner.get("sub_class")
    return None


def post_identity(post, epoch_index):
    return (epoch_index, post.get("game_id"), post.get("player_index"),
            post.get("actor_uid"), post.get("master_slot"))


def active_owner_segments(posts, post_epoch):
    segments = []
    current = None
    for post in sorted(posts, key=lambda r: r["qpc_ns"]):
        identity = post_identity(post, post_epoch[id(post)])
        qpc = post["qpc_ns"]
        if current is not None and identity == current["identity"]:
            if qpc - current["end_ns"] <= POST_CONTIGUITY_NS:
                current["posts"].append(post)
                current["end_ns"] = qpc
                continue
            transition = "gap"
        elif current is None:
            transition = "start"
        else:
            transition = "identity"
        current = {
            "index": len(segments),
            "identity": identity,
            "epoch_index": identity[0],
            "game_id": identity[1],
            "player_index": identity[2],
            "actor_uid": identity[3],
            "master_slot": identity[4],
            "transition_in": transition,
            "start_ns": qpc,
            "end_ns": qpc,
            "posts": [post],
            "expected_class": None,
            "class_resolved": False,
            "client_identity": None,
        }
        segments.append(current)
    return segments


def pair_trajectories(samples, posts, owner, epochs, run_id, game_id, player_index,
                      max_dt_ms=250.0):
    reasons = {}
    gaps = {}

    def note(bucket, key):
        bucket[key] = bucket.get(key, 0) + 1

    if not epochs:
        note(reasons, "no_instance_epochs")
    eligible_posts = []
    post_epoch = {}
    for post in posts:
        if not run_id_matches(post, run_id):
            continue
        if player_index is None or post.get("player_index") != player_index:
            continue
        if game_id is not None and post.get("game_id") != game_id:
            continue
        matches = epoch_indices(epochs, post.get("qpc_ns"))
        if len(matches) != 1:
            if len(matches) > 1:
                note(reasons, "post_epoch_ambiguous")
            else:
                note(gaps, "post_outside_instance_epoch")
            continue
        eligible_posts.append(post)
        post_epoch[id(post)] = matches[0]
    segments = active_owner_segments(eligible_posts, post_epoch)
    transitions = {}
    for segment in segments:
        note(transitions, segment["transition_in"])
        expected = active_class(owner, segment["master_slot"])
        segment["expected_class"] = expected
        segment["class_resolved"] = valid_class_type(expected)
        if not segment["class_resolved"]:
            note(reasons, "active_class_unresolved")
    eligible_samples = []
    sample_epoch = {}
    for sample in samples:
        if not run_id_matches(sample, run_id):
            continue
        if sample.get("qpc_ns") is None:
            note(gaps, "sample_qpc_missing")
            continue
        if sample.get("actor_id") is None:
            note(reasons, "sample_actor_id_missing")
            continue
        if not valid_class_type(sample.get("class_type")):
            note(reasons, "sample_class_type_invalid")
            continue
        matches = epoch_indices(epochs, sample["qpc_ns"])
        if len(matches) != 1:
            if len(matches) > 1:
                note(reasons, "sample_epoch_ambiguous")
            else:
                note(gaps, "sample_outside_instance_epoch")
            continue
        eligible_samples.append(sample)
        sample_epoch[id(sample)] = matches[0]
    assignments = {}
    ambiguous = 0
    unpaired = 0
    for sample in eligible_samples:
        qpc = sample["qpc_ns"]
        hits = [segment for segment in segments
                if segment["epoch_index"] == sample_epoch[id(sample)]
                and segment["start_ns"] <= qpc <= segment["end_ns"]]
        if len(hits) != 1:
            if len(hits) > 1:
                ambiguous += 1
                note(reasons, "sample_segment_ambiguous")
            else:
                unpaired += 1
                note(gaps, "sample_outside_actor_segment")
            continue
        segment = hits[0]
        if not segment["class_resolved"]:
            continue
        if int(sample["class_type"]) != int(segment["expected_class"]):
            note(reasons, "client_class_mismatch")
            continue
        assignments.setdefault(id(segment), []).append(sample)
    pairs = []
    skipped_dt = 0
    for segment in segments:
        segment_samples = assignments.get(id(segment), [])
        identities = {(sample.get("actor_id"), int(sample.get("class_type")))
                      for sample in segment_samples}
        if len(identities) > 1:
            note(reasons, "segment_client_identity_unstable")
            continue
        if identities:
            actor_id, class_type = next(iter(identities))
            segment["client_identity"] = {"local_actor_id": actor_id, "class_type": class_type}
        group = segment["posts"]
        j = 0
        for sample in sorted(segment_samples, key=lambda r: r["qpc_ns"]):
            while (j + 1 < len(group)
                   and abs((group[j + 1]["qpc_ns"] or 0) - sample["qpc_ns"])
                   <= abs((group[j]["qpc_ns"] or 0) - sample["qpc_ns"])):
                j += 1
            post = group[j]
            dt_ms = ((post["qpc_ns"] or 0) - sample["qpc_ns"]) / 1e6
            if abs(dt_ms) > max_dt_ms:
                skipped_dt += 1
                continue
            pairs.append({
                "qpc_ns": sample["qpc_ns"],
                "server_qpc_ns": post["qpc_ns"],
                "run_id": sample.get("run_id"),
                "game_id": post.get("game_id"),
                "player_index": post.get("player_index"),
                "actor_uid": post.get("actor_uid"),
                "master_slot": post.get("master_slot"),
                "expected_class": segment["expected_class"],
                "client_actor_id": sample.get("actor_id"),
                "client_class_type": sample.get("class_type"),
                "segment_index": segment["index"],
                "dt_ms": dt_ms,
                "client": sample["feet"],
                "server": post["pos"],
                "dist": distance(sample["feet"], post["pos"]),
                "server_sim_t": post.get("sim_t"),
                "server_update_seq": post.get("update_seq"),
                "client_frame": sample.get("frame"),
                "grounded": post.get("grounded"),
                "move_speed": post.get("move_speed"),
            })
    pairs.sort(key=lambda r: r["qpc_ns"] or 0)
    resolved_segments = [segment for segment in segments if segment["client_identity"]]
    stats = {
        "pairing": "monotonic_nearest_within_active_identity_segment",
        "candidate_samples": len(eligible_samples),
        "candidate_posts": len(eligible_posts),
        "identity_segments": len(segments),
        "segment_transitions": transitions,
        "resolved_segments": len(resolved_segments),
        "pairs": len(pairs),
        "skipped_dt_over_limit": skipped_dt,
        "ambiguous_epoch_samples": ambiguous,
        "unpaired_samples": unpaired,
        "reasons": reasons,
        "gaps": gaps,
        "identity_ok": not reasons and bool(resolved_segments),
        "max_dt_ms": max((abs(p["dt_ms"]) for p in pairs), default=None),
        "mean_dt_ms": (sum(p["dt_ms"] for p in pairs) / len(pairs)) if pairs else None,
    }
    return pairs, stats, segments


def segment_summary(segment):
    return {
        "epoch_index": segment["epoch_index"],
        "game_id": segment["game_id"],
        "player_index": segment["player_index"],
        "actor_uid": segment["actor_uid"],
        "master_slot": segment["master_slot"],
        "expected_class": segment["expected_class"],
        "transition_in": segment["transition_in"],
        "start_ns": segment["start_ns"],
        "end_ns": segment["end_ns"],
        "posts": len(segment["posts"]),
        "class_resolved": segment["class_resolved"],
        "client_identity": segment["client_identity"],
    }


def decimate(pairs, count=24):
    if len(pairs) <= count:
        return pairs
    step = max(1, len(pairs) // count)
    return pairs[::step][:count]


def trajectory_report(pairs, anchors, tolerance, pair_stats=None):
    if not pairs:
        return {"samples": 0, "max_dist": None, "mean_dist": None, "first_divergence": None,
                "segments": {}, "pairing": pair_stats}
    dists = [p["dist"] for p in pairs]
    dts = [p["dt_ms"] for p in pairs]
    report = {
        "samples": len(pairs),
        "max_dist": max(dists),
        "mean_dist": sum(dists) / len(dists),
        "dt_ms_min": min(dts),
        "dt_ms_max": max(dts),
        "dt_ms_mean": sum(dts) / len(dts),
        "dt_ms_abs_max": max(abs(d) for d in dts),
        "first_divergence": None,
        "segments": {},
        "pairing": pair_stats,
    }
    for name, (lo, hi) in anchors.items():
        seg = [p for p in pairs if lo <= p["qpc_ns"] <= hi]
        if seg:
            report["segments"][name] = {
                "samples": len(seg),
                "t_start_ns": seg[0]["qpc_ns"],
                "t_end_ns": seg[-1]["qpc_ns"],
                "client_start": seg[0]["client"], "client_end": seg[-1]["client"],
                "server_start": seg[0]["server"], "server_end": seg[-1]["server"],
                "client_displacement": distance(seg[0]["client"], seg[-1]["client"]),
                "server_displacement": distance(seg[0]["server"], seg[-1]["server"]),
                "max_dist": max(p["dist"] for p in seg),
                "dt_ms_mean": sum(p["dt_ms"] for p in seg) / len(seg),
                "decimated": decimate(seg),
            }
    for index, pair in enumerate(pairs):
        if pair["dist"] > tolerance:
            report["first_divergence"] = {
                "qpc_ns": pair["qpc_ns"],
                "dist": pair["dist"],
                "tolerance": tolerance,
                "client": pair["client"],
                "server": pair["server"],
                "index": index,
            }
            break
    return report


def load_inputs(run_dir):
    return read_json(os.path.join(run_dir, "inputs.json"), None) or {}


def exec_records(client_records):
    return [r for r in client_records if r.get("event") == "cmd_exec"]


def client_corrections(client_records):
    return [r for r in client_records if r.get("event") == "correction"]


def pair_summary(pairs):
    if not pairs:
        return {"samples": 0, "max_dist": None, "mean_dist": None, "dt_ms_abs_max": None,
                "dt_ms_mean": None}
    dists = [p["dist"] for p in pairs]
    dts = [p["dt_ms"] for p in pairs]
    return {
        "samples": len(pairs),
        "max_dist": max(dists),
        "mean_dist": sum(dists) / len(dists),
        "dt_ms_abs_max": max(abs(d) for d in dts),
        "dt_ms_mean": sum(dts) / len(dts),
    }


def correction_reason(observation):
    if observation["reasons"]:
        return "|".join(sorted(observation["reasons"]))
    return "correction_observation_outside_active_segment"


def parse_correction_observation(rec):
    reasons = []
    if rec.get("observation") != CORRECTION_OBSERVATION:
        reasons.append("observation_not_post_handler")
    qpc = rec.get("qpc_ns")
    if qpc is None:
        reasons.append("missing_qpc_ns")
    elif isinstance(qpc, bool) or not isinstance(qpc, (int, float)):
        reasons.append("invalid_qpc_ns")
        qpc = None
    actor_id = rec.get("local_actor_id")
    if actor_id is None:
        reasons.append("missing_local_actor_id")
    elif (isinstance(actor_id, bool) or not isinstance(actor_id, (int, float))
          or int(actor_id) <= 0):
        reasons.append("invalid_local_actor_id")
        actor_id = None
    frame = rec.get("frame")
    if frame is None:
        reasons.append("missing_frame")
    values = {"feet": vec3(rec.get("feet")), "before_feet": vec3(rec.get("before_feet")),
              "visual": vec3(rec.get("visual"))}
    for key in CORRECTION_VEC_FIELDS:
        if values[key] is None:
            reasons.append("invalid_" + key)
    return {
        "qpc_ns": qpc,
        "local_actor_id": actor_id,
        "frame": frame,
        "feet": values["feet"],
        "before_feet": values["before_feet"],
        "visual": values["visual"],
        "delta_units": distance(values["before_feet"], values["feet"])
        if values["before_feet"] is not None and values["feet"] is not None else None,
        "observation": rec.get("observation"),
        "resolved": not reasons,
        "reasons": reasons,
        "segment_index": None,
        "attribution": "unparsed",
        "server_matched": False,
    }


def locate_correction_segment(qpc, segments, epochs, actor_uid=None, local_actor_id=None,
                              player_index=None, game_id=None):
    if qpc is None:
        return None
    matches = epoch_indices(epochs, qpc)
    if len(matches) != 1:
        return None
    hits = []
    for segment in segments:
        if segment["epoch_index"] != matches[0]:
            continue
        if not (segment["start_ns"] <= qpc <= segment["end_ns"]):
            continue
        if player_index is not None and segment["player_index"] != player_index:
            continue
        if game_id is not None and segment["game_id"] != game_id:
            continue
        if actor_uid is not None and not same_id(segment["actor_uid"], actor_uid):
            continue
        if local_actor_id is not None:
            identity = segment["client_identity"]
            if identity is None or not same_id(identity["local_actor_id"], local_actor_id):
                continue
        hits.append(segment["index"])
    if len(hits) != 1:
        return None
    return hits[0]


def correction_report(client_events, server_events, segments, epochs):
    observations = [parse_correction_observation(rec) for rec in client_events]
    by_index = {segment["index"]: segment for segment in segments}
    located = {}
    unresolved_observations = []
    unattributed_observations = []
    for observation in observations:
        index = locate_correction_segment(observation["qpc_ns"], segments, epochs,
                                          local_actor_id=observation["local_actor_id"])
        observation["segment_index"] = index
        if index is None:
            observation["attribution"] = "unlocated"
            unattributed_observations.append(observation)
            continue
        located.setdefault(index, []).append(observation)
        if observation["resolved"]:
            observation["attribution"] = "segment"
        else:
            observation["attribution"] = "unresolved"
            unresolved_observations.append(observation)
    server_matched = 0
    server_shadow = {}
    server_unattributed = []
    server_attributed = []
    for event in server_events:
        qpc = event.get("qpc_ns")
        index = locate_correction_segment(qpc, segments, epochs,
                                          actor_uid=event.get("actor_uid"),
                                          player_index=event.get("player_index"),
                                          game_id=event.get("game_id"))
        if index is None:
            server_unattributed.append({
                "qpc_ns": qpc,
                "actor_uid": event.get("actor_uid"),
                "segment_index": None,
                "reason": "server_correction_outside_active_segment",
            })
            continue
        candidates = [observation for observation in located.get(index, [])
                      if observation["resolved"]
                      and abs(observation["qpc_ns"] - qpc) <= CORRECTION_MATCH_NS]
        if not candidates:
            server_shadow.setdefault(index, []).append(qpc)
            server_unattributed.append({
                "qpc_ns": qpc,
                "actor_uid": event.get("actor_uid"),
                "segment_index": index,
                "reason": "server_correction_without_client_observation",
            })
            continue
        server_matched += 1
        for observation in candidates:
            observation["server_matched"] = True
        nearest = min(candidates, key=lambda o: abs(o["qpc_ns"] - qpc))
        server_attributed.append({
            "qpc_ns": qpc,
            "actor_uid": event.get("actor_uid"),
            "segment_index": index,
            "client_actor_id": (by_index[index]["client_identity"] or {}).get("local_actor_id"),
            "observation_qpc_ns": nearest["qpc_ns"],
            "observations_in_window": len(candidates),
        })
    states = []
    for segment in segments:
        index = segment["index"]
        segment_observations = located.get(index, [])
        influence = [observation["qpc_ns"] for observation in segment_observations
                     if observation["resolved"] and observation["qpc_ns"] is not None]
        shadow = [(observation["qpc_ns"], correction_reason(observation))
                  for observation in segment_observations
                  if not observation["resolved"] and observation["qpc_ns"] is not None]
        shadow.extend((qpc, "server_correction_without_client_observation")
                      for qpc in server_shadow.get(index, []))
        shadow.sort(key=lambda entry: entry[0])
        states.append({
            "segment_index": index,
            "actor_uid": segment["actor_uid"],
            "master_slot": segment["master_slot"],
            "client_actor_id": (segment["client_identity"] or {}).get("local_actor_id"),
            "observations": len(segment_observations),
            "influence_ns": min(influence) if influence else None,
            "unresolved_ns": shadow[0][0] if shadow else None,
            "unresolved_reason": shadow[0][1] if shadow else None,
        })
    global_reasons = [correction_reason(observation) for observation in unattributed_observations]
    global_reasons.extend(event["reason"] for event in server_unattributed
                          if event["segment_index"] is None)
    accounted = not unresolved_observations and not unattributed_observations \
        and not server_unattributed
    return {
        "client_events": len(observations),
        "client_resolved": len([o for o in observations if o["resolved"]]),
        "client_unresolved": len(unresolved_observations) + len(unattributed_observations),
        "client_unresolved_events": [
            {"qpc_ns": o["qpc_ns"], "local_actor_id": o["local_actor_id"], "frame": o["frame"],
             "segment_index": o["segment_index"], "reasons": o["reasons"]}
            for o in observations if not o["resolved"] or o["segment_index"] is None],
        "observations": observations,
        "server_events": len(server_events),
        "server_matched": server_matched,
        "server_unattributed": len(server_unattributed),
        "server_unattributed_events": server_unattributed,
        "server_attributed_events": server_attributed,
        "matched_server_events": server_matched,
        "unmatched_server_events": [event["qpc_ns"] for event in server_unattributed],
        "segments": states,
        "influenced_segments": len([state for state in states
                                    if state["influence_ns"] is not None]),
        "match_ns": CORRECTION_MATCH_NS,
        "global_unresolved": bool(global_reasons),
        "global_unresolved_reason": global_reasons[0] if global_reasons else None,
        "coverage_ok": accounted,
        "accounted": accounted,
    }


def partition_correction_pairs(pairs, evidence):
    states = {state["segment_index"]: state for state in evidence.get("segments", [])}
    independent = []
    affected = []
    unresolved = []
    reasons = {}
    for pair in pairs:
        state = states.get(pair.get("segment_index")) or {}
        influence = state.get("influence_ns")
        shadow = state.get("unresolved_ns")
        if influence is not None and pair["qpc_ns"] >= influence:
            affected.append(pair)
        elif evidence.get("global_unresolved"):
            unresolved.append(pair)
            key = evidence.get("global_unresolved_reason") or "correction_attribution_unresolved"
            reasons[key] = reasons.get(key, 0) + 1
        elif shadow is not None and pair["qpc_ns"] >= shadow:
            unresolved.append(pair)
            key = state.get("unresolved_reason") or "correction_evidence_unresolved"
            reasons[key] = reasons.get(key, 0) + 1
        else:
            independent.append(pair)
    return independent, affected, unresolved, reasons


def a_motion_interval(window, probe_execs):
    start = window.get("a_down_ns")
    end = window.get("tail_end_ns")
    if start is None or end is None:
        return None
    cast_ns = window.get("matched_cast_ns")
    clip = None
    if cast_ns is not None:
        trailing = [p["qpc_ns"] for p in probe_execs
                    if p.get("qpc_ns") is not None and cast_ns < p["qpc_ns"] <= end]
        if trailing:
            clip = min(trailing)
    return {"start_ns": start, "end_ns": end if clip is None else clip, "clip_ns": clip}


def a_motion_block(pairs, correction_evidence):
    first = pairs[0]
    last = pairs[-1]
    client_disp = [b - a for a, b in zip(first["client"], last["client"])]
    server_disp = [b - a for a, b in zip(first["server"], last["server"])]
    disagreement = [c - s for c, s in zip(client_disp, server_disp)]
    dists = [p["dist"] for p in pairs]
    dts = [p["dt_ms"] for p in pairs]
    independent, affected, unresolved, reasons = \
        partition_correction_pairs(pairs, correction_evidence)
    client_len = distance(first["client"], last["client"])
    server_len = distance(first["server"], last["server"])
    return {
        "segment_index": first.get("segment_index"),
        "actor_uid": first.get("actor_uid"),
        "master_slot": first.get("master_slot"),
        "client_actor_id": first.get("client_actor_id"),
        "client_class_type": first.get("client_class_type"),
        "expected_class": first.get("expected_class"),
        "pair_count": len(pairs),
        "t_start_ns": first["qpc_ns"],
        "t_end_ns": last["qpc_ns"],
        "client_start": first["client"],
        "client_end": last["client"],
        "server_start": first["server"],
        "server_end": last["server"],
        "client_displacement": client_disp,
        "client_displacement_dist": client_len,
        "server_displacement": server_disp,
        "server_displacement_dist": server_len,
        "endpoint_disagreement": disagreement,
        "endpoint_disagreement_dist": distance(client_disp, server_disp),
        "endpoint_length_delta": abs(client_len - server_len),
        "max_dist": max(dists),
        "mean_dist": sum(dists) / len(dists),
        "dt_ms_min": min(dts),
        "dt_ms_max": max(dts),
        "dt_ms_mean": sum(dts) / len(dts),
        "dt_ms_abs_max": max(abs(d) for d in dts),
        "correction": {
            "independent": len(independent),
            "affected": len(affected),
            "unresolved": len(unresolved),
            "max_dist_independent": max((p["dist"] for p in independent), default=None),
            "reasons": reasons,
        },
    }


def a_motion_window(window, pairs, correction_evidence, probe_execs):
    interval = a_motion_interval(window, probe_execs)
    start = interval["start_ns"]
    end = interval["end_ns"]
    last_ns = end - 1 if interval["clip_ns"] is not None else end
    in_window = [p for p in pairs
                 if start <= p["qpc_ns"] <= last_ns
                 and start <= p["server_qpc_ns"] <= last_ns]
    grouped = {}
    for pair in in_window:
        grouped.setdefault(pair.get("segment_index"), []).append(pair)
    blocks = [a_motion_block(block_pairs, correction_evidence)
              for _, block_pairs in sorted(grouped.items(),
                                           key=lambda item: item[1][0]["qpc_ns"])]
    dists = [p["dist"] for p in in_window]
    dts = [p["dt_ms"] for p in in_window]
    correction = {"independent": 0, "affected": 0, "unresolved": 0, "reasons": {}}
    for block in blocks:
        for key in ("independent", "affected", "unresolved"):
            correction[key] += block["correction"][key]
        for reason, count in block["correction"]["reasons"].items():
            correction["reasons"][reason] = correction["reasons"].get(reason, 0) + count
    return {
        "cmd_seq": window.get("cmd_seq"),
        "a_down_ns": window.get("a_down_ns"),
        "matched_cast_ns": window.get("matched_cast_ns"),
        "tail_end_ns": window.get("tail_end_ns"),
        "interval_start_ns": start,
        "interval_end_ns": end,
        "probe_clip_ns": interval["clip_ns"],
        "tail_ok": window.get("tail_ok"),
        "tail_probe_execs": window.get("tail_probe_execs", 0),
        "pair_count": len(in_window),
        "block_count": len(blocks),
        "max_dist": max(dists) if dists else None,
        "mean_dist": (sum(dists) / len(dists)) if dists else None,
        "dt_ms_abs_max": max((abs(d) for d in dts), default=None),
        "dt_ms_mean": (sum(dts) / len(dts)) if dts else None,
        "correction": correction,
        "blocks": blocks,
    }


def a_motion_report(a_window, pairs, correction_evidence, probe_execs):
    windows = []
    skipped = []
    for window in a_window.get("windows", []):
        if not window.get("a_proven") or window.get("matched_cast_ns") is None:
            skipped.append({
                "cmd_seq": window.get("cmd_seq"),
                "a_down_ns": window.get("a_down_ns"),
                "reason": ("probe_contaminated" if window.get("probe_contaminated")
                           else "a_cast_not_proven"),
            })
            continue
        windows.append(a_motion_window(window, pairs, correction_evidence, probe_execs))
    return {
        "windows": windows,
        "skipped_windows": skipped,
        "proven_windows": len(windows),
        "skipped": len(skipped),
        "pair_count": sum(window["pair_count"] for window in windows),
    }


def focus_report(controller_records):
    events = [r for r in controller_records if r.get("event") in
              ("foreground_change", "cursor_motion", "monitor_start", "monitor_stop",
               "monitor_heartbeat", "guard_mutation", "client_error", "abort", "run_error")]
    client_focus = [r for r in events if r.get("event") == "foreground_change" and r.get("client")]
    owned_focus = [r for r in events if r.get("event") == "foreground_change" and r.get("owned")]
    moves = [r for r in events if r.get("event") == "cursor_motion"]
    mutations = [r for r in events if r.get("event") == "guard_mutation"]
    aborts = [r for r in events if r.get("event") in ("abort", "run_error")]
    transitions = [r for r in events if r.get("event") == "foreground_change"]
    return {
        "events": len(events),
        "foreground_transitions": len(transitions),
        "client_focus_events": len(client_focus),
        "owned_focus_events": len(owned_focus),
        "cursor_motions": len(moves),
        "guard_mutations": len(mutations),
        "aborts": aborts,
        "evidence": bool(events),
        "unfocused": bool(events) and not client_focus and not owned_focus and not mutations,
    }


def steps_by_cmd_seq(steps):
    mapping = {}
    for step in steps or []:
        if step.get("cmd_seq") is not None:
            mapping[step["cmd_seq"]] = step
    return mapping


def input_state_evidence(client_records, steps):
    by_cmd = steps_by_cmd_seq(steps)
    states = []
    for rec in client_records:
        if rec.get("event") != "input_state":
            continue
        held = rec.get("held_types")
        if not isinstance(held, list):
            held = []
        held = [int(value) for value in held
                if isinstance(value, (int, float)) and not isinstance(value, bool)]
        activated = rec.get("activated_types", [])
        if not isinstance(activated, list):
            activated = []
        step = by_cmd.get(rec.get("cmd_seq")) or {}
        states.append({
            "qpc_ns": rec.get("qpc_ns"),
            "cmd_seq": rec.get("cmd_seq"),
            "modifier_vk": rec.get("modifier_vk"),
            "held_types": held,
            "activated_types": activated,
            "label": step.get("label"),
            "trial": step.get("trial"),
            "skillup_held": HELD_INPUT_SKILLUP_1 in held,
            "quickslot_dispatched": HELD_INPUT_QUICKSLOT_1 in activated,
        })
    learn = [state for state in states if state["skillup_held"]]
    quickslot = [state for state in states if state["quickslot_dispatched"]]
    release = [state for state in states
               if state["modifier_vk"] == -1 and not state["skillup_held"]]
    learn_cmd_seqs = {state["cmd_seq"] for state in learn if state["cmd_seq"] is not None}
    return {
        "events": len(states),
        "learn_events": len(learn),
        "quickslot_events": len(quickslot),
        "learn_ok": bool(learn),
        "quickslot_ok": bool(quickslot),
        "ctrl_release_observed": bool(release),
        "release_events": len(release),
        "learn_cmd_seqs": sorted(learn_cmd_seqs),
        "states": states,
    }, learn_cmd_seqs


def flag_true(value):
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return value != 0
    if isinstance(value, (list, tuple, dict)):
        return bool(value)
    return False


def released_ids(event):
    released = event.get("released")
    if not isinstance(released, list):
        return []
    return [int(value) for value in released
            if isinstance(value, (int, float)) and not isinstance(value, bool)]


def disconnect_evidence(controller_records, client_records):
    events = [r for r in controller_records if r.get("event") == "disconnect_release"]
    native_stops = [r for r in client_records if r.get("event") == "stop_release"]
    expected = [A_VK, VK_CONTROL]
    verified = []
    for event in events:
        release_qpc = event.get("release_qpc_ns")
        if release_qpc is None:
            continue
        if released_ids(event) != expected:
            continue
        if not (flag_true(event.get("verified")) and flag_true(event.get("cancelled_pending"))
                and flag_true(event.get("continued_after_pending"))
                and flag_true(event.get("input_released"))):
            continue
        matches = [rec for rec in native_stops if rec.get("qpc_ns") == release_qpc]
        if len(matches) != 1 or released_ids(matches[0]) != expected:
            continue
        verified.append(event)
    return {
        "events": len(events),
        "verified_events": len(verified),
        "verified": bool(events) and len(verified) == len(events),
        "expected_released": expected,
        "released": [event.get("released") for event in events],
        "release_qpc_ns": [event.get("release_qpc_ns") for event in events],
        "native_stop_release_events": len(native_stops),
        "cancelled_pending": [event.get("cancelled_pending") for event in events],
        "continued_after_pending": [event.get("continued_after_pending") for event in events],
        "input_released": [event.get("input_released") for event in events],
        "raw": events,
    }


def a_trial_names(steps):
    names = set()
    for step in steps or []:
        if step.get("label") == "skill_a_down" and step.get("trial") is not None:
            names.add(step.get("trial"))
    if names:
        return names
    for step in steps or []:
        if (step.get("op") == "key" and step.get("vk") == A_VK
                and step.get("down") in (1, True) and step.get("trial") is not None
                and step.get("trial") != "probe"):
            names.add(step.get("trial"))
    return names


def attribute_a_casts(casts, exec_events, steps):
    by_cmd = steps_by_cmd_seq(steps)
    trials = a_trial_names(steps)
    a_downs = []
    probe_execs = []
    for exec_rec in exec_events:
        step = by_cmd.get(exec_rec.get("cmd_seq")) or {}
        trial = step.get("trial")
        if trial == "probe":
            probe_execs.append(exec_rec)
        if exec_rec.get("op") != "key" or exec_rec.get("vk") != A_VK:
            continue
        if exec_rec.get("down") not in (1, True):
            continue
        if trial in trials:
            a_downs.append(exec_rec)
    a_downs.sort(key=lambda r: r.get("qpc_ns") or 0)
    probe_execs.sort(key=lambda r: r.get("qpc_ns") or 0)
    windows = []
    proven = []
    used = set()
    for down in a_downs:
        down_ns = down.get("qpc_ns")
        if down_ns is None:
            continue
        match = None
        for index, cast in enumerate(casts):
            if index in used:
                continue
            if cast.get("skill_id") != A_SKILL_ID:
                continue
            if (cast.get("qpc_ns") or 0) < down_ns:
                continue
            match = (index, cast)
            break
        if match is None:
            windows.append({"cmd_seq": down.get("cmd_seq"), "at_ms": down.get("at_ms"),
                            "a_down_ns": down_ns, "matched_cast_ns": None, "a_proven": False,
                            "probe_contaminated": False, "tail_end_ns": None, "tail_ok": None})
            continue
        index, cast = match
        used.add(index)
        cast_ns = cast.get("qpc_ns") or 0
        contamination = [p for p in probe_execs if down_ns <= (p.get("qpc_ns") or 0) <= cast_ns]
        tail_end = cast_ns + A_TAIL_NS
        tail_probe = [p for p in probe_execs if cast_ns < (p.get("qpc_ns") or 0) <= tail_end]
        if not contamination:
            proven.append(cast)
        windows.append({
            "cmd_seq": down.get("cmd_seq"),
            "at_ms": down.get("at_ms"),
            "a_down_ns": down_ns,
            "matched_cast_ns": cast_ns,
            "cast_skill_id": cast.get("skill_id"),
            "a_proven": not contamination,
            "probe_contaminated": bool(contamination),
            "probe_execs_before_cast": len(contamination),
            "tail_end_ns": tail_end,
            "tail_ok": not tail_probe,
            "tail_probe_execs": len(tail_probe),
        })
    return {
        "skill_id": A_SKILL_ID,
        "a_trials": sorted(t for t in trials if t is not None),
        "a_down_cmd_seqs": [d.get("cmd_seq") for d in a_downs],
        "probe_execs": len(probe_execs),
        "windows": windows,
        "proven_casts": len(proven),
    }, proven, a_downs, probe_execs


def cast_attribution(cast, proven, a_downs, probe_execs):
    for candidate in proven:
        if candidate is cast:
            return "a_skill", True
    cast_ns = cast.get("qpc_ns") or 0
    if probe_execs:
        lo = probe_execs[0].get("qpc_ns") or 0
        hi = (probe_execs[-1].get("qpc_ns") or 0) + A_TAIL_NS
        if lo <= cast_ns <= hi:
            return "probe", False
    if any((down.get("qpc_ns") or 0) <= cast_ns for down in a_downs):
        return "after_a_down_unproven", False
    return "unattributed", False


def forward_evidence(exec_events):
    downs = [e for e in exec_events
             if e.get("op") == "key" and e.get("vk") == FORWARD_VK and e.get("down") in (1, True)]
    ups = [e for e in exec_events
           if e.get("op") == "key" and e.get("vk") == FORWARD_VK and e.get("down") in (0, False)]
    return downs, ups




def stop_evidence(client_records, exec_events):
    order = []
    checks = []
    released = []
    events = sorted((r for r in client_records
                     if r.get("event") in ("cmd_exec", "stop_release")),
                    key=lambda r: r.get("qpc_ns", 0))
    for rec in events:
        if rec.get("event") == "cmd_exec":
            if rec.get("op") != "key" or rec.get("vk") is None:
                continue
            vk = rec["vk"]
            if rec.get("down") == 1 and vk not in order:
                order.append(vk)
            elif rec.get("down") == 0 and vk in order:
                order.remove(vk)
            continue
        values = rec.get("released")
        expected = list(reversed(order))
        checks.append({"qpc_ns": rec.get("qpc_ns"), "held": list(order),
                       "released": values, "expected": expected, "ok": values == expected})
        if isinstance(values, list):
            released.extend(values)
            for vk in values:
                if vk in order:
                    order.remove(vk)
    exec_releases = [e for e in exec_events if e.get("op") == "stop_release"]
    return {
        "event_count": len(checks),
        "released": released,
        "exec_releases": [{"cmd_seq": e.get("cmd_seq"), "vk": e.get("vk"), "down": e.get("down")}
                          for e in exec_releases],
        "held_at_stop": [vk for check in checks for vk in check["held"]],
        "reverse_order_expected": [vk for check in checks for vk in check["expected"]],
        "release_checks": checks,
        "forward_released": FORWARD_VK in released,
        "reverse_order_ok": bool(checks) and all(check["ok"] for check in checks),
    }


def build_report(run_dir, tolerance=25.0):
    data = RunData(run_dir)
    server = parse_server(data.server)
    samples = parse_client_samples(data.client, phase="post")
    if not samples:
        samples = parse_client_samples(data.client, phase=None)
    owner = owner_player(server)
    join_reason = owner_join_reason(server)
    game_id = owner_game_id(server)
    player_index = owner.get("player_index") if owner else None
    epochs = instance_epochs(server)
    owner_epochs = [e for e in epochs if game_id is None or e.get("game_id") == game_id]
    pairs, pair_stats, segments = pair_trajectories(
        samples, server["post_sim"], owner, owner_epochs, data.run_id, game_id, player_index)
    identity_reasons = dict(pair_stats["reasons"])
    if join_reason:
        identity_reasons[join_reason] = identity_reasons.get(join_reason, 0) + 1
    if owner is not None and (game_id is None or player_index is None):
        identity_reasons["owner_identity_incomplete"] = 1
    pairing_ok = pair_stats["identity_ok"] and not identity_reasons
    inputs = load_inputs(run_dir)
    steps = inputs.get("steps") or []
    exec_events = sorted(exec_records(data.client), key=lambda r: r.get("qpc_ns") or 0)
    anchors = {}
    if steps:
        by_label = {}
        for step in steps:
            if step.get("label"):
                by_label[step["label"]] = step.get("scheduled_ns")
        ordered = sorted(((k, v) for k, v in by_label.items() if v), key=lambda kv: kv[1])
        for i, (label, ts) in enumerate(ordered):
            end = ordered[i + 1][1] if i + 1 < len(ordered) else (ts + 2_000_000_000)
            anchors[label] = (ts, end)
    trajectory = trajectory_report(pairs, anchors, tolerance, pair_stats)
    guard = guard_summary(data.client)
    focus = focus_report(data.controller)
    phases = client_phases(data.client)
    corrections = server["corrections"]
    client_correction_events = client_corrections(data.client)
    relevant_corrections = corrections
    if player_index is not None:
        relevant_corrections = [c for c in relevant_corrections
                                if c.get("player_index") == player_index]
    if game_id is not None:
        relevant_corrections = [c for c in relevant_corrections
                                if c.get("game_id") == game_id]
    correction_evidence = correction_report(client_correction_events, relevant_corrections,
                                            segments, owner_epochs)
    independent_pairs, affected_pairs, unresolved_pairs, pair_reasons = \
        partition_correction_pairs(pairs, correction_evidence)
    trajectory["independent"] = pair_summary(independent_pairs)
    trajectory["corrected"] = pair_summary(affected_pairs)
    trajectory["correction_unresolved"] = pair_summary(unresolved_pairs)
    probe_cfg = inputs.get("probe") or {}
    probe_requested = bool(probe_cfg.get("enabled")) or any(
        step.get("trial") == "probe" for step in steps)
    disconnect_cfg = inputs.get("disconnect_release_check") or {}
    disconnect_requested = bool(disconnect_cfg.get("enabled"))
    modifier, learn_cmd_seqs = input_state_evidence(data.client, steps)
    learn_cmd_seqs = set(learn_cmd_seqs)
    learn_cmd_seqs.update(
        step["cmd_seq"] for step in steps
        if step.get("cmd_seq") is not None
        and (step.get("trial") == LEARN_TRIAL or step.get("label") == LEARN_LABEL))
    modifier["excluded_cmd_seqs"] = sorted(learn_cmd_seqs)
    disconnect = disconnect_evidence(data.controller, data.client)
    a_events = [e for e in exec_events if e.get("cmd_seq") not in learn_cmd_seqs]
    a_window, a_proven, a_downs, probe_execs = attribute_a_casts(
        sorted(server["casts"], key=lambda r: r["qpc_ns"] or 0), a_events, steps)
    a_motion = a_motion_report(a_window, pairs, correction_evidence, probe_execs)
    report = {
        "run_dir": run_dir,
        "run_id": data.run_id,
        "run_id_mismatches": data.run_id_mismatches,
        "forbidden_pose_fields": data.forbidden_pose_fields,
        "files": {
            "client": [os.path.basename(p) for p in data.client_files],
            "server": [os.path.basename(p) for p in data.server_files],
            "controller": os.path.basename(data.controller_path),
        },
        "identity": {
            "game_id": game_id,
            "player_index": player_index,
            "resolved": bool(pairing_ok),
            "reasons": sorted(identity_reasons),
            "reason_counts": identity_reasons,
            "run_id_evidence": data.identity_evidence(),
            "owner": {k: owner.get(k) for k in ("player_index", "user_id", "master_uid",
                                                "main_class", "sub_class", "is_bot", "game_id")}
            if owner else None,
            "main_class": owner.get("main_class") if owner else None,
            "sub_class": owner.get("sub_class") if owner else None,
            "class_by_slot": {str(slot): active_class(owner, slot) for slot in (0, 1)},
            "instance_epochs": epochs,
            "pairing_segments": [segment_summary(segment) for segment in segments],
        },
        "counts": {
            "client_records": len(data.client),
            "server_records": len(data.server),
            "client_samples": len(samples),
            "server_post_sim": len(server["post_sim"]),
            "paired": len(pairs),
            "paired_independent": len(independent_pairs),
            "paired_correction_affected": len(affected_pairs),
            "paired_correction_unresolved": len(unresolved_pairs),
            "identity_segments": pair_stats["identity_segments"],
            "resolved_segments": pair_stats["resolved_segments"],
            "cast_accept": len(server["casts"]),
            "skill_exec": len(server["skill_exec"]),
            "skill_command": len(server["skill_commands"]),
            "skill_done": len(server["skill_done"]),
            "position_correction": len(corrections),
            "client_correction": len(client_correction_events),
            "cmd_exec": len(exec_events),
            "input_state": modifier["events"],
            "disconnect_release": disconnect["events"],
            "stop_release": len([r for r in data.client if r.get("event") == "stop_release"]),
            "trace_overflow": len(server["trace_overflow"]) + len(
                [r for r in data.client if r.get("event") == "trace_overflow"]),
            "not_ready_events": len(phases["not_ready"]),
        },
        "trajectory": trajectory,
        "casts": [],
        "a_window": a_window,
        "a_motion": a_motion,
        "trace_overflow": server["trace_overflow"],
        "corrections": corrections,
        "client_corrections": client_correction_events,
        "correction_evidence": correction_evidence,
        "modifier_dispatch": modifier,
        "disconnect_release": disconnect,
        "focus": focus,
        "guard": guard,
        "client_phases": phases,
        "inputs": inputs,
        "exec_events": exec_events,
    }
    casts = sorted(server["casts"], key=lambda r: r["qpc_ns"] or 0)
    for cast in casts:
        skill_id = cast.get("skill_id")
        cast_ns = cast.get("qpc_ns") or 0
        cmd_count = len([c for c in server["skill_commands"]
                         if c.get("skill_id") == skill_id and c.get("game_id") == cast.get("game_id")
                         and (c.get("qpc_ns") or 0) >= cast_ns])
        done = [d for d in server["skill_done"] if d.get("skill_id") == skill_id
                and d.get("game_id") == cast.get("game_id")
                and abs((d.get("qpc_ns") or 0) - cast_ns) < 5_000_000_000]
        attribution, a_proven_flag = cast_attribution(cast, a_proven, a_downs, probe_execs)
        client_t = cast.get("client_t")
        sim_t = cast.get("sim_t")
        report["casts"].append({
            "qpc_ns": cast.get("qpc_ns"), "game_id": cast.get("game_id"), "skill_id": skill_id,
            "player_index": cast.get("player_index"), "actor_uid": cast.get("actor_uid"),
            "master_slot": cast.get("master_slot"), "action_state": cast.get("action_state"),
            "cast_pos": cast.get("cast_pos"), "caster_pos": cast.get("caster_pos"),
            "target_count": cast.get("target_count"),
            "server_minus_client_time_s": (sim_t - client_t)
            if (client_t is not None and sim_t is not None) else None,
            "follow_up_commands": cmd_count,
            "skill_done_reason": done[-1].get("reason") if done else None,
            "attribution": attribution,
            "a_proven": a_proven_flag,
        })
    forward_downs, forward_ups = forward_evidence(exec_events)
    stop = stop_evidence(data.client, exec_events)
    forbidden = bool(data.forbidden_pose_fields)
    independent_summary = trajectory["independent"]
    if forbidden:
        physics = "invalid_destpos_used_as_client_pose"
    elif independent_summary["samples"]:
        physics = ("matched_within_tolerance" if independent_summary["max_dist"] <= tolerance
                   else "diverged")
    elif unresolved_pairs:
        physics = "correction_evidence_unresolved"
    elif pairs:
        physics = "correction_only_agreement"
    else:
        physics = "no_paired_evidence"
    run_identity = data.identity_evidence()
    run_id_ok = (run_identity["client"]["consistent"] and run_identity["server"]["consistent"]
                 and (run_identity["controller"]["consistent"]
                      or not run_identity["controller"]["records"]))
    required = {
        "forward_cmd_exec": bool(forward_downs and forward_ups),
        "a_cmd_exec": bool(a_downs),
        "a_cast_accepted": bool(a_proven),
        "run_id_consistent": run_id_ok,
        "identity_matched_poses": bool(pairing_ok) and bool(independent_pairs) and not forbidden,
        "corrections_separate": correction_evidence["accounted"],
        "unfocused": focus["unfocused"],
        "guard_clean": guard["events"] > 0 and guard["mutations"] == 0
        and focus["guard_mutations"] == 0,
        "stop_release": bool(stop["event_count"] or stop["exec_releases"])
        and stop["forward_released"] and stop["reverse_order_ok"],
        "modifier_dispatch": (not probe_requested)
        or (modifier["learn_ok"] and modifier["quickslot_ok"]),
        "disconnect_release": (not disconnect_requested) or disconnect["verified"],
    }
    report["verdict"] = {
        "complete": all(required.values()),
        "required": required,
        "cast_proven": required["a_cast_accepted"],
        "cast_skills": sorted({c["skill_id"] for c in report["casts"]
                               if c["skill_id"] is not None}),
        "cast_a_attributed": len(a_proven),
        "cast_probe_derived": len([c for c in report["casts"] if c["attribution"] == "probe"]),
        "cast_unattributed": len([c for c in report["casts"]
                                  if c["attribution"] in ("unattributed", "after_a_down_unproven")]),
        "cast_follow_up_commands": sum(c["follow_up_commands"] for c in report["casts"]
                                       if c["a_proven"]),
        "a_window_tail_ok": all(w.get("tail_ok") for w in a_window["windows"]
                                if w.get("matched_cast_ns") is not None)
        if a_window["windows"] else None,
        "exec_events": len(exec_events),
        "exec_evidence": bool(exec_events),
        "physics_pairs": len(pairs),
        "physics_independent_pairs": len(independent_pairs),
        "physics_correction_affected_pairs": len(affected_pairs),
        "physics_correction_unresolved_pairs": len(unresolved_pairs),
        "correction_pair_reasons": pair_reasons,
        "physics": physics,
        "physics_independent": independent_summary,
        "physics_corrected": trajectory["corrected"],
        "first_divergence": trajectory["first_divergence"],
        "pair_dt_ms_abs_max": trajectory.get("dt_ms_abs_max"),
        "identity_resolved": bool(pairing_ok),
        "identity_reasons": sorted(identity_reasons),
        "corrections_separate_from_physics": correction_evidence["accounted"],
        "correction_evidence": correction_evidence,
        "corrections_count": len(relevant_corrections) + len(client_correction_events),
        "unfocused": focus["unfocused"],
        "focus_evidence": focus["evidence"],
        "guard_mutations": guard["mutations"],
        "run_id_consistent": run_id_ok,
        "run_id_evidence": run_identity,
        "client_final_phase": phases["final_phase"],
        "stop_release": stop,
        "probe_requested": probe_requested,
        "disconnect_requested": disconnect_requested,
        "modifier_dispatch": {
            "required": probe_requested,
            "learn_ok": modifier["learn_ok"],
            "quickslot_ok": modifier["quickslot_ok"],
            "ctrl_release_observed": modifier["ctrl_release_observed"],
            "learn_events": modifier["learn_events"],
            "quickslot_events": modifier["quickslot_events"],
            "excluded_cmd_seqs": modifier["excluded_cmd_seqs"],
            "held_input_skillup_1": HELD_INPUT_SKILLUP_1,
            "held_input_quickslot_1": HELD_INPUT_QUICKSLOT_1,
        },
        "disconnect_release": disconnect,
    }
    return report


def summarize(report):
    lines = []
    verdict = report["verdict"]
    identity = report["identity"]
    lines.append("run_id=%s game_id=%s player_index=%s owner=%s" % (
        report["run_id"], identity["game_id"], identity["player_index"], identity["owner"]))
    lines.append("counts=%s" % json.dumps(report["counts"], sort_keys=True))
    lines.append("complete=%s required=%s" % (
        verdict["complete"], json.dumps(verdict["required"], sort_keys=True)))
    lines.append("casts=%d a_attributed=%d probe=%d skills=%s follow_up_cmds=%d exec_events=%d" % (
        len(report["casts"]), verdict["cast_a_attributed"], verdict["cast_probe_derived"],
        verdict["cast_skills"], verdict["cast_follow_up_commands"], verdict["exec_events"]))
    for cast in report["casts"]:
        lines.append("  cast skill=%s slot=%s t=%s attribution=%s clock_delta_s=%s done=%s cmds=%d" % (
            cast["skill_id"], cast["master_slot"], cast["qpc_ns"], cast["attribution"],
            cast["server_minus_client_time_s"], cast["skill_done_reason"],
            cast["follow_up_commands"]))
    traj = report["trajectory"]
    indep = traj.get("independent") or {}
    lines.append("physics=%s pairs=%s independent_pairs=%s max_dist=%s mean_dist=%s "
                 "dt_ms_abs_max=%s corrected_pairs=%s corrected_max_dist=%s" % (
                     verdict["physics"], traj["samples"], indep.get("samples"),
                     indep.get("max_dist"), indep.get("mean_dist"), indep.get("dt_ms_abs_max"),
                     (traj.get("corrected") or {}).get("samples"),
                     (traj.get("corrected") or {}).get("max_dist")))
    for name, seg in traj.get("segments", {}).items():
        lines.append(
            "  segment %s: client_disp=%.2f server_disp=%.2f max_dist=%.2f samples=%d" % (
                name, seg["client_displacement"], seg["server_displacement"], seg["max_dist"],
                seg["samples"]))
    if traj["first_divergence"]:
        fd = traj["first_divergence"]
        lines.append("  first_divergence dist=%.2f > %.2f at t=%s" % (
            fd["dist"], fd["tolerance"], fd["qpc_ns"]))
    motion = report.get("a_motion") or {}
    lines.append("a_motion proven_windows=%s skipped=%s pairs=%s" % (
        motion.get("proven_windows"), motion.get("skipped"), motion.get("pair_count")))
    for window in motion.get("windows", []):
        correction = window["correction"]
        lines.append(
            "  a_motion window cmd_seq=%s down_ns=%s cast_ns=%s interval=%s..%s clip_ns=%s "
            "tail_ok=%s tail_probes=%s pairs=%s blocks=%s max_dist=%s mean_dist=%s "
            "dt_abs_max=%s independent=%s affected=%s unresolved=%s reasons=%s" % (
                window["cmd_seq"], window["a_down_ns"], window["matched_cast_ns"],
                window["interval_start_ns"], window["interval_end_ns"], window["probe_clip_ns"],
                window["tail_ok"], window["tail_probe_execs"], window["pair_count"],
                window["block_count"], window["max_dist"], window["mean_dist"],
                window["dt_ms_abs_max"], correction["independent"], correction["affected"],
                correction["unresolved"], json.dumps(correction["reasons"], sort_keys=True)))
        for block in window["blocks"]:
            lines.append(
                "    a_motion block seg=%s uid=%s slot=%s class=%s pairs=%s t=%s..%s "
                "client_disp=%s len=%.2f server_disp=%s len=%.2f disagreement=%s len=%.2f "
                "len_delta=%.2f max_dist=%.2f mean_dist=%.2f dt_abs_max=%.2f "
                "independent=%s affected=%s unresolved=%s" % (
                    block["segment_index"], block["actor_uid"], block["master_slot"],
                    block["expected_class"], block["pair_count"], block["t_start_ns"],
                    block["t_end_ns"], format_vec(block["client_displacement"]),
                    block["client_displacement_dist"], format_vec(block["server_displacement"]),
                    block["server_displacement_dist"],
                    format_vec(block["endpoint_disagreement"]),
                    block["endpoint_disagreement_dist"], block["endpoint_length_delta"],
                    block["max_dist"], block["mean_dist"], block["dt_ms_abs_max"],
                    block["correction"]["independent"], block["correction"]["affected"],
                    block["correction"]["unresolved"]))
    stop = verdict["stop_release"]
    lines.append("stop released=%s forward_released=%s reverse_order_ok=%s" % (
        stop["released"], stop["forward_released"], stop["reverse_order_ok"]))
    lines.append("corrections=%d accounted=%s matched_server=%s unmatched_server=%s "
                 "client_unresolved=%s affected_pairs=%s unresolved_pairs=%s reasons=%s" % (
                     verdict["corrections_count"],
                     verdict["corrections_separate_from_physics"],
                     verdict["correction_evidence"]["matched_server_events"],
                     len(verdict["correction_evidence"]["unmatched_server_events"]),
                     verdict["correction_evidence"]["client_unresolved"],
                     verdict["physics_correction_affected_pairs"],
                     verdict["physics_correction_unresolved_pairs"],
                     json.dumps(verdict["correction_pair_reasons"], sort_keys=True)))
    modifier = verdict["modifier_dispatch"]
    disconnect = verdict["disconnect_release"]
    lines.append(
        "modifier_dispatch required=%s learn_ok=%s quickslot_ok=%s ctrl_release=%s "
        "input_states=%d disconnect_required=%s verified=%s released=%s release_qpc_ns=%s "
        "cancelled_pending=%s continued_after_pending=%s input_released=%s" % (
            modifier["required"], modifier["learn_ok"], modifier["quickslot_ok"],
            modifier["ctrl_release_observed"], report["counts"]["input_state"],
            verdict["disconnect_requested"], disconnect["verified"], disconnect["released"],
            disconnect["release_qpc_ns"], disconnect["cancelled_pending"],
            disconnect["continued_after_pending"], disconnect["input_released"]))
    guard = report["guard"]
    lines.append("guard os_guard_events=%d blocked=%d mutations=%d apis=%s" % (
        guard["events"], guard["blocked"], guard["mutations"], sorted(guard["apis"])))
    lines.append("client_final_phase=%s not_ready=%d errors=%d" % (
        verdict["client_final_phase"], len(report["client_phases"]["not_ready"]),
        len(report["client_phases"]["errors"])))
    focus = report["focus"]
    lines.append(
        "focus unfocused=%s transitions=%d client_focus=%d owned_focus=%d cursor_motions=%d aborts=%d"
        % (verdict["unfocused"], focus["foreground_transitions"], focus["client_focus_events"],
           focus["owned_focus_events"], focus["cursor_motions"], len(focus["aborts"])))
    lines.append("run_id_consistent=%s identity_resolved=%s identity_reasons=%s "
                 "forbidden_pose_fields=%d" % (
                     verdict["run_id_consistent"], verdict["identity_resolved"],
                     verdict["identity_reasons"], len(report["forbidden_pose_fields"])))
    return "\n".join(lines)


def main(argv=None):
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-dir", required=True)
    parser.add_argument("--tolerance", type=float, default=25.0)
    parser.add_argument("--out", default=None)
    args = parser.parse_args(argv)
    report = build_report(args.run_dir, tolerance=args.tolerance)
    out = args.out or os.path.join(args.run_dir, "compare.json")
    write_json(out, report)
    print(summarize(report))
    print("compare_json=%s" % out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
