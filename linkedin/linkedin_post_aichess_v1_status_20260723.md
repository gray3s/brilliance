# LinkedIn Draft: AIchess v1 Status

AIH AIchess v1 reached an important clarification point this week.

The basic local stack configuration is now in place in the general sense: a local harness can talk to Ollama, run a local model, capture the exact harness/Ollama I/O, derive a move from the model's reported board transition, and score it with a deterministic referee.

The early version of the harness let the model see the board state and a list of legal moves. That was useful for debugging parser behavior, but it also did too much of the work for the agent. A model that picks from a supplied legal list is not proving that it understands the board.

The current v1 test is stricter.

The agent receives the current board state as FEN and must return only the board state before and after its move:

```text
bf=<board before move>
af=<board after move>
```

The harness then derives the move from that transition and checks it against a hidden deterministic legal-move set. The model no longer gets the legal move list by default.

That changed the result immediately. Qwen4 could play short games when given the legal move list, but failed the unassisted board-transition test on the first move by returning an invalid after-state. That is not a minor formatting issue. It is a state-fidelity failure that the earlier prompt was masking.

This is important because it shows something useful and concrete: this local stack cannot yet play even a minimal legal game of chess correctly without handholding from the harness. At least not yet, and not with the current list of Qwen models I have running through Ollama. It can follow a constrained selection task better than it can maintain and update a game state on its own.

In this case, handholding can literally make or break the game.

This gives us a cleaner Class1 AIH test:

- Can the agent preserve the current board state?
- Can it produce a valid next board state?
- Can the harness derive exactly one legal move from that transition?
- Does the model still fail when formatting help and legal-move hints are removed?

V2 should stay local. The next step is to check whether other Ollama-compatible local models can pass the same unassisted board-transition task before going too far into broader local agentic AI stack experiments. Cloud agent comparisons can wait for V3, where token use per move will also need to be measured.

The next layer is AIH scoring: probably straightforward at first, but necessarily a bit ad hoc until enough runs show which failure modes matter most.

There is also plumbing work left: the Qwen/Ollama boundary needs more validation so we know the local model is receiving and returning exactly what the harness thinks it is. At minimum, the next instrumentation step is to capture the exact Ollama request and response bodies and determine whether any deeper Qwen/Ollama stream is observable. If the relevant layer is open-source and modifiable, we may be able to have the Qwen layer or Ollama layer log its own I/O for verification. If the boundary is closed, the test necessarily rests on observable I/O assumptions.

The point is not to make chess hard for its own sake. The point is to expose when an agentic stack is depending on the test harness to do the state work. If the stack cannot make one legal board transition without handholding, that is exactly the kind of limitation an AIH test should reveal.
