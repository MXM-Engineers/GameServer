import argparse
from dataclasses import dataclass
from decimal import Decimal, InvalidOperation
from pathlib import Path
import xml.etree.ElementTree as ET
import struct


DIRECTIONS = ("Stand", "Front", "Left", "Right", "Back")
REPO = Path(__file__).resolve().parents[1]


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


def normalized_path(text):
    return text.replace("\\", "/").casefold()


@dataclass(frozen=True)
class Curve:
    duration: Decimal
    heights: tuple
    animation: int
    branch: str
    duration_source: str

    def numeric_motion(self):
        return self.duration, self.heights


def jump_curves(node, sequences, directions=DIRECTIONS, branch=""):
    kind = node.get("Type")
    children = node.findall("StatePlay")
    if kind == "Animation":
        animation = int(node.attrib["AnimationIndex"])
        heights = tuple(number(node.get(f"V_Y{i}")) for i in range(11))
        v_time = number(node.get("V_Time", "0"))
        if v_time < 0:
            raise ValueError(f"Negative V_Time for animation {animation}")
        duration_source = "V_Time" if v_time > 0 else "AniLength"
        duration = v_time if v_time > 0 else number(sequences[animation].get("_SeqTime"))
        if duration <= 0:
            raise ValueError(f"Nonpositive duration for animation {animation}")
        if any(any(f"V_Y{i}" in child.attrib for i in range(11)) for child in node.iter() if child is not node):
            raise ValueError(f"Nested vertical motion in animation {animation}")
        curve = Curve(duration, heights, animation, branch, duration_source)
        return {direction: [curve] for direction in directions}
    if kind in ("MoveType", "CurrentScene", "Stance"):
        labels = [node.get(f"CaseValue{i}") for i in range(len(children))]
        case_keys = {key for key in node.attrib if key.startswith("CaseValue")}
        if any(label is None for label in labels) or len(set(labels)) != len(labels) or len(case_keys) != len(children):
            raise ValueError(f"Invalid {kind} routing: {node.attrib}")
        if kind == "MoveType" and set(labels) != {direction.upper() for direction in DIRECTIONS}:
            raise ValueError(f"Incomplete MoveType routing: {labels}")
        branches = []
        for label, child in zip(labels, children):
            selected = tuple(d for d in directions if d.upper() == label) if kind == "MoveType" else directions
            if not selected:
                continue
            child_branch = branch if kind == "MoveType" else "/".join(filter(None, (branch, f"{kind}={label}")))
            branches.append((selected, jump_curves(child, sequences, selected, child_branch)))
        if any(result for _, result in branches):
            for selected, result in branches:
                if set(result) != set(selected):
                    raise ValueError(f"Missing motion in {kind} branch: {node.attrib}")
        result = {}
        for _, child_result in branches:
            for direction, curves in child_result.items():
                result.setdefault(direction, []).extend(curves)
        return result
    if kind not in ("ActionState", "Group", "Parallel"):
        if any(any(f"V_Y{i}" in child.attrib for i in range(11)) for child in node.iter()):
            raise ValueError(f"Unsupported motion container {kind}: {node.attrib}")
        return {}
    result = {}
    for child in children:
        child_result = jump_curves(child, sequences, directions, branch)
        overlap = set(result) & set(child_result)
        if overlap:
            raise ValueError(f"Multiple active vertical curves in {kind}: {sorted(overlap)}")
        result.update(child_result)
    return result


def extract(data_root, characters, ani_length):
    entities = {}
    for entity in ET.parse(ani_length).getroot().findall("./EntityType[@_Type='PC']/Entity"):
        key = entity.attrib["_Key"].casefold()
        if key in entities:
            raise ValueError(f"Duplicate AniLength key: {key}")
        entities[key] = entity
    option = ET.parse(resolve_case(data_root, "Design/GAMEINFO/Option.xml")).getroot().find("OptionTableData")
    gravity = number(option.get("CreatureGravity"))
    if gravity >= 0:
        raise ValueError(f"CreatureGravity must be negative: {gravity}")
    output = ET.Element("JumpMotion", creatureGravity=numeric_text(gravity))
    masters = sorted(ET.parse(characters).getroot().findall("INFO"), key=lambda node: int(node.attrib["ID"]))
    seen = set()
    for master in masters:
        class_name = master.find("StatsComData").attrib["_class"]
        key = master.find("Kfm_ComData").attrib["KeyName"]
        if class_name in seen:
            raise ValueError(f"Duplicate loaded class: {class_name}")
        seen.add(class_name)
        asset = resolve_case(data_root, f"Graphic/Creature/Pc/{key}/{key}.xml")
        source = asset.relative_to(data_root).as_posix()
        tree = ET.parse(asset).getroot()
        entity = entities[key.casefold()]
        character = tree.find("Character")
        if normalized_path(character.attrib["FileName"]) != normalized_path(entity.attrib["_KfmPath"]):
            raise ValueError(f"Kfm identity mismatch for {class_name}: {source}")
        sequences = {}
        for sequence in entity.findall("AnimationInfo"):
            animation = int(sequence.attrib["_SeqID"])
            if animation in sequences:
                raise ValueError(f"Duplicate animation {animation} for {key}")
            sequences[animation] = sequence
        states = tree.findall(".//StatePlay[@Type='ActionState'][@State='JUMP_START_MOVESTATE']")
        if len(states) != 1:
            raise ValueError(f"Expected one ordinary jump state for {class_name}, found {len(states)}")
        try:
            profiles = jump_curves(states[0], sequences)
        except (KeyError, ValueError) as error:
            raise ValueError(f"{class_name} ({source}): {error}") from error
        if set(profiles) != set(DIRECTIONS):
            raise ValueError(f"Incomplete directions for {class_name}: {sorted(profiles)}")
        for direction in DIRECTIONS:
            curves = profiles[direction]
            base = [curve for curve in curves if "Stance=STANCE_DEFAULT" in curve.branch.split("/")]
            if len(base) > 1:
                raise ValueError(f"Ambiguous base jump for {class_name} {direction}: {base}")
            curve = base[0] if base else curves[0]
            for variant in curves:
                if variant.numeric_motion() == curve.numeric_motion():
                    continue
                base_bits = struct.unpack("<I", struct.pack("<f", float(curve.duration)))[0]
                variant_bits = struct.unpack("<I", struct.pack("<f", float(variant.duration)))[0]
                if not base or variant.heights != curve.heights or abs(base_bits - variant_bits) > 1:
                    raise ValueError(f"Materially different jump variants for {class_name} {direction}: {curves}")
                print(f"Base stance retained: {class_name} {direction}: {curve.duration}; {variant.branch}: {variant.duration} (float export roundoff)")
            attributes = {
                "class": class_name,
                "direction": direction,
                "source": source,
                "animation": str(curve.animation),
                "duration": numeric_text(curve.duration),
                "durationSource": curve.duration_source,
            }
            if len(curves) > 1:
                attributes["branch"] = curve.branch
                print(f"Base profile: {class_name} {direction}: {curve.branch}; inspected {len(curves)} variants")
            attributes.update((f"V_Y{i}", numeric_text(height)) for i, height in enumerate(curve.heights))
            ET.SubElement(output, "Profile", attributes)
    if not masters:
        raise ValueError("No loaded masters found")
    return ET.ElementTree(output), len(masters)


def main():
    parser = argparse.ArgumentParser(description="Extract loaded masters' ordinary jump curves from client StatePlay assets.")
    parser.add_argument("data_root", type=Path)
    parser.add_argument("--characters", type=Path, default=REPO / "gamedata/CREATURE_CHARACTER.xml")
    parser.add_argument("--ani-length", type=Path, default=REPO / "gamedata/AniLength.xml")
    parser.add_argument("--output", type=Path, default=REPO / "gamedata/JumpMotion.xml")
    args = parser.parse_args()
    tree, count = extract(args.data_root.resolve(), args.characters, args.ani_length)
    ET.indent(tree, space="  ")
    tree.write(args.output, encoding="utf-8", xml_declaration=True)
    print(f"Wrote {count * len(DIRECTIONS)} profiles for {count} loaded masters to {args.output}")
    print(f"CreatureGravity: {tree.getroot().attrib['creatureGravity']}; no coverage gaps or material branch differences")


if __name__ == "__main__":
    main()
