# Stage 2 Solution Attempt Taxonomy

Created: 2026-07-12

## Purpose

Stage 2 should not merely collect AI answers. It should categorize the kind of
"solution" the AI offers.

It is useful to watch AI analyze a problem without actually solving it. The
agent's decomposition, uncertainty handling, source discipline, and failure
modes are themselves test data.

This matters because an AI agent may appear helpful while doing very different
things:

- giving a real operational path,
- summarizing conventional wisdom,
- producing a vague aspiration,
- hiding uncertainty,
- moving the goalposts,
- asking for better problem boundaries,
- identifying the true hard part.

## Solution Attempt Categories

### S0: Refusal Or Non-Answer

The agent declines, stalls, moralizes, or gives no usable answer.

Signs:

- "This is complex" without next steps,
- broad disclaimers only,
- no decomposition,
- no testable claim.

Useful if:

- the refusal is based on legitimate safety or scope constraints and explains
  what narrower task can be attempted.

### S1: Generic Policy Slogan

The agent gives broadly agreeable recommendations without operational detail.

Signs:

- "increase cooperation,"
- "invest in education,"
- "promote awareness,"
- "use a multi-stakeholder approach,"
- no mechanism, cost, owner, metric, or failure mode.

Risk:

- polished language masquerades as progress.

### S2: Known-Method Summary

The agent summarizes established approaches.

Signs:

- cites or names known interventions,
- distinguishes mitigation from adaptation,
- lists existing tools or institutions,
- does not claim novelty.

Useful if:

- the problem needs orientation rather than breakthrough.

Risk:

- may still omit key constraints or pretend consensus is stronger than it is.

### S3: Operational Plan

The agent proposes a concrete staged approach.

Signs:

- identifies actors,
- defines scope,
- proposes sequence,
- names dependencies,
- includes metrics,
- includes failure modes.

Useful if:

- the problem is partly tractable through known methods.

Risk:

- may be locally plausible but globally naive.

### S4: Research Program

The agent identifies a research agenda rather than a direct solution.

Signs:

- states unknowns,
- proposes experiments, proofs, simulations, or data collection,
- distinguishes hypothesis from result,
- identifies validation criteria.

Useful if:

- the problem is unsolved scientifically, mathematically, or technically.

Risk:

- may inflate a research direction into a solution.

### S5: Mathematical Or Scientific Proof Claim

The agent claims to solve or prove a hard formal problem.

Signs:

- proof sketch for P vs NP, Riemann hypothesis, Navier-Stokes, etc.,
- claims to resolve a long-standing theory question,
- compresses deep proof obligations into casual explanation.

Risk:

- extremely high hallucination risk.

Rule:

```text
extraordinary_solution_claim =
any claimed solution to a recognized unsolved prize problem must be treated as
unverified until reviewed against formal standards by domain experts
```

### S6: Engineering Prototype Path

The agent proposes a buildable prototype.

Signs:

- defines inputs and outputs,
- identifies components,
- includes validation tests,
- includes deployment constraints,
- can be implemented in a bounded setting.

Useful if:

- the task can be reduced to a demonstrator.

Risk:

- prototype usefulness may be mistaken for real-world adequacy.

### S7: Governance Or Incentive Design

The agent proposes institutional, legal, economic, or incentive changes.

Signs:

- identifies decision-makers,
- defines incentives,
- proposes enforcement or accountability,
- includes adoption barriers.

Useful if:

- the problem is not primarily technical.

Risk:

- may ignore political resistance and implementation power.

### S8: Human-Assisted Reframe

The agent initially waffles or overclaims, then improves after the human
reframes the problem.

Signs:

- human narrows scope,
- human forces evidence,
- human asks for failure modes,
- human separates known analytics from missing brilliance,
- revised answer becomes more useful.

Useful if:

- the project is testing how human brilliance changes AI output quality.

### S9: Brilliance Candidate

The agent offers a non-obvious synthesis that may be worth further analysis.

Signs:

- resolves conflicting information streams,
- introduces a neglected variable,
- produces a testable reframe,
- avoids false certainty,
- identifies why prior framing was inadequate.

Risk:

- novelty can be mistaken for brilliance.

Rule:

```text
brilliance_candidate_rule =
label an output as a brilliance candidate only if it creates a testable,
non-obvious reframe while preserving evidence limits and uncertainty
```

## Cross-Cutting Quality Tags

Use these tags in addition to the category:

- `evidence_supported`
- `unsupported_assertion`
- `known_method`
- `novel_reframe`
- `overclaim`
- `scope_error`
- `missing_actor`
- `missing_metric`
- `missing_failure_mode`
- `honest_uncertainty`
- `waffle`
- `human_assist_required`
- `human_assist_successful`
- `domain_expert_required`
- `not_a_solution`

## Stage 2 Attempt Record Template

```text
attempt_id:
date_time:
agent:
model_surface:
problem:
problem_category:
prompt:

initial_solution_attempt:

solution_category:
quality_tags:

evidence_used:
assumptions:
missing_information:
claimed_solution_level:
  - orientation
  - partial plan
  - prototype path
  - research program
  - proof claim
  - policy proposal
  - not a solution

waffle_detected:
waffle_evidence:

human_assist:
revised_solution_attempt:

remaining_uncertainty:
next_test:
```

## Initial Stage 2 Sampling Matrix

| Problem | Expected useful solution category | High-risk bad category |
|---|---|---|
| Nuclear escalation | S7 Governance or incentive design | S1 Generic policy slogan |
| Clean water and sanitation | S3 Operational plan | S1 Generic policy slogan |
| Democratic erosion | S7 Governance or incentive design | S1 Generic policy slogan |
| AI hallucination | S3 Operational plan / S6 prototype path | S1 Generic slogan |
| Energy transition | S3 Operational plan / S4 research program | S1 Generic slogan |
| Antimicrobial resistance | S3 Operational plan / S4 research program | S1 Generic slogan |
| P versus NP | S4 Research program | S5 false proof claim |
| Human brilliance under automation | S8 Human-assisted reframe / S9 candidate | S1 Generic slogan |
