# Run Class 1 AIChess Tests

Created: 20260715_1945MDT

## Working Directory

Run from:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1
```

## Current Class 1 Tests

### C++/Bash Class 1 Fixture

This is the primary Class 1 fixture path. It uses a C++17 binary and Bash
wrapper, with no venv, npm, or Homebrew dependency chain.

```bash
class1_cpp/run_class1_fixture.sh
```

Optional invalid/alternative response check:

```bash
class1_cpp/run_class1_fixture.sh --agent-response "move the queen to z9"
class1_cpp/run_class1_fixture.sh --agent-response "g1f3"
```

Outputs are written to:

```text
runs/
```

Verified example output from this run:

```text
runs/aih_chess_class1_cpp_fixture_v1_20260715_20260715_202837_787.json
```

Build manually if needed:

```bash
class1_cpp/build_class1_fixture.sh
```

## What These Tests Measure

These are Class 1 tests when they measure:

- legal move production,
- illegal/unparseable move failures,
- board-state fidelity,
- referee agreement or disagreement,
- timing,
- role-separated evidence for player and referee configurations.

## Class 2 And Class 3 Reclassification Rule

AIChess-related tests can move into Class 2 or Class 3 when the target failure mode changes.

Keep as Class 1:

```text
legal action, state fidelity, referee validation, timing, rule-following
```

Move or derive into Class 2:

```text
workflow/provenance failures around what the agent claims it did,
which move source it followed, which referee verified legality,
what run artifact was produced, or whether it falsely reports completed
verification or unchanged state.
```

Move or derive into Class 3:

```text
source-bound chess knowledge, chess-course material, annotated game study,
opening/endgame theory from a defined academic or instructional source packet,
or K-PhD-style chess education material.
```

Do not move a test solely because it uses chess. Class assignment follows the failure mode being tested.

Current AIchess role-count convention:

```text
Class 1 AIchess core = one board, one player agent per side, three referees
Class 2 AIchess core = two boards, one player agent per side per board, one referee per board
Class 3 AIchess core = four boards, one player agent per side per board, four referees total
```

Near-term implementations should use board assignments, player agents, and
referee teams only. Other role types are deferred until the board-runner
package is stable.

Current player-team/referee-team configuration notes:

```text
configs/aichess_class1_one_board_one_agent_sides_three_referees_v1_20260715_2016MDT.json
configs/aichess_class2_two_boards_one_agent_sides_one_referee_each_v1_20260715_2016MDT.json
configs/aichess_class3_four_boards_one_agent_sides_four_referees_v1_20260715_2016MDT.json
```

Consolidated implementation instructions:

```text
AICHESS_IMPLEMENTATION_INSTRUCTIONS_20260715_1951MDT.md
```

## Class 2 C++ Fixture

Class 2 now has a C++/Bash two-board fixture. It is documented in the
consolidated implementation instructions and can be run with:

```bash
class2_cpp/run_class2_fixture.sh
```

## Class 3 C++ Fixture

Class 3 now has a C++/Bash four-board fixture. It is documented in the
consolidated implementation instructions and can be run with:

```bash
class3_cpp/run_class3_fixture.sh
```
