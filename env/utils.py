from .constants import *
import csv
import io


def encode_vint(value: int) -> list[int]:
    """
    Encodes a variable-length integer (VInt).
    Writes bytes to the buffer list.

    Args:
        buffer: A list to append bytes to
        value: The integer value to encode
    """
    buffer = []
    temp = (value >> 25) & 0x40
    flipped = value ^ (value >> 31)

    temp |= value & 0x3F
    value >>= 6

    flipped >>= 6
    if flipped == 0:
        buffer.append(temp)
        return buffer

    buffer.append(temp | 0x80)

    # do-while equivalent: execute at least once
    while True:
        flipped >>= 7
        buffer.append((value & 0x7F) | (0x80 if flipped != 0 else 0))
        value >>= 7
        if flipped == 0:
            break
    return buffer


def parse_spells_csv(f):
    records = []
    reader = csv.reader(f)

    headers = next(reader)
    datatypes = next(reader)

    # Strip whitespace from headers and datatypes
    headers = [h.strip() for h in headers]
    datatypes = [d.strip().lower() for d in datatypes]

    # Ensure we have a datatype for each header, defaulting to string
    assert len(datatypes) == len(headers)

    for row_idx, row in enumerate(reader, start=3):
        # Skip empty rows or trailing blank lines
        if not row or all(cell.strip() == "" for cell in row):
            continue

        record = {}
        for col_idx, col_name in enumerate(headers):
            # If row is shorter than headers, pad with empty string
            val = row[col_idx].strip() if col_idx < len(row) else ""
            dtype = datatypes[col_idx]

            # Conversion based on the datatype defined in row 2
            if val == "":
                # Map empty fields to appropriate None/False
                if dtype == "boolean":
                    parsed_val = False
                elif dtype == "int":
                    parsed_val = None
                else:
                    parsed_val = None
            else:
                if dtype == "int":
                    try:
                        parsed_val = int(val)
                    except ValueError:
                        parsed_val = val  # Fallback to string if not integer
                elif dtype == "boolean":
                    parsed_val = val.lower() in ("true", "1")
                else:
                    parsed_val = val

            record[col_name] = parsed_val
        records.append(record)

    return records


def parse_spells_csv_string(csv_content: str):
    return parse_spells_csv(io.StringIO(csv_content))


def print_observation(obs):
    def card_str(card: dict):
        return f"{card['name']}({card['level']})"

    def get_hand_content(hand, deck):
        return [card_str(deck[hand_idx]) for hand_idx in hand]

    print("\033[H\033[J", end="")
    print("tick:", obs[TRAINER]["tick"])
    for p in obs:
        o = obs[p]
        name = "TRAINER" if p == 0 else "PLAYER"
        print(name)
        print(
            "\telixir:",
            o["elixir"][0],
            "\tfrac:",
            o["elixir"][1],
            "/",
            o["elixir_denominator"],
        )
        print("\tdeck:", [card_str(c) for c in o["deck"]])
        print("\thand:", get_hand_content(o["hand"], o["deck"]))
        print("\tnext_card:", card_str(o["deck"][o["next_card"]]))
        print("\tcrown:", o["crown"])
        print()

    def print_object(obj):
        print(
            f"{obj["name"]}({"T" if obj["owner"] == 0 else "P"}) {f"deploy_time: {obj["deploy_time"]}" if obj.get("deploy_time", 0) > 0 else ""} {f"hp: {obj["hp"]}" if "hp" in obj else ""} {f"shield: {obj["shield"]}" if "shield" in  obj else ""} (x,y,h):({obj["x"]}, {obj["y"]}, {obj["h"]}) {f"heading: ({obj["heading_x"]},{obj["heading_y"]}))" if "heading_x" in obj else ""} {f"buffs: {obj["buffs"]}" if "buffs" in obj else ""}"
        )

    def print_objects(objs):
        non_characters = []
        for obj in objs:
            if "hp" not in obj:
                non_characters.append(obj)
            else:
                print_object(obj)
        print()
        for obj in non_characters:
            print_object(obj)

    print_objects(obs[PLAYER]["game_objects"])
