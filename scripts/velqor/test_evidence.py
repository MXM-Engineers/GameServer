import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from scripts.velqor import common, compare

GAME_ID = 7
PLAYER_INDEX = 0
OWNER = {"main_class": 3, "sub_class": 4}
EPOCHS = [{"game_id": GAME_ID, "start_ns": 0, "end_ns": None, "index": 0}]
ACTOR_A = 11
ACTOR_B = 22
LOCAL_A = 1001
LOCAL_B = 2002
LOCAL_C = 3003
MS = 1_000_000


def post_record(qpc_ns, actor_uid, master_slot):
    return {"qpc_ns": qpc_ns, "game_id": GAME_ID, "player_index": PLAYER_INDEX,
            "actor_uid": actor_uid, "master_slot": master_slot, "pos": [0.0, 0.0, 0.0],
            "sim_t": qpc_ns / 1e9, "update_seq": 1, "grounded": True, "move_speed": 0.0}


def sample_record(qpc_ns, local_actor_id, class_type):
    return {"event": "sample", "phase": "post", "qpc_ns": qpc_ns,
            "local_actor_id": local_actor_id, "class_type": class_type,
            "frame": 1, "feet": [0.0, 0.0, 0.0]}


def correction_record(qpc_ns, local_actor_id, observation="post_handler", frame=1,
                      feet=(1.0, 2.0, 3.0), before_feet=(0.0, 0.0, 0.0)):
    return {"event": "correction", "observation": observation, "qpc_ns": qpc_ns,
            "local_actor_id": local_actor_id, "frame": frame,
            "feet": list(feet), "before_feet": list(before_feet),
            "visual": [0.0, 0.0, 0.0]}


def tag_record(qpc_ns, actor_uid, main_chara_id, class_type):
    return {"event": "tag_apply", "qpc_ns": qpc_ns, "game_id": GAME_ID,
            "player_index": PLAYER_INDEX, "actor_uid": actor_uid,
            "main_chara_id": main_chara_id, "class_type": class_type}


def trajectories(posts, raw_samples, tags=None):
    pairs, _, segments = trajectory_evidence(posts, raw_samples, tags)
    return pairs, segments


def trajectory_evidence(posts, raw_samples, tags=None):
    samples = compare.parse_client_samples(list(raw_samples), phase="post")
    return compare.pair_trajectories(
        samples, posts, OWNER, EPOCHS, None, GAME_ID, PLAYER_INDEX,
        tag_applies=tags)


class NdjsonTailTest(unittest.TestCase):

    def test_fragmented_records_delivered_exactly_once(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, "client_0.ndjson")
            tail = common.NdjsonTail(os.path.join(tmp, "client_*.ndjson"))
            delivered = []
            with open(path, "wb") as f:
                f.write(b'{"event":"first"}\n{"event":"second"')
            delivered.extend(tail.poll())
            self.assertEqual(tail.poll(), [])
            with open(path, "ab") as f:
                f.write(b'}\n{"event":"third"')
            delivered.extend(tail.poll())
            self.assertEqual(tail.poll(), [])
            with open(path, "ab") as f:
                f.write(b'}\n')
            delivered.extend(tail.poll())
            self.assertEqual(tail.poll(), [])
            self.assertEqual([rec["event"] for rec in delivered],
                             ["first", "second", "third"])


class StopEvidenceTest(unittest.TestCase):

    def _client_records(self, second_release):
        return [
            {"event": "cmd_exec", "op": "key", "vk": compare.FORWARD_VK, "down": 1,
             "qpc_ns": 10},
            {"event": "cmd_exec", "op": "key", "vk": compare.VK_CONTROL, "down": 1,
             "qpc_ns": 20},
            {"event": "cmd_exec", "op": "key", "vk": compare.A_VK, "down": 1, "qpc_ns": 30},
            {"event": "stop_release", "qpc_ns": 40,
             "released": [compare.A_VK, compare.VK_CONTROL, compare.FORWARD_VK]},
            {"event": "cmd_exec", "op": "key", "vk": compare.VK_CONTROL, "down": 1,
             "qpc_ns": 50},
            {"event": "cmd_exec", "op": "key", "vk": compare.A_VK, "down": 1, "qpc_ns": 60},
            {"event": "stop_release", "qpc_ns": 70, "released": second_release},
        ]

    def test_release_batches_use_their_own_held_stack(self):
        stop = compare.stop_evidence(
            self._client_records([compare.A_VK, compare.VK_CONTROL]), [])
        self.assertEqual(stop["event_count"], 2)
        first, second = stop["release_checks"]
        self.assertEqual(first["held"], [compare.FORWARD_VK, compare.VK_CONTROL, compare.A_VK])
        self.assertEqual(first["expected"], [compare.A_VK, compare.VK_CONTROL, compare.FORWARD_VK])
        self.assertTrue(first["ok"])
        self.assertEqual(second["held"], [compare.VK_CONTROL, compare.A_VK])
        self.assertEqual(second["expected"], [compare.A_VK, compare.VK_CONTROL])
        self.assertTrue(second["ok"])
        self.assertTrue(stop["reverse_order_ok"])
        self.assertTrue(stop["forward_released"])

    def test_non_reverse_release_order_fails(self):
        stop = compare.stop_evidence(
            self._client_records([compare.VK_CONTROL, compare.A_VK]), [])
        first, second = stop["release_checks"]
        self.assertTrue(first["ok"])
        self.assertEqual(second["held"], [compare.VK_CONTROL, compare.A_VK])
        self.assertFalse(second["ok"])
        self.assertFalse(stop["reverse_order_ok"])


class TrajectorySegmentTest(unittest.TestCase):

    def test_identity_round_trip_is_not_bridged(self):
        posts = [post_record(0, ACTOR_A, 0), post_record(100 * MS, ACTOR_A, 0),
                 post_record(200 * MS, ACTOR_B, 1), post_record(300 * MS, ACTOR_A, 0)]
        epochs = {id(p): 0 for p in posts}
        segments = compare.active_owner_segments(posts, epochs)
        self.assertEqual([s["actor_uid"] for s in segments], [ACTOR_A, ACTOR_B, ACTOR_A])
        self.assertEqual([s["transition_in"] for s in segments],
                         ["start", "identity", "identity"])
        self.assertEqual([p["qpc_ns"] for p in segments[0]["posts"]], [0, 100 * MS])
        self.assertEqual([p["qpc_ns"] for p in segments[2]["posts"]], [300 * MS])


class CorrectionInfluenceTest(unittest.TestCase):

    def test_influence_persists_beyond_contiguity_window(self):
        posts = [post_record(0, ACTOR_A, 0), post_record(400 * MS, ACTOR_A, 0),
                 post_record(700 * MS, ACTOR_A, 0)]
        pairs, segments = trajectories(posts, [
            sample_record(60 * MS, LOCAL_A, 3),
            sample_record(700 * MS, LOCAL_A, 3),
        ])
        evidence = compare.correction_report([correction_record(50 * MS, LOCAL_A)], [],
                                             segments, EPOCHS)
        independent, affected, unresolved, _ = \
            compare.partition_correction_pairs(pairs, evidence)
        self.assertEqual(len(segments), 1)
        self.assertEqual(evidence["segments"][0]["influence_ns"], 50 * MS)
        self.assertEqual(independent, [])
        self.assertEqual(unresolved, [])
        self.assertEqual(sorted(p["qpc_ns"] for p in affected), [60 * MS, 700 * MS])

    def test_influence_does_not_leak_across_identity_round_trip(self):
        posts = [post_record(0, ACTOR_A, 0), post_record(100 * MS, ACTOR_A, 0),
                 post_record(200 * MS, ACTOR_B, 1), post_record(300 * MS, ACTOR_B, 1),
                 post_record(700 * MS, ACTOR_A, 0)]
        pairs, segments = trajectories(posts, [
            sample_record(60 * MS, LOCAL_A, 3),
            sample_record(250 * MS, LOCAL_B, 4),
            sample_record(700 * MS, LOCAL_A, 3),
        ])
        evidence = compare.correction_report([correction_record(50 * MS, LOCAL_A)], [],
                                             segments, EPOCHS)
        independent, affected, unresolved, _ = \
            compare.partition_correction_pairs(pairs, evidence)
        self.assertEqual([s["actor_uid"] for s in segments], [ACTOR_A, ACTOR_B, ACTOR_A])
        self.assertEqual([p["qpc_ns"] for p in affected], [60 * MS])
        self.assertEqual([p["qpc_ns"] for p in independent], [250 * MS, 700 * MS])
        self.assertEqual(unresolved, [])

    def test_legacy_and_malformed_corrections_stay_unresolved(self):
        posts = [post_record(0, ACTOR_A, 0), post_record(100 * MS, ACTOR_A, 0)]
        pairs, segments = trajectories(posts, [sample_record(60 * MS, LOCAL_A, 3)])
        events = [correction_record(50 * MS, LOCAL_A, observation="pre_handler"),
                  correction_record(70 * MS, LOCAL_A, frame=None, feet=(1.0, 2.0))]
        evidence = compare.correction_report(events, [], segments, EPOCHS)
        independent, affected, unresolved, reasons = \
            compare.partition_correction_pairs(pairs, evidence)
        self.assertFalse(evidence["accounted"])
        self.assertEqual(evidence["client_unresolved"], 2)
        self.assertEqual(unresolved, pairs)
        self.assertEqual(reasons, {"observation_not_post_handler": 1})
        self.assertEqual(independent, [])
        self.assertEqual(affected, [])


class AMotionBoundaryTest(unittest.TestCase):

    def _posts(self, entries):
        posts = []
        for qpc, actor_uid, master_slot, x in entries:
            post = post_record(qpc, actor_uid, master_slot)
            post["pos"] = [float(x), 0.0, 0.0]
            posts.append(post)
        return posts

    def _samples(self, entries):
        samples = []
        for qpc, local_actor_id, class_type, z in entries:
            sample = sample_record(qpc, local_actor_id, class_type)
            sample["feet"] = [0.0, 0.0, float(z)]
            samples.append(sample)
        return samples

    def _window(self, down_ns, cast_ns, tail_end_ns, tail_ok=True, tail_probes=0):
        return {"windows": [{
            "cmd_seq": 3, "at_ms": 1, "a_down_ns": down_ns, "matched_cast_ns": cast_ns,
            "cast_skill_id": compare.A_SKILL_ID, "a_proven": True,
            "probe_contaminated": False, "probe_execs_before_cast": 0,
            "tail_end_ns": tail_end_ns, "tail_ok": tail_ok,
            "tail_probe_execs": tail_probes}]}

    def test_window_excludes_pairs_outside_a_interval(self):
        ramp = [(index * 400 * MS, ACTOR_A, 0, 378.0 * (index - 1) / 10.0)
                for index in range(0, 13)]
        posts = self._posts(ramp)
        samples = self._samples([(qpc, LOCAL_A, 3, 0.0) for qpc, _, _, _ in ramp])
        pairs, segments = trajectories(posts, samples)
        evidence = compare.correction_report([], [], segments, EPOCHS)
        motion = compare.a_motion_report(self._window(400 * MS, 400 * MS, 4400 * MS),
                                         pairs, evidence, [])
        window = motion["windows"][0]
        self.assertEqual(window["pair_count"], 11)
        self.assertEqual(window["block_count"], 1)
        block = window["blocks"][0]
        self.assertEqual(block["t_start_ns"], 400 * MS)
        self.assertEqual(block["t_end_ns"], 4400 * MS)
        self.assertAlmostEqual(block["client_displacement_dist"], 0.0, places=6)
        self.assertAlmostEqual(block["server_displacement_dist"], 378.0, places=6)
        self.assertAlmostEqual(block["endpoint_disagreement"][0], -378.0, places=6)
        self.assertAlmostEqual(block["endpoint_disagreement_dist"], 378.0, places=6)
        self.assertAlmostEqual(block["endpoint_length_delta"], 378.0, places=6)
        self.assertAlmostEqual(block["max_dist"], 378.0, places=6)
        self.assertAlmostEqual(block["mean_dist"], 189.0, places=6)
        self.assertEqual(window["correction"]["independent"], 11)
        self.assertEqual(window["correction"]["affected"], 0)

    def test_window_clips_at_tail_probe_execution(self):
        ramp = [(qpc * MS, ACTOR_A, 0, float(index))
                for index, qpc in enumerate((0, 400, 800, 1000, 1200))]
        posts = self._posts(ramp)
        samples = self._samples([(qpc * MS, LOCAL_A, 3, 0.0)
                                 for qpc in (0, 400, 800, 990, 1000, 1200)])
        pairs, segments = trajectories(posts, samples)
        evidence = compare.correction_report([], [], segments, EPOCHS)
        probe_execs = [{"qpc_ns": 1000 * MS, "cmd_seq": 9, "op": "key", "vk": 1, "down": 1}]
        motion = compare.a_motion_report(
            self._window(0, 0, 4800 * MS, tail_ok=False, tail_probes=1),
            pairs, evidence, probe_execs)
        window = motion["windows"][0]
        self.assertEqual(window["probe_clip_ns"], 1000 * MS)
        self.assertEqual(window["interval_end_ns"], 1000 * MS)
        self.assertEqual(window["pair_count"], 3)
        self.assertEqual(window["blocks"][0]["t_end_ns"], 800 * MS)

    def test_window_splits_at_identity_change(self):
        posts = self._posts([(0, ACTOR_A, 0, 0.0), (400 * MS, ACTOR_A, 0, 100.0),
                             (2000 * MS, ACTOR_B, 1, 500.0), (2400 * MS, ACTOR_B, 1, 900.0)])
        samples = self._samples([(0, LOCAL_A, 3, 0.0), (400 * MS, LOCAL_A, 3, 0.0),
                                 (2000 * MS, LOCAL_B, 4, 10.0), (2400 * MS, LOCAL_B, 4, 10.0)])
        pairs, segments = trajectories(posts, samples)
        evidence = compare.correction_report([], [], segments, EPOCHS)
        motion = compare.a_motion_report(self._window(0, 0, 4000 * MS), pairs, evidence, [])
        window = motion["windows"][0]
        self.assertEqual(window["block_count"], 2)
        first, second = window["blocks"]
        self.assertEqual(first["actor_uid"], ACTOR_A)
        self.assertEqual(second["actor_uid"], ACTOR_B)
        self.assertAlmostEqual(first["server_displacement_dist"], 100.0, places=6)
        self.assertAlmostEqual(second["server_displacement_dist"], 400.0, places=6)
        self.assertEqual(second["client_start"], [0.0, 0.0, 10.0])
        self.assertAlmostEqual(second["client_displacement_dist"], 0.0, places=6)

    def test_window_carries_correction_classification(self):
        posts = self._posts([(0, ACTOR_A, 0, 0.0), (400 * MS, ACTOR_A, 0, 1.0),
                             (800 * MS, ACTOR_A, 0, 2.0), (1200 * MS, ACTOR_A, 0, 3.0)])
        samples = self._samples([(0, LOCAL_A, 3, 0.0), (400 * MS, LOCAL_A, 3, 0.0),
                                 (800 * MS, LOCAL_A, 3, 0.0), (1200 * MS, LOCAL_A, 3, 0.0)])
        pairs, segments = trajectories(posts, samples)
        evidence = compare.correction_report([correction_record(700 * MS, LOCAL_A)], [],
                                             segments, EPOCHS)
        motion = compare.a_motion_report(self._window(0, 0, 1200 * MS), pairs, evidence, [])
        window = motion["windows"][0]
        self.assertEqual(window["correction"]["independent"], 2)
        self.assertEqual(window["correction"]["affected"], 2)
        self.assertEqual(window["correction"]["unresolved"], 0)
        block = window["blocks"][0]
        self.assertEqual(block["correction"]["independent"], 2)
        self.assertEqual(block["correction"]["affected"], 2)


class TagAckValidationTest(unittest.TestCase):

    def _posts(self):
        return [post_record(0, ACTOR_A, 0), post_record(100 * MS, ACTOR_A, 0),
                post_record(200 * MS, ACTOR_B, 1), post_record(300 * MS, ACTOR_B, 1),
                post_record(400 * MS, ACTOR_B, 1)]

    def test_delayed_ack_retains_old_and_new_rows(self):
        tags = [tag_record(150 * MS, ACTOR_B, 1, 4)]
        samples = [sample_record(60 * MS, LOCAL_A, 3),
                   sample_record(210 * MS, LOCAL_A, 3),
                   sample_record(230 * MS, LOCAL_A, 3),
                   sample_record(260 * MS, LOCAL_B, 4),
                   sample_record(300 * MS, LOCAL_B, 4)]
        for sample in samples:
            if sample["local_actor_id"] == LOCAL_A and sample["qpc_ns"] >= 200 * MS:
                sample["feet"] = [50.0, 0.0, 0.0]
        pairs, stats, segments = trajectory_evidence(self._posts(), samples, tags)
        self.assertEqual([p["qpc_ns"] for p in pairs],
                         [60 * MS, 210 * MS, 230 * MS, 260 * MS, 300 * MS])
        self.assertEqual(stats["reasons"], {})
        self.assertTrue(stats["identity_ok"])
        pending = [p for p in pairs if p["pending_ack"]]
        self.assertEqual([p["qpc_ns"] for p in pending], [210 * MS, 230 * MS])
        for pair in pending:
            self.assertEqual(pair["client_actor_id"], LOCAL_A)
            self.assertEqual(pair["client_class_type"], 3)
            self.assertEqual(pair["client_old_actor_id"], LOCAL_A)
            self.assertEqual(pair["client_old_class_type"], 3)
            self.assertEqual(pair["server_new_actor_uid"], ACTOR_B)
            self.assertEqual(pair["server_new_class_type"], 4)
            self.assertEqual(pair["tag_qpc_ns"], 150 * MS)
            self.assertEqual(pair["tag_ack_qpc_ns"], 260 * MS)
            self.assertEqual(pair["dist"], 50.0)
        self.assertFalse(pairs[0]["pending_ack"])
        self.assertEqual(pairs[0]["identity_role"], "ack")
        self.assertEqual(segments[1]["client_identity"],
                         {"local_actor_id": LOCAL_B, "class_type": 4})
        ack = segments[1]["tag_ack"]
        self.assertEqual(ack["server_qpc_ns"], 150 * MS)
        self.assertEqual(ack["ack_qpc_ns"], 260 * MS)
        self.assertEqual(ack["client_old_identity"],
                         {"local_actor_id": LOCAL_A, "class_type": 3})
        self.assertEqual(ack["client_new_identity"],
                         {"local_actor_id": LOCAL_B, "class_type": 4})
        self.assertEqual(ack["transition_start_ns"], 150 * MS)
        self.assertEqual(ack["transition_end_ns"], 260 * MS)
        self.assertEqual(ack["pending_samples"], 2)
        report = compare.trajectory_report(pairs, {}, 25.0)
        self.assertEqual(report["max_dist"], 50.0)
        self.assertEqual(report["first_divergence"]["qpc_ns"], 210 * MS)

    def test_unknown_identity_still_fails(self):
        tags = [tag_record(150 * MS, ACTOR_B, 1, 4)]
        samples = [sample_record(60 * MS, LOCAL_A, 3),
                   sample_record(210 * MS, LOCAL_A, 3),
                   sample_record(230 * MS, LOCAL_C, 7),
                   sample_record(260 * MS, LOCAL_B, 4)]
        pairs, stats, segments = trajectory_evidence(self._posts(), samples, tags)
        self.assertEqual(stats["reasons"], {"client_class_mismatch": 1})
        self.assertFalse(stats["identity_ok"])
        qpcs = [p["qpc_ns"] for p in pairs]
        self.assertIn(210 * MS, qpcs)
        self.assertNotIn(230 * MS, qpcs)

    def test_missing_ack_fails(self):
        tags = [tag_record(150 * MS, ACTOR_B, 1, 4)]
        samples = [sample_record(60 * MS, LOCAL_A, 3),
                   sample_record(210 * MS, LOCAL_A, 3)]
        pairs, stats, segments = trajectory_evidence(self._posts(), samples, tags)
        self.assertEqual(stats["reasons"].get("missing_tag_ack"), 1)
        self.assertFalse(stats["identity_ok"])
        self.assertIn(210 * MS, [p["qpc_ns"] for p in pairs])
        self.assertIsNone(segments[1]["client_identity"])

    def test_wrong_local_actor_fails(self):
        tags = [tag_record(150 * MS, ACTOR_B, 1, 4)]
        samples = [sample_record(60 * MS, LOCAL_A, 3),
                   sample_record(210 * MS, LOCAL_A, 3),
                   sample_record(230 * MS, LOCAL_A, 4),
                   sample_record(260 * MS, LOCAL_B, 4)]
        pairs, stats, segments = trajectory_evidence(self._posts(), samples, tags)
        self.assertEqual(stats["reasons"].get("client_local_actor_mismatch"), 1)
        self.assertFalse(stats["identity_ok"])
        self.assertNotIn(230 * MS, [p["qpc_ns"] for p in pairs])

    def _round_trip(self):
        return [post_record(0, ACTOR_A, 0), post_record(100 * MS, ACTOR_A, 0),
                post_record(200 * MS, ACTOR_B, 1), post_record(300 * MS, ACTOR_B, 1),
                post_record(800 * MS, ACTOR_A, 0), post_record(900 * MS, ACTOR_A, 0)]

    def test_round_trip_tag_does_not_reuse_wrong_segment(self):
        tags = [tag_record(150 * MS, ACTOR_B, 1, 4), tag_record(500 * MS, ACTOR_A, 0, 3)]
        samples = [sample_record(60 * MS, LOCAL_A, 3),
                   sample_record(210 * MS, LOCAL_A, 3),
                   sample_record(260 * MS, LOCAL_B, 4),
                   sample_record(810 * MS, LOCAL_A, 3)]
        pairs, stats, segments = trajectory_evidence(self._round_trip(), samples, tags)
        self.assertEqual([s["actor_uid"] for s in segments], [ACTOR_A, ACTOR_B, ACTOR_A])
        self.assertEqual(segments[1]["tag_ack"]["server_qpc_ns"], 150 * MS)
        self.assertEqual(segments[2]["tag_ack"]["server_qpc_ns"], 500 * MS)
        self.assertEqual(stats["reasons"], {})
        self.assertTrue(stats["identity_ok"])

    def test_identity_change_without_tag_is_unrelated(self):
        tags = [tag_record(150 * MS, ACTOR_B, 1, 4)]
        samples = [sample_record(60 * MS, LOCAL_A, 3),
                   sample_record(210 * MS, LOCAL_A, 3),
                   sample_record(260 * MS, LOCAL_B, 4),
                   sample_record(810 * MS, LOCAL_A, 3)]
        pairs, stats, segments = trajectory_evidence(self._round_trip(), samples, tags)
        self.assertEqual(stats["reasons"].get("unrelated_uid_change"), 1)
        self.assertFalse(stats["identity_ok"])

    def test_tag_does_not_bridge_a_post_gap(self):
        posts = [post_record(0, ACTOR_A, 0), post_record(100 * MS, ACTOR_A, 0),
                 post_record(800 * MS, ACTOR_B, 1), post_record(900 * MS, ACTOR_B, 1)]
        tags = [tag_record(750 * MS, ACTOR_B, 1, 4)]
        samples = [sample_record(60 * MS, LOCAL_A, 3),
                   sample_record(810 * MS, LOCAL_A, 3),
                   sample_record(860 * MS, LOCAL_B, 4)]
        pairs, stats, segments = trajectory_evidence(posts, samples, tags)
        self.assertFalse(stats["identity_ok"])
        self.assertFalse(any(pair["pending_ack"] for pair in pairs))

    def test_tag_does_not_bridge_instance_epochs(self):
        epochs = [{"game_id": GAME_ID, "start_ns": 0, "end_ns": 150 * MS, "index": 0},
                  {"game_id": GAME_ID, "start_ns": 151 * MS, "end_ns": None, "index": 1}]
        samples = compare.parse_client_samples([
            sample_record(60 * MS, LOCAL_A, 3),
            sample_record(210 * MS, LOCAL_A, 3),
            sample_record(260 * MS, LOCAL_B, 4)], phase="post")
        pairs, stats, segments = compare.pair_trajectories(
            samples, self._posts(), OWNER, epochs, None, GAME_ID, PLAYER_INDEX,
            tag_applies=[tag_record(180 * MS, ACTOR_B, 1, 4)])
        self.assertFalse(stats["identity_ok"])
        self.assertFalse(any(pair["pending_ack"] for pair in pairs))


if __name__ == "__main__":
    unittest.main()
