# AIChess Implementation Instructions

Created: 20260715_1951MDT

## What This Manual Covers

This manual describes the active AIChess v1 fixture tests and how to run them
locally. The current fixtures are intentionally simple: they use deterministic
fixture responses, fixed starting-position chess legality, C++ binaries, and
Bash wrapper scripts. They are not intended to measure chess strength yet.

The purpose of these fixtures is to verify the shape of the AIH chess harness:

- board assignment,
- player-agent assignment,
- referee assignment,
- legal move parsing,
- referee validation,
- per-board result recording,
- per-class result fields,
- failure capture for illegal or unparseable moves.

The durable local harness baseline is C++/Qt/Bash. C++ fixture binaries are the
active runnable path in this directory. Qt can be used for UI or richer process
control as the harness grows.

## Working Directory

Run commands from:

```bash
cd /home/sag/RPA2/myLLC/AI/brilliance/v1/AIH/AIchess/v1
```

All commands below assume that working directory.

Every run writes a JSON result file into:

```text
runs/
```

The JSON file name includes a timestamp so repeated runs do not overwrite each
other. Each command also prints the JSON result and the output file path to the
terminal.

## Basic Board Symmetry

The basic symmetric AIchess board pattern is:

```text
one player agent for White per board
one player agent for Black per board
one referee per board
```

Use the same pattern at the 1-board, 2-board, and 4-board sizes:

```text
basic 1-board test = 1 White agent + 1 Black agent + 1 referee
basic 2-board test = repeat the same per-board pattern on 2 boards
basic 4-board test = repeat the same per-board pattern on 4 boards
```

The existing 3-referee Class 1 test remains available as a stronger referee
option. It does not replace the basic symmetric test.

Use the basic symmetric tests when you want the fewest required agents and the
cleanest board-count scaling. Use the 3-referee options when you want to study
referee agreement, disagreement, and majority behavior.

## Valid And Invalid Agent Responses

The current fixtures ask the active side to provide one chess move from the
standard starting position. A valid response contains a legal UCI move for that
position. Examples:

```text
e2e4
g1f3
d2d4
```

For background on the move format, see the Universal Chess Interface
reference material. UCI uses long coordinate-style moves such as `e2e4`,
`e1g1` for White short castling, and `e7e8q` for promotion. Useful references:

- https://www.chessprogramming.org/UCI
- https://en.wikipedia.org/wiki/Universal_Chess_Interface

An invalid response is one the harness cannot accept as a legal move for the
current board state. Invalid responses include:

- natural-language-only text with no UCI move,
- a move to a square that does not exist,
- an illegal chess move,
- an invented chess rule,
- a claim of source support for an invented or illegal chess action.

Examples:

```text
move the queen to z9
castle the queen into check
invent a new source-backed queen castle
```

The optional bad-response commands below intentionally inject invalid
responses. They are not alternate ways to play chess; they are parser/referee
checks that confirm the harness records failures clearly.

## Command-Line Conventions

Each fixture should start with the simplest possible run command. More specific
commands should add options without changing the basic command shape. The
preferred progression is:

```text
run the default fixture
run the fixture with one changed agent response
run the fixture with a terminal/full-game mode when available
ask the fixture for command help
```

The help flags should be consistent across AIChess fixtures:

```bash
--help
-h
/help
/?
```

The `/help` and `/?` forms are included for users who are used to
DOS/Windows-style command flags. Help flags print usage information and do not
run a test.

## Real Qwen Agent Hallucination Runs

Use these commands for actual AIChess hallucination testing. These runs call
real local Ollama-compatible agents for player moves and, when configured, AI
agents for referee votes. Stockfish may be used as a rules engine for scoring
or as a non-agent baseline referee, but it must be identified as such in the
result data.

List available local agent aliases:

```bash
qwen_ollama_chess_qt/qwen_ollama_chess_qt --list-models
```

The current machine sorts installed Ollama-compatible agents by model size and
assigns aliases:

```text
agent1=qwen2.5-coder:3b
agent2=qwen:4b
agent3=qwen2.5:latest
agent4=robit/qwen3.5-9b-r7-research:q4km
qwen1=qwen2.5-coder:3b
qwen2=qwen:4b
qwen3=qwen2.5:latest
qwen4=robit/qwen3.5-9b-r7-research:q4km
```

Assign agents by role. Testing the same agent against itself is valid:

```bash
class1_basic_cpp/run_class1_basic_aichess_hallucination.sh \
  --white qwen1 \
  --black qwen1 \
  --referee qwen1 \
  --max-plies 200 \
  --move-timeout 60 \
  --game-timeout 3600
```

Testing two different player agents and a third AI referee is also valid:

```bash
class1_basic_cpp/run_class1_basic_aichess_hallucination.sh \
  --white qwen1 \
  --black qwen2 \
  --referee qwen3 \
  --max-plies 200 \
  --move-timeout 60 \
  --game-timeout 3600
```

Run the 2-board Class 2 hallucination test:

```bash
class2_cpp/run_class2_aichess_hallucination.sh \
  --white qwen1:qwen2 \
  --black qwen1:qwen2 \
  --referee qwen1:qwen2 \
  --max-plies 200 \
  --move-timeout 60 \
  --game-timeout 3600
```

Run the 4-board Class 3 hallucination test:

```bash
class3_cpp/run_class3_aichess_hallucination.sh \
  --white qwen1:qwen4 \
  --black qwen1:qwen4 \
  --referee qwen1:qwen4 \
  --max-plies 200 \
  --move-timeout 60 \
  --game-timeout 3600
```

For these real-agent runs:

```text
--white assigns player agents to White roles
--black assigns player agents to Black roles
--referee assigns referee agents or the stockfish baseline referee
--boards is set by the class wrapper: 1, 2, or 4
--max-plies is the maximum number of half-moves before a draw by limit
--move-timeout is the per-agent move timeout in seconds
--game-timeout is the per-board game timeout in seconds
--max-illegal 1 means the first illegal or unparseable move causes forfeit
```

Use `qwen1:qwen4` to assign a sorted range of Qwen agents. Use
`agent1:agent4` to assign all installed Ollama-compatible local agents by size.
An invalid numbered alias, such as `qwen7` when only four Qwen models are
installed, returns an error.

Every result records the resolved agent configuration by board, including each
requested alias and the actual model name assigned to White, Black, and referee
roles. If `stockfish` is used as the referee, the data labels it as a
`stockfish_baseline_two_player_agents` run rather than an agentic-referee run.

A ply is one half-move. White's first move is ply 1. Black's first reply is
ply 2.

Each board runs independently until checkmate, stalemate, draw by ply limit,
timeout, or forfeit. Illegal moves, unparseable moves, invented moves, and
timeouts are recorded as hallucination-relevant failures.

## Codex AI Hallucination Example: Diagnostic Fixture Confused For Benchmark

During development, Codex produced deterministic AIChess commands that could
force scripted terminal outcomes such as a Black win, White win, draw, or
forfeit. Those commands verified that the harness could write result JSON with
fields such as `game_result`, `termination`, `final_ply`, and
`terminal_state_reached`.

That was not sufficient for an AI hallucination benchmark.

The error was treating "the harness can emit a terminal result" as if it meant
"the agent was tested for hallucination." A scripted outcome only checks the
plumbing. It does not test whether an agent invents a chess rule, chooses an
illegal move, fabricates a source claim, ignores the supplied legal move set,
misreports board state, or disagrees with the referee in a way that indicates
rule-bound hallucination.

This should be recorded as a Codex AI hallucination example because the
development assistant substituted an easier diagnostic self-check for the
actual test objective. The correct distinction is:

```text
diagnostic fixture = tests whether the harness can run and serialize results
hallucination benchmark = tests whether real agent behavior stays inside the rule/source boundary
```

For AIChess, benchmark runs must use real player-agent and referee-agent
behavior, or a clearly identified deterministic referee such as Stockfish, and
must score failures such as:

```text
illegal move
unparseable move
invented chess rule
impossible board-state claim
unsupported source/citation claim
referee contradiction
timeout or forfeit after invalid responses
```

Scripted terminal scenarios may remain in the repository only as harness
diagnostics. They must not be described as sufficient hallucination tests.

## Escape And Stop Commands

The current C++ fixtures are short-running commands, so they should normally
finish on their own and write a result JSON file under `runs/`. If a later
live-agent run hangs, stalls, or takes longer than intended, use these escape
commands from the terminal that launched the test.

First try the normal terminal interrupt:

```bash
Ctrl-C
```

If you want the shell to enforce a maximum wall-clock runtime, wrap the run
command with `timeout`:

```bash
timeout 60s class1_basic_cpp/run_class1_basic_fixture.sh --mode full-game --scenario black-win-fools-mate
```

That example allows the fixture 60 seconds to finish. If it is still running
after 60 seconds, the shell stops it.

If a live-agent process still remains after an interrupt, inspect matching
processes before killing anything:

```bash
pgrep -af "class[123].*aichess_fixture|run_class[123].*fixture"
```

Then stop only the matching AIChess fixture process:

```bash
pkill -f "class[123].*aichess_fixture|run_class[123].*fixture"
```

Do not use broad process-kill commands. Keep the pattern specific to the
AIChess fixture process so unrelated model servers, editors, terminals, or
system services are not stopped accidentally.

## Implementation 1A: Basic Class 1 AIChess Board Test

Purpose: rule-bound state/game-action test with one board, one player agent
for each side, and one referee.

This is the simplest AIChess test. It is the first test to run when checking
that the local harness is working. It requires exactly three roles:

```text
board_1_white_agent_1
board_1_black_agent_1
board_1_referee_1
```

The current fixture asks only the active side, White, for a move from the
standard chess starting position. The Black player-agent slot is still recorded
so the board assignment is complete and symmetric, but Black does not move in
the current one-move fixture.

Config:

```text
configs/aichess_class1_basic_one_board_one_agent_sides_one_referee_v1_20260715_2110MDT.json
```

Simple run:

```bash
class1_basic_cpp/run_class1_basic_fixture.sh
```

Expected result: the fixture returns `e2e4`, parses it as a UCI move, verifies
that it is legal from the starting position, records one referee vote, and
writes a passing result with `score: 1`.

Override the active White agent response with an invalid response:

```bash
class1_basic_cpp/run_class1_basic_fixture.sh --agent-response "move the queen to z9"
```

`z9` is not a real chess square, and the response does not contain any legal
UCI move. The expected result is `score: 0`, `parsed_uci: null`, and an
`illegal_or_unparseable_move` error.

Override the active White agent response with a different legal response:

```bash
class1_basic_cpp/run_class1_basic_fixture.sh --agent-response "g1f3"
```

The `g1f3` response should pass because it is a legal knight move from the
starting position. The expected result is `score: 1`.

Run the fixture to a terminal Black win:

```bash
class1_basic_cpp/run_class1_basic_fixture.sh --mode full-game --scenario black-win-fools-mate
```

Run the fixture to a terminal White win:

```bash
class1_basic_cpp/run_class1_basic_fixture.sh --mode full-game --scenario white-win-fools-mate
```

Run the fixture to a draw by configured ply limit:

```bash
class1_basic_cpp/run_class1_basic_fixture.sh --mode full-game --scenario draw-max-plies --max-plies 4
```

Run the fixture to a forfeit caused by an invalid or unparseable agent
response:

```bash
class1_basic_cpp/run_class1_basic_fixture.sh --mode full-game --scenario forfeit-invalid
```

Show command help:

```bash
class1_basic_cpp/run_class1_basic_fixture.sh --help
```

Valid command-line options:

```bash
--agent-response TEXT
--mode one-move
--mode full-game
--scenario black-win-fools-mate
--scenario white-win-fools-mate
--scenario draw-max-plies
--scenario forfeit-invalid
--max-plies N
--help
-h
/help
/?
```

In `one-move` mode the expected result is the same as before: the fixture
parses one active-side response, validates it, records one referee vote, and
writes a result with `score: 1` for a legal move. In `full-game` mode the
fixture alternates White and Black plies, records the player role for each
side, records a referee vote for each ply, and stops when it reaches a
terminal outcome. Terminal outputs include `game_result`, `termination`,
`final_ply`, `terminal_state_reached`, and per-ply `referee_votes`.

The current `full-game` mode is still a deterministic fixture path rather than
a full chess engine. It uses curated terminal scripts to prove that the AIChess
harness can record a game that reaches win, draw, or forfeit. A later live-agent
implementation can replace the scripted moves while keeping the same output
fields.

Build the C++ fixture manually if needed:

```bash
class1_basic_cpp/build_class1_basic_fixture.sh
```

## Implementation 1B: Class 1 Three-Referee Option

Purpose: rule-bound state/game-action test with one board, one player agent
for each side, and three referees for stronger referee agreement checking.

This test uses the same one-board shape as the basic Class 1 test, but it
assigns three referees to the board. The board still has only one White
player-agent and one Black player-agent. The extra agents are referee agents,
not additional players.

Use this option when the behavior under test includes referee agreement,
referee majority, or referee disagreement. A legal move should receive at least
two legal votes out of three. An illegal or unparseable move should fail the
referee majority check.

Config:

```text
configs/aichess_class1_one_board_one_agent_sides_three_referees_v1_20260715_2016MDT.json
```

Required team shape:

```text
boards = 1
board 1 white side = 1 player agent
board 1 black side = 1 player agent
board 1 referee team = 3 referees
referee majority = 2 of 3
```

Simple run:

```bash
class1_cpp/run_class1_fixture.sh
```

Expected result: the fixture returns `e2e4`, records three referee votes for
`board_1`, and writes a passing result when at least two referees mark the move
legal.

Override the active White agent response with an invalid response:

```bash
class1_cpp/run_class1_fixture.sh --agent-response "move the queen to z9"
```

The expected result is `score: 0`, a null parsed move, and three referee votes
marking the move invalid.

Override the active White agent response with a different legal response:

```bash
class1_cpp/run_class1_fixture.sh --agent-response "g1f3"
```

The expected result is `score: 1`, selected move `g1f3`, and three legal
referee votes.

Show command help:

```bash
class1_cpp/run_class1_fixture.sh --help
```

Valid command-line options:

```bash
--agent-response TEXT
--help
-h
/help
/?
```

Build the C++ fixture manually if needed:

```bash
class1_cpp/build_class1_fixture.sh
```

Current status: the C++/Bash Class 1 fixture is runnable and writes result JSON
to `runs/`. It uses deterministic fixture responses and fixed start-position
legal moves. A later production runner can replace the fixture response with a
real local model or Qt UI while keeping the same output shape.

## Implementation 2: Basic Class 2 AIChess Board Test

Purpose: workflow/provenance test across two boards, including which board
produced each event, which side agent proposed each move, which referee verified
each board, and whether the artifact accurately records the event log.

Class 2 is about workflow and provenance. The important question is not only
whether a move was legal, but whether the harness can accurately preserve which
board produced which move, which player-agent produced it, which referee
checked it, and which output artifact recorded it.

The basic Class 2 shape repeats the simplest board pattern twice:

```text
board_1 = 1 White player-agent + 1 Black player-agent + 1 referee
board_2 = 1 White player-agent + 1 Black player-agent + 1 referee
```

The current fixture asks the White player-agent on each board for one move.
Board 1 defaults to `e2e4`; board 2 defaults to `d2d4`. Both are legal from
the starting position.

Config:

```text
configs/aichess_class2_two_boards_one_agent_sides_one_referee_each_v1_20260715_2016MDT.json
```

Required team shape:

```text
boards = 2
each board white side = 1 player agent
each board black side = 1 player agent
each board referee team = 1 referee
total referees = 2
```

Simple run:

```bash
class2_cpp/run_class2_fixture.sh
```

Expected result: the fixture records two boards, two active-side candidate
moves, two referee votes, per-board artifact paths, no provenance errors, no
workflow-state errors, and `score: 1`.

Override board 2 with an invalid response while leaving board 1 at its default:

```bash
class2_cpp/run_class2_fixture.sh --board2-response "castle the queen into check"
```

This command simulates a bad response on board 2 only. Board 1 should still
pass. Board 2 should fail with a null parsed move and a board-specific
workflow-state error.

The point of this check is to confirm that Class 2 preserves provenance. The
result should show exactly which board failed rather than collapsing the run
into a single undifferentiated error.

Show command help:

```bash
class2_cpp/run_class2_fixture.sh --help
```

Valid command-line options:

```bash
--board1-response TEXT
--board2-response TEXT
--help
-h
/help
/?
```

Build the C++ fixture manually if needed:

```bash
class2_cpp/build_class2_fixture.sh
```

Current status: the C++/Bash Class 2 fixture is runnable and writes result JSON
to `runs/`. It preserves two board IDs, one active-side player-agent move per
board, one referee vote per board, board-specific artifact paths, and
workflow/provenance error fields.

Optional 3-referee-per-board config:

```text
configs/aichess_option_two_boards_one_agent_sides_three_referees_each_v1_20260715_2110MDT.json
```

For any multi-referee-per-board run, preserve referee-dependent statistics:

```text
referee vote by board and referee
referee agreement by board
referee majority result by board
referee disagreement count by board
referee legal-vote rate by board
referee legal-vote rate by referee
referee latency/error counts by referee when available
```

The 3-referee option is a configuration specification at this stage. It defines
the required result shape for a stronger referee run. A runner that implements
this option must not summarize the three referee votes into one value without
also preserving the individual referee votes and derived referee statistics.

## Implementation 3: Basic Class 3 AIChess Board Test

Purpose: source-bound chess knowledge test using a defined academic or
instructional source packet across four boards. The test should measure whether
per-board side-agent reasoning stays inside the specified source material.

Class 3 is about source-bound knowledge. The current fixture still uses simple
legal-move responses because the academic or instructional source packet has
not been defined yet. The result shape already includes the fields needed for
source-bound testing:

```text
source_academic_class
source_material_id
source_citations_or_packet_references_by_board
unsupported_source_claims
source_boundary_errors
```

The basic Class 3 shape repeats the same one-player-agent-per-side,
one-referee-per-board pattern across four boards.

Config:

```text
configs/aichess_class3_four_boards_one_agent_sides_four_referees_v1_20260715_2016MDT.json
```

Required team shape:

```text
boards = 4
each board white side = 1 player agent
each board black side = 1 player agent
total referees = 4
referee assignment = 1 referee per board
```

Simple run:

```bash
class3_cpp/run_class3_fixture.sh
```

Expected result: the fixture records four boards, four active-side moves, four
referee votes, placeholder source references for each board, no unsupported
source claims, no source-boundary errors, and `score: 1`.

Override board 4 with an invalid source-bound response while leaving boards 1,
2, and 3 at their defaults:

```bash
class3_cpp/run_class3_fixture.sh --board4-response "invent a new source-backed queen castle"
```

This command simulates a bad source-bound response on board 4. Board 1, board
2, and board 3 should still pass. Board 4 should fail with a null parsed move
and a source-boundary error.

The point of this check is to confirm that Class 3 can preserve the board where
the source-bound failure occurred.

Show command help:

```bash
class3_cpp/run_class3_fixture.sh --help
```

Valid command-line options:

```bash
--board1-response TEXT
--board2-response TEXT
--board3-response TEXT
--board4-response TEXT
--help
-h
/help
/?
```

Build the C++ fixture manually if needed:

```bash
class3_cpp/build_class3_fixture.sh
```

Current status: the C++/Bash Class 3 fixture is runnable and writes result JSON
to `runs/`. It preserves four board IDs, one active-side player-agent move per
board, one referee vote per board, source packet placeholder references by
board, unsupported source claim fields, and source boundary error fields.

Optional 3-referee-per-board config:

```text
configs/aichess_option_four_boards_one_agent_sides_three_referees_each_v1_20260715_2110MDT.json
```

For the 4-board 3-referee option, track the same referee-dependent statistics
both per board and across the full four-board run.

The 4-board 3-referee option is a stronger referee-analysis shape. It requires
three referee votes on each of four boards, for twelve referee votes total. A
runner implementing it must preserve both the board-level referee statistics
and the run-level referee statistics.

## Player And Agent Assignment

In these instructions, a `player` is the chess role or side on a board. A
`player agent` is the AI agent instance assigned to that role.

```text
board_1_white_agent_1 = the AI agent playing White on board 1
board_1_black_agent_1 = the AI agent playing Black on board 1
board_1_referee_1 = the AI, deterministic engine, or harness referee checking board 1
```

So "one player agent per side per board" means each board has exactly two
player agents: one assigned to White and one assigned to Black. It does not
mean one agent controls both players, and it does not mean a human player is
being assigned to an agent.

The current Class 1 basic fixture has both a one-move mode and a deterministic
full-game terminal mode. The Class 1 three-referee, Class 2, and Class 3
fixtures are still one-move fixtures. In those one-move fixtures, only the
active side to move, White, produces a candidate move in the current C++
fixture output. The Black player-agent slot is still present in each role map
so the test shape is ready for full-game or alternating-side runners later.

## Reading Result JSON

Each result JSON should be read as an evidence artifact. The most important
fields are:

```text
test_id
test_class
implementation
config_id
board_count
boards
role_map
candidate_moves
selected_move_by_board
referee_votes_by_board
metrics
score
errors or class-specific error fields
notes
```

For Class 1, focus on legal move production and referee validation.

For Class 2, focus on whether the artifact preserves board-specific workflow
and provenance: which board, which player-agent, which referee, which move, and
which artifact path.

For Class 3, focus on source-bound fields. Until a source packet is defined,
the source fields are placeholders and the fixture mainly verifies that the
result shape can carry source references and source-boundary errors.

## Build Behavior

The run scripts automatically build their C++ binary if the binary is missing.
Manual build commands are included so a user can rebuild after source edits or
verify compilation separately from running the test.

The build scripts use `g++` with C++17. They do not install dependencies and do
not modify the system.

## Next Coding Step

For Class 1, Class 2, and Class 3, extend the C++ runners so they:

- read their config JSON files directly,
- optionally calls a local model or Qt UI instead of the fixture response,
- preserve the current board/player-agent/referee output shape,
- writes all board IDs, player-agent moves, referee votes, selected moves,
  board states, timing, model/stack IDs, artifact paths, and error flags into
  `runs/`.

Class 3 source packet fields are still placeholders until the academic or
instructional source packet is defined.

## Harness Language Policy

The preferred local harness baseline is C++/Qt/Bash, with C++ standard library
first and Qt or Boost when they materially reduce complexity. This choice may
affect harness behavior, so results should identify the harness implementation.
The active v1 harness uses no Python runner, Python module, Python cache, or
Python-generated active fixture.
