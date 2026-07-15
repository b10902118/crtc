The environment follows [PettingZoo's Parallel API](https://pettingzoo.farama.org/api/parallel/), which is a multi-agent version of Gymnasium.

## `CRTC` Class

The main environment class, inheriting from `pettingzoo.ParallelEnv`.

### `__init__(self, cmd=None, summon_delay=20, display_dim=(450, 800), trainer_view=False, render_mode=None, fps=15)`

Initializes the environment.

- `cmd` (list[str]): Command arguments to launch the loader. Default `None` to use `./build_loader/run.sh`
- `summon_delay` (int): The delay between playing a card and really summoning it, in ticks (default: 20, which is 1s, not sure the game's real value)
- `display_dim` (tuple): Width and height of the rendered image (default: `(450, 800)`).
- `trainer_view` (bool): If `True`, the rendered bottom player will be trainer. No effect to coordinates.
- `render_mode` (str): `"human"`, `"rgb_array"`, or `None` (headless).
- `fps` (int): If > 0, then max fps for rendering in `"human"` mode. (default: -1).

### `reset(self, seed=None, options=None) -> (observations, infos)`

Resets the game session.

- `seed` (int): Seed for randomness (e.g., deck shuffling).
- `options` (dict): Optional settings:
  - `decks` (dict): Dict mapping agent IDs (`0` for `TRAINER`, `1` for `PLAYER`) to custom decks (list of 8 card dicts with `name` and optional `level`).
  - `shuffle` (bool): Whether to shuffle the player/trainer decks before starting.

### `step(self, actions) -> (observations, rewards, terminations, truncations, infos)`

Advances the game state by one tick.

- `actions` (dict): Dict mapping agent IDs to actions:
  - An action is `(hand_idx, grid_x, grid_y)`. `hand_idx = -1` is nop.
  - `grid_x`, `grid_y` is the `int` coordinates with origin (0, 0) at **the top-left cell viewed from each player's side** (TRAINER's origin is mirrored to PLAYER's bottom-right)

### `render(self) -> np.ndarray | None`

Renders the environment screen. If `render_mode` is `"human"`, updates the display window. If `"rgb_array"`, returns a numpy array representing the image.

### `close(self)`

Terminates the simulator subprocess and cleans up Tkinter window components.

### `get_card_data(self, card_name) -> dict | None`

Retrieves the card's attributes read from CSV.

### `get_object_data(self, obj_name) -> dict | None`

Retrieves the game object's attributes read from CSV.

## Observation Space

Each agent's observation contains:

- `tick` (int): 1 tick is 0.05s

- `elixir` (tuple[int, int]): the first is the whole number and the second represents the fraction with `elixir_denominator` as denominator

- `elixir_denominator` (int): 280000 in single elixir and 140000 in double elixir

- `deck` (tuple[{"name": str, "level": int}, ...], length=8)

- `hand` (tuple[int, int, int, int]): the indices of the hand cards in the deck

- `next_card` (int): the index of the next card in the deck

- `crown` (int): the number of enemy towers destoryed

- `game_objects`: (list[dict]): each game object is either `Character`, `Building`, `Projectile`, or `Area Effect`
  - **Characters & Buildings**
    - `name` (str)
    - `owner` (int): The owner's account index (`TRAINER(0)`, `PLAYER(1)`)
    - `x` (int): 1000 for 1 grid
    - `y` (int): 1000 for 1 grid
    - `h` (int)
    - `heading_x` (float): (`heading_x`, `heading_y`) normalized to 1
    - `heading_y` (float): (`heading_x`, `heading_y`) normalized to 1
    - `hp` (int)
    - `shield` (int)
    - `state` (int): `IDLE(0)`, `MOVING(1)`,`ATTACKING(2)`, `DEPLOYING(5)`. The other values are unknown
    - `attack_duration` (int): The time length of its most recent consecutive attack in ms. Use this to compute attack cooldown
    - `deploy_time` (int): The remaining deploy time in ms
    - `buffs` (list[str]): The names of the buffs the character has
  - **Projectiles & Area Effects**
    - `name` (str)
    - `owner` (int): The owner's account index (`TRAINER(0)`, `PLAYER(1)`)
    - `x` (int): 1000 for 1 grid
    - `y` (int): 1000 for 1 grid
    - `h` (int)

_Note: Special object states, such as charge and summon cooldowns, are currently under development._
