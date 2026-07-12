# Brilliance Project Development Plan

Created: 2026-07-12

## Goal

Build a small public project that demonstrates how an AI agent behaves when
asked to address major human problems and recognized prize/challenge problems.

The intended product is source material for public discussion, not a claim of
complete solutions.

## Stage 0: Project Seed

State:

- local project folder exists,
- problem description exists,
- representative problem list exists,
- publication intent is recorded.

Deliverables:

- `README.md`
- `misc/problem_description_20260712.md`
- source anchors for global risks, Nobel categories, Clay problems, and prize
  challenges.

Exit criteria:

- a reader can understand the purpose without asking for chat context.

## Stage 1: Problem Inventory

State:

- major problem classes are listed,
- prize/challenge problems are included,
- each problem has a short description and source where applicable.

Deliverables:

- structured problem inventory,
- categorized Stage 1 inventory,
- tags for domain, scale, time horizon, evidence type, and solution difficulty.

Likely implementation:

- Markdown table first,
- later optional CSV/JSON schema.

Exit criteria:

- the inventory contains enough variety to test AI behavior across policy,
  science, engineering, mathematics, medicine, economics, and ethics.

## Stage 2: AI Attempt Log

State:

- an AI agent is asked to propose solution paths for selected problems,
- responses are preserved,
- uncertainty, evasion, overclaiming, and useful reasoning are marked.

Deliverables:

- attempt log,
- prompt log,
- response log,
- issue tags,
- categorized solution-attempt taxonomy.

Likely implementation:

- one Markdown file per run,
- stable template for each attempted problem.

Exit criteria:

- the project contains multiple examples of AI attempts, not just a manifesto.

## Stage 3: Waffle Detection And Human Assistance

State:

- waffling is classified,
- human interventions are recorded,
- revised AI outputs are compared to original outputs.

Deliverables:

- waffle taxonomy,
- intervention examples,
- before/after response comparisons.

Likely implementation:

- incident-style records with fields:
  problem, prompt, initial answer, failure mode, human assist, revised answer,
  remaining uncertainty.

Exit criteria:

- readers can see how human structure changes AI performance.

## Stage 4: Brilliance/Analytics Split

State:

- each problem is evaluated for routine analytics versus creative/brilliant
  synthesis,
- known methods are separated from unresolved leaps.

Deliverables:

- analytics/brilliance classification,
- examples where AI gives useful analytics,
- examples where AI lacks the missing creative move.

Exit criteria:

- the project can explain why "more information" is not the same as
  "brilliance."

## Stage 5: Public Discussion Package

State:

- LinkedIn draft exists,
- supporting links exist,
- claims are calibrated,
- public repository is readable.

Deliverables:

- LinkedIn post,
- GitHub repository link,
- concise source list,
- caveats about scope and overclaiming.

Exit criteria:

- the post invites discussion without claiming that the project has solved the
  world's problems.

## Stage 6: Iteration

State:

- feedback from readers is incorporated into pending notes,
- published updates are batched,
- hallucination incidents are logged.

Deliverables:

- pending update log,
- revision history,
- updated problem inventory,
- improved tests.

Exit criteria:

- the project can evolve without losing provenance.
