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


def trajectories(posts, raw_samples):
    samples = compare.parse_client_samples(list(raw_samples), phase="post")
    pairs, _, segments = compare.pair_trajectories(
        samples, posts, OWNER, EPOCHS, None, GAME_ID, PLAYER_INDEX)
    return pairs, segments


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


if __name__ == "__main__":
    unittest.main()
