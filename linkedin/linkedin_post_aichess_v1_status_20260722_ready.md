20260722 0230MT

AIH AIchess v1 progress

```text
Alias   Qwen model                             Size
qwen1   qwen2.5-coder:3b                       1.9 GB
qwen2   qwen:4b                                2.3 GB
qwen3   qwen2.5:latest                         4.7 GB
qwen4   robit/qwen3.5-9b-r7-research:q4km      5.6 GB
```

The basic local stack is now in place: a local harness can talk to Ollama, run a local Qwen model, capture the harness/Ollama I/O, derive a chess move from the model's reported board transition, and score it with a deterministic referee.

The early version let the model see the board state and a list of legal moves. That did too much of the work for the Qwen models. A model that picks from a supplied legal list is not proving that it understands the board.

The stricter v1 test now gives the model the current board state as FEN and asks it to return only:

bf=<board before move>

af=<board after move>

The harness then derives the move from that transition and checks it against a deterministic legal-move set. The model no longer gets the legal move list by default.

That changed the result immediately.

Qwen4 could play short games when given the legal move list, but failed the unassisted board-transition test on the first move by returning an invalid after-state. This is a state-fidelity failure that the earlier prompt was masking.

This is useful because it shows something concrete: this local stack cannot yet play even a minimal legal game of chess correctly without handholding from the harness. At least not yet, and not with the current list of Qwen models I have running through Ollama.

In this case, handholding can literally make or break the game.

This gives us a cleaner Class1 AIH test:

- Can the agent preserve the current board state?

- Can it produce a valid next board state?

- Can it make even one legal move, much less finish a game?

- Does the model still fail when formatting help and legal-move hints are removed?

V2 should stay local. The next step is to check whether other Ollama-compatible local models can pass the same unassisted board-transition task before going too far into broader local agentic AI stack experiments. Cloud agent comparisons can wait for V3, where token use per move will also need to be measured.

The next layer is AIH scoring, plus better instrumentation of the Qwen/Ollama boundary. If the relevant layer is open-source and modifiable, we may be able to have the model or Ollama layer log its own I/O for verification. If not, the test necessarily rests on observable I/O assumptions.

AIH AIchess v1 repo folder:

https://github.com/gray3s/brilliance/tree/main/aih/aichess/v1

The point is not to make chess hard for its own sake. The point is to expose when an agentic stack is depending on the test harness to do the state work. If the stack fails to start much less finish a complete game, that is exactly the kind of limitation an AIH test should reveal.
