20260713

(draft based on the current HybridAI / AIH chess harness work)

I think we just reached a useful milestone in the HybridAI test work.

The current experiment is deliberately simple:

Can a local Qwen agent, running through the HybridAI Qwen/Ollama stack, keep
making legal chess moves when an external referee owns the board?

The answer is already interesting.

The test harness is deterministic C++.

Stockfish owns chess legality.

The harness asks Stockfish for the current FEN and legal move list, gives that
state to the Qwen model, parses the model's response as a UCI move, checks it
against Stockfish's legal list, records the raw response, and advances the game
only through validated moves.

That means the model is not being asked to be a chess engine.

It is being asked to do something more basic:

Stay inside a formal rule system as the state changes.

The early result is that an untuned Qwen/Ollama baseline can make legal opening
moves, but it can also fall into stale-response behavior. For example, after
`e2e4` is no longer legal, the model may continue to propose `e2e4`.

That is not a matter of chess strength.

It is a state-fidelity failure.

The useful part is that chess is almost free of noise:

- the state is explicit,
- the legal moves are objective,
- the referee is external,
- the game can be replayed,
- every invalid move can be preserved as evidence.

So "hallucination" stops being a vague complaint.

It becomes a recorded event:

- exact FEN,
- exact legal move list,
- exact prompt,
- raw model response,
- parsed move,
- whether the parsed move appears in the referee legal list,
- fallback behavior,
- timing.

We also added a termination rule:

If the model produces too many consecutive invalid moves, the harness stops the
game with:

`sustained_invalid_play_refuses_or_fails_rules`

That wording matters.

Whether the cause is hallucination, stale state, prompt fixation, or some other
failure mode, the practical result is the same:

The agent is no longer reliably playing by the rules.

The broader implication is that claims like "this agentic AI implementation is
appropriate for X" are not meaningful unless they are tied to a specific model,
runtime, prompt, harness, referee, state representation, and failure policy.

For now, the test is intentionally limited:

- 1v1 self-play,
- local Qwen agents,
- Ollama runtime,
- Stockfish referee,
- deterministic C++ harness,
- no prompt tuning yet,
- no model fine-tuning yet,
- no alternate agent framework yet.

The real value here is not testing every possible stack configuration.

The larger industry can do broad model and stack benchmarking.

The value here is defining, implementing, and evaluating hallucination tests
that are formal enough to preserve evidence and make failure hard to hide.

Near-term next step:

Run enough local Qwen/Ollama baseline cases to validate the test method, then
focus on improving the hallucination test contract rather than chasing every
possible configuration.

This feels like the right kind of AIH test: small, formal, reproducible, and
hard for fluent language to hide inside.

Public evidence pointers:

- HybridAI repository snapshot:
  `https://github.com/gray3s/hybridai/commit/8e0a3de`

- HybridAI v1 README:
  `https://github.com/gray3s/hybridai/tree/main/v1`

- Harness definition:
  `https://github.com/gray3s/hybridai/blob/main/v1/misc/test1_harness_definition_20260713.md`

- Curated 1300MT result artifacts:
  `https://github.com/gray3s/hybridai/tree/main/v1/results/20260713_1300MT`

TODO before posting:

- decide whether to name specific local model results in the public version,
- add a short note that these are untuned baseline runs, not final model claims.
