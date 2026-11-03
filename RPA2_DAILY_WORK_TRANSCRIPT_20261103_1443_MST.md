# RPA2 daily work transcript — cppllama, carcase mirror, DeepSeek research, and security

Created: 2026-11-03 14:43 MST  
Covered date: 2026-11-03 MST  
Record type: reconstructed conversational work transcript

## Record limitation

This file reconstructs today’s conversation from the context available to the assistant at creation time. It is not a byte-for-byte
platform export. User requests are quoted or closely transcribed where available; assistant work, tool calls, and long command output are
summarized. Project evidence files and installed artifacts named below remain the authority for exact technical details.

## 1. Initial cppllama direction

The user described a perceived difference between vLLM and llama.cpp and said the custom C++ implementation should be called
**cllama.cpp**. The user asked how those differences should be reconciled, then requested a new **cppllama** subproject with a `misc`
folder, timestamped project goals, and timestamped implementation plans. The requested direction was to check the existing llama.cpp
codebase for upgrades and, if the stack remained llama.cpp-based, selectively migrate current llama.cpp and vLLM features into cppllama.

The user added these required control goals:

- automatically manage load using desired temperature up/down ramps;
- distinguish user responsiveness from background-system throughput;
- monitor foreground load and reduce background load accordingly; and
- respect hard limits derived from established stability criteria.

The cppllama project was created at:

`/home/sag/RPA2/devneutral/cppllama`

The initial native C++20 skeleton included request lifecycle and admission contracts, CMake configuration, tests, goals, implementation
plans, upstream assessment, local-delta inventory, a five-hour work charter, and a resume checkpoint.

The existing llama.cpp checkout was assessed without merging or resetting it:

`/home/sag/RPA2/devneutral/servers/llama.cpp/relocated-gpt-llama.cpp-20260905_114935_MDT`

The checkout and configured upstream both resolved to commit `458681e1d5d4a29a1463c4732e03226cf384b997`, tag/build `b10729`. Local native
changes and intentional no-Python deletions were preserved.

## 2. Carcase storage and subtree review

The user asked how large the carcase folder was and whether the v3 subtree was missing anything present in v1 or v2.

The recorded source size was `4,970,953,433` bytes, approximately 4.7 GiB on disk. The comparison found that v3/Pass1 corresponded to
v2/Pass10 plus new v3 documentation and notices. Files from v1 were represented by content hash. Three older unique source PDFs were
identified as material worth preserving, and they remained covered because the entire carcase subtree was later mirrored.

## 3. Lexar 32 GB archive mirror and secondary swap

The user decided that the 32 GB Lexar removable drive should contain:

- an 8 GB carcase mirror partition with growth space; and
- the remaining capacity as secondary swap, balanced with the internal swapfile when the Lexar is present.

The user explicitly authorized formatting the Lexar. Its original DataShield contents were first preserved at:

`/home/sag/RPA2/devneutral/removable-media-backups/Lexar-original-20261103_1245_MST`

The Lexar was partitioned as:

- `/dev/sdb1`: ext4, label `CARCASE_MIRROR`, mounted at `/mnt/carcase-mirror`, UUID `faca1191-9f43-4791-8a95-58af4fd781f3`;
- `/dev/sdb2`: Linux swap, label `CARCASE_SWAP`, UUID `bfc93dff-e905-4df7-b01a-0b18e0198c3b`.

The full source tree `/home/sag/RPA2/general/legal/carcase/` was mirrored to `/mnt/carcase-mirror/carcase/` with metadata and hard-link
preservation.

Validation results:

- checksum dry-run: exit code 0, no differences;
- source and mirror: `4,970,953,433` bytes each;
- source and mirror: 8,735 regular files each;
- mirror partition: about 4.7 GB used and 3.2 GB free at validation time.

The Lexar swap partition provided `22,446,841,856` bytes. It and the internal 16 GiB `/swapfile` were both activated at priority `-1`,
allowing Linux to balance new swap allocations across equal-priority areas.

Persistent UUID entries were added to `/etc/fstab` with `nofail` and a bounded device timeout. A duplicate existing `/swapfile` line was
removed. The prior file was backed up as:

`/etc/fstab.backup-carcase-20261103-1306-MST`

The safety rule communicated to the user was: disable Lexar swap before physical removal, then unmount the mirror.

## 4. Lexar bootstrap and shutdown lifecycle

At the user’s request, two host-side executable helpers were created:

- `/home/sag/.local/bin/bootstrap-carcase-lexar`
- `/home/sag/.local/bin/shutdown-carcase-lexar`

The bootstrap helper mounts the archive partition using the persistent fstab entry and activates the Lexar swap at priority `-1`. The
shutdown helper calls `sync`, disables Lexar swap, calls `sync` again, and unmounts the archive partition. Both helpers are UUID-based,
idempotent, and root-aware: they run directly as root under systemd or request authorization during interactive use.

The helpers were integrated through:

`/etc/systemd/system/carcase-lexar-lifecycle.service`

The service is enabled at `multi-user.target`, runs bootstrap during startup, and runs shutdown during service teardown. Final checks
showed the service enabled and active, the mirror mounted read-write, and both swap areas active at priority `-1`.

## 5. DeepSeek “super-user mode” clarification

The user asked about a supposed DeepSeek “super-user mode” in the context of a frontier-model trend: increasing model size for
capacity/context while also making models and inference more space-efficient.

Current official sources did not identify a DeepSeek feature literally named “super-user mode.” The likely conflated features were
documented as:

- Expert Mode or Think Max: increased inference-time reasoning effort;
- DeepSeek Harness: background work, local files, terminal operations, plugins, subagents, scheduled tasks, and experimental auto-approval;
- Creator Mode: conversational creation and installation of Harness plugins.

The distinction recorded was that model capacity, active parameters, context memory, inference-time deliberation, tool access, and
operating-system authority are separate controls. cppllama should not combine them into one ambiguous “power mode.”

The clarification was saved and opened in gedit as:

`/home/sag/RPA2/devneutral/cppllama/misc/CPPLLAMA_DEEPSEEK_SUPER_USER_MODE_CLARIFICATION_20261103_1323_MST.md`

## 6. Open-source technology watch for DTA/E and cppllama

The user clarified that the project should look for any good ideas for the next DTA/E model and cppllama builds. The DeepSeek note was
expanded using current primary project sources.

Relevant projects and ideas included:

- **FreeToken:** bandwidth-adaptive, edge-native MoE serving; dynamic expert residency; CPU/GPU execution; agent-state reuse; runtime
  memory mapping.
- **SGLang:** Paged Experts; file-backed L3 prefix cache; quantized-MoE LoRA; speculative decoding; persistent fused decode kernels.
- **vLLM:** compressed and low-precision KV state; sparse attention; fine-grained prefix reuse; async scheduling; adaptive speculative
  budgets.
- **llama.cpp expert-cache RFC:** protected persistent accelerator slots for host-offloaded experts, with separate prefill and decode
  behavior.
- **FlashInfer:** increasingly specialized attention, quantization, MoE, and communication kernels.
- **XGrammar:** native structured generation and JSON-schema enforcement for DTA/E proposal records.
- **LMCache:** tiered KV-cache interfaces, observability, non-prefix reuse, and transfer backends.
- **KTransformers:** heterogeneous sparse-model inference and LoRA/full/hybrid adaptation across CPU and GPU resources.
- **bitnet.cpp:** native ternary inference and compact embedding models for possible routing, evidence retrieval, deduplication, and cache
  admission.

The no-Python boundary was preserved: Python-oriented projects may serve as architectural references, not as runtime dependencies under
RPA2.

Priority recommendations included constrained DTA/E candidate generation, tiered cache observability, protected decode expert sets,
phase-aware scheduling, deterministic replay identities, foreground/background control, and measured flash-endurance policies.

## 7. cppllama active-build provenance

Three distinct `llama-server` executables were found under the current source tree:

- `build/bin/llama-server`, SHA-256 `60f8c27337ec25d54072670b8af1268dd159baa38c801f80d36e73e7406bf580`;
- `build-local-native/bin/llama-server`, SHA-256 `a7834487c88cffb90c60a2252fc23c5c91f967d6a391a49dac2cb95f0d804c70`;
- `build-devneutral-resume1-20260905_120410_MDT/bin/llama-server`, SHA-256
  `be28bce3e80d19fade51ccb2e0b3673f2a5ae7513d0948d5f083e813bcc15ecf`.

The nominal `build/bin` executable failed direct invocation because `libllama-server-impl.so` was not resolved. The other two reported
llama.cpp version `0.3.0-dev`, build `10729`, commit `458681e1d`, compiled with GNU 13.3.0 for Linux x86_64.

Consumer review found that FlowiseCPP and several EFFVC paths defaulted to the nominal `build/bin` path, while AIGPTChessPlay could
dynamically select the newest runnable build. The resulting requirement was to identify each runtime by canonical path and content hash
rather than source commit alone.

The provenance report is:

`/home/sag/RPA2/devneutral/cppllama/misc/CPPLLAMA_ACTIVE_CONSUMER_AND_BUILD_PROVENANCE_20261103_1330_MST.md`

## 8. cppllama prototype build and local deployment

The following native components were added:

- `ExecutionBackend` capability interface;
- native `LlamaCppExecutableProbe`;
- direct fork/exec informational probing without a shell;
- bounded timeout and diagnostic capture;
- positive and negative parsing tests;
- a deterministic `LoadGovernor` state machine.

The probe invokes only `--version` and `--help`; it does not load a model or open a server port. It reports backend identity and detected
capabilities and rejects missing, timed-out, unrecognized, or loader-broken executables.

The load governor supports:

- bounded worker ramp-up;
- thermal ramp-down;
- foreground-pressure yielding;
- hard temperature and memory/I/O-pressure stops;
- recovery dwell before work resumes;
- fail-closed behavior for invalid or stale observations; and
- execution envelopes bounded by a non-overridable stability envelope.

Build and validation results:

- Debug build: passed;
- Release build: passed;
- native contract test: one of one passed;
- AddressSanitizer and UndefinedBehaviorSanitizer: passed;
- LeakSanitizer: unavailable inside the ptrace-managed environment, recorded as an environmental limitation rather than a product failure;
- live probe of the explicit runnable backend: passed;
- broken-loader and missing-executable negative paths: correctly rejected;
- no-Python audit: clean.

Local deployment:

- `/home/sag/.local/bin/cppllama-probe`
- `/home/sag/.local/lib/libcppllama.a`
- `/home/sag/.local/include/cppllama`

The installed probe SHA-256 was `3a448193de24d9d7d8543c18a4726dffc6604b7a86da50cb97054678090c7d3c`.

No active consumer was switched to cppllama.

## 9. Attack residue beyond patching and reinstall

The user requested current security developments concerning attacks that leave residue not removed by a bug fix or reinstall.

The resulting assessment emphasized that patching prevents or constrains future exploitation but does not prove eviction. An ordinary
reinstall may leave persistence in:

- EFI partitions, boot configuration, UEFI variables, or Secure Boot keys;
- UEFI/BIOS or component firmware;
- BMCs and independent management controllers;
- routers, firewalls, switches, and other infrastructure;
- OAuth grants, service principals, API keys, tokens, MFA methods, sessions, and SaaS integrations;
- MDM/RMM, domain policy, recovery systems, synchronized browser state, and backups; or
- source control, CI/CD, registries, signing systems, build workers, and package caches.

The documented defensive sequence was: contain from a clean control point, preserve evidence, evict identity persistence, audit management
planes, restore platform trust, rebuild all relevant storage including EFI, recover data selectively, verify independently, and replace
unverifiable hardware when high assurance requires it.

The assessment is stored at:

`/home/sag/RPA2/devneutral/misc/ATTACK_RESIDUE_BEYOND_PATCH_AND_REINSTALL_20261103_1333_MST.md`

## 10. State at transcript creation

The most recent cppllama resume checkpoint is:

`/home/sag/RPA2/devneutral/cppllama/misc/CPPLLAMA_RESUME_CHECKPOINT_20261103_1337_MST.md`

The next safe implementation action recorded there is:

1. implement a read-only native `HostSampler`;
2. implement an append-only governor evidence sink;
3. capture thermal-zone, CPU/load, memory/swap-pressure, and PSI observations;
4. validate synthetic and live low-risk timelines; and
5. do not connect governor output to an active llama.cpp process until sensor identity, stale-data rules, evidence format, and live
   thresholds are frozen.

## 11. New logs-subproject instruction

The user requested a new subproject at `/home/sag/RPA2/logs`, a `misc` subfolder, a `2611` subfolder for November 2026, and one
appropriately titled, minute-stamped daily transcript. The existing logs directory was preserved; the two requested subfolders and this
transcript were added without altering the pre-existing log files.

## 12. Document reformatting

At the user's request, the two Markdown documents open in gedit and this daily transcript were conservatively wrapped to approximately 140
characters per line. Existing Markdown paragraph, list, heading, and code-fence boundaries were preserved. Validation found zero lines
longer than 140 characters in all three documents.

The two gedit documents were:

- `/home/sag/RPA2/devneutral/cppllama/misc/CPPLLAMA_FIVE_HOUR_WORK_CHARTER_20261103_1236_MST.md`; and
- `/home/sag/RPA2/devneutral/cppllama/misc/CPPLLAMA_DEEPSEEK_SUPER_USER_MODE_CLARIFICATION_20261103_1323_MST.md`.

## 13. Native host monitoring and evidence prototype

The next cppllama implementation stage added a read-only native `HostSampler`, an installed `cppllama-host-sample` diagnostic, and a
`GovernorEvidenceSink`. The sampler records thermal-zone observations, temperature slope, CPU utilization, one-minute load, available
memory, swap use, and memory/I/O pressure-stall indicators. The evidence sink writes a versioned tab-separated schema, sanitizes free-text
fields, flushes records to storage, and now uses an exclusive file lock to serialize concurrent appenders.

Release and sanitizer builds passed, and the native contract suite passed one of one tests in each configuration. The installed diagnostic
successfully discovered 15 thermal sensors. Its 14:52 MST observation reported a maximum temperature of 84,000 mC, CPU use of 478 basis
points, a one-minute load of 690 milli-load units, 4,601,032 KiB available memory, and 2,326,404 KiB swap in use. Memory and I/O full-stall
PSI avg10 were both zero.

No active llama.cpp consumer, process, port, or launch default was changed.

## 14. Chrome-to-Firefox extension migration project

At 14:54 MST, the user introduced a future project concerning a service-provided Chrome extension that they would prefer to use in Firefox
rather than resume using Chrome. The service and extension have not yet been identified, acquired, inspected, executed, or installed.

The paused project was created at:

`/home/sag/RPA2/jobsearch/extensions/migrate`

Its timestamped goals and development plan are stored in the project's `misc` directory. They establish an authorized-artifact and
provenance workflow, a static Chrome-to-Firefox compatibility and security assessment, least-privilege porting, isolated-profile testing,
native reproducible packaging, licensing and signing review, and explicit stop gates. The plan preserves the `/home/sag/RPA2` no-Python
rule and prohibits placing credentials or session tokens in project files or transcripts.

The first action when work resumes is to record the service and official extension source, obtain the exact artifact with user
authorization, preserve and hash an untouched original, and complete the static compatibility and security inventory before executing
extension code.

Per the user's instruction, project work stopped for the day after this transcript update.

