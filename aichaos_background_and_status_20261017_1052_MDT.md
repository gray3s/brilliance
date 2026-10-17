# Aichaos: Can Order Be Recovered from Prompt Disorder?

Timestamp: 2026-10-17 10:52 MDT

## Background idea

“Will order evolve from vibecoding chaos?” began as a deliberately provocative model-evaluation question. The first concept was to give a local language model 10,000 random printable characters, treat its response as the next prompt, and repeat until the output became recognizable to a human.

That version exposed a basic experimental problem. Random text has high statistical information, but it has no known intended meaning for the model to recover. If coherent prose eventually appeared, we could measure its form, cost, and timing, but we could not show that the model had reconstructed anything true. It might simply have replaced noise with plausible language. We therefore moved the pure random-seed experiment to a future “rev-N”: an advanced stress test rather than the foundation of the project.

The current rev2 asks a narrower, more useful question: can a model recover a known technical prompt after a controlled and reproducible process disorders it?

## Rev2 benchmark design

The ground truth is a self-authored technical procedure written in the style and spirit of **ASD-STE100 Simplified Technical English (STE)**. The project does not reproduce the standard and does not claim formal ASD-STE100 conformance. STE is useful here because it favors direct, structured, low-ambiguity technical communication.

The procedure contains ten explicit meaning units, including the actor, input, output, checksum manifest, exclusions, timestamp, verification method, failure behavior, report format, and success criterion. A seeded corruption process then introduces bounded character swaps, punctuation damage, whitespace damage, local token shuffling, and configured deletions. The corruption manifest makes the damaged prompt reproducible.

The local model receives that disordered prompt through llama.cpp. Each response becomes the next prompt. The model is instructed to reconstruct a coherent, fully understandable technical prompt, preserve recoverable requirements, and introduce no new requirements.

## What the evaluator measures

The benchmark separates direct observations from interpretations. Direct observations include returned text, wall-clock latency, prompt tokens, completion tokens, and total tokens. Deterministic proxy measurements include:

- character-sequence similarity to the ground truth;
- token multiset F1;
- token-bigram F1;
- retention of all ten required meaning units;
- a generic prose-order score based on visible language structure, repetition, punctuation, and compressibility;
- a weighted composite recovery score.

These are proxies, not proof of semantic equivalence. A provisional success requires an exact completion marker, retention of all ten meaning units, token F1 of at least 0.70, and a generic order score above its configured threshold.

Sampling is measured in prompt-returns. The current policy scores and retains full text at loop 0, every tenth return, and all marker, success, final, and error events. Other returns retain hashes, lengths, latency, marker state, and token usage. This preserves an audit trail without storing redundant raw text. Stagnation and maximum-return rules prevent unlimited compute consumption.

## Current status

The project directory and rev2 specification exist. Several smaller local instruction models produced invalid rev1 implementations, which was useful evidence about implementation reliability but did not count as experiment returns. A Nemotron 3 Nano agentic model is now implementing rev2 through a local llama.cpp server. Codex is acting as orchestrator and verifier rather than directly authoring the evaluator, in accordance with the project constraint that the local model do the lion’s share.

The local server is CPU-only. Observed generation has been approximately 3.2–4.1 tokens per second, making latency and tokens-to-recovery meaningful benchmark dimensions in their own right.

No rev2 recovery run has completed yet. Accordingly, the project makes no claim that order has been recovered. The present result is the experimental design, provenance, and an implementation in progress.

## Why it may be useful

A conventional benchmark often asks whether a model knows an answer. Aichaos asks whether it can restore structure and requirements under controlled damage, how much it loses or invents, how many recursive returns it needs, and what that recovery costs. Future revisions can vary corruption severity, technical domains, model sizes, and quantization levels while preserving the same evidence model.

The aim is deliberately modest: reproducible observations, explicit proxies, falsifiable thresholds, and enough retained evidence for engineers and skeptics to disagree productively.

