Today’s AIH / HybridAI work has reached a useful baseline across four active
repository areas.

The point is not that any one repo is finished.

The point is that the work is starting to separate into the right categories:
test artifacts, theory, evidence, and project-management discipline.

1. AIST-official

This is the original agentic-AI sanity-test seed: rules, scope, hallucination
notes, evaluation prompts, and early project-development material. It is the
starting point for asking whether an agent can follow explicit constraints and
then explain its own failure modes.

2. AIST-official working copy under AI/github

This second working tree preserves related AIST material plus newer notes on
AI, brilliance, and hallucination. It is useful as a staging area for the
conceptual bridge between hallucination testing and the larger question of what
human brilliance contributes when AI can reproduce large parts of established
technique.

3. Brilliance

This repo is the broader AIH framing layer. It holds the public-facing
hallucination-test theory, LinkedIn drafts, problem-analysis material, and the
developing argument that AI usefulness depends on what survives verification,
not merely what can be generated fluently.

4. HybridAI

This is now the concrete technical evidence layer. Today we published a
curated v1 snapshot with deterministic C++ harnesses, Qwen/Ollama baseline
tests, Stockfish as an external chess referee, and selected run artifacts.

That matters because chess gives us a low-noise hallucination test:

state -> prompt -> model output -> parse -> external referee validation ->
record -> apply or terminate

The larger project is not to create an exhaustive list of every possible AI
hallucination.

The project is to build finite, testable prototypes that turn observed failure
into named modes, evidence records, rejection metrics, and process controls.

That is also why the repository split matters.

Technical claims need technical evidence.
Theory needs scoped writeups.
Publication needs character-budget discipline.
Project development needs its own rules for scope, autonomy, evidence, and
future work.

From here, progress can be marked against this baseline: better tests, cleaner
evidence, sharper definitions, and clearer management rules for deciding when
agentic AI is actually useful.

HybridAI public snapshot:
https://github.com/gray3s/hybridai/commit/8e0a3de
