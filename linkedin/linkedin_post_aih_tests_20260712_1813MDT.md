20260712 1813MDT

(the following is straight from Codex based on my prompts)

We are separating the Brilliance idea into practical AIH tests.

AIH means AI Hallucination testing, but the goal is broader than catching
wrong facts.

The goal is to evaluate agentic AI systems under bounded conditions where
state, evidence, timing, rule-following, and uncertainty can be checked.

The current AIH ladder has three levels:

1. AIchess:

a formal state-fidelity test.

The AI stack must play chess through an external referee that owns the board
state, validates legal moves, records timing, and classifies the game ending.

Each stack should eventually play:

- against itself,
- against a reference agent,
- against peer candidate stacks.

2. Intermediate AIH test:

a bounded certificate-style test.

This should sit between chess and the broad knowledge-ladder tests. A practical
AI/ML certificate-style exam is a good candidate because it can test applied
reasoning, factual discipline, source boundaries, and grading behavior without
trying to cover all of education.

3. k-phd:

a broad knowledge-ladder path.

This can eventually range from K-12 through college, graduate, and PhD-level
work, with limits on time, I/O, grading, and reference sources.

That is the long-term path, not the first implementation target.

The practical lesson is simple:

Do not ask whether one AI stack is generally "better."

Ask what it does under the same bounded test.

Does it preserve state?

Does it follow the rules?

Does it invent unsupported certainty?

Does it know when the evidence is missing?

Can it produce a result that can be scored, repeated, and compared?

For now, AIchess is the lowest-level smoke test. The intermediate certificate
test is the next likely step. The k-phd path is the broad extension.

Project analysis:

https://github.com/gray3s/brilliance/blob/main/PROBLEM_ANALYSIS.md

AIH test project goals:

TODO: add GitHub link after publication.
