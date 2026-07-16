# K-PhD Knowledge-Ladder AIH Test Family

Created: 2026-07-12

Canonical path:

```text
v1/AIH/k-phd/
```

Versioned test definitions live under:

```text
v1/AIH/k-phd/v1/
v1/AIH/k-phd/v2/
...
v1/AIH/k-phd/vn/
```

New tests can be added as the project identifies them.

## Purpose

This is one of the three distinct AIH test families.

The K-PhD test evaluates whether an agentic AI system can answer bounded
academic questions across educational levels while respecting time, I/O,
grading, and reference-source limits.

K-PhD tests should be broken down by academic year for K-12 material and by
college course-year level for bachelor's and master's-level material. Ph.D.
work is generalized to third- and fourth-year graduate work.

```text
K-12: K, grade_01, grade_02, ..., grade_12
bachelor's/master's: 1xx, 2xx, 3xx, 4xx
Ph.D.: grad_year_3, grad_year_4
```

The source academic class or course packet should still be recorded for each
test. The detailed taxonomy rule is:

```text
v1/AIH/class_taxonomy_breakdown_20260715_1931MDT.md
```

## Position In AIH v1

```text
AIH test family 1: personal-history agentic-AI failure tests
AIH test family 2: K-PhD knowledge-ladder tests
AIH test family 3: AI Chess Match state-fidelity tests
```

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

## Initial Artifact

- `exams/wikipedia_only/wikipedia_only_exam_v1_20260712.md`

This first exam is intentionally small. It exists to test whether the reference
rules, grading, and agent-output capture process work before building larger
K-12, college, graduate, and PhD-level exams.
