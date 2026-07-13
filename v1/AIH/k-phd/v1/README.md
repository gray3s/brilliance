# K-PhD Knowledge-Ladder AIH Test

Created: 2026-07-12

## Purpose

This is one of the three distinct AIH test families.

The K-PhD test evaluates whether an agentic AI system can answer bounded
academic questions across educational levels while respecting time, I/O,
grading, and reference-source limits.

## Position In AIH v1

```text
AIH class 1: rule-bound state/game-action hallucination
AIH class 2: provenance/workflow-history hallucination
AIH class 3: source-bound knowledge/education-ladder hallucination
```

K-PhD is the current Class 3 prototype family.

## Core Idea

Instead of having human students take the exams, a human chooses an agentic AI
agent to take the exam. The result can then be graded, challenged, and reviewed.

```text
AI answer -> grading decision -> human/agent challenge -> review outcome
```

## Reference Controls

Each exam should define the permitted reference mode:

```text
closed_book
provided_sources_only
wikipedia_only
approved_reference_set
open_reference
```

Reference constraints prevent the tested agent from using another remote AI
agent as an untracked source.

## Resource Controls

Each exam should define:

```text
max_prompt_input_tokens
max_agent_output_tokens
max_tool_calls
max_external_sources
max_time_per_question
max_time_per_exam
allowed_retry_count
allowed_human_assistance
```

## Subtrees

- `exams/wikipedia_only/` - first concrete K-PhD test artifact.

## Initial Artifacts

- `exams/wikipedia_only/wikipedia_only_exam_v1_20260712.md`
- `run_kphd_wikipedia_probe_v1.py`

The original Wikipedia-only exam is intentionally small. The newer v1 runner
uses the shared MIT/Stanford academic AI/ML source packet so it can consolidate
with the intermediate certificate-style test while still asking ladder-style
questions.

## Track Notes

- `kphd_track_taxonomy_notes_20260713.md`

This note defines the banded K-PhD ladder and keeps gifted/talented,
arts/music, and trades/technical certificate paths separate from the first
academic AI/ML source-packet prototype.

## Bridge Test Environment

`test_env/run_bridge_quiz_env.py` runs the shared three-question
AI/ML certificate and K-PhD bridge quiz as a K-PhD knowledge-ladder
environment.

It writes JSON results to `test_env/runs/`.
