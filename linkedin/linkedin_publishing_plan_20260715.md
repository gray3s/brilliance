# LinkedIn Publishing Plan - Brilliance Repository

Created: 2026-07-15

Scope: all publishable work completed since the `brilliance` repository was created on 2026-07-12, including AIH framework work, HybridAI framing, test-family development, Qwen/Ollama/Stockfish progress, and today's AIChess C++/Qt runner.

Repository:

```text
https://github.com/gray3s/brilliance
```

## Publication Objective

Publish a LinkedIn update that explains the whole Brilliance effort as a coherent benchmark project, not merely today's AIChess implementation.

The post should communicate:

- why the repository exists,
- what has been built since creation,
- why the three AIH test families matter,
- why AIChess belongs in `brilliance`,
- what was added today,
- and what the next measurement step is.

## Core Message

AI evaluation should test brilliance under constraints.

A fluent model can still fail when the task requires grounded memory, bounded state, academic reasoning, legal action, or disciplined uncertainty. The Brilliance repository is a public working tree for turning those failure modes into repeatable tests.

The current framework treats hallucination control as measurable behavior, not merely a warning label.

## Work Completed Since Repository Creation

### 1. Brilliance Project Frame

Created a public source-material package around AI, human brilliance, hallucination control, and bounded problem analysis.

Core files:

```text
README.md
PROJECT_GOALS.md
PROBLEM_ANALYSIS.md
TREE.md
```

Purpose:

- define the project,
- explain why bounded problem framing matters,
- preserve uncertainty,
- and create public material for future LinkedIn discussion.

### 2. AIH Framework

Built the AIH structure for analyzing AI hallucination behavior.

Key artifacts:

```text
v1/AIH_APPROACH_v2_20260712.md
v1/AIH_TEST_SUITE_v1_20260712.md
v1/AIH/
docs/AIH_TEST_PROJECT_GOALS_20260712.txt
docs/AIH_HALLUCINATION_MODE_TAXONOMY_20260713.md
docs/AIH_three_tier_refinement_20260712.txt
```

Themes:

- hallucination classes,
- bounded evaluation,
- repeatable probes,
- state fidelity,
- factual grounding,
- and classification of failure modes.

### 3. Three AIH Test Families

Organized the test project into three broad classes:

```text
v1/AIH/AIhistory/
v1/AIH/k-phd/
v1/AIH/intermediate_cert/
v1/AIH/AIchess/
```

Working interpretation:

- Personal-history / provenance tests probe grounded memory and factual continuity.
- K-PhD / academic-ladder tests probe formal knowledge and academic reasoning.
- AIChess tests probe bounded state, legal action, planning, and termination behavior.

### 4. HybridAI Relationship

Connected the AIH test framework to HybridAI execution ideas.

Key artifacts:

```text
docs/HYBRIDAI_TEST_PROJECT_GOALS_20260712.txt
docs/HYBRIDAI_AGENTIC_LAYER_MATRIX_20260713.md
docs/program-development-plan-DA_20260712.txt
```

Themes:

- local versus hosted model stacks,
- agentic layers,
- testing workflows,
- timing/error/statistics capture,
- and repeatable comparison across model/tool configurations.

### 5. AIChess Track

Expanded the chess benchmark path because chess is a credible test of bounded state, rule-following, and strategic behavior.

Key earlier artifact:

```text
v1/agent_chess_project_development_plan_20260712.md
```

Today's additions:

```text
v1/AIH/AIchess/v1/qwen_ollama_chess_qt/
v1/AIH/AIchess/v1/qwen_ollama_chess_timeout_runbook_20260715.md
v1/AIH/AIchess/v1/upgrade_ollama_tinyproxy_20260715.sh
```

Today's implementation:

- C++/Qt console runner.
- Ollama/Qwen model invocation.
- Stockfish-backed legal move referee.
- One-move and bounded-game modes.
- Per-move and full-game timeout controls.
- JSONL and Markdown summary output.
- Runtime-stack version checking for Ollama and Tinyproxy.

### 6. LinkedIn Draft Trail

Prepared multiple LinkedIn post drafts documenting project stages and public-facing messages.

Examples:

```text
linkedin/linkedin_post_aih_framework_20260712_1800MT.md
linkedin/linkedin_post_aih_hybridai_combined_20260712_1816MDT.md
linkedin/linkedin_post_hybridai_tests_20260712_1813MDT.md
linkedin/linkedin_post_hybridai_qwen_stockfish_progress_20260713.md
linkedin/linkedin_post_aih_classes_chrr_20260714.md
linkedin/linkedin_post_prototype_communication_chain_20260713.md
```

These can be used as supporting material or future posts rather than one overloaded daily post.

## Before Publishing

1. Confirm the staged `brilliance` changes are intended.
2. Commit the full publishable batch.
3. Push to GitHub.
4. Confirm the GitHub repository renders correctly.
5. Capture account usage:

```text
/status
/usage daily
```

6. Save the usage snapshot for the follow-up cost/effort summary.

## Main LinkedIn Post Draft

Since creating the Brilliance repository on July 12, I have been turning a broad question into a testable framework:

How do we evaluate AI systems for brilliance without confusing fluency for reliability?

The repository now organizes that question into an AI Hallucination / Brilliance test structure. The work covers several dimensions:

- grounded memory and provenance,
- academic reasoning from intermediate certification through K-PhD levels,
- bounded state and legal action through chess,
- local model-stack testing,
- timing, error, and statistics capture,
- and LinkedIn-readable public progress notes.

Today I pushed the AIChess track forward. The new work adds a C++/Qt runner for local Qwen-family Ollama models, using Stockfish as the external chess referee. The model proposes moves; the referee owns the board state. The runner records legal moves, illegal or unparseable moves, per-move timeouts, game-level termination, and whether a model can complete a bounded chess task.

This is not primarily about chess strength. It is about measurable behavior under rules.

Chess belongs in this framework for the same reason academic tests do: it creates a constrained environment where state, reasoning, and action can be checked. If an AI system drifts outside the rules, the failure is observable.

Repository:
https://github.com/gray3s/brilliance

Next step: run the Qwen/Ollama AIChess probes repeatedly, summarize termination behavior, and compare the experimental work against account-usage statistics.

## Short LinkedIn Variant

The Brilliance repository is now moving from idea to repeatable AI behavior tests.

Since July 12, I have organized the work around AI hallucination and brilliance evaluation: personal-history/provenance tests, academic-ladder tests, HybridAI stack comparisons, and AIChess state-fidelity tests.

Today's addition is a C++/Qt AIChess runner for local Qwen-family Ollama models, with Stockfish acting as the external referee.

The point is not chess strength. The point is bounded behavior: legal moves, illegal moves, timeouts, completion, and state fidelity.

Repository:
https://github.com/gray3s/brilliance

Next: run the models, summarize results, and compare the work against account-usage statistics.

## If Space Allows

Optional closing line:

Good benchmarks should make failure visible.

## What Not To Claim Yet

Do not claim:

- Qwen can or cannot complete games until repeated runs are summarized.
- The benchmark is complete.
- The three test families are statistically validated.
- Local Ollama results generalize to all AI systems.

Safe claim:

The repository now contains a repeatable framework and early runnable probes for measuring bounded AI behavior.

## Follow-Up Result Post Template

After running the probes:

```text
Follow-up on the Brilliance / AIChess benchmark run:

I tested local Qwen-family Ollama models under a Stockfish-refereed C++/Qt harness. The models were not judged on chess strength, but on bounded behavior: legal move rate, illegal or unparseable moves, timeout behavior, completion, and state fidelity.

Settings:
- models:
- move timeout:
- game timeout:
- max plies:
- illegal move limit:

Results:
- completed games:
- move timeouts:
- illegal move limits:
- highest legal move count:
- shortest failure:
- longest run:

Repository:
https://github.com/gray3s/brilliance
```

## Suggested Posting Sequence

1. Today: broad Brilliance repository progress post.
2. After first AIChess batch: result-summary post.
3. Later this week: academic-ladder / K-PhD benchmark post.
4. Later this week: HybridAI stack comparison post.
5. Later this week: account-usage and cost-of-experimentation post.
