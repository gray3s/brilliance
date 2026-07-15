20260712 1816MDT

(the following is straight from Codex based on my prompts)

I think the Brilliance project is starting to separate into two practical
tracks:

AIH tests, and HybridAI tests.

AIH means AI Hallucination testing, but the point is broader than catching
wrong facts.

The point is to test agentic AI systems under bounded conditions where state,
evidence, timing, rule-following, uncertainty, and task suitability can be
checked.

The current AIH ladder has three levels:

1. AIchess

This is the lowest-level formal test.

An AI stack plays chess through an external referee. The referee owns the
board, validates legal moves, records timing, logs faults, and classifies the
game ending.

Each stack should eventually be tested:

- against itself,
- against a reference agent,
- against peer candidate stacks.

2. Intermediate AIH test

This should sit between chess and the full knowledge-ladder project.

A certificate-style AI/ML test is a good candidate because it can test applied
reasoning, factual discipline, source boundaries, grading behavior, and
uncertainty without trying to cover all of education.

3. k-phd

This is the broad knowledge-ladder path.

It could eventually range from K-12 through college, graduate, and PhD-level
work, with limits on time, I/O, grading, and reference sources.

That is the long-term extension, not the first implementation target.

The HybridAI side is different.

HybridAI is not the test.

HybridAI is one implementation under test.

The useful question is not whether a local AI stack is generally "better" than
Codex, a cloud agent, or another local model.

The useful question is whether each stack can run the same bounded test and
produce comparable results.

The minimum useful HybridAI run looks like this:

1. define the test,

2. run the agent,

3. capture the raw response,

4. parse the response,

5. score it externally,

6. record timing, errors, and statistics,

7. use the result to guide the next implementation.

That gives us a cleaner development loop:

AIH defines the tests.

HybridAI runs the tests.

The results guide the next HybridAI version.

The public repositories should also stay disciplined.

They should act as curated project archives: enough source, configuration,
scripts, dependency notes, test definitions, and reviewed result summaries to
rebuild the current subproject version.

Not the full local workbench.

That also gives us a dated public record of independent project development
and provenance.

AIH test project goals:

https://github.com/gray3s/brilliance/blob/main/docs/AIH_TEST_PROJECT_GOALS_20260712.txt

HybridAI test project goals:

https://github.com/gray3s/brilliance/blob/main/docs/HYBRIDAI_TEST_PROJECT_GOALS_20260712.txt

Project analysis:

https://github.com/gray3s/brilliance/blob/main/PROBLEM_ANALYSIS.md
