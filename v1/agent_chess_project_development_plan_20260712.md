# Brilliance v1 Candidate: Agentic AI Chess Self-Play Test

Created: 2026-07-12

## Proposed Name

Working name:

```text
AI Chess Match
```

Expanded name:

```text
AI Chess Match: Agentic-AI Hallucination and State-Fidelity Validation Test
```

Rationale:

This should be treated as a legitimate AIH validation test rather than as
another HybridAI version by itself.

HybridAI versions can be developed as standalone local-agent implementations.
The AI Chess Match can then be used as a validation harness against those
implementations, cloud agents, and mixed local/cloud pairings.

This separation keeps the architecture cleaner:

```text
HybridAI v1, v2, v3, ... = agent implementations
AI Chess Match           = AIH validation and comparison harness
Brilliance v1            = broader problem-analysis context
```

In that model, HybridAI is one class of system under test. AI Chess Match is
the AIH test environment.

Subproject nuance:

AI Chess Match can still live as a subproject of HybridAI when the work is
being used to develop, smoke-test, or compare HybridAI versions. There is no
conflict as long as the role is clear:

```text
as AIH artifact:
  AI Chess Match is a validation test for hallucination, state fidelity, and
  bounded agent behavior.

as HybridAI artifact:
  AI Chess Match is a HybridAI subproject and smoke test used to guide local stack
  development and compare HybridAI versions against other agentic AI stacks.
```

So the distinction is not ownership-exclusive. It is role-based.

AIH test development:

AI Chess Match should also help develop the broader concept of an AIH test.
It is one concrete test case, but the design lessons should generalize.

Questions for the broader AIH test concept:

- What makes a test bounded enough to be fair?
- What evidence must be preserved?
- How should hallucination be distinguished from ordinary error?
- How should state fidelity be measured?
- How should timing and resource limits be handled?
- How should local and cloud agents be compared?
- How should human assistance be recorded?
- What counts as a useful failure?
- What result is strong enough to guide the next HybridAI version?
- How should AI performance be measured beyond win/loss?
- How reliable is the agent across repeated runs?
- How suitable is a given agent or stack for the task being tested?
- What cost, latency, maintainability, and operational constraints affect
  suitability?

In that sense, AI Chess Match is both a test and a test-design laboratory.

Scope warning:

The AIH test space is large enough that the project could spend the next year
writing hallucination tests without exhausting the subject.

Therefore, AIH development needs a disciplined selection process:

- choose small bounded tests,
- prefer tests that preserve evidence automatically,
- prefer tests that compare agents under identical conditions,
- prefer tests that reveal multiple failure modes at once,
- avoid building a huge test catalog before any test is run,
- convert lessons from each test into reusable AIH design rules.

AI Chess Match is valuable partly because it is a first concrete test rather
than an endless taxonomy exercise.

Multi-agent extension:

AI Chess Match should include a planned path for 2-, 3-, 4-, and 5-agent
Class 1 tests. The useful extension is not just more players. It is role
separation:

```text
primary player
planner
spare recommender
referee
observer/logger
arbiter
```

Two agents can establish the player/referee baseline. Three and four agents can
test planning plus alternative move recommendations from spare agents. Five
agents can add multiple referees or an arbiter. This should be directly useful
because local resource use should scale with agent size and role: smaller local
agents may be enough for spare recommendations or logging, while stronger
agents can be reserved for planning or final move selection.

The same Class 1 test may also mix different agents or stacks in different
roles. Mixed-agent tests should record the model/stack used for each role and
preserve referee disagreement, alternative recommendations, final move choice,
and legality outcomes.

Detailed starter plan:

```text
v1/AIH/AIchess/v1/multi_agent_chess_test_plan_20260715_1942MDT.md
```

Benchmark package direction:

AI Chess Match should also support a local-hardware vs cloud-software benchmark
package. The useful comparison is not broad model superiority. It is bounded
Class 1 behavior under identical prompts, positions, time controls, output
schemas, and referee rules.

The package should preserve local hardware manifests, cloud software manifests,
run logs, result JSON, scoring summaries, cost/resource notes, and role maps for
single-agent and multi-agent configurations.

Detailed benchmark package note:

```text
v1/AIH/AIchess/v1/local_vs_cloud_benchmark_package_20260715_1943MDT.md
```

Related AIH suite draft:

```text
v1/AIH_TEST_SUITE_v1_20260712.md
```

Current project implication:

The project already has a working HybridAI v1 realization. AI Chess Match can
now become a concrete AIH test applied to that working v1 stack. Future
HybridAI development can then branch or version around the chess validation
target as needed:

```text
HybridAI v1          = existing working local stack
HybridAI v1-chess    = v1 adapted for AI Chess Match validation
HybridAI v2          = later implementation informed by v1/v1-chess results
AI Chess Match       = independent AIH test harness
```

The exact branch/version naming can remain open, but the architectural point is
fixed: the chess match validates agent implementations; it is not itself the
agent implementation.

Recommended versioning strategy:

Create an AI Chess variant for each HybridAI version:

```text
HybridAI v1        -> HybridAI v1-chess
HybridAI v2        -> HybridAI v2-chess
HybridAI v3        -> HybridAI v3-chess
...
HybridAI vn        -> HybridAI vn-chess
```

Process:

1. Start with the current HybridAI version.
2. Apply the AI Chess Match harness to create the corresponding chess-adapted
   variant.
3. Run the smoke test and collect results.
4. Use those results to guide the next HybridAI version.
5. Repeat for the next version.

This gives the project a legitimate and useful smoke test for future HybridAI
versions. The test does not need to prove intelligence in general. It only
needs to reveal whether the current agent stack can maintain state, follow
rules, produce valid actions, recover from errors, and operate under timing
constraints inside a bounded formal environment.

Strategic reframing:

The goal is no longer to build a local AI stack that is simply "better than
Codex" in some broad sense.

The goal is to evaluate HybridAI relative to Codex or any other agentic AI
stack under the same bounded validation harness.

```text
old_goal =
build a local AI stack better than Codex

new_goal =
measure HybridAI, Codex, and other agentic AI stacks against the same formal
AIH smoke tests and compare their observed strengths, weaknesses, costs,
latency, hallucination rates, state fidelity, and error recovery
```

This is a stronger engineering posture. It avoids vague superiority claims and
replaces them with comparable evidence.

## Concept

Use chess as a bounded test environment for comparing agentic AI behavior.

Two agentic AI agents play a complete game of chess automatically. The agents
choose colors at random, start the game, make legal moves, and finish the game
without human intervention.

The test does not require the agents to be elite chess engines. The value is
watching how agentic systems behave inside a fully bounded rule system where:

- legal moves are objectively defined,
- board state is observable,
- win/loss/draw outcomes are measurable,
- illegal moves can be detected,
- decision traces can be logged,
- repeated games can produce statistical profiles.

## Core Hypothesis

If the same agentic AI implementation plays against itself repeatedly under
the same rules, its behavior should follow a relatively stable statistical
model.

If different agentic AI implementations play against each other repeatedly,
each pairing should produce an associated statistical model.

Those statistical models can then be compared.

## Why Chess Fits Brilliance v1

Chess is useful because it separates several layers of agent behavior:

1. Rule compliance:
   Can the agent make only legal moves?

2. Board-state fidelity:
   Can the agent maintain an accurate representation of the game?

3. Planning:
   Can the agent reason beyond the next move?

4. Error recovery:
   What happens if the agent proposes an illegal or incoherent move?

5. Style:
   Does the agent play aggressively, defensively, randomly, repetitively, or
   strategically?

6. Brilliance candidate behavior:
   Does the agent ever produce a non-obvious but objectively strong move?

7. Hallucination behavior:
   Does the agent invent board states, claim impossible moves, misread pieces,
   or overstate its reasoning?

## Board Representation

Use a standard chess coordinate system. If a simplified internal naming scheme
is needed, rank squares alphabetically by file and numerically by rank:

```text
a1, a2, ... a8,
b1, b2, ... b8,
...
h1, h2, ... h8
```

For implementation, standard algebraic notation or UCI notation should be
preferred because existing chess libraries already support them.

Examples:

```text
e2e4
g8f6
e7e8q
```

## Recommended Implementation Boundary

Do not hand-roll chess legality if a library is available.

Use a proven chess rules library for:

- legal move generation,
- board-state validation,
- check/checkmate/stalemate detection,
- FEN/PGN export,
- draw rules where feasible.

Likely implementation choice:

```text
python-chess
```

Reason:

The test is about agentic decision behavior, not whether this project can
reimplement chess rules correctly.

## Project-Development Plan Layer

This work is also a project-development-plan project.

The chess harness is not only a test of model behavior. It is also a practical
case study in how a human/AI development process should decide when an agent is
ready to run bounded research work with less supervision.

That means the project must track two layers:

1. Test implementation:
   referee, prompts, move parsing, legality checks, timing, records, and
   metrics.

2. Research-management implementation:
   scope control, time boxes, stopping rules, progress reports, evidence
   preservation, escalation to human input, and limits on autonomous test
   execution.

The second layer is not optional. Without it, the agent may be able to run
commands while still being unsuitable for managing a research process.

## Autonomous Research Readiness Path

Before an AI agent is allowed to run AIH chess research autonomously, the
project should pass through staged readiness levels:

1. Human-directed single run.
2. Human-approved small batch.
3. Agent-managed batch under fixed written constraints.
4. Agent-managed batch with automatic result summaries and artifact links.
5. Agent-proposed next experiment, requiring human approval before execution.

Each stage must define:

- allowed models and runtimes;
- maximum runtime per run and per batch;
- maximum invalid attempts;
- maximum invalid-attempt duration;
- stop conditions;
- required progress-update cadence;
- required result artifacts;
- what the agent may not change without approval;
- when the agent must ask for user input.

The agent is not ready for autonomous research management until it can stop
when the test is no longer producing useful evidence, preserve the evidence it
already has, and ask for human input before proceeding down an unclear,
unsafe, or hostile path.

## Shared-File Referee Architecture

The Python layer should be treated as the referee, not as an agent.

A shared-file design makes that role explicit:

```text
referee process:
  owns board_state.json
  owns game_log.jsonl
  validates proposed moves
  updates board state
  sets board status flags

agent_white:
  watches board_state.json
  writes proposals/white_move.json

agent_black:
  watches board_state.json
  writes proposals/black_move.json
```

The agents can watch the board in near real time, but neither agent is allowed
to directly mutate the board. Only the referee can apply a move.

### Referee-Owned Board State

`board_state.json` should contain:

```json
{
  "game_id": "game_001",
  "ply": 0,
  "side_to_move": "white",
  "status": "waiting_for_white",
  "fen": "startpos-or-current-fen",
  "legal_moves": ["e2e4", "d2d4"],
  "last_move": null,
  "last_move_legal": null,
  "result": null,
  "termination": null,
  "white_time_bank_s": 20.0,
  "black_time_bank_s": 20.0,
  "move_time_increment_s": 10.0,
  "updated_at": "timestamp"
}
```

### Board Status Flags

Use explicit status flags so agents and humans can understand the board state
without inferring it from partial data:

```text
initializing
waiting_for_white
waiting_for_black
validating_white_move
validating_black_move
move_applied
illegal_move_retry
move_fault
time_fault
checkmate
stalemate
draw
game_over
aborted
```

### Candidate Clock Models

Timing should be treated as part of the experiment design, not settled too
early.

One candidate is a sliding time-window clock.

Candidate rule:

If player A takes time `t_move` to return a valid move, compare `t_move` to
the time available for that move:

```text
available_time = player_time_bank + Tlimit_per_move

if t_move <= available_time:
    player_time_bank = available_time - t_move
    move may be validated and applied
else:
    player loses by time_fault
```

This is essentially a `Tlimit_next = Tlimit_previous + unused_time` idea. It is
interesting, but not yet accepted as the final timing rule.

Possible alternatives:

1. Fixed per-move timeout.

```text
each move must complete within Tlimit_per_move
```

Simple and harsh. Good for latency testing, but may unfairly punish slower
models before they can show useful reasoning.

2. Conventional chess clock.

```text
each side starts with T_total
each move consumes t_move
optional increment adds T_increment after a valid move
```

Closer to real chess timing and easier to explain publicly.

3. Warm-up-excluded clock.

```text
run one untimed model warm-up call
then start timing actual game moves
```

Useful because first local model calls can include model-load overhead that is
not really chess reasoning time.

4. Sliding time-window / accumulating reserve.

```text
available_time = player_time_bank + Tlimit_per_move
unused time remains in the player's bank
```

Useful for rewarding speedy play and letting agents accumulate flexibility,
but it may over-reward fast trivial moves.

5. Unused-time feedback clock.

Another candidate is to compute the unused time for step `i`, add that unused
time to the current limit for step `i`, and use the result as the time limit
for step `i + 1`:

```text
Tlimit[i + 1] = Tlimit[i] + (Tlimit[i] - t_move[i])
```

Interpretation:

The time limit grows when a player moves faster than the current limit. This
rewards efficient move production and creates a reserve-like effect without
directly rewarding slow moves.

Expected practical effect:

This will probably give each player more than enough time after the first few
successful moves. If a player still exceeds the available limit, that failure
is more clearly a real `time_fault` rather than an arbitrary timeout artifact.

Open concern:

This may over-reward very fast shallow moves and create runaway timing
advantages. If used, it should be tested separately from fixed-time and
conventional-clock variants so its effect on game outcome is visible rather
than hidden.

Monitoring requirement:

The referee must monitor the timing model as the game progresses. For the
unused-time feedback clock, log the full sequence:

```text
move_index
t_move[i]
Tlimit[i]
Tlimit[i + 1]
time_fault_margin
```

The referee should flag cases where the timing rule appears to dominate the
game more than chess quality does, for example:

- time limits grow without bound,
- one player receives a runaway timing advantage,
- slow invalid moves are indirectly rewarded,
- the winner is determined primarily by clock mechanics rather than legal move
  quality,
- the timing model prevents meaningful comparison between local and cloud
  stacks.

Evaluation purpose:

- Fast play is rewarded because unused time remains in the player's time bank.
- Slow play is allowed if the player has accumulated enough time.
- A slow first move can be handled by giving each player a reasonable starting
  time bank or by running a warm-up call before the timed game begins.
- Move time becomes an experimental factor rather than only a diagnostic log.

Any timing model should make it possible to evaluate:

- whether a larger/slower AI stack plays better when given enough time,
- whether a smaller/faster stack wins by speed despite weaker reasoning,
- whether time pressure increases illegal moves or hallucinated board states,
- whether a local stack has a practical latency advantage over a cloud stack,
- whether extra thinking time correlates with better moves or only with slower
  failure.

### Proposal Files

Agents write proposal files instead of editing board state:

```json
{
  "game_id": "game_001",
  "ply": 3,
  "agent_id": "hybridai_v1_local",
  "side": "white",
  "proposed_move": "g1f3",
  "raw_response": "g1f3",
  "confidence": null,
  "created_at": "timestamp"
}
```

The referee reads the proposal, validates it, logs it, and then either applies
the move or updates the status to `illegal_move_retry`, `move_fault`, or
`time_fault`.

### Advantages

- Both agents can watch the same board in real time.
- The board has one source of truth.
- Slow agents do not block the referee from logging status.
- Human observers can inspect the current state without parsing terminal logs.
- Local, cloud, and manual agents can all use the same adapter pattern.
- The same design can later be moved from Python to C++ if needed.

### C++ Migration Note

Python is useful for the first referee because `python-chess` is mature and
fast to integrate. A later HybridAI-native version can move the shared-file
referee into C++ while preserving the same file protocol.

## Stage 0: Design

Deliverables:

- exact game protocol,
- board representation standard,
- move format,
- agent prompt template,
- logging schema,
- invalid-move handling rule.

Exit criteria:

- a human reader can understand how two agents will play without ambiguity.

## Stage 1: Single-Agent Self-Play Harness

Run the same agent implementation as both White and Black.

Process:

1. Randomly assign internal agent instance A or B to White.
2. Present current board state to the side to move.
3. Ask for one legal move in the required format.
4. Validate the move with the chess library.
5. If legal, apply it.
6. If illegal, log the failure and apply the invalid-move policy.
7. Continue until checkmate, stalemate, draw, resignation, move limit, or
   repeated invalid-move termination.

Exit criteria:

- the system can complete at least one full game without manual intervention.

## Stage 2: Repeated Self-Play Statistical Model

Run many games with the same agent against itself.

Metrics:

- White win rate,
- Black win rate,
- draw rate,
- average game length,
- illegal move rate,
- repeated position frequency,
- opening distribution,
- material blunder frequency,
- checkmate/stalemate frequency,
- termination reason,
- move-time or token-cost per move,
- self-reported confidence per move, if available.

Purpose:

Build the same-agent baseline statistical profile.

Expected result:

The same agent should not produce identical games every time, but repeated
games should reveal a behavior distribution.

## Stage 3: Cross-Agent Pairing

Run different agent implementations against each other.

Examples:

```text
AgentA vs AgentB
AgentA vs AgentC
AgentB vs AgentC
```

For each pairing, randomize color assignment and run enough games to produce a
preliminary statistical model.

Metrics:

- pairing win/loss/draw rates,
- color advantage,
- illegal move asymmetry,
- average game length,
- style differences,
- hallucination incidents,
- recovery behavior,
- prompt sensitivity.

## Stage 3A: Local Stack vs Cloud Stack Comparison

Use the same chess harness to compare a local AI stack against a cloud AI
stack.

Purpose:

Determine whether local and cloud agentic systems differ in measurable ways
when forced to operate inside the same bounded chess environment.

Comparison conditions:

- same board representation,
- same move format,
- same invalid-move policy,
- same prompt template,
- same legal-move visibility condition,
- same move limit,
- same logging schema,
- randomized color assignment,
- enough repeated games to avoid overreading a single result.

Candidate pairings:

```text
local_stack_agent vs local_stack_agent
cloud_stack_agent vs cloud_stack_agent
local_stack_agent vs cloud_stack_agent
cloud_stack_agent vs local_stack_agent
```

The local stack should not be limited to Qwen. The harness should use an
adapter boundary so multiple top-level local agent layers can be compared:

```text
Ollama/Qwen adapter
Ollama/non-Qwen adapter
qwen CLI adapter
HybridAI UI-mediated adapter
future local agent adapter
cloud API adapter
manual cloud-agent adapter
online chess-bot adapter
```

This prevents the experiment from confusing "HybridAI v1" with one specific
model family. Qwen is the first available local family, not the entire test
space.

Online chess-bot option:

AI Chess Match could also test an agent against an online chess bot, provided
the platform rules and API allow automation.

This should be treated as a future integration, not the default first run.
Before attempting it, the project should verify:

- platform terms of service,
- bot/API rules,
- whether automated play is allowed,
- account requirements,
- rate limits,
- whether games can be exported as PGN,
- whether the online bot's strength/settings are stable enough for comparison.

Metrics:

- legal move rate,
- illegal move recovery rate,
- board-state fidelity,
- hallucinated board-state claims,
- average game length,
- win/loss/draw rate,
- termination reason,
- latency per move,
- token or compute cost per move,
- local resource usage,
- cloud API/resource cost,
- repeatability across runs,
- sensitivity to prompt scaffolding,
- effect of human/tool assistance.

Clock rule:

Each side should have a time bank. On each move, the side receives a per-move
time allowance. If actual move time `t_move` is less than the per-move limit
`Tlimit_per_move`, then the unused difference remains available in that side's
time bank.

```text
available_time_before_move = side_time_bank + Tlimit_per_move
t_move = elapsed time for the move decision

if t_move <= available_time_before_move:
    side_time_bank = available_time_before_move - t_move
else:
    side loses by time-fault
```

Purpose:

This rewards speedy valid play and lets the experiment evaluate move-time as a
factor in overall success. A fast but legal agent accumulates time flexibility;
a slow agent may lose even if its moves are legal.

Relevant timing metrics:

- average move time,
- maximum move time,
- time-bank remaining at game end,
- time-fault rate,
- relationship between move time and move quality,
- relationship between move time and illegal-move rate.

Important separation:

```text
local_vs_cloud_result =
agent behavior under a specific harness, model, prompt, runtime, and tool
configuration; not a universal claim that local AI or cloud AI is better
overall
```

This comparison should record full run identity:

```text
local_stack:
  model:
  quantization:
  hardware:
  inference runtime:
  prompt:
  temperature_or_sampling:

cloud_stack:
  provider:
  model:
  API surface:
  prompt:
  temperature_or_sampling:
  tool availability:
```

Expected value:

This creates a practical way to compare local AI and cloud AI without relying
on vague impressions. The chess harness gives both systems the same formal
task and exposes differences in state handling, rule compliance, cost, speed,
and hallucination behavior.

## Stage 4: Human-Assisted Variant

Introduce a human assistance rule after agent waffling or illegal moves.

Examples:

- remind the agent of legal move format,
- provide the list of legal moves,
- provide a board diagram,
- ask the agent to explain its candidate move before finalizing,
- require the agent to choose from legal moves only.

Compare:

```text
unassisted_agent_performance
assisted_agent_performance
```

Question:

Does structured human assistance improve legality, coherence, planning, or
outcome quality?

## Stage 5: Brilliance / Hallucination Analysis

Classify notable moves and failures.

Possible classifications:

- legal but weak,
- legal and routine,
- legal and strong,
- non-obvious candidate move,
- illegal move,
- impossible board-state claim,
- invented tactical justification,
- self-correction after validation failure,
- confidence mismatch,
- genuine uncertainty,
- waffle.

Important caution:

A move should not be called brilliant merely because the agent describes it
beautifully. It should be evaluated against board state and, where possible,
by a chess engine or human chess review.

## Invalid-Move Policy Options

Option A: Strict termination.

```text
First illegal move loses.
```

Useful for measuring raw rule compliance.

Option B: Retry limit.

```text
Allow N retries, then forfeit.
```

Useful for testing correction behavior.

Option C: Legal-move list assist.

```text
If illegal, provide the legal move list and ask the agent to choose again.
```

Useful for testing human/tool assistance.

Recommendation:

Run all three variants separately. Do not mix them inside one statistical
series.

## Logging Schema

Each game should record:

```text
game_id:
date_time:
agent_white:
agent_black:
agent_versions:
color_assignment_method:
rules_library:
invalid_move_policy:
initial_position:
move_limit:

move_log:
  - move_number:
    side_to_move:
    board_fen_before:
    agent_prompt:
    agent_raw_response:
    parsed_move:
    legal:
    board_fen_after:
    validation_notes:
    confidence:
    time_or_tokens:

result:
termination_reason:
summary_metrics:
hallucination_incidents:
brilliance_candidates:
```

## Statistical Model

For each agent or agent pairing, produce:

```text
agent_pair:
number_of_games:
white_win_rate:
black_win_rate:
draw_rate:
illegal_move_rate:
average_game_length:
opening_distribution:
termination_distribution:
hallucination_rate:
assistance_effect:
notable_patterns:
```

The first model does not need to be statistically definitive. It only needs to
be honest about sample size and uncertainty.

## Test Plan

### Test 1: Rules Harness

Goal:

Verify that the chess library rejects illegal moves and accepts legal moves.

Pass condition:

Known legal and illegal move fixtures behave correctly.

### Test 2: One Complete Self-Play Game

Goal:

Run one same-agent game to completion or controlled termination.

Pass condition:

Every turn is logged and the result is explainable.

### Test 3: Repeated Self-Play

Goal:

Run a small batch of same-agent games.

Pass condition:

Produce preliminary metrics without hiding invalid moves or failures.

### Test 4: Cross-Agent Pairing

Goal:

Run at least one different-agent pairing.

Pass condition:

Produce comparable metrics for both sides and both color assignments.

### Test 5: Assistance Comparison

Goal:

Compare unassisted play with assisted play.

Pass condition:

The report identifies whether assistance reduced illegal moves, improved
coherence, or changed outcomes.

## v1 Fit

This chess test could become a real `brilliance/v1` candidate because it is:

- bounded,
- repeatable,
- measurable,
- understandable to the public,
- good at exposing hallucination,
- good at separating fluent explanation from valid action,
- useful for comparing local and cloud AI stacks under the same harness,
- suitable for comparing same-agent and cross-agent statistical behavior.

It is not a cash-reward problem by itself, but it may be a strong preliminary
benchmark before attempting prize/cash-reward problems.

## Assistant Analysis

This is a strong Brilliance candidate because it converts an abstract question
about "AI brilliance" into a bounded behavioral test.

The key strength is that chess gives the project an objective rules layer.
Many AI-evaluation conversations drift because the target is subjective:
usefulness, insight, fluency, or satisfaction. Chess does not eliminate
judgment, but it anchors the experiment in observable facts:

- Was the move legal?
- Did the board state update correctly?
- Did the game terminate for a real chess reason?
- Did the agent describe a position that actually exists?
- Did the agent's confidence match the move quality?

That makes chess useful for hallucination analysis. An agent can hallucinate a
file, a fact, or an argument; in chess it can hallucinate a legal move, a
piece, a threat, a checkmate, or a board state. Those failures are easier to
detect because the rules are formal.

The plan also has value because same-agent self-play creates a baseline.
If one agent implementation plays itself repeatedly, its statistical behavior
should reveal a characteristic profile: illegal-move rate, game length,
opening choices, repetition, blunder frequency, and response to correction.
Different agent pairings can then be compared against that baseline.

The most important design decision is to keep chess legality outside the
agent. A rules library should be the referee. The agent should decide moves,
but the system should validate them. This prevents the experiment from
accidentally becoming a test of whether the agent can pretend to be a chess
rules engine.

The most useful early version is probably:

```text
python-chess referee
UCI move format
FEN + legal-move list prompt
retry-limit invalid-move policy
full raw-response logging
small batch of same-agent self-play games
```

That version would quickly show whether the agent can operate inside a formal
state machine without losing state or inventing legality.

The Brilliance value is not that the agent wins at chess. The value is that
chess exposes the gap between language about reasoning and valid action inside
a constrained system.

## Devil's-Advocate Critique

The strongest objection is that this may not test "brilliance" at all.

It may test prompt compliance, formatting discipline, and tool integration
more than deep reasoning. If the agent is given a legal-move list, then even a
weak agent can choose a legal move. If it is not given a legal-move list, then
many language agents may fail for interface reasons rather than chess-reasoning
reasons.

A second objection is that chess is already a solved domain for specialized
engines. If the goal is to evaluate intelligence or brilliance, using chess may
invite a misleading comparison between general-purpose language agents and
purpose-built chess engines. A poor result might only show that an LLM is not a
chess engine. A good result might only show that it has memorized common chess
patterns.

Third, same-agent self-play may not produce a clean statistical model. Modern
agentic systems can be nondeterministic across prompts, context lengths,
sampling settings, tool availability, hidden model changes, and response
formatting. If those variables are not controlled, the measured distribution
may reflect runtime noise rather than stable agent behavior.

Fourth, "brilliant move" is hard to define. A move can look surprising but be
bad. A move can be objectively strong only because it follows a known opening
book or memorized tactic. Without chess-engine evaluation or expert review,
the project risks mistaking novelty, confidence, or explanation quality for
brilliance.

Fifth, the experiment could become too artificial. Real-world brilliance often
involves reframing the problem, choosing which game to play, or rejecting a
bad premise. Chess deliberately removes that freedom. That is useful for
testing rule-following, but it may understate the kind of brilliance the
larger Brilliance project cares about.

Sixth, two copies of the same agent playing each other may not be meaningfully
independent. If both agents share the same model, prompt structure, and
failure modes, the game may become an echo chamber of similar weaknesses
rather than a robust adversarial test.

Seventh, chess success can be inflated by hidden assistance. If the prompt
includes legal moves, board evaluations, engine hints, or previous move
recommendations, then the measured performance belongs partly to the harness,
not the agent. The report must clearly separate:

```text
agent_decision_quality
referee_rule_validation
human_assistance
tool_assistance
engine_assistance
prompt_scaffolding
```

Devil's-advocate conclusion:

The chess experiment is valuable only if its claims are modest. It should not
be presented as a general intelligence test or proof of AI brilliance. It is
best treated as a bounded hallucination and state-discipline benchmark for
agentic systems.

## Mitigations For The Critique

To keep the chess test honest:

1. Report prompt conditions exactly.
2. Separate runs with legal-move lists from runs without legal-move lists.
3. Keep the rules library as referee, not as strategist.
4. Use chess-engine analysis only after games unless explicitly testing
   engine-assisted agents.
5. Record raw agent responses.
6. Label sample sizes clearly.
7. Avoid claiming statistical stability until enough games exist.
8. Treat "brilliance candidates" as candidates only, pending engine or expert
   review.
9. Compare agents under identical harness conditions.
10. Treat the experiment as a bounded precursor to cash-reward problem tests,
    not a replacement for them.

## Open Questions

- Which two agentic AI systems should be compared first?
- Should the first implementation use strict termination, retry limit, or
  legal-move-list assistance?
- How many games are enough for a first preliminary model?
- Should a chess engine be used only for post-game evaluation, or also during
  play as a referee?
- Should agents see only FEN, a board diagram, legal moves, or all three?
