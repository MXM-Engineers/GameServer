import argparse
from dataclasses import dataclass
from decimal import Decimal, InvalidOperation
from pathlib import Path
import xml.etree.ElementTree as ET


REPO = Path(__file__).resolve().parents[1]
LABELED = ("MoveType", "CurrentScene", "Stance", "PrevActionState", "Status", "Random", "VariationType", "Speed")
UNLABELED = ("ActionState", "Group", "Parallel", "LoopPlay")
MAX_SAMPLES = 32


def number(text):
    try:
        value = Decimal(text)
    except (InvalidOperation, TypeError) as error:
        raise ValueError(f"Invalid numeric value: {text!r}") from error
    if not value.is_finite():
        raise ValueError(f"Nonfinite numeric value: {text!r}")
    return value


def numeric_text(value):
    text = format(value, "f")
    return text.rstrip("0").rstrip(".") if "." in text else text


def resolve_case(root, relative):
    path = root
    for part in relative.replace("\\", "/").split("/"):
        if part in ("", ".", ".."):
            raise ValueError(f"Invalid asset path: {relative}")
        matches = sorted(p for p in path.iterdir() if p.name.casefold() == part.casefold())
        if len(matches) != 1:
            raise ValueError(f"Expected one case-insensitive match for {path / part}: {matches}")
        path = matches[0]
    return path


def hy_count(attrib):
    count = 0
    while f"H_Y{count}" in attrib:
        count += 1
    extra = [key for key in attrib if key.startswith("H_Y") and key[3:].isdigit() and int(key[3:]) >= count]
    if extra:
        raise ValueError(f"Gapped H_Y attributes: {sorted(extra)}")
    return count


@dataclass(frozen=True)
class Curve:
    duration: Decimal
    samples: tuple
    animation: int
    branch: str
    duration_source: str
    h_time: Decimal
    horizon_rotate: Decimal

    def numeric_motion(self):
        return self.duration, self.samples


def horz_curves(node, sequences, branch=""):
    kind = node.get("Type")
    children = node.findall("StatePlay")
    if kind == "Animation":
        count = hy_count(node.attrib)
        nested = any(hy_count(child.attrib) for child in node.iter() if child is not node)
        if count == 0:
            if nested:
                raise ValueError(f"Nested horizontal motion in animation {node.get('AnimationIndex')}")
            return []
        if nested:
            raise ValueError(f"Nested horizontal motion in animation {node.attrib.get('AnimationIndex')}")
        if count > MAX_SAMPLES:
            raise ValueError(f"Too many H_Y samples for animation {node.get('AnimationIndex')}: {count}")
        random_value = node.get("H_Random")
        if random_value is not None and number(random_value) != 0:
            raise ValueError(f"Nonzero H_Random for animation {node.get('AnimationIndex')}")
        animation = int(node.attrib["AnimationIndex"])
        samples = tuple(number(node.get(f"H_Y{i}")) for i in range(count))
        h_time = number(node.get("H_Time", "0"))
        if h_time < 0:
            raise ValueError(f"Negative H_Time for animation {animation}")
        duration_source = "H_Time" if h_time > 0 else "AniLength"
        duration = h_time if h_time > 0 else number(sequences[animation].get("_SeqTime"))
        if duration <= 0:
            raise ValueError(f"Nonpositive duration for animation {animation}")
        rotate = number(node.get("HorizonRotate", "0"))
        return [Curve(duration, samples, animation, branch, duration_source, h_time, rotate)]
    if kind in LABELED:
        labels = [node.get(f"CaseValue{i}") for i in range(len(children))]
        case_keys = {key for key in node.attrib if key.startswith("CaseValue")}
        if any(label is None for label in labels) or len(set(labels)) != len(labels) or len(case_keys) != len(children):
            raise ValueError(f"Invalid {kind} routing: {node.attrib}")
        result = []
        for label, child in zip(labels, children):
            child_branch = "/".join(filter(None, (branch, f"{kind}={label}")))
            result.extend(horz_curves(child, sequences, child_branch))
        return result
    if kind not in UNLABELED:
        if any(hy_count(child.attrib) for child in node.iter()):
            raise ValueError(f"Unsupported motion container {kind}: {node.attrib}")
        return []
    result = []
    for child in children:
        result.extend(horz_curves(child, sequences, branch))
    return result


def stance_label(stance):
    if not stance:
        return None
    if not stance.startswith("STANCE_TYPE_"):
        raise ValueError(f"Unexpected Stance {stance}")
    suffix = stance[len("STANCE_TYPE_"):]
    return "STANCE_DEFAULT" if suffix == "DEFAULT" else f"STANCE_{suffix}"


def eligible_curves(curves, animation, stance):
    matching = list(curves)
    label = stance_label(stance)
    if label:
        stance_match = [curve for curve in matching if f"Stance={label}" in curve.branch.split("/")]
        if stance_match:
            matching = stance_match
        elif any("Stance=" in curve.branch for curve in matching):
            raise ValueError(f"Missing Stance={label} for animation {animation}")
    move = [curve for curve in matching if any(part.startswith("MoveType=") for part in curve.branch.split("/"))]
    if move:
        return move
    matching = [curve for curve in matching if curve.animation == animation]
    if not matching:
        raise ValueError(f"No H graph for animation {animation}")
    return matching


def branch_attributes(curve):
    attributes = {
        "animation": str(curve.animation),
        "duration": numeric_text(curve.duration),
        "durationSource": curve.duration_source,
        "H_Time": numeric_text(curve.h_time),
        "HorizonRotate": numeric_text(curve.horizon_rotate),
        "sampleCount": str(len(curve.samples)),
    }
    for part in filter(None, curve.branch.split("/")):
        kind, _, value = part.partition("=")
        if kind == "MoveType":
            attributes["moveType"] = value
        elif kind == "Random":
            attributes["randomCaseValue"] = value
        elif kind == "Stance":
            attributes["stance"] = value
    attributes.update((f"H_Y{i}", numeric_text(sample)) for i, sample in enumerate(curve.samples))
    return attributes


def skill_actions(skills_root, skill_ids):
    wanted = {skill_id for skill_id in skill_ids}
    actions = set()
    for node in skills_root.findall("INFO"):
        if node.attrib.get("ID") not in wanted:
            continue
        common = node.find("ST_COMMONSKILL")
        action = common.get("_Action") if common is not None else None
        if action:
            actions.add(action)
    return actions


def extract(data_root, characters, ani_length, action_base, skills):
    entities = {}
    for entity in ET.parse(ani_length).getroot().findall("./EntityType[@_Type='PC']/Entity"):
        key = entity.attrib["_Key"].casefold()
        if key in entities:
            raise ValueError(f"Duplicate AniLength key: {key}")
        sequences = {}
        for sequence in entity.findall("AnimationInfo"):
            animation = int(sequence.attrib["_SeqID"])
            if animation in sequences:
                raise ValueError(f"Duplicate animation {animation} for {key}")
            sequences[animation] = sequence
        entities[key] = sequences

    skills_root = ET.parse(skills).getroot()
    masters = []
    seen = set()
    key_by_class = {}
    short_to_class = {}
    reachable_actions = {}
    for master in sorted(ET.parse(characters).getroot().findall("INFO"), key=lambda node: int(node.attrib["ID"])):
        class_name = master.find("StatsComData").attrib["_class"]
        key = master.find("Kfm_ComData").attrib["KeyName"]
        if class_name in seen:
            raise ValueError(f"Duplicate loaded class: {class_name}")
        seen.add(class_name)
        masters.append(class_name)
        key_by_class[class_name] = key
        short_to_class[class_name.replace("CLASS_TYPE_", "").casefold()] = class_name
        short_to_class[key.casefold()] = class_name
        skill_ids = [node.get("_Index") for node in master.find("SkillComData").findall("_SkillID")]
        reachable_actions[class_name] = skill_actions(skills_root, skill_ids)

    class_cache = {}
    selected = {}
    reachable_horz = set()
    for row in ET.parse(action_base).getroot().findall("ActionBase"):
        if row.get("CommandType") != "GRAPH_MOVE_HORZ":
            continue
        class_name = short_to_class.get(row.get("Class", "").casefold())
        if class_name is None:
            continue
        state = row.get("BehaviorState")
        if state not in reachable_actions[class_name]:
            continue
        identity = (class_name, state)
        reachable_horz.add(identity)
        animation = int(row.get("AnimationId"))
        stance = row.get("Stance")
        key = key_by_class[class_name]
        if class_name not in class_cache:
            asset = resolve_case(data_root, f"Graphic/Creature/Pc/{key}/{key}.xml")
            tree = ET.parse(asset).getroot()
            states = {}
            for node in tree.findall(".//StatePlay[@Type='ActionState']"):
                states.setdefault(node.get("State"), []).append(node)
            class_cache[class_name] = (asset.relative_to(data_root).as_posix(), states, entities[key.casefold()])
        source, states, sequences = class_cache[class_name]
        nodes = states.get(state, [])
        if len(nodes) != 1:
            raise ValueError(f"{class_name} ({source}): expected one {state}, found {len(nodes)}")
        try:
            curves = eligible_curves(horz_curves(nodes[0], sequences), animation, stance)
        except (KeyError, ValueError) as error:
            raise ValueError(f"{class_name} ({source}) {state}: {error}") from error
        previous = selected.get(identity)
        if previous and (previous["animation"] != animation or previous["stance"] != stance):
            print(
                f"GetSkillAction last ActionBase GRAPH_MOVE_HORZ retained: {class_name} {state}: "
                f"animation {previous['animation']} {previous['stance']} -> animation {animation} {stance}"
            )
        selected[identity] = {
            "class": class_name,
            "action": state,
            "source": source,
            "animation": animation,
            "curves": curves,
            "param2": row.get("Param2"),
            "stance": stance,
        }

    missing = [f"{class_name} {state}" for class_name, state in sorted(reachable_horz) if (class_name, state) not in selected]
    if missing:
        raise ValueError("Missing reachable GRAPH_MOVE_HORZ profiles: " + ", ".join(missing))

    output = ET.Element("HorizontalMotion")
    order = {name: index for index, name in enumerate(masters)}
    split_shapes = 0
    for identity in sorted(selected, key=lambda item: (order.get(item[0], len(order)), item[0], item[1])):
        record = selected[identity]
        attributes = {
            "class": record["class"],
            "action": record["action"],
            "source": record["source"],
            "animation": str(record["animation"]),
        }
        if record["stance"]:
            attributes["stance"] = record["stance"]
        if record["param2"]:
            attributes["param2"] = record["param2"]
        profile = ET.SubElement(output, "Profile", attributes)
        shapes = {curve.numeric_motion() for curve in record["curves"]}
        if len(shapes) > 1:
            split_shapes += 1
        for curve in record["curves"]:
            ET.SubElement(profile, "Branch", branch_attributes(curve))
    if not selected:
        raise ValueError("No horizontal motion profiles found")
    return ET.ElementTree(output), selected, split_shapes


def main():
    parser = argparse.ArgumentParser(description="Extract SkillComData-reachable GRAPH_MOVE_HORZ H graphs from client StatePlay assets.")
    parser.add_argument("data_root", type=Path)
    parser.add_argument("--characters", type=Path, default=REPO / "gamedata/CREATURE_CHARACTER.xml")
    parser.add_argument("--ani-length", type=Path, default=REPO / "gamedata/AniLength.xml")
    parser.add_argument("--action-base", type=Path, default=REPO / "gamedata/ActionBase.xml")
    parser.add_argument("--skills", type=Path, default=REPO / "gamedata/SKILL.xml")
    parser.add_argument("--output", type=Path, default=REPO / "gamedata/HorizontalMotion.xml")
    args = parser.parse_args()
    tree, selected, split_shapes = extract(
        args.data_root.resolve(), args.characters, args.ani_length, args.action_base, args.skills
    )
    ET.indent(tree, space="  ")
    tree.write(args.output, encoding="utf-8", xml_declaration=True)
    print(
        f"Wrote {len(selected)} SkillComData-reachable profiles "
        f"({split_shapes} with material MoveType/Random sample splits) to {args.output}"
    )


if __name__ == "__main__":
    main()
