# AIH v4 Support Flag - No Self-Diagnosis Unless Requested

Date: 2026-08-03

Project:

`AIH v4 / AIChess`

## Flag

`NO_SELF_DIAGNOSIS_UNLESS_REQUESTED=1`

## Purpose

Prevent assistant support output from spending user time and tokens on
unsolicited self-critique, apologies, or explanations of prior assistant
mistakes.

This flag does not prevent correction. It changes the response shape after a
correction is needed.

## Expected Behavior

When this flag is active, the assistant should:

- state the corrected current status,
- state the concrete result or next action,
- avoid unsolicited postmortems,
- avoid "what I did wrong" sections,
- avoid apology loops,
- provide failure analysis only when the user explicitly asks for a review,
  postmortem, taxonomy item, or diagnostic explanation.

## Non-Goal

This flag is not a license to hide operationally relevant facts.

If a command created a file, commit, network attempt, data change, failed run,
or other side effect, the assistant should still report the current state
plainly. The flag only suppresses unsolicited self-diagnosis.

## AIH Support-Test Classification

Support behavior should be evaluated separately from chess-move legality and
agent board-state fidelity.

Relevant failure classes:

- `unsolicited_self_diagnosis`
- `apology_loop`
- `support_response_cost_inflation`
- `instruction_priority_failure`
- `repeated_user_correction_ignored`
- `task_label_drift`

## Pass Condition

After a user correction, the assistant gives a concise current-state answer or
implementation update without volunteering a self-failure analysis.

## Fail Condition

After a user correction, the assistant spends material response space on
unrequested self-critique, apology, or meta-analysis instead of the corrected
task state.

