# AIH Hallucination Mode Taxonomy

Created: 2026-07-13

## Working Definition

A hallucination is a workflow disturbance where an agent or human acts from an
unsupported, false, stale, or evidence-resistant model of the current state.

This definition is broader than fabricated facts. It includes any mode where
the operating model diverges from the available evidence and the participant
continues acting from that divergence.

## Scope

These modes can apply to:

- AI agents,
- human reviewers,
- human-agent pairs,
- project workflows,
- summaries and project plans derived from earlier work.

The reliability unit is therefore not only:

```text
agent
```

It is:

```text
human + agent + harness + evidence process
```

## Common Hallucination Modes

1. Fabricated fact

   A participant asserts a fact not supported by evidence.

2. Stale state

   A participant acts as though the old state is still current after the state
   has changed.

3. False memory

   A participant relies on remembered context that does not match the current
   evidence.

4. Unsupported inference

   A participant treats a plausible inference as established fact.

5. Overconfidence

   A participant reports uncertainty-sensitive claims as though they are
   verified.

6. Missed evidence

   A participant overlooks an available artifact, result, contradiction, or
   constraint.

7. Evidence resistance

   A participant refuses to update after contrary evidence appears.

8. Refusal to cooperate with the evidence process

   A participant ignores validation rules, stop conditions, uncertainty labels,
   or required checks.

9. Repeated invalid action

   A participant keeps producing actions that are invalid under the governing
   rules.

10. Unsafe continuation

    A participant continues acting after evidence shows the current path is
    invalid, unclear, unauthorized, or potentially hostile.

11. Scope expansion from false premises

    A participant expands the project or test plan based on an unsupported
    assumption.

12. Summary drift

    A summary compresses or alters the evidence chain enough that later work
    relies on a distorted version of events.

13. Normalization of small failures

    Repeated small unsupported claims become accepted as the cost of using the
    tool, reducing the reliability standard over time.

14. Human-agent co-hallucination

    The agent produces a false claim, the human accepts or overlooks it, and
    the project proceeds from the combined false state.

15. Project-destructive insistence

    A participant prioritizes the project narrative, preferred plan, or desire
    to continue over the evidence chain, creating risk of wrecking the project
    through accumulated false premises.

## AIchess Example

In AIchess, stale state is easy to observe:

```text
Ply 1: e2e4 is legal.
Later ply: the board has changed and e2e4 is no longer legal.
Agent still returns e2e4.
```

Because Stockfish defines the legal moves from the exact FEN, this is not a
subjective interpretation. The harness can record:

- FEN,
- legal move list,
- exact prompt,
- raw response,
- parsed move,
- whether the parsed move is in the legal list,
- applied move or termination.

This turns a vague claim about hallucination into a reproducible event.

## Human Oversight Risk

Human review is not a perfect safety layer. The human reviewer can miss or
normalize the same kinds of hallucination modes that affect the agent.

If the human overlooks an agent hallucination, the workflow can become unstable:

```text
agent false claim -> human accepts it -> project state changes -> future work
relies on false state
```

This is why evidence-preserving design is required. The project should not rely
on human memory or attentiveness as the primary hallucination control.

## Containment Principle

Hallucination should be allowed to manifest enough to measure, but not allowed
to dominate the work.

For AIchess this means:

- record invalid attempts,
- allow bounded retries where useful,
- cap invalid-attempt count,
- cap invalid-attempt duration,
- terminate after sustained invalid play.

For project development this means:

- preserve artifacts,
- require explicit stop conditions,
- escalate to human input before unsafe or unclear paths,
- avoid treating fluent summaries as primary evidence,
- keep evidence links close to claims.

## Agent Admission And Escalation

A mature agent should be able to admit that it cannot proceed reliably.

This should be treated differently from hallucinating an invalid action.

Possible non-action outcomes:

- admission of inability,
- request for clarification,
- request for state refresh,
- request for user input,
- escalation before risk,
- refusal to proceed down an unsafe path.

The deeper test is not whether an agent always acts. The deeper test is whether
it can distinguish when it can act validly from when it should stop, ask, or
escalate.
