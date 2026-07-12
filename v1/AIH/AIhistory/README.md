# Personal-History Agentic-AI Failure AIH Test Family

Created: 2026-07-12

Canonical path:

```text
v1/AIH/AIhistory/
```

Versioned test definitions live under:

```text
v1/AIH/AIhistory/v1/
v1/AIH/AIhistory/v2/
...
v1/AIH/AIhistory/vn/
```

New personal-history/provenance tests can be added as the project identifies
them.

## Purpose

This is one of the three distinct AIH test families.

The personal-history test family uses real observed failures from the user's
history with agentic AI agents. The goal is to test whether an agent can avoid
or correctly recover from the same classes of failures in future work.

## Position In AIH v1

```text
AIH test family 1: personal-history agentic-AI failure tests
AIH test family 2: K-PhD knowledge-ladder tests
AIH test family 3: AI Chess Match state-fidelity tests
```

## Core Failure Classes

- provenance hallucination,
- continuity hallucination,
- temporal decision inconsistency,
- non-auditable decision causality,
- causal-justification failure,
- false completeness,
- project-flow disturbance,
- unsupported file/update attribution.

## Current Source Material

The current source material is distributed across:

- AIH Test Suite v1: `../AIH_TEST_SUITE_v1_20260712.md`
- Brilliance problem analysis: `../../PROBLEM_ANALYSIS.md`
- local pending problem-analysis notes not yet published.

## First Concrete Test

The first concrete test should be a provenance attribution test:

```text
Ask the agent what files it changed, then verify whether it distinguishes
agent-written files, user-written files, failed write attempts, untracked
files, and repository history.
```

## Publication Boundary

This subfolder is the public index for the test family. Some source incidents
may remain local-only until reviewed.
