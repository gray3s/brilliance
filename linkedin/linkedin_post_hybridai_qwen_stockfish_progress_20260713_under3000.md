I think we reached a useful milestone in the HybridAI / AIH test work.

The current experiment is deliberately simple:

Can a local Qwen agent, running through a Qwen/Ollama stack, keep making legal
chess moves when an external referee owns the board?

The harness is deterministic C++.

Stockfish owns chess legality.

The harness asks Stockfish for the current FEN and legal move list, gives that
state to Qwen, parses the model response as a UCI move, checks it against
Stockfish's legal list, records the raw response, and advances the game through
the harness.

That means the model is not being asked to be a chess engine.

It is being asked to do something more basic:

Stay inside a formal rule system as the state changes.

The early result is already useful. An untuned Qwen/Ollama baseline can make
legal opening moves, but it can also fall into stale-response behavior. For
example, after `e2e4` is no longer legal, the model may continue to propose
`e2e4`.

That is not chess strength.

That is state fidelity.

The useful part is that chess is almost free of noise:

- the state is explicit,
- the legal moves are objective,
- the referee is external,
- the game can be replayed,
- invalid output can be preserved as evidence.

So "hallucination" stops being a vague complaint.

It becomes a recorded event:

state -> prompt -> model output -> parse -> external referee validation ->
record -> apply or terminate

This is also where the broader AI-management question starts to become
measurable.

The usefulness of agentic AI is not just what it can generate. It is what
survives the task contract, independent verification, rejection rules, and
recovery cost.

For now, the test is intentionally limited:

- 1v1 self-play,
- local Qwen/Ollama baseline,
- Stockfish referee,
- deterministic C++ harness,
- no prompt tuning,
- no model fine-tuning,
- no broad stack benchmark claim.

The value here is defining and evaluating hallucination tests that are formal
enough to preserve evidence and make invalid behavior hard to hide.

Public snapshot:
https://github.com/gray3s/hybridai/commit/8e0a3de
