# Brilliance v1 Candidate: Agentic AI Chess Self-Play Test

Created: 2026-07-12

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
