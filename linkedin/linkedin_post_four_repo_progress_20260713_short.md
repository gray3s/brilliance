Today’s AIH / HybridAI work has reached a useful public baseline across four
active repository areas.

1. AIST-official

Original agentic-AI sanity-test material: rules, scope, hallucination notes,
evaluation prompts, and early project-development framing.

2. AIST-official working copy under AI/github

Related AIST material plus newer notes connecting hallucination, AI usefulness,
and human brilliance.

3. Brilliance

The AIH framing layer: hallucination-test theory, LinkedIn drafts,
problem-analysis material, and the developing argument that AI usefulness
depends on what survives verification.

4. HybridAI

The concrete technical evidence layer. Today we published a curated v1 snapshot
with deterministic C++ harnesses, Qwen/Ollama baseline tests, Stockfish as an
external chess referee, and selected run artifacts.

The larger project is not to claim an exhaustive taxonomy of every possible AI
hallucination.

It is to build finite, testable prototypes that turn observed failure into
named modes, evidence records, rejection metrics, and process controls.

Chess is the first clean example:

state -> prompt -> model output -> parse -> external referee validation ->
record -> apply or terminate

From here, progress can be marked against this baseline: better tests, cleaner
evidence, sharper definitions, and clearer rules for deciding when agentic AI is
actually useful.

HybridAI public snapshot:
https://github.com/gray3s/hybridai/commit/8e0a3de
