# K-PhD Track Taxonomy Notes

Created: 2026-07-13

## Purpose

Record early track-design choices for the k-phd AIH knowledge-ladder path.

These notes are not the v1 runnable prototype. They are guardrails for later
curriculum and source-reference design.

## Main Academic Ladder

The main k-phd ladder should be subdivided into bounded education bands:

```text
K-3
3-6
6-9
9-12
first-year undergraduate by major
second-year undergraduate by major
third-year undergraduate by major
fourth-year undergraduate by major, where applicable
master's coursework/research track by field
PhD coursework/research framing by field
```

For early AIH runs, this ladder should be sampled lightly and bounded by
provided source packets rather than broad open-web access.

The undergraduate, master's, and PhD bands should be keyed by major or field.
The purpose is not to claim that one general quiz measures all education. The
purpose is to make the level, field, source packet, and grading expectation
explicit enough that hallucination can be observed and compared.

## Gifted And Talented Path

Gifted and talented education should not be forced into the ordinary K-12 track
by default.

Treat it as a separate online research project and a possible alternate path
into college-level material:

```text
gifted/talented -> early college bridge -> undergraduate track
```

Open design questions:

- how to identify appropriate acceleration without simply raising difficulty;
- how to test reasoning depth, creativity, and maturity separately;
- how to avoid confusing advanced vocabulary with advanced understanding;
- how to document source material and grading standards.

## Arts And Music Specializations

Music and art should be undersampled in early k-phd tests unless a strong rubric
is available.

Reason: grading creative work requires criteria that are not the same as
multiple-choice factual tests or formal state machines.

Possible low-risk early probes:

- identify concepts from a provided source packet;
- distinguish technique, interpretation, and historical context;
- explain uncertainty in aesthetic judgment;
- avoid pretending that preference is objective proof.

## Trades And Technical Certificates

Machine shop, welding, carpentry, auto mechanics, and similar technical
certificate paths should be treated as separate applied-technical tracks.

They should also be undersampled early because credible grading may require:

- safety rules,
- tool/material constraints,
- diagrams or measurements,
- code/regulatory references,
- hands-on procedural knowledge,
- expert review.

Possible low-risk early probes:

- safety-rule recognition from a provided packet;
- tool/process matching;
- sequence-order checks;
- refusal when the prompt lacks safety-critical context.

## Current Recommendation

For v1, keep the runnable k-phd prototype focused on bounded academic AI/ML
source packets.

For later versions, add tracks explicitly:

```text
k-phd/academic/k_3
k-phd/academic/3_6
k-phd/academic/6_9
k-phd/academic/9_12
k-phd/academic/undergraduate/<major>/<year>
k-phd/academic/masters/<field>
k-phd/academic/phd/<field>
k-phd/gifted_talented
k-phd/arts_music
k-phd/trades_technical
```

Do not imply that one simple exam covers all of these paths.
