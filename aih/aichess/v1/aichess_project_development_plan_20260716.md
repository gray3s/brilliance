# AIchess Project Development Plan 2026-07-16

## Purpose

Use the upgraded local memory profile, approximately 24 GB combined RAM plus
swap, to clean up the AIchess move-generation harness and make the failure
classification logically defensible before expanding agent coverage.

Today has two main workstreams:

1. Remove the prompt and retry kludges added to help chatbots recover from
   hallucinated chess moves.
2. Rebuild the move-validation and return-string handling path so illegal or
   malformed moves are treated as measured hallucinations unless a deterministic
   chess rule layer says otherwise.

## Current Concern

The runner currently helps the model with formatting language such as:

```text
Return a UCI move of at least 4 characters.
```

That may improve output shape, but it risks hiding the exact behavior the test
is meant to measure. If a model returns `e4`, and `e4` is not a valid move in
the required protocol for the current board state, the harness should record
that as an invalid or hallucinated move rather than coach the model around it.

The relevant implementation targets are:

```text
./aichess.sh
qwen_ollama_chess_qt/main.cpp
qwen_ollama_chess_qt/qwen_ollama_chess_qt
```

## Workstream 1: Remove Chatbot-Aid Kludges

### Objective

Undo the ad hoc prompt reinforcements and correction behavior that were added
to help hallucinating models produce UCI-shaped output.

### Tasks

- Inventory all model-facing prompts in `main.cpp`.
- Remove prompt text that teaches the model the answer shape during move
  generation, especially "at least 4 characters" wording.
- Preserve neutral task instructions that define the protocol, board state,
  side to move, and legal context.
- Separate normal move prompts from correction prompts.
- Ensure correction prompts do not give the model a free explanatory channel
  that causes the parser to re-read the old illegal move.
- Add a run-mode or config note if we want an explicitly coached experiment
  later, but make uncoached behavior the default.

### Done Criteria

- Default player prompts no longer contain the "at least 4 characters" helper.
- Any 2-character answer such as `e4` is captured as invalid for the current
  protocol unless a future notation parser explicitly supports it.
- Output JSON preserves raw response, parsed move, and failure class.

## Workstream 2: Deterministic Legal-Move Layer

### Objective

Determine whether the legal move rules are deterministic and make that layer
authoritative. If the board state plus chess rules deterministically reject a
move, the harness should not ignore that hallucination.

### Working Name

Use `legal_move_validation_layer` as the working name for the "LUI/LIU" item
until the intended acronym is confirmed.

### Tasks

- Identify which component owns truth for board state and legal move generation.
- Verify Stockfish-backed legal move lists are deterministic for the same FEN.
- Verify the same FEN plus same legal-move list yields stable acceptance or
  rejection for a candidate move.
- Treat legality as a rule-engine result, not an agent opinion, in the default
  measurement path.
- Keep agent referees as comparison data only unless an explicit agent-only
  experiment is selected.
- Define failure classes:
  - malformed response,
  - syntactically valid but illegal move,
  - legal but weak move,
  - board-state hallucination,
  - parser ambiguity,
  - timeout or transport failure.

### Done Criteria

- A move rejected by the deterministic legal-move layer is counted and logged.
- Agent referee disagreement does not erase deterministic illegality.
- Bad legal chess is separated from hallucinated or invalid chess.

## Board Visibility Audit

### Objective

Find out whether the chatbot can see the entire board and whether the runner is
actually giving it the full board before move generation.

### Tasks

- Inspect every player prompt path:
  - Stockfish-backed move prompt,
  - agent-only move prompt,
  - correction prompt,
  - referee prompt.
- Confirm whether each prompt includes:
  - side to move,
  - complete board state,
  - FEN,
  - legal move list,
  - prior move list,
  - current ply number.
- Decide which representation is canonical for the player:
  - FEN only,
  - FEN plus legal moves,
  - board diagram plus FEN,
  - move history plus FEN.
- Log the exact prompt used for each move in the JSON result, or store a stable
  prompt hash plus enough diagnostic detail to reproduce it.
- Add a preflight probe that asks a model to restate the board state from the
  prompt, then compare the answer with the deterministic board state.

### Done Criteria

- We can prove from logs what board state the model received before each move.
- If the model was not given the whole board, that is classified as a harness
  design problem, not a model hallucination.
- If the model was given the whole board and still invents state or moves, that
  is classified as model hallucination or chess-rule failure.

## 2026-07-22 Board-Transition Prompt Policy

V1 now has the basic local stack configuration in the general sense: the
harness can run locally, call Ollama, capture exact harness/Ollama I/O, derive
`mv` from the reported board-state transition, and score `rf` with a
deterministic referee. The remaining question is model/stack capability, not
whether the basic local harness path exists.

The next scoring layer is AIH scoring. This should be fairly straightforward,
though initially somewhat ad hoc: map deterministic outcomes such as valid
`bf`, valid `af`, derived legal `mv`, illegal transition, malformed response,
timeout, retry recovery, and assisted/unassisted mode into a Class1 AIH score.
Keep this scoring transparent and revise it once enough runs show which failure
classes matter most.

Another required work item is the Qwen/Ollama communication layer. Before
over-interpreting model failures, verify that the Ollama API payload,
model-specific options, `think:false` behavior, response-field extraction,
timeout handling, and output-token limits are not distorting the prompt Qwen
receives or the response the harness records. This layer should have its own
small probes so we can separate model state-fidelity failure from transport,
adapter, or configuration failure.

Instrumentation goal: find a way to tap the exact Qwen/Ollama I/O stream. At a
minimum, capture the exact Ollama API request body and exact Ollama API response
body for each move. Then determine whether Ollama exposes any deeper
model-internal Qwen prompt/response stream. If it does not, document `qi/qo` as
inferred from Ollama traffic rather than directly observed. If at least one side
of the Qwen/Ollama boundary is actually open-source and modifiable, it should be
practical to add an I/O tap there rather than treating the boundary as a purely
opaque black box. With closed-source code, this kind of AIH test is necessarily
based on observable I/O assumptions rather than direct instrumentation of the
model boundary. Preferred open-source instrumentation paths: have the
Qwen/model layer log its own received input and emitted output, and/or have the
Ollama layer log the exact prompt and response it passes across the model
boundary. Either tap would let the harness verify model-boundary I/O directly.

The normal player-agent move prompt should not give the agent a list of legal
moves. Giving the legal move list to the player does too much of the chess
work for the agent and weakens the AIH state-fidelity measurement.

Current intended default player protocol:

```text
FEN: <current full FEN>
bf=<the full current board FEN exactly as given>
af=<the full board FEN after your move>
```

The player sees the current game state but not the hidden legal move list, and
does not return a move field. The harness parses `bf` and `af`, then validates
the response privately:

- `bf` must match the harness current FEN.
- `af` must match the deterministic FEN produced by exactly one hidden legal
  move from the current board.
- `mv` is derived by the harness from the matching `bf` -> `af` transition.
- The private legal move set remains an internal referee/checker input and is
  not sent to the player in the normal move request.

This makes the default test closer to "can the agent understand the board and
produce a valid board transition" instead of "can the agent choose an item from
a supplied legal list."

An assisted comparison flag is allowed for controlled A/B testing:

```text
--legal-list
```

When enabled, the move prompt includes both the current FEN and the legal UCI
move list. This is not the default AIH measurement path. It allows the agent to
choose from a supplied legal list and should be interpreted as assisted
move-selection behavior, not proof that the agent independently derived a legal
move from board state.

For `--loglvl 2`, the runner should show only the observed harness/Ollama I/O
and decision strings:

```text
ho: "<full prompt sent by harness to Ollama>"
hi: "<full response returned from Ollama to harness>"
mv: "<parsed move>"
rf: "<referee/checker result>"
```

`ho` and `hi` must preserve the original line breaks and must not merge
multiple prompts or multiple returns. Higher log levels can include timing,
hashes, board summaries, route diagnostics, and other harness internals.

## V2 Local Agent Search And V3 Cloud Baseline

The qwen4 board-transition run showed that removing the legal move list can
make the model fail on the first move. A future research pass should look for a
local, preferably open-source, agentic AI stack that can make at least one legal
move from board state without receiving the legal move list. Candidate stacks
must be able to run locally and should be evaluated for compatibility with
Ollama and/or Tinyproxy/OpenClaw-style local routing.

V2 should stay local. Its main goal is to get a local stack to play a game on
its own with minimal assistance. V2 should explore other local stack
configurations, try to isolate open-source stacks, and determine whether any
local stack can pass the unassisted Class1 board-transition task without
`--legal-list`.

Before moving too far into broader local agentic AI stacks, V2 should first
check other Ollama-compatible local models. The current negative result applies
to the current Qwen/Ollama model list, not necessarily to every model that can
run through Ollama.

V3 should handle cloud-based agentic AI. The same unassisted `bf -> af`
transition test can then be run against OpenAI-hosted agents/cloud models for
comparison. The OpenAI comparison should use the same prompt contract, hidden
legal-move verifier, `mv` derivation, and `rf` scoring as the local runs.

Cloud/OpenAI runs in V3 must also record token use per move/ply. Token
accounting is part of the cloud-stack measurement surface and should be
captured alongside latency, parsed `mv`, `rf`, raw `ho`/`hi`, retries, and
final board result.

V2 priority order: find a local, preferably open-source, stack that can pass
this basic Class1 board-transition task. If no suitable local stack can pass,
record that as the V2 outcome rather than immediately switching to cloud.

V3 can later normalize against a ChatGPT-5 performance baseline. The goal will
be to determine whether one or more local stacks can match the baseline cloud
agent's performance on the same Class1 AIchess board-transition task. Do not
compare raw pass/fail rates across stack categories until the normalization
boundary and baseline target are explicit.

The full local/cloud comparison creates a four-level Class1 AIchess AIH outcome
classification, but the cloud half belongs to V3:

- Both local/open-source stacks and cloud/OpenAI stacks can perform the basic
  unassisted board-transition task.
- Local/open-source stacks can perform the task, but cloud/OpenAI stacks cannot.
- Cloud/OpenAI stacks can perform the task, but local/open-source stacks cannot.
- Neither local/open-source nor cloud/OpenAI stacks can perform the task.

These outcomes should be recorded separately from assisted `--legal-list`
results, because assisted move-selection tests do not measure the same
independent board-state reasoning capability.

Do not advance look-ahead-level experiments until at least one local or cloud
agent can reliably produce a valid single-ply `bf -> af` board transition
without the assisted `--legal-list` prompt.

After candidate discovery and baseline comparison, generate or select a local
open-source stack from the normalized candidate list for continued Class1
AIChess development. The generated/selected stack should be treated as the
preferred local reference path if it can match the ChatGPT-5 baseline on the
basic board-transition task.

V2 should also add an explicit version-reporting flag for the runner, such as
`--version`, so scripts and run artifacts can identify the AIchess harness
version without inferring it from filenames or repository state.

## Move Format Policy

### Objective

Move-shape requirements should come from the chess protocol and parser policy,
not from ad hoc prompt coaching.

### Tasks

- Define the accepted input protocol for v1 as UCI unless explicitly expanded.
- Treat 4-character normal moves such as `e2e4` as UCI candidates.
- Treat 5-character promotion moves such as `e7e8q` as UCI candidates.
- Treat 2-character moves such as `e4` as invalid in UCI mode.
- Record short notation responses as malformed/protocol hallucinations unless
  SAN/LAN support is deliberately added later.
- Avoid asking the model to satisfy the length rule as a hint; let the parser
  and deterministic validator enforce it.

### Done Criteria

- The harness does not need prompt wording to enforce UCI shape.
- The result file says why a 2-character response was rejected.

## Custom Return-String Parser

### Objective

Replace "first UCI-looking token wins" with a parser that is designed to find
the move the model is actually proposing, not the error it is discussing.

### Parser Inputs

The parser may need:

- prompt text that produced the reply,
- raw model return string,
- prompt type,
- side to move,
- current FEN,
- legal move list,
- previously rejected move,
- correction attempt number,
- accepted notation mode.

### Parser Behavior

- Prefer exact machine-readable responses if present.
- Always keep the prompt/reply pair available to the parser or parser log.
- Extract all candidate UCI-shaped tokens, not just the first one.
- In correction mode, ignore the previously rejected move unless the model
  explicitly returns only that same move.
- Prefer legal candidates over illegal candidates when the prompt asked for a
  move and multiple candidates appear.
- Preserve all candidates and parser reasoning in JSON.
- Never convert explanatory text into success unless a move candidate survives
  deterministic validation.

### Done Criteria

- A response such as "b1c1 is illegal, try g1f3" does not re-submit `b1c1`.
- Parser ambiguity is visible in the run artifact.
- The parser is tested with fixture strings before live agent runs.

## Agent Evaluation Expansion

### Objective

After the core AIchess algorithm makes logical sense, compare the smallest
local Qwen agent against one cloud move-proposal agent. Do not run larger local
Qwen models during this comparison pass.

### Gate

Do not run cloud comparison probes until the fundamentals are organized:

- prompt policy is neutral,
- move parser behavior is explicit,
- deterministic legality is authoritative,
- board visibility is auditable,
- result artifacts separate invalid format, illegal move, legal weak move,
  timeout, and parser ambiguity.

### Local Baseline

Use only:

```text
qwen1 = qwen2.5-coder:3b
```

No larger or fatter local agents in this pass.

### Deferred Local Qwen Candidates

- Current Qwen aliases through Ollama:
  - `qwen2`
  - `qwen3`
  - `qwen4`

These are explicitly deferred until after the qwen1-vs-cloud comparison.

### Cloud Candidate

Add one cloud-backed move-proposal backend and run it through the same
deterministic parser and Stockfish validation path.

The cloud agent must not replace Stockfish as rules authority.

### Later Stack Foundation Work

Changing the wider agent stack foundation or adding non-Qwen local
Ollama-capable agents should be handled last and tracked as a separate follow-on
item.

### Constraints

- Do not compare agents until move validation and parser behavior are stable.
- Each agent run must record:
  - model or provider,
  - proxy path,
  - prompt text or prompt hash,
  - raw return,
  - parsed candidates,
  - deterministic legality result,
  - timeout and transport status.

### Done Criteria

- qwen1 and the cloud agent are compared under the same cleaned prompt, parser,
  and Stockfish validation path.
- Larger local Qwen models remain deferred.
- Non-Qwen local stack work remains deferred.

## Suggested Order For Today

1. Confirm current swap state and model runtime health.
2. Snapshot current AIchess behavior with one short baseline run.
3. Remove prompt-format kludges from move and correction prompts.
4. Define UCI-only parser policy for v1.
5. Implement or specify the custom return-string parser.
6. Add fixture tests for malformed, illegal, explanatory, and correction
   responses.
7. Audit prompt board visibility and logging.
8. Run one local Qwen baseline after cleanup.
9. Add or configure one cloud move-proposal backend.
10. Run qwen1 versus the cloud backend under the same baseline probe.

## Non-Goals For Today

- Do not optimize chess strength.
- Do not hide illegal moves through retry coaching.
- Do not let an agent referee override deterministic move legality.
- Do not expand to larger local models during the qwen1-vs-cloud pass.
- Do not change the stack foundation during the fundamentals pass.

## Primary Deliverables

- Cleaned AIchess prompt policy.
- Deterministic legal-move validation policy.
- Board-visibility audit result.
- Custom parser design or implementation.
- Fixture tests for parser and legality classification.
- A qwen1-vs-cloud comparison sequence for after the fundamentals pass.

## Late-Night Development Notes 2026-07-21/2026-07-22

### Qwen Return-String Failure Mode

User observation: the Qwen agentic move generator sometimes returns a paragraph
of commentary when asked for a new move. The reply may discuss an old move and
never provide a new move, or it may bury a new move inside discussion of the old
move. This is a harness-relevant behavior, not just a weak chess behavior.

Implications for the parser:

- Preserve the full raw response.
- Extract every UCI-shaped candidate from the response.
- In correction mode, ignore the previously rejected move unless the model only
  repeats that move.
- Prefer a deterministic legal candidate from the current legal-move set.
- Log parser ambiguity separately from transport failure, timeout, illegal move,
  and no-candidate output.

### Qwen4 Run Attempt

Requested check: see whether `qwen4` can actually play a game.

Current alias resolution with the installed Ollama models:

```text
qwen1=qwen2.5-coder:3b
qwen2=qwen:4b
qwen3=qwen2.5:latest
qwen4=robit/qwen3.5-9b-r7-research:q4km
```

Direct runner command used:

```text
./qwen_ollama_chess_qt/qwen_ollama_chess_qt --mode aichess --boards 1 --loops 1 --referee-count 1 --max-illegal 1 --models qwen2.5-coder:3b,qwen:4b,qwen2.5:latest,robit/qwen3.5-9b-r7-research:q4km -w qwen4 -b qwen4 -r harness --gmto 120 --mxply 6 --stack-timeout 90 --otkns 96
```

Result artifact:

```text
runs/qwen_ollama_chess_timeout_probe_qt_20260715/qwen_ollama_chess_timeout_probe_qt_20260715_20260721_235001_summary.md
```

Observed result: the model reached Ollama and completed, but returned an empty
visible response twice with `done_reason="length"`. The game ended before ply 1:

```text
termination=white_forfeit_invalid_or_unparseable_move
plies=0
legal_moves=0
illegal/unparseable=1
elapsed=96.766s
```

Follow-up direct `ollama run` probes showed the same practical problem from the
other direction: qwen4 starts with extended reasoning/commentary about the board
and legal moves instead of promptly returning a single UCI move. A `/no_think`
probe did not quickly produce a clean visible UCI answer before the probe was
stopped.

Updated result: qwen4 requires API-level `think:false` in the Ollama
`/api/generate` payload. Prompt-only `/no_think` still consumed the response
budget in the separate `thinking` field and left the visible `response` empty.
After adding `payload["think"] = false` in `askOllama`, qwen4 played a short
game under the harness.

Successful run command:

```text
./aichess.sh --models qwen2.5-coder:3b,qwen:4b,qwen2.5:latest,robit/qwen3.5-9b-r7-research:q4km -nb 1 -nl 1 -gmto 240 -cnrtlm 1 -mxply 6 -sto 120 -otkns 64 -lkahdlvl 0 qwen4
```

Successful result artifact:

```text
runs/qwen_ollama_chess_timeout_probe_qt_20260715/qwen_ollama_chess_timeout_probe_qt_20260715_20260722_002409_summary.md
```

Observed result:

```text
termination=draw_by_configured_ply_limit
plies=6
legal_moves=6
illegal/unparseable=0
elapsed=153.078s
```

Move sequence observed in stderr: `e2e4 e7e5 e1e2 d7d5 e4d5 d8d5`.

### Flag-Name Cleanup Backlog

User wants to revisit the AIchess flag names later this week and make them more
obvious. Keep current behavior stable for now, but plan a flag naming pass that
maps shorthand flags to clearer names in help text and examples.

Reason: several current flags are hard to remember without reading the
documentation. The next pass should prioritize obvious, self-documenting aliases
for normal use while retaining compatibility aliases for existing scripts and
run logs.

Likely cleanup targets:

- `-nb` -> board count wording should be clearer.
- `-nl` -> loop/batch count wording should be clearer.
- `-mxply`, `-mt`, `-sto`, `-otkns`, `-lkahdlvl`, `-bap`, `-cnrtlm`, and
  `-gmto` need more obvious user-facing names or aliases.
- Keep compatibility aliases until existing run logs and scripts are migrated.

### Codex / Agentic Stack Planning

User used a limit reset to get Codex running tonight. It reportedly remains good
until mid-August 2026. Track this as operational context, not as a technical
guarantee.

Open research item for later: find another open-source agentic AI agent that can
work with Tinyproxy and/or Ollama from OpenClaw. Do not spend a large research
budget on this during the late-night pass.

### Class1 Arithmetic Test Notes

Found the incoming Brilliance notes for the new Class1 arithmetic experiment:

```text
/home/sag/RPA2/myLLC/AI/brilliance/incoming/New_Class1_AIH_test-no2-2026720.txt
/home/sag/RPA2/myLLC/AI/brilliance/incoming/20260720-213000MT__AIH_Class1_test2_cpp_v1(1).zip
```

Summary: generate 10 random integer sets, choose a target integer greater than
or equal to the sum of the set, ask an AI to use arithmetic operations to get as
close as possible to the target while using as many numbers as possible, then
score the answer deterministically. The note explicitly frames this as a Class1
AIH supplement to AIchess and raises the broader point that AIH can be avoided
by using AI to build deterministic programs and then checking answers with a
deterministic verifier.

Do not expand Class2 or Class3 AIH test background research tonight; user
suspects that earlier background research for those tests drained the available
token allocation.

### ezfile.azcourts.gov Filing Limits

Incoming note found:

```text
/home/sag/RPA2/incoming/ezfile-azcourts-gov_Filing_Restrictions_20260721.txt
```

Operational filing constraints to remember:

- Individual document file limit: 9.5 MB.
- Combined submission target: no more than 100 MB.
- Submissions over 80 MB risk transmission failure.
- Allowed formats: `.PDF`, `.DOCX`, `.ODT`.
- Proposed orders should be modifiable text such as `.DOCX` or `.ODT`.
- Avoid commas, apostrophes, and special characters in filenames.
- For oversized PDFs, use 300 DPI maximum scanner settings or compress the PDF.

Source URL noted in incoming file:

```text
https://www.azcourts.gov/efilinginformation/eFiling-Requirements
```
