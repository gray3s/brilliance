20260712 1813MDT

(the following is straight from Codex based on my prompts)

The next practical question is not whether a local AI stack can beat Codex in
some vague general sense.

That is the wrong comparison.

The better question is:

Can HybridAI, Codex, local models, cloud agents, and other agentic AI stacks
run the same bounded tests and produce comparable results?

That is what the AIH test work is for.

HybridAI should be treated as an implementation under test.

For each HybridAI version, the useful question is:

Can this version accept a bounded task, produce parseable output, preserve
state, respect evidence limits, and generate a result that an external referee
or grader can score?

The minimum useful run looks like this:

1. define the test,

2. run the agent,

3. capture the raw response,

4. parse the response,

5. score it externally,

6. record timing, errors, and statistics,

7. use the result to guide the next implementation.

AIchess is the first low-level candidate because chess gives us a formal state
machine.

The referee owns the board.

The agent proposes moves.

The harness records legality, timing, move faults, board-state errors, and the
game ending.

That matters because it lets each stack be tested:

- against itself,
- against a reference agent,
- against peer candidate stacks.

On a small local machine, this has to start modestly.

An 8GB i3 may not be the right machine for large repeated local-agent matches.
So the first useful tests may be one-move probes, short smoke games, small
models, and selective reference-agent comparisons.

That is still useful.

The goal is not to publish every local log, failed build attempt, transcript,
or scratch run.

The online repositories should act as curated project archives: enough source,
configuration, scripts, dependency notes, test definitions, and reviewed
results to rebuild the current subproject version.

Not the full local workbench.

That gives the project two benefits:

1. a practical way to improve HybridAI versions,

2. a dated public record of independent development and project provenance.

Project analysis:

https://github.com/gray3s/brilliance/blob/main/PROBLEM_ANALYSIS.md

HybridAI test project goals:

TODO: add GitHub link after publication.
