# AIH v4 project goals - 2026-07-29

AIH v4 should test whether the available AI agents can establish a ranking
among themselves by playing complete chess games against each other.

At this stage, the AI agents are being used as test subjects while the
surrounding AIH test environment is firmed up. Early v4 runs should therefore
be treated as environment-hardening and instrumentation runs, not final agent
ranking claims.

The confidence will never be 100%, but it can become high enough to make the
associated relative performance results and AIH ranking reasonably usable in
terms of relevance and trustworthiness.

## Pairwise Agent Games

For AIH v4, divide the available agents into pairs and have the paired agents
play against each other until they complete a game.

The goal is not only to test whether an agent can survive a scaffolded move
request. The goal is to see whether pairwise play can prove that each agent can
establish a ranking among the available agents.

The v4 runner should still track and handle invalid moves, but the primary
test unit should become an agent-vs-agent game that reaches a terminal game
result or a configured fatal-error result.

The first v4 implementation should not depend on a clock-based agent turn
timeout or move timeout. Before adding per-turn timeout pressure, v4 should
observe what agents do when left to their own devices.

AIH v4 should not use the AIH v3 configured-ply/scaffolded-cell approach to
rate agents on a relative scale. The v3 approach is only good for establishing
that AI agents will actually continue to play chess against themselves for more
plies as the maxply limit increases. It cannot establish a relative ranking
among agents by itself.

To establish relative performance and an AIH ranking, v4 needs to measure both
relative chess performance and AIH behavior.

Relative chess performance should come from paired games between agents, with
the game result recorded for each matchup.

Each non-skipped pairing should be tested in both color assignments: agent A
as white against agent B as black, and agent B as white against agent A as
black. Color-swapped results should be preserved so ranking analysis can
separate agent performance from first-move and color effects.

In result tables, the model-pairing display convention should be white player
on the top line and black player on the second line. Reversed-role games can
then be displayed concisely by reversing those two lines in the paired result
row.

When the color-swapped role results do not show significant statistical
variation resulting from a swap of the colors at the current confidence level,
the reporting layer may condense the reversed-role display instead of showing
two full rows. When the color swap appears to affect outcomes, timing, retry
behavior, or error rates, the reversed-role rows should remain explicit.
The report should also include summary statistics for variation resulting
from color swaps and from the other controlled run parameters, even when the
detailed reversed-role display is condensed. Variation from other run
parameters may need to be shown in a separate summary table so it does not
confuse the color-swap or pairing display.

The summary statistical data is the primary reporting target for v4. Pairing
displays are lower-level supporting data used to inspect and explain the
summary statistics, not the final analytical product by themselves.

The practical end goal for this reporting stream is to determine which agents
should be used for the AIH AIChess test and why. The recommendation should be
based on summary statistics first, with lower-level pairing records,
color-swap behavior, run-parameter variation, transport/stack reliability,
retry/error behavior, cost, and elapsed-time data used as supporting evidence.
Classification of the AIH tests themselves is a separate issue and should not
be conflated with agent-selection reporting.

V4 should allow the same base model to play against itself when the two sides
use different model modes, such as different reasoning, verbosity, stack,
prompt, or adapter configurations. Direct same-model same-mode comparisons
should be skipped because they do not add useful relative ranking information
at this stage.

AIH behavior should be measured from the agent's ability to keep participating
in the harness correctly, including response validity, invalid-move handling,
retry behavior, concessions caused by agent response errors, and the structured
error configs observed during play.

V4 should also measure the time required to get an agent response to each
prompt, including any required prompt repeats.

For each turn, record the elapsed time from the first prompt for that turn
through the referee's final determination of the turn end-state.

The turn end-state timing should include:

- color played
- first prompt timestamp
- each prompt-repeat timestamp
- each agent response timestamp
- referee determination timestamp
- final turn end-state
- total turn elapsed time from first prompt to referee determination

This adds move-time and number-of-moves as additional v4 data streams.

V4 should preserve gameplay records on a per-run basis. The color played for
each move can be retrieved from the gameplay records, but it should also be
recorded in the per-turn data for now. That redundancy is acceptable during
early v4 development, and efficiency can be cleaned up later.

## Smoke Test Staging

AIH v4 smoke testing should proceed in stages.

The first smoke stage should use free local installed agents only, at low
`maxply` values, with the harness/rules referee. This stage is meant to prove
that the v4 runner executes pairwise games, records prompt/response evidence,
repeats failed prompts, and resolves game termination without burning cloud
budget.

Cloud agents should not be excluded from v4. They should be introduced after
the local smoke stage is running, one provider at a time, to test the associated
provider keys and cloud stack paths individually.

Provider-key cloud smoke should use low or medium thought settings first. High
and extra-high thought modes should be reserved for later smoke stages after
the provider key path, transport behavior, output capture, and referee
resolution are known to work.

The cloud smoke goal is to avoid building gaps into the test data while also
avoiding unnecessary cloud-token burn during early v4 harness development.

The initial run-control stages are:

- `local-progress`: free local installed agents only, low `maxply`, one
  response attempt, and enough scaffolding to confirm that pairwise play can
  make valid progress.
- `local-retry`: free local installed agents only, low `maxply`, three
  response attempts, and explicit retry/concede behavior.
- `local-expand`: more local pairs and a modestly higher `maxply`, used only
  after the progress and retry stages are working.
- `cloud-provider-key`: one cloud provider at a time, low `maxply`, one
  response attempt, key preflight, and medium/low thought settings to verify
  provider licensing, agent/model entitlement, and provider transport without
  burning higher thought modes.
- `full-agent-set`: later-stage smoke only, after local and provider-key
  smoke tests are already passing. This stage must be given an explicit roster
  through the run controls; it must not silently treat the local Ollama cache
  as the full v4 roster.

The v4 executable and wrapper should default cloud reasoning/verbosity controls
to medium for smoke testing. High and extra-high modes must be selected
deliberately.

V4 must be able to run cloud-capable agents across a controlled reasoning
range and, where supported by the provider stack, a controlled verbosity
range. This should be implemented as an explicit run-control matrix, not as an
implicit accidental multiplication of cloud calls.

The v4 reasoning range is:

- `low`
- `medium`
- `high`
- `xhigh`

The v4 verbosity range is:

- `low`
- `medium`
- `high`

The default provider-key smoke stage should continue to run a single medium
configuration unless the reasoning matrix is explicitly requested. When the
matrix is requested, the wrapper should run one engine invocation for each
reasoning/verbosity configuration, stamp the reference config with the
selected pair, and export the corresponding provider controls for that run.

The public v4 run-control name should be `verbosity`, not `text-verbosity`.
At this stage all captured AI output is string-derived, so a separate text/data
verbosity distinction would add confusion without adding measurement value.
Provider-specific adapters may still map the v4 `verbosity` control into a
provider field such as OpenAI text verbosity.

The public matrix controls should be named `--allowed-reasonings={...}` and
`--allowed-verbosity={...}`, with environment equivalents
`AIH_V4_ALLOWED_REASONINGS` and `AIH_V4_ALLOWED_VERBOSITY`. The generic runtime
verbosity environment should be `AICHESS_VERBOSITY`. Provider-specific adapter
variables may still exist for compatibility, but they should not be the primary
v4 control names.

Data verbosity can still be relevant, but it is a separate harness concern. It
should describe how much run data is saved, summarized, indexed, or processed
inline from the captured streams. It should not be mixed with agent verbosity,
which controls the agent's generated response style.

If v4 adds data-verbosity controls later, they should apply to persistence and
stream-processing choices such as raw response retention, prompt/response
artifact writing, inline summarization, trace length, per-turn evidence
density, and derived-data extraction. They should not change the prompt sent to
the agent or the provider reasoning/verbosity settings.

Verbosity should only multiply runs for provider stacks that actually use that
control. In the current v4 harness, verbosity maps to the OpenAI text verbosity
field. Non-OpenAI cloud sweeps should hold verbosity at `medium` unless and
until an equivalent provider-specific verbosity control is implemented.

The JSONL output should record both the requested v4 reasoning mode and the
provider-effective control values, so later analysis can distinguish the v4
logical test range from provider-specific mappings.

Before any cloud-looking model spec is run, the wrapper should preflight the
relevant provider key or equivalent licensing token without printing the key.
For Google/Gemini paths, `GOOGLE_API_KEY` or `GOOGLE_GENAI_API_KEY` may be used
as a fallback source for `GEMINI_API_KEY`.

Run controls should be adjusted from observed effort. The first local progress
run reached four legal plies but took roughly 162 seconds with high trace
settings. That is enough to split "can make valid progress" from
"retry/concede behavior" and to reduce default output budget/log volume for
routine local progress smoke while retaining full JSONL artifacts.

The 2026-07-29 v4 local-progress runs after the reasoning/verbosity control
changes reached four legal plies in roughly 142 seconds with no
illegal/unparseable moves and no transport failures. The corrected run also
recorded `move_calls_requested=1` and `move_calls_attempted=1` for every ply.
That confirms the local-progress stage can make valid progress, but it is
still slow and verbose enough that routine local-progress should keep one
response attempt, four plies, harness referee, and `loglvl=2`. Higher trace
settings should be selected deliberately when debugging parser or transport
details.

While v4 is still under active algorithm and harness correction, the default
stage should remain a retry/debug confidence stage, not the quieter
local-progress stage. Quieter `loglvl=2` local-progress runs are appropriate
for release-style smoke once there is sufficient confidence in the current
code path. Until then, retry/debug stages should remain the default entry
point so failures preserve enough live detail for correction.

The 2026-07-29 default retry/debug run exposed and corrected a retry parser
issue. Exact-prompt retries should parse the agent's actual response each time;
they should not suppress a repeated illegal UCI candidate as if it were
unparseable. After correction, the observed retry failure was recorded as
three illegal attempts, zero invalid/unparseable attempts, and a forfeit after
the configured retry budget. Summary artifacts should distinguish failed turns
from rejected attempts.

V4 run settings must balance speed, quality, and cost appropriately. Debug
settings should spend more time and output detail when that buys evidence
needed to correct the harness. Release settings should use the same code path
with tighter live output, deliberate run scale, deliberate reasoning/verbosity
ranges, and controlled cloud inclusion. Cloud stages must also account for
token cost and provider licensing/entitlement risk. Local stages avoid token
cost but still consume wall time and machine resources, so local maxply,
pair-count, retry-count, and log settings should be scaled deliberately.

V4 should use one primary maxply setting and derive cloud depth from it. Local
runs should use `AIH_V4_LOCAL_MAXPLYS`; cloud runs should derive their maxply
from the local maxply divided by the local/cloud maxply ratio. The older
generic `AIH_V4_MAXPLYS` may remain a compatibility fallback, but it should
not be the primary control for staged v4 work.

Those maxply settings should also be exposed as run flags so one-off runs do
not require exporting environment variables. The wrapper should accept
`--local-maxplys=N` and `--local-cloud-maxply-ratio=N`, with the environment
variables still available for batch scripts. The generic engine `--mxply` flag
should remain an internal runner argument after the v4 wrapper has selected
the stage-appropriate limit. For v4, the local/cloud maxply ratio should
default to 4 and be constrained to the range 2 through 4 until further notice.
The local maxply cap should be 40, producing a derived cloud maxply cap of 10.

The v4 binary entry point should also be able to regenerate the preliminary
summary publication artifacts without another manual editing pass. A run with
`--publish-summary` should execute the selected smoke stage and then refresh
the v4 preliminary Markdown/HTML result files plus the AIChess README from the
newest summary artifact. A run with `--publish-only` should refresh those
publication files from the newest existing summary without launching another
game. Pushing to GitHub should remain a separate deliberate action, exposed as
`--publish-and-push`, so routine local result generation does not unexpectedly
modify the remote repository.

The default local maxply should be increased from the first release-mode
baseline. The current baseline used local maxply 8 against cloud maxply 2, a
4x multiplier. The next v4 default should use local maxply 40 and derive cloud
maxply 10 from the default 4x local/cloud ratio. Raising the default local side
while halving the multiplier means the cloud baseline moves up deliberately,
with cloud cost still protected by the existing reasoning-sweep acceptance
gate.

Cloud staging should broaden the allowed provider thought/reasoning levels
before increasing cloud maxply. Keep cloud maxply low while testing provider
keys, model entitlement, transport behavior, and the `--allowed-reasonings`
range. Only after the cloud reasoning/thought-level sweep is explicitly
accepted should the local maxply or local/cloud ratio-derived cloud depth be
raised.

Expanding allowed reasoning levels should not automatically expand verbosity.
Those are separate cost and quality levers. A cloud reasoning sweep should hold
verbosity at the current default unless `--allowed-verbosity` or
`AIH_V4_ALLOWED_VERBOSITY` is explicitly set.

Cloud cost should be treated as measured run telemetry, not just a static
assumption. Published list prices, quota behavior, model entitlements, caching,
reasoning settings, output limits, and provider-side accounting can all affect
the effective cost of a run. V4 should preserve provider usage metadata when
available, including input tokens, output tokens, reasoning tokens or equivalent
provider counters, model/mode, and response identifiers. When exact cost cannot
be derived locally, the run should still preserve enough usage data to estimate
cost later and compare speed/quality/cost tradeoffs across run settings.

CLI cloud key administration should be deferred to AIH v5. V4 should still
preflight provider keys, classify missing-key and entitlement failures, and
avoid printing key material in logs or artifacts, but admin-managed key
selection, monitoring, rotation, and adjustment belong in the v5 design.
The v5 administration path should be available through CLI controls so monthly
key quotas, token refreshes, provider entitlements, and key rotation behavior
can be confirmed from the same operational surface used to launch AIH runs.
With adequate CLI key/quota administration, monthly quota boundaries should
have limited effect on AIH cloud-agent continuity, and any remaining failures
can be classified separately from harness, transport, stack, prompt, or agent
AIH errors.

AIH v5 can also begin introducing additional AIH tests beyond AIChess. The v4
work should provide better control over the run-parameter space, agent
selection, color/role effects, reasoning/verbosity settings, retry behavior,
transport/stack error separation, and summary statistical reporting. V5 should
use that improved control before broadening the test suite, especially after
establishing better control over token budgets and token expenditure rates.
Before launching a cloud-backed test configuration, the harness should be able
to verify that the selected cloud agents have enough available token capacity
to run that configuration, including expected prompt, response, reasoning, and
retry overhead.
Preliminary and debug stages should also be token-budget aware. They should
use low-cost settings, limited cloud depth, bounded reasoning/verbosity ranges,
and explicit run controls so cloud tokens are not wasted while the harness,
transport, parser, prompt, and retry behavior are still being debugged.

## Local Stack Expansion

AIH v4 should not assume that local AI means only Ollama.

AIH v4 should also not confuse a local executable with local AI. Any AI stack
that depends on a cloud token provider for inference is cloud-backed, not
local, even if the command is installed on the local machine and even if the
tokens are free of charge.

The operational test is simple: if the AI cannot run without a working
internet connection, then it is not local.

This includes cloud licensing restrictions. API keys are a cloud licensing and
access-control mechanism. Any AI stack that requires an API key or equivalent
provider token to perform inference is not local under the v4 classification.

A provider key passing one agentic AI path does not prove that every agent,
model, model mode, or stack path under that provider is authorized. V4 should
separate provider-key availability from agent-level entitlement. If a
candidate "local" agent needs a provider key, token, account entitlement, or
cloud licensing grant to run inference, that candidate is invalidated as local
AI and must be reclassified as a cloud-backed provider path.

Agent-level authorization failures should be tracked as their own cloud
failure class. They are not the same as a missing provider key, and they are
not agent hallucination. The error config should identify the provider, model,
agent label, requested model mode, stack module, required key/token class, and
the exact authorization or entitlement signal returned by the stack.

When these failures happen during a v4 game run, the run configuration should
be marked invalid rather than scored as a chess loss. The JSONL response
record should preserve a failure class such as `missing_provider_key`,
`cloud_authorization_or_entitlement_failure`, or
`suspected_remote_disablement_or_stack_availability`.

The same invalidation rule applies to cloud AI agents. If a cloud agent,
model, model mode, or stack path cannot run with the available key, token,
account entitlement, or cloud licensing grant, that specific run configuration
is invalidated until the authorization problem is corrected. The failure should
be recorded as a cloud authorization or entitlement failure, not as a chess
failure and not as AIH hallucination.

This also raises a separate control-plane risk: remote disablement of a
software element. A stack path can fail because a provider, license server,
remote model registry, account policy, or update channel disables a model,
feature, binary, adapter, or execution mode after the local harness was
configured.

V4 should track remote disablement as a distinct stack availability and
licensing-control failure class. It is not the same as:

- a missing local executable
- a missing provider key
- a provider key that exists but lacks agent/model entitlement
- a network transport failure
- an output timeout
- an agent hallucination
- an invalid chess move

When suspected remote disablement is detected, the error config should record
the software element affected, provider or upstream control point, local
version if known, requested model or feature, authorization material required,
the exact disablement signal or error text, first-seen timestamp, and
last-known-good evidence if available.

Ollama is the first free local stack used for v4 smoke testing because it is
already installed and has a working adapter path in the inherited AIChess
engine. That is a starting point, not the endpoint.

V4 should actively discover and onboard additional free local AI stacks while
they are available, including future local inference CLI adapters, local HTTP
adapters, and local model runtimes that do not depend on Ollama.

To count as local AI for this purpose, the stack must perform inference against
local model weights or a local inference runtime without requiring an internet
connection or a cloud provider token. Cloud-backed CLIs, apps, daemons, or HTTP
adapters should be tracked separately as cloud/provider stack paths and should
enter only through the cloud smoke stages.

Candidate local stacks must not be integrated blindly.

Before a new local stack is admitted to the smoke roster, v4 should require:

- a provenance record for where the stack came from
- an explicit adapter boundary
- a static scan for obvious hostile or high-risk behavior
- a sandboxed liveness probe
- no unauthorized access to cloud API keys or unrelated environment variables
- no unexplained network exfiltration behavior
- no privileged install, persistence, or destructive filesystem behavior
- quarantine and manual review for suspicious findings

The static scan is a triage gate, not a proof of safety. It can catch obvious
bad patterns, but it must be combined with sandboxing, provenance checks,
runtime observation, and conservative adapter design.

Anything brought down from the internet must go through an intake quarantine
before it can be unpacked, marked executable, linked into the repo, or used by
the v4 runner.

This is an appropriate place to reintroduce sandboxing. The normal v4 smoke
runner may need direct access to local runtimes such as Ollama, but
internet-downloaded stack candidates should be treated differently. They should
be inspected and liveness-tested under sandbox restrictions until the adapter
boundary and runtime behavior are understood.

The internet intake rule is:

- download only into a quarantine directory
- do not execute the downloaded artifact
- do not make the downloaded artifact executable
- record URL, timestamp, byte count, file type, and SHA-256
- list archive contents before extraction when the artifact is an archive
- extract only into quarantine after review, preferably inside a sandbox
- run the hostility precheck scanner against extracted contents
- run sandboxed liveness only after provenance and scan review
- do not expose cloud API keys or unrelated environment variables during
  liveness checks
- disable network access for first-pass liveness unless the adapter explicitly
  requires a local loopback server
- allow only the minimum filesystem access needed for the candidate runtime,
  its model files, and the test harness
- keep candidate stack execution separate from the normal trusted smoke runner

An internet-downloaded stack candidate should be treated as hostile until it
passes the intake gate.

## Initial Error Handling Rule

For the first v4 implementation, when the harness detects an error in an agent
response, repeat the previous prompt exactly.

Allow up to three attempts before declaring a fatal agent hallucination.

After three failed response attempts, execute the default error response:

- mark the side to move as conceding the game
- mark the game as a concede to the opposing player
- record the fatal result as an agent response failure after three attempts

This initial v4 behavior is intentionally simple. There will be cases of real
errors, but v4 should not confuse AI hallucination with transport, stack, or
prompt errors.

It is critical to separate AIH behavior from transport, stack, and possible
prompt errors. Transport errors, stack errors, and prompt errors must be
recorded separately from agent response failures. They must not be counted as
agent hallucination or AIH behavior failures.

For the first pass, repeat the same prompt up to three times before conceding
the game to the opposing player only when the harness has a completed agent
response path and detects an agent response error. Transport failures, stack
failures, adapter failures, timeouts, output-token ceilings, and suspected
prompt errors should be logged as separate non-AIH failure classes.

## V4 Response Terminator Marker

AIH v4 should add an explicit response terminator marker to agent prompts, for
example `AIH_RESPONSE_END` on its own line.

The harness should capture output until the terminator, timeout, transport
failure, or configured output limit. Agent replies may be verbose, so the
parser should not assume the chosen UCI move is the only text in the response.

If the terminator is missing after a normal complete response, and the harness
can prove the response was not truncated, timed out, transport-failed, or
stopped by an output-token ceiling, classify that as an agent
instruction-following failure or hallucination.

If the terminator is missing because of an I/O ceiling, timeout, or adapter
failure, classify it separately as a harness or transport limit.

## V4 I/O Wait Logging

AIH v4 should not emit repeated dot-progress lines while waiting for agent I/O.

For each agent I/O request, log one pending line:

```text
i/o pending...
```

Then log one terminal line when the request resolves:

```text
i/o complete
```

or:

```text
i/o error: timeout
```

This logging change is a v4 cleanup item, not a v3 correction item.

## Future Failure Mechanism Tracking

After the basic three-attempt retry behavior works, v4 should start tracking
what the harness suspects to be the failure mechanism.

Before over-analyzing causes, first generate the cross-correlations across the
recorded error configs.

The explicit first-pass analysis goal is: generate the cross-correlations.

The first analysis pass should report direct cross-correlations between:

- agents and error configs
- prompts and error configs
- stack configurations and error configs
- agents and prompts
- agents and stack configurations
- prompts and stack configurations

This cross-correlation pass should be descriptive. It should show which agents,
prompts, and stack configurations co-occur with which errors, without claiming
causality.

Future v4 modifications can then separate suspected failure mechanisms instead
of treating every failure as an agent hallucination.

The project goal is to eventually predict the error probability for each
observed failure pattern and separate that error probability into components:

- an agent component
- a stack component
- a prompt component

## Error Config Database

AIH v4 should build a database of errors as the v4 tests run.

For every detected error, record specifically what caused the harness to think
there was an error.

Only add unique error configs to the error-config table.

Each unique error config should identify the specific prompt, the specific AI
agent involved, and the specific stack configuration involved.

When the same error config occurs again, do not add a duplicate config row.
Instead, increment the instance count for that existing error config.

The database should make it possible to see which agents, prompts, and stack
configurations are generating errors.

The database should eventually support estimating and predicting the error
probability for a given agent, prompt, and stack configuration.

## Required Error Config Fields

Each unique error config should include at least:

- the specific reason the harness detected an error
- the prompt text or a stable prompt hash plus prompt artifact path
- the AI agent identifier
- the AI agent display label
- the provider
- the model
- the model mode or thinking mode
- the stack module
- the stack kind
- the stack name
- the board FEN
- the side to move
- the ply index
- the parser or validation rule that detected the error
- an instance count
- first-seen timestamp
- last-seen timestamp

Attempt-level logs should preserve the concrete response evidence separately
from the unique error config, including response artifact paths, response
hashes, attempt number, elapsed time, and transport or adapter status when
available.

## Non-Goals For The First V4 Pass

The first v4 pass does not need to fully diagnose whether each error came from
the agent, the prompt, the adapter, the transport, or the stack.

The first v4 pass should record enough structured evidence to let later v4
work separate those causes.

The first v4 pass should not overclaim causality. It should say exactly what
caused the harness to mark an error and preserve enough detail to analyze the
failure later.
