from .utils import encode_vint, parse_spells_csv_string
from .constants import *

import subprocess
import time
import json
import copy
import os
import sys
import random
from collections import deque
from gymnasium import spaces
from pettingzoo import ParallelEnv
import numpy as np
import base64
import lzma


class PlayerState:
    def __init__(
        self,
        deck: list[str],
        hand: list[int],
        elixir=None,
        locked_elixir=0,
        locked_hand=None,
    ):
        # the returned is tuple but list for internal
        self.deck = deck[:]
        self.hand = hand[:]
        self.elixir = elixir if elixir else []
        self.locked_elixir = locked_elixir
        self.locked_hand = locked_hand if locked_hand else []
        self.action_queue = deque()

    def update(self, hand, elixir):
        self.elixir = elixir
        self.hand = hand
        for i in self.locked_hand:
            self.hand[i] = -1

    def can_play(self, hand_idx, card_cost, summon_delay, elixir_denominator):
        if hand_idx in self.locked_hand:
            return False
        elixir, elixir_frac = self.elixir
        elixir -= self.locked_elixir
        return (
            elixir + (elixir_frac + ELIXIR_GEN_RATE * summon_delay) / elixir_denominator
            >= card_cost
        )


class CRTC(ParallelEnv):
    metadata = {
        "render_modes": ["human", "rgb_array"],
        "name": "crtc_v0",
    }

    def __init__(
        self,
        cmd: list[str] = None,
        summon_delay=20,
        display_dim=(450, 800),
        trainer_view=False,
        render_mode=None,
        fps=-1,
    ):
        """
        Initialize the environment.

        :param cmd: List of command arguments to launch cr_loader.
                    If None, defaults to ["./cr_loader"].
        :param summon_delay: delay for summon
        :param display_dim: (width, height) of the display window
        """
        super().__init__()

        self.render_mode = render_mode
        self.headless = render_mode is None

        if cmd is None:
            module_path = os.path.dirname(os.path.realpath(__file__))
            run_script_path = os.path.join(module_path, "./build_loader/run.sh")
            cmd = [run_script_path]
            if not self.headless:
                cmd.append("--no-headless")
                cmd.append(f"--display-dim={display_dim[0]},{display_dim[1]}")
            if trainer_view:
                cmd.append("--trainer-view")

        self.cmd = cmd
        self.display_dim = display_dim
        self.summon_delay = summon_delay

        self._display_fps = fps
        self._last_display_update = 0.0
        self.terminated = True

        self.possible_agents = AGENTS[:]
        self.agents = AGENTS[:]

        single_action_space = spaces.Tuple(
            (
                spaces.Discrete(n=5, start=-1),
                spaces.Box(
                    low=np.array([0, 0]),
                    high=np.array([BOARD_W_GRID - 1, BOARD_H_GRID - 1]),
                    shape=(2,),
                    dtype=int,
                ),
            )
        )
        self.action_spaces = {
            agent: single_action_space for agent in self.possible_agents
        }

        # Observation Space is represented as a Tuple of two Dicts (up, low) containing the structured JSON fields.
        single_observation_space = spaces.Dict(
            {
                "tick": spaces.Discrete(MAX_TICK + 1),
                "elixir": spaces.Tuple(
                    (
                        spaces.Discrete(11),
                        spaces.Discrete(280001),
                    )
                ),
                "elixir_denominator": spaces.Discrete(280001),
                "hand": spaces.Tuple(
                    [
                        spaces.Discrete(n=9, start=-1),
                    ]
                    * 4
                ),
                "deck": spaces.Tuple(
                    [
                        spaces.Dict(
                            {
                                "name": spaces.Text(max_length=64),
                                "level": spaces.Discrete(n=13, start=1),
                            }
                        )
                    ]
                    * 8
                ),
                "next_card": spaces.Discrete(n=9, start=-1),
                "crown": spaces.Discrete(4),
                "game_objects": spaces.Sequence(
                    spaces.Dict(
                        {
                            "name": spaces.Text(max_length=64),
                            "owner": spaces.Discrete(2),
                            "x": spaces.Box(low=0, high=BOARD_W, shape=(), dtype=int),
                            "y": spaces.Box(low=0, high=BOARD_H, shape=(), dtype=int),
                            "h": spaces.Discrete(10000),  # not sure
                            "heading_x": spaces.Box(
                                low=-1.0, high=1.0, shape=(), dtype=float
                            ),
                            "heading_y": spaces.Box(
                                low=-1.0, high=1.0, shape=(), dtype=float
                            ),
                            "hp": spaces.Discrete(10000),
                            "shield": spaces.Discrete(10000),
                            "state": spaces.Discrete(10),
                            "attack_duration": spaces.Discrete(MAX_TICK * MS_PER_TICK),
                            "deploy_time": spaces.Discrete(MAX_TICK * MS_PER_TICK),
                            "buffs": spaces.Sequence(spaces.Text(max_length=64)),
                        }
                    )
                ),
            }
        )
        self.observation_spaces = {
            agent: single_observation_space for agent in self.possible_agents
        }

        # Start the process immediately and block until C++ initialization completes
        self._start_process()
        self.load_game_data()

    def action_space(self, agent):
        return self.action_spaces[agent]

    def observation_space(self, agent):
        return self.observation_spaces[agent]

    def _start_process(self):
        # Start subprocess with pipes.
        # stderr goes to sys.stderr so C++ logging is visible.
        # In environments like ipynb, sys.stderr does not have a valid
        # file descriptor. We fall back to a pipe and a background thread.
        stderr_arg = None
        if sys.stderr is not None:
            try:
                sys.stderr.fileno()
                stderr_arg = sys.stderr
            except Exception:
                stderr_arg = subprocess.PIPE

        self.proc = subprocess.Popen(
            self.cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=stderr_arg,
            text=True,
            bufsize=1,  # Line buffered
        )

        if stderr_arg == subprocess.PIPE:
            import threading

            def forward_stderr(source, target):
                try:
                    for line in source:
                        target.write(line)
                        target.flush()
                except Exception:
                    pass

            threading.Thread(
                target=forward_stderr,
                args=(self.proc.stderr, sys.stderr),
                daemon=True,
            ).start()

        ready_line = self.proc.stdout.readline()
        if not ready_line:
            poll_val = self.proc.poll()
            raise RuntimeError(f"Loader exited prematurely with code {poll_val}.")
        try:
            status = json.loads(ready_line.strip())
            if status.get("status") != "ready":
                raise RuntimeError(
                    f"Unexpected initialization response: {ready_line.strip()}"
                )
        except json.JSONDecodeError as e:
            raise RuntimeError(
                f"Failed to parse C++ initialization response: {ready_line.strip()}"
            ) from e

    def add_default_level(self, deck: list[dict]):
        for card in deck:
            if "level" not in card:
                rarity = self.card_data[card["name"]]["Rarity"]
                if rarity == "Common":
                    card["level"] = 9
                elif rarity == "Rare":
                    card["level"] = 7
                elif rarity == "Epic":
                    card["level"] = 4
                elif rarity == "Legendary":
                    card["level"] = 1

    def encode_deck(self, deck: list[dict]) -> str:
        if len(deck) != 8:
            raise RuntimeError(f"Deck length not 8 (is {len(deck)})")
        buffer = []
        for card in deck:
            name = card["name"]
            card_info = self.card_data.get(name)
            if not card_info:
                raise RuntimeError(f"Card `{name}` does not exist.")
            buffer += encode_vint(card_info["id"])

            buffer += encode_vint(card["level"] - 1)

        return bytes(buffer).hex()

    def update_states(self, obs):
        self.tick = obs["tick"]
        self.elixir_denominator = obs["elixir_denominator"]
        for p in AGENTS:
            self.players[p].update(obs["hands"][p], obs["elixirs"][p])
            # assert self.players[p].elixir[0] >= 0
        self.update_crowns(obs["game_objects"])

    def update_crowns(self, game_objects):
        king_hps = [0, 0]
        princesses_hps = [[], []]
        for obj in game_objects:
            if obj["name"] == "KingTower":
                king_hps[obj["owner"]] = obj["hp"]
            elif obj["name"] == "PrincessTower":
                princesses_hps[obj["owner"]].append(obj["hp"])

        down_towers = [None, None]
        for p in AGENTS:
            if king_hps[p] == 0:
                down_towers[p] = 3
            else:
                princesses_hps[p] = [hp for hp in princesses_hps[p] if hp > 0]
                down_towers[p] = 2 - len(princesses_hps[p])
        self.crowns = down_towers[::-1]

    def process_observation(self, obs):
        # obs may be stored, so always make copy for returned
        player_obs = copy.deepcopy(obs)
        trainer_obs = self.transform_observation(copy.deepcopy(obs))
        ret = {TRAINER: trainer_obs, PLAYER: player_obs}

        # only keep their own information
        for p, o in ret.items():
            o.pop("decks")
            o["deck"] = tuple(self.players[p].deck)
            for ent in ["elixirs", "hands", "next_cards"]:
                val = o[ent][p]
                single_ent = ent[:-1]  # remove s
                o.pop(ent)
                o[single_ent] = val
            o["elixir"] = tuple(o["elixir"])
            o["hand"] = tuple(o["hand"])
            o["crown"] = self.crowns[p]
        return ret

    def transform_observation(self, obs):
        for obj in obs["game_objects"]:
            obj["x"] = BOARD_W - obj["x"]
            obj["y"] = BOARD_H - obj["y"]
            if "heading_x" in obj:
                obj["heading_x"] = -obj["heading_x"]
                obj["heading_y"] = -obj["heading_y"]
        return obs

    def reset(self, seed=None, options=None):
        """
        (Re)start a training camp.
        """
        if seed is not None:
            random.seed(seed)

        if options is None:
            options = {}

        decks = options.get("decks", {})
        for p in AGENTS:
            decks[p] = decks.get(p, self.default_deck[:])
            self.add_default_level(decks[p])

        if options.get("shuffle"):
            for p in AGENTS:
                random.shuffle(decks[p])

        if self.proc is None or self.proc.poll() is not None:
            raise RuntimeError("Server process is dead")

        self._write_cmd(
            f"reset {self.encode_deck(decks[TRAINER])} {self.encode_deck(decks[PLAYER])}"
        )
        obs = self._read_response()
        self.tick = -1

        # not use obs["decks"], which doesn't have level
        self.players = [PlayerState(decks[p], obs["hands"][p]) for p in AGENTS]
        self.terminated = False
        self.update_states(obs)

        if self.render_mode == "human":
            self.render()
            self._last_display_update = time.time()

        observations = self.process_observation(obs)
        infos = {
            TRAINER: {},
            PLAYER: {},
        }
        return observations, infos

    def step(self, actions):
        """
        Run one step of the environment (advances the game by 1 tick / 50ms).

        :param actions: Dict mapping agent names to actions.
                        Example: {TRAINER: (0, 12, 30), PLAYER: (-1, 0, 0)}
                        Each action is either None or (card_idx, grid_x, grid_y).
                        (0, 0) at the upper-left corner with respect to each player's view
                        card_idx must be 0, 1, 2, or 3. Use None or card_idx=-1 to skip summoning.
        :return: (observations, rewards, terminations, truncations, infos)
        """

        info = {"warnings": []}
        if self.terminated:
            # TODO: soft warning?
            raise RuntimeError("Env terminated, need reset")

        # play card
        for p in AGENTS:
            hand_idx, x, y = actions[p]
            player = self.players[p]

            if not (-1 <= hand_idx <= 3):
                info["warnings"].append(
                    f"{"TRAINER" if p == TRAINER else "PLAYER"} hand_idx ({hand_idx}) not in [-1, 3]"
                )
                continue

            if hand_idx == -1:
                continue

            deck_idx = player.hand[hand_idx]
            if deck_idx == -1:
                info["warnings"].append(
                    f"{"TRAINER" if p == TRAINER else "PLAYER"} deck_idx at hand index {hand_idx} is -1"
                )
                continue

            # Convert grid level coordinates to middle pixel coordinates
            # final avaliable placement will be handled by libg
            x_pixel = x * GRID_PIXELS + GRID_PIXELS // 2
            y_pixel = y * GRID_PIXELS + GRID_PIXELS // 2

            # Mirror coordinate for TRAINER
            if p == TRAINER:
                x_pixel = BOARD_W - x_pixel
                y_pixel = BOARD_H - y_pixel

            card_name = player.deck[deck_idx]["name"]
            card_cost = self.card_data[card_name]["ManaCost"]  # TODO: handle mirror
            if player.can_play(
                hand_idx, card_cost, self.summon_delay, self.elixir_denominator
            ):
                player.action_queue.append(
                    (
                        self.tick + self.summon_delay,
                        hand_idx,  # sent to server
                        deck_idx,  # local lookup
                        x_pixel,
                        y_pixel,
                    )
                )
                player.locked_elixir += card_cost
                player.locked_hand.append(hand_idx)
            else:
                info["warnings"].append(
                    f"{"TRAINER" if p == TRAINER else "PLAYER"} cannot summon {card_name} at hand index {hand_idx}, cost ({card_cost}) > avaliable elixir ({player.elixir}, locked: {player.locked_elixir})"
                )

        # pop action_queue
        action_str = ""
        for p in AGENTS:
            player = self.players[p]
            if player.action_queue and player.action_queue[0][0] <= self.tick:
                _, hand_idx, deck_idx, x, y = player.action_queue.popleft()
                action_str += f" {hand_idx} {x} {y}"
                card_name = player.deck[deck_idx]["name"]
                card_cost = self.card_data[card_name]["ManaCost"]  # TODO: handle mirror

                # the libg func we called doesn't check elixir, so we have to debug here
                # assert (
                #    player.elixir[0] >= card_cost
                # )

                player.locked_elixir -= card_cost
                player.locked_hand.remove(hand_idx)
            else:
                action_str += f" -1 0 0"

        cmd_str = "step" + action_str
        self._write_cmd(cmd_str)
        obs = self._read_response()
        self.update_states(obs)

        # non-headless mode, the start will be delayed until loading screen ends
        while self.tick == 0:
            self._write_cmd(
                "step -1 0 0 -1 0 0"
            )  # the previous summon will be processed next tick
            obs = self._read_response()
            self.update_states(obs)

        winner = self.determine_winner()
        self.terminated = winner != -1 or self.tick == MAX_TICK

        if self.render_mode == "human":
            if self._display_fps > 0:
                now = time.time()
                if now - self._last_display_update >= 1.0 / self._display_fps:
                    self.render()
                    self._last_display_update = now
            else:
                self.render()

        observations = self.process_observation(obs)

        rewards = {
            TRAINER: 0.0,
            PLAYER: 0.0,
        }

        terminations = {
            TRAINER: self.terminated,
            PLAYER: self.terminated,
        }

        truncations = {
            TRAINER: False,
            PLAYER: False,
        }

        infos = {
            TRAINER: copy.deepcopy(info),
            PLAYER: copy.deepcopy(info),
        }

        return observations, rewards, terminations, truncations, infos

    def determine_winner(self):
        """
        ret: -1, TRAINER, PLAYER, 2 (draw)
        """
        if self.tick < SUDDEN_DEATH_TICK:
            if self.crowns[TRAINER] == 3 and self.crowns[PLAYER] == 3:
                return 2
            elif self.crowns[TRAINER] == 3:
                return TRAINER
            elif self.crowns[PLAYER] == 3:
                return PLAYER
            return -1
        else:
            if self.crowns[TRAINER] == 3 and self.crowns[PLAYER] == 3:
                return 2
            elif self.crowns[TRAINER] == self.crowns[PLAYER]:
                return -1
            elif self.crowns[TRAINER] > self.crowns[PLAYER]:
                return TRAINER
            return PLAYER

    def _write_cmd(self, cmd_str):
        if self.proc is None or self.proc.poll() is not None:
            raise RuntimeError("Loader process is not running.")
        self.proc.stdin.write(cmd_str + "\n")
        self.proc.stdin.flush()

    def _read_response(self):
        if self.proc is None or self.proc.poll() is not None:
            raise RuntimeError("Loader process is not running.")
        line = self.proc.stdout.readline()
        if not line:
            return None
        try:
            obs = json.loads(line.strip())
            return obs
        except json.JSONDecodeError as e:
            print(f"Error decoding JSON response: {line.strip()}", file=sys.stderr)
            raise e

    def close(self):
        """Close the process cleanly."""
        if hasattr(self, "_root") and self._root is not None:
            try:
                self._root.destroy()
            except Exception:
                pass
            self._root = None
        if self.proc is not None:
            try:
                self._write_cmd("exit")
            except Exception:
                pass
            self.proc.terminate()
            try:
                self.proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.proc.kill()
            self.proc = None

    def render(self):
        if self.render_mode is None:
            return

        if self.render_mode == "human":
            img = self._get_screenshot_img()
            self._display_image(img, block=False)
        elif self.render_mode == "rgb_array":
            return np.array(self._get_screenshot_img())

    def _get_screenshot_img(self):
        if self.headless:
            raise RuntimeError("Display is not supported in headless mode.")

        self._write_cmd("screenshot")
        response = self._read_response()
        if not response:
            print("No response from simulator for screenshot command.", file=sys.stderr)
            return None

        if "error" in response:
            print(f"Screenshot error: {response['error']}", file=sys.stderr)
            return None

        if "pixels" not in response:
            print("Response does not contain pixel data.", file=sys.stderr)
            return None

        try:
            pixel_bytes = base64.b64decode(response["pixels"])
            width, height = self.display_dim

            from PIL import Image

            img = Image.frombytes("RGB", (width, height), pixel_bytes)
            return img

        except Exception as e:
            print(f"Failed to process screenshot: {e}", file=sys.stderr)
            return None

    def _display_image(self, img, block=False):
        try:
            import tkinter as tk
            from PIL import ImageTk

            # Create root window if it doesn't exist or was closed/destroyed
            if not hasattr(self, "_root") or self._root is None:
                self._root = tk.Tk()
                self._root.title("CRTC")
                self._label = tk.Label(self._root)
                self._label.pack()

                self._root.update_idletasks()
                w = self._root.winfo_width()
                h = self._root.winfo_height()
                x = (self._root.winfo_screenwidth() // 2) - (w // 2)
                y = (self._root.winfo_screenheight() // 2) - (h // 2)
                self._root.geometry(f"+{x}+{y}")

                def on_close():
                    self._root.destroy()
                    self._root = None
                    self._label = None
                    self._tk_img = None

                self._root.protocol("WM_DELETE_WINDOW", on_close)

            # Update the image content
            self._tk_img = ImageTk.PhotoImage(img)
            self._label.config(image=self._tk_img)
            self._label.image = self._tk_img

            if block:
                self._root.mainloop()
            else:
                self._root.update()
        except Exception as e:
            print(f"Error displaying image in window: {e}", file=sys.stderr)

    def read_csv(self, path: str) -> str:
        self._write_cmd(f"readFile {path}")
        response = self._read_response()
        if response is None:
            raise RuntimeError("No response from simulator for readFile command.")

        if "status" in response and response["status"] == "ok":
            data = base64.b64decode(response["data"])
            data = data[:9] + (b"\x00" * 4) + data[9:]
            decompressed = lzma.decompress(data)
            return decompressed.decode("utf-8")
        else:
            raise RuntimeError(f"Error reading csv: {response}")

    def load_cards(self):
        spell_files = [
            "csv_logic/spells_characters.csv",
            "csv_logic/spells_buildings.csv",
            "csv_logic/spells_other.csv",
        ]
        spell_data = []
        for filename in spell_files:
            csv_content = self.read_csv(filename)
            spell_data.extend(parse_spells_csv_string(csv_content))
        self.card_data = {}
        for i, d in enumerate(spell_data):
            d["id"] = i + 1
            self.card_data[d["Name"]] = d

        self.default_deck = [{"name": d["Name"]} for d in spell_data[:8]]

    def load_objects(self):
        object_files = [
            "csv_logic/characters.csv",
            "csv_logic/buildings.csv",
            "csv_logic/projectiles.csv",
            "csv_logic/area_effect_objects.csv",
        ]
        object_data = []
        for filename in object_files:
            csv_content = self.read_csv(filename)
            object_data.extend(parse_spells_csv_string(csv_content))
        self.object_data = {}
        for i, d in enumerate(object_data):
            self.object_data[d["Name"]] = d

    def load_game_data(self):
        self.load_cards()
        self.load_objects()

    # TODO: support level
    def get_card_data(self, card_name):
        return self.card_data.get(card_name)

    # TODO: support level
    def get_object_data(self, obj_name):
        return self.object_data.get(obj_name)
