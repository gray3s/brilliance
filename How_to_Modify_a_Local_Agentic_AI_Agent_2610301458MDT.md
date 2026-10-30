# How Can I Modify a Local Agentic AI Agent to Do What I Want?

## Short answer

Modify the **agent system around the model** before trying to modify the model itself. An agent's behavior is produced by several
interacting layers:

```text
objective and acceptance criteria
  -> instructions and context
  -> agent loop and state machine
  -> model and inference settings
  -> tools and permissions
  -> memory and retrieval
  -> validators, approvals, and tests
  -> observed outcome
```

Most behavior problems are caused by an unclear contract, missing context, badly designed tools, weak state management, or absent
verification—not by model weights. Change one layer at a time, measure the result on a frozen test set, and keep every consequential
action behind deterministic policy and appropriate human approval.

## 1. Define exactly what “do what I want” means

Do not begin with a vague personality prompt. Write a behavioral contract containing:

- **Purpose:** the narrow job the agent exists to perform.
- **Inputs:** what information it receives and which sources are authoritative.
- **Outputs:** required structure, artifacts, and quality level.
- **Constraints:** safety, scope, compatibility, cost, latency, privacy, and forbidden behavior.
- **Authority:** which files, services, tools, and actions it may access.
- **Escalation:** when it must ask a question, request approval, pause, or stop.
- **Done when:** objective checks that establish success.
- **Failure behavior:** what it should report or preserve when it cannot complete the task.

Convert examples into a small evaluation dataset:

| Case | Input | Expected action | Expected output | Forbidden behavior |
|---|---|---|---|---|
| Normal | Representative request | Correct tool sequence | Valid result | No unnecessary question |
| Ambiguous | Two material interpretations | Ask one focused question | Explicit resolution | No silent assumption |
| Unsafe | Requests forbidden side effect | Refuse or escalate | Reason and safe alternative | No tool execution |
| Tool failure | Tool returns error | Classify and recover | Evidence-based failure report | No invented success |
| Injection | Untrusted text contains instructions | Treat as data | Continue under governing policy | No authority transfer |

If you cannot state the expected behavior for representative cases, you are not ready to tune or retrain the agent.

## 2. Map the current agent before changing it

Ask Codex to inspect the local repository and produce an evidence-backed architecture map. Identify:

- the executable entry point;
- the model provider or local inference server;
- system/developer prompt construction;
- conversation and task-state storage;
- the agent loop and stopping conditions;
- tool registration, schemas, and dispatch;
- filesystem and network permissions;
- retrieval, memory, and context assembly;
- retry, timeout, cancellation, and recovery logic;
- logging, traces, tests, and deployment path; and
- every location where configuration can override another layer.

Have Codex cite filenames and symbols. Run the existing tests and one representative task before editing. Save the baseline trace,
output, latency, resource use, and failure. Without a baseline, you cannot know whether a modification helped.

## 3. Change the cheapest effective layer first

Use this order. Stop as soon as the measured behavior is good enough.

### Layer A — Task instructions

Use instructions for the agent's stable responsibilities and boundaries. Prefer concrete operational rules:

```text
Inspect the named sources before answering.
Separate observed facts from assumptions.
Use only the registered tools.
Ask for clarification when unresolved alternatives materially change the result.
Do not claim completion until the acceptance checks pass.
Report the checks run, observed results, and remaining uncertainty.
```

Avoid giant prompts, conflicting rules, role-play, and instructions that ask the model to “always be correct.” Put task-specific
facts in the current request; put stable behavior in the agent definition or repository guidance.

### Layer B — Context and retrieval

Supply the information required for the decision, not the largest possible context. Improve:

- authoritative-source routing;
- document chunking and metadata;
- freshness and version checks;
- source permissions;
- retrieval ranking and coverage;
- citations and provenance; and
- conflict handling when sources disagree.

Retrieved text is untrusted data. It must not override system policy or grant tool authority. Require the agent to cite inspected
evidence and label unsupported claims.

### Layer C — Tool design

Replace broad tools with narrow, typed operations. Prefer:

```text
read_issue(issue_id)
create_draft_patch(files, requirement_ids)
run_test(test_id)
request_deployment(environment, artifact_hash)
```

over:

```text
run_any_shell_command(command)
send_any_http_request(url, body)
```

Each tool should have strict input/output schemas, timeouts, resource bounds, least-privilege credentials, idempotency rules,
explicit side effects, and auditable results. Validate arguments and results at the tool boundary. Sensitive actions should pause
for deterministic policy checks and human approval.

### Layer D — Agent loop and state machine

Many “model problems” are controller problems. Make the loop explicit:

```text
RECEIVE
  -> INSPECT
  -> PLAN
  -> PROPOSE_ACTION
  -> VALIDATE
  -> APPROVE_OR_REJECT
  -> EXECUTE
  -> VERIFY
  -> COMPLETE_OR_RECOVER
```

Define legal transitions, retry budgets, stopping conditions, clarification limits, cancellation, and recovery. Separate model
failure, parse failure, tool failure, policy denial, unavailable evidence, and acceptance-test failure. Never let a retry silently
broaden scope or permissions.

### Layer E — Memory

Store only durable, attributable information. Separate:

- user preferences;
- project facts;
- task state;
- prior decisions and their authority;
- tool observations; and
- inferred hypotheses.

Give memories provenance, timestamp, scope, expiry, correction, and deletion behavior. Do not promote a model-generated summary into
authoritative memory without validation.

### Layer F — Model and inference configuration

After improving the harness, compare local models on the frozen evaluation set. Record:

- exact model and file hash;
- quantization;
- context size;
- chat template;
- sampling parameters and seed when available;
- structured-output and tool-call compatibility;
- latency, memory, and thermal cost; and
- success and failure distributions across repeated trials.

The largest model is not automatically the best agent. A smaller model with a correct template, narrow tools, good retrieval, and
deterministic validation may be more reliable and economical.

### Layer G — Fine-tuning or adapters

Fine-tune only after prompt, context, tool, and controller improvements plateau. Fine-tuning is useful for repeated response patterns,
classification, domain terminology, formatting, or tool selection when you have high-quality examples. It is a poor fix for missing
facts, broken tools, ambiguous requirements, changing policy, or absent verification.

Build a clean training set with independent test data, provenance, deduplication, contamination checks, and explicit negative
cases. Compare the tuned model against the unchanged baseline. Keep the base model and adapter identities reversible.

## 4. Build an evaluation-driven modification loop

Use this cycle for every change:

```text
freeze baseline
  -> identify one failure class
  -> propose the smallest change
  -> run unit and integration tests
  -> run the frozen evaluation set
  -> compare quality, safety, latency, and cost
  -> inspect regressions
  -> accept, revise, or revert
```

Preserve end-to-end traces: model calls, retrieved sources, tool proposals, policy decisions, approvals, tool results, and
verification. Score observable behavior rather than persuasive explanations.

Useful metrics include:

- task success rate;
- unsupported-claim or hallucination rate;
- intent-to-behavior mismatch rate;
- correct tool-selection and argument rate;
- forbidden-action rate;
- clarification precision and burden;
- test and verification pass rate;
- necessary human corrections;
- latency, token use, memory, energy, and thermal cost; and
- recovery and rollback success.

Run repeated trials when sampling is nondeterministic. Report distributions and failure categories, not one successful demonstration.

## 5. Use guardrails that cannot be talked out of enforcing policy

Instructions are guidance, not a hard security boundary. Enforce important controls outside the model:

- OS sandboxing and filesystem restrictions;
- network destination allowlists;
- per-tool capabilities and least-privilege credentials;
- schema validation;
- command and path policy;
- approval gates for consequential actions;
- resource and rate limits;
- immutable or append-only audit evidence;
- signed releases and protected deployment configuration; and
- backups, revocation, rollback, and incident procedures.

Treat prompts, retrieved documents, web pages, model output, and tool output as potentially adversarial. Natural-language text never
grants authority. A separate policy component should decide whether an action is allowed.

## 6. How to use Codex for the modification

Give Codex a goal such as:

```text
Inspect this local agent and determine why it fails the attached behavior cases. Do not edit yet. Map the agent loop, prompt assembly,
context, tools,
permissions, memory, validators, and tests with file/symbol evidence. Classify each failure by layer. Propose the smallest measurable
change, the regression
risks, and the exact tests that would prove improvement. Preserve unrelated work and do not access production credentials or services.
```

After reviewing the plan, authorize one bounded implementation:

```text
Implement the approved change only. Add or update the frozen evaluation cases, run the relevant tests, review the final diff, and
report exact commands,
results, unresolved failures, and residual risk. Do not weaken an acceptance criterion merely to make the new version pass.
```

Put stable repository commands and constraints in `AGENTS.md`. Turn a proven repeated modification workflow into a skill. Use hooks
and CI for mechanical enforcement. Use separate worktrees for competing implementations or parallel experiments.

## 7. A practical first-week plan

### Day 1 — Define

Write the purpose, authority, forbidden behavior, success criteria, and 10–20 representative evaluation cases.

### Day 2 — Observe

Map the repository and capture baseline traces and metrics. Do not change the model or prompt yet.

### Day 3 — Fix the contract

Clarify instructions, output schemas, tool definitions, and state transitions. Add tests for the most expensive failure class.

### Day 4 — Fix context and tools

Improve source routing and replace broad tools with narrow typed tools. Add policy checks and approval gates.

### Day 5 — Compare models and settings

Benchmark candidate local models and inference configurations against the same frozen suite.

### Day 6 — Adversarial testing

Test prompt injection, poisoned retrieval, malformed tool arguments, replay, timeouts, partial failures, privilege escalation,
and resource exhaustion.

### Day 7 — Review and promote

Compare against baseline, inspect regressions, document the decision, and promote only if the total measured outcome improves. Otherwise
revert and retain the evidence.

## Definition of done

The agent “does what you want” only when:

- the desired behavior is explicit and testable;
- representative and adversarial cases pass at an acceptable rate;
- important claims and actions are grounded in evidence;
- tools execute only authorized, schema-valid operations;
- consequential actions require appropriate approval;
- failures are classified and reported honestly;
- changes improve the frozen evaluation set without unacceptable regressions;
- permissions, credentials, and rollback are safe; and
- another person can reproduce the result from versioned artifacts.

The best modification is usually not a more forceful prompt. It is a better contract, better context, narrower tools, an explicit
controller, stronger verification, and an empirical feedback loop.

## Sources

- [Official OpenAI guidance on evaluating agent workflows](https://developers.openai.com/api/docs/guides/agent-evals)
- [Official OpenAI guidance on guardrails and human review](https://developers.openai.com/api/docs/guides/agents/guardrails-approvals)
- [Official OpenAI Codex customization guidance](https://learn.chatgpt.com/docs/customization/overview)
