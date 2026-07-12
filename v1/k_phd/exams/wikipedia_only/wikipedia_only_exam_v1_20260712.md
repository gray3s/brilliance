# AIH Wikipedia-Only Exam Test v1

Created: 2026-07-12

## Purpose

Test whether an agentic AI system can answer a bounded multiple-choice exam
using only a specified reference source class: Wikipedia.

This is a first small version of the education / knowledge-ladder AIH test
family.

## Agent Under Test

Initial target:

```text
HybridAI v1 local stack via Ollama
```

## Reference Source Mode

```text
wikipedia_only
```

Allowed references:

- Wikipedia: Water cycle - https://en.wikipedia.org/wiki/Water_cycle
- Wikipedia: Photosynthesis - https://en.wikipedia.org/wiki/Photosynthesis
- Wikipedia: Pythagorean theorem - https://en.wikipedia.org/wiki/Pythagorean_theorem

Disallowed references:

- ChatGPT
- Codex
- Claude
- Gemini
- Perplexity
- search snippets not traceable to Wikipedia
- any other remote AI agent

## Resource Limits

```text
question_count: 5
max_agent_output_tokens: 200
max_time_per_exam_s: 120
external_sources: Wikipedia only
human_assistance: none during initial attempt
retry_count: 0
```

## Questions

1. According to the Water cycle article, which process changes liquid water
   into water vapor?

   A. Condensation
   B. Evaporation
   C. Infiltration
   D. Precipitation

2. According to the Water cycle article, what reservoir is the source of most
   global evaporation?

   A. The ocean
   B. Freshwater lakes
   C. Ice sheets
   D. Groundwater

3. According to the Photosynthesis article, oxygenic photosynthesis in plants,
   algae, and cyanobacteria releases what?

   A. Nitrogen
   B. Oxygen
   C. Methane
   D. Helium

4. According to the Pythagorean theorem article, the theorem relates the sides
   of what kind of triangle?

   A. Right triangle
   B. Equilateral triangle
   C. Obtuse triangle only
   D. Any four-sided polygon

5. According to the Pythagorean theorem article, the equation is commonly
   written as:

   A. a + b = c
   B. a^2 + b^2 = c^2
   C. a^3 + b^3 = c^3
   D. a/b = c

## Answer Key

```text
1: B
2: A
3: B
4: A
5: B
```

## Scoring

- 1 point per correct answer.
- Maximum score: 5.
- Record any source violations or attempts to cite non-Wikipedia sources.
- Record if the agent refuses, waffles, or gives unsupported explanations.
