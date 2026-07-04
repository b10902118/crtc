from .constants import PLAYER, TRAINER
from .crtc import CRTC
from .utils import print_observation
import time
import sys
import select


# TODO: allow supplying env args
def main():
    print("Game loading. Press Enter to pause and play cards...\n")
    env = CRTC(render_mode="human")
    observations, infos = env.reset()
    print_observation(observations)
    term = False
    while not term:
        rlist, _, _ = select.select([sys.stdin], [], [], 0)
        if rlist:
            sys.stdin.readline()  # Consume the newline
            print("\n=== PAUSED ===")
            print(
                "Action form: `{hand_idx} {x_grid} {y_grid}`. (0, 0) at the upper-left corner with respect to each player's view"
            )

            # P0/Trainer action
            p0_input = input("Trainer action or Enter to skip: ").strip()
            if p0_input:
                try:
                    p0_act = tuple(map(int, p0_input.replace(",", " ").split()))
                    if len(p0_act) != 3:
                        raise ValueError
                except ValueError:
                    print("Invalid input, using (-1, 0, 0)")
                    p0_act = (-1, 0, 0)
            else:
                p0_act = (-1, 0, 0)

            # P1/Player action
            p1_input = input("Player action or Enter to skip: ").strip()
            if p1_input:
                try:
                    p1_act = tuple(map(int, p1_input.replace(",", " ").split()))
                    if len(p1_act) != 3:
                        raise ValueError
                except ValueError:
                    print("Invalid input, using (-1, 0, 0)")
                    p1_act = (-1, 0, 0)
            else:
                p1_act = (-1, 0, 0)

            print(f"Stepping with: Trainer={p0_act}, Player={p1_act}\n")
            observations, _, terms, _, infos = env.step((p0_act, p1_act))
        else:
            observations, _, terms, _, infos = env.step(((-1, 0, 0), (-1, 0, 0)))

        print_observation(observations)
        term = terms[PLAYER]

        time.sleep(1 / 60)  # some delay for watching

    print("Game finished!")
    print(
        f"Final Crowns: Trainer={observations[TRAINER]['crown']}, Player={observations[PLAYER]['crown']}"
    )
    input("Press Enter to quit...")
    env.close()


if __name__ == "__main__":
    main()
