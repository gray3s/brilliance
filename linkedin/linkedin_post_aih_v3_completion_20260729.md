AIH v3 is complete for the question it was designed to answer.

That question was not "which agent is best?" That would be the wrong conclusion from v3.

The v3 question was narrower: if we increase the configured max-ply budget, do the AI agents continue playing chess against themselves for more plies, and can the harness expose where they stop?

The completed v3 data now covers 42 tested agent lanes across OpenAI, Google, Anthropic, Ollama, and Codex. The final merged table contains 518 AIChess summary rows.

A late repair pass reran all 147 cloud rows that had previously failed because the required cloud key was unavailable. No targeted repair cells were skipped, and the repaired rows were merged back without overwriting the existing Google, Anthropic, Ollama, Codex, or already-valid OpenAI data.

That gives us enough for the v3 conclusion: the harness can show continuation behavior as maxply increases.

The maximum observed ply count reached 94. At low maxply caps, some agents reached the configured cap. At larger caps, the failure modes became more visible: invalid or unparseable moves, transport failures, game timeouts, token ceilings, and unsupported cloud configuration paths.

That distinction matters. We should not confuse:

- an AI agent continuing to play chess under the harness
- a clean relative performance ranking

AIH v3 gives us the first. It does not give us the second.

There are statistical distortions in the v3 table because part of the data came from a later repair run. The only way to remove that artifact would be a full rerun under one consistent environment. For the current v3 goal, the data is good enough. For relative performance and AIH ranking, v4 needs a different design.

That is the handoff.

AIH v4 moves from configured-ply self-play cells to paired agent-vs-agent games, richer error attribution, per-run gameplay records, timing data, and cross-correlations across agent, prompt, and stack configuration.

AIH v3 got the harness to the point where we can move forward with some assurance.

AIH v4 is where relative ranking starts.

AIH v3 completion README:
https://github.com/gray3s/brilliance/blob/main/aih/v3/AIH_V3_COMPLETION_README_20260729.md

#AI #AgentEval #LLMOps #LocalAI #AIInfrastructure
