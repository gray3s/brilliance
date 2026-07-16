# AIhistory Reference Material

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

New personal-history/provenance reference notes and derived probes can be added
as the project identifies them.

## Purpose

AIhistory is reference/evidence material that justifies the broader
hallucination examination effort. It collects observed failures from work with
agentic AI systems so they can be converted into public-safe, bounded tests.

AIhistory is not itself one of the three ordered hallucination-test classes.
It primarily supports Class 2: provenance/workflow-history hallucination.
Class 2 tests derived from AIhistory material should be broken down by subject
matter or subject field, such as legal/evidence workflow, software/project
workflow, academic/research workflow, repository/version-control workflow, or
personal-history/provenance workflow. A starter information-technology tools
and support workflow candidate list is recorded in:

```text
v1/class2_it_tools_procedures_candidate_tests_20260715_1939MDT.md
```

## Position In AIH v1

```text
AIH class 1: rule-bound state/game-action hallucination
AIH class 2: provenance/workflow-history hallucination
AIH class 3: source-bound knowledge/education-ladder hallucination
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
