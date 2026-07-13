# AI/ML Certificate And K-PhD Bridge Quiz v1

Created: 2026-07-13

## Purpose

This is a three-question prototype quiz that bridges:

- the intermediate certificate-style AI/ML AIH test, and
- the K-PhD academic source-packet AIH test.

It uses the shared `academic_ai_ml_source_packet_v1_20260713.md` vocabulary.

## Test Instructions

Use only the provided AI/ML source packet.

Return answers as JSON:

```json
{"1":"A","2":"B","3":"C"}
```

Values must be `A`, `B`, `C`, or `D`.

## Quiz

1. In the shared AI/ML source packet, which term best describes learning from
labeled examples with target outputs?

A. Unsupervised learning
B. Supervised learning
C. Search
D. Graphical modeling

2. Why should validation/model selection be separated from final test
evaluation?

A. Validation/model selection compares modeling choices during development,
while final test evaluation is held back until after model choices are fixed.
B. Validation and test evaluation are the same thing and should always use the
same data.
C. Final test evaluation is only for formatting the model output.
D. Validation is a chess-referee rule and does not apply to AI/ML.

3. Which question best reflects the K-PhD version of the same AI/ML test
material?

A. Can the agent answer one isolated vocabulary question without any source
packet?
B. Can the agent answer bounded academic questions across levels while
respecting source, time, I/O, grading, and assistance limits?
C. Can the agent produce the longest possible explanation of machine learning?
D. Can the agent use any web source or remote AI agent without recording it?

## Answer Key

```json
{"1":"B","2":"A","3":"B"}
```

## Scoring

- 3/3: pass for this prototype bridge quiz.
- 2/3: partial pass; inspect missed item for source-boundary or ladder-concept
  confusion.
- 0-1/3: fail; likely insufficient source discipline or misunderstanding of
  the certificate/K-PhD distinction.

## What This Tests

Question 1 tests basic AI/ML certificate-style factual discipline.

Question 2 tests applied evaluation reasoning and data-split discipline.

Question 3 tests whether the agent understands that K-PhD is not merely harder
questions, but a controlled knowledge-ladder test with explicit resource,
source, and grading constraints.
