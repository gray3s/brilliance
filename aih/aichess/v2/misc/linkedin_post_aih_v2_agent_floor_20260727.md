AIH v2 update, July 26, 2026, 2130 MT

Agent list:
https://github.com/gray3s/brilliance/blob/main/aih/aichess/v2/misc/aih_v2_agent_list_20260726.md

Test notes:
https://github.com/gray3s/brilliance/blob/main/aih/aichess/v2/misc/aih_v2_test_documentation_20260726.md

Today was a compatibility day rather than a benchmark-expansion day.

The current AIH v2 goal is to get preschool and AIChess smoke tests running
across every agentic AI agent under consideration before treating deeper
comparisons as meaningful.

The current list includes 16 local agents and 9 cloud agents:
https://github.com/gray3s/brilliance/blob/main/aih/aichess/v2/misc/aih_v2_agent_list_20260726.md

The immediate question was basic:

Can each agent pass a fair admission floor before we start ranking deeper capability?

That led to a useful design rule:

If a preschool test is not passable by every active, responsive agent, it is
not preschool. Promote it to K-level or diagnostics.

The same principle applies to AIChess. The highest clue level should not let
the harness play the game by itself, but it also should not reject an agent just
because it did not speak our preferred protocol. Ask for a legal move naturally,
then grade the answer evidence.

Progress:

- Cloud agents 01-09 now pass the AIH v2 cloud smoke panel:
  - preschool response floor
  - AIChess at clue level 6

- Gemini AIChess was fixed by moving from the CLI subprocess path to the direct
  Gemini generateContent API path.

- GPT-5 nano now passes AIChess clue level 6 after the top scaffold was changed
  to accept direct legal-move evidence.

- Preschool was reduced to the currently proven floor:
  - "What is 0 + 1?"
  - "Name a letter."

- Copy/return tests were promoted out of preschool for now. They remain useful
  K-level or diagnostic tests.

- Local agents gave useful evidence:
  - Most responsive local agents pass the preschool floor.
  - Slot 16, robit/qwen3.5-9b-r7-research:q4km, remains a no-response readiness problem.
  - The local AIChess subset is mixed at clue level 6; some need chess-admission
    diagnostics before being treated as AIChess-capable.

The lesson is that AI evaluation needs admissions tests at each level,
not just one ladder where every lower rung is mandatory. A system may pass a
math task while failing a formatting test. A chess-capable system may express a
move in prose.

So AIH is separating:

- admission floors
- diagnostic ladders
- protocol compliance
- semantic correctness
- domain-specific readiness
- reliability across repeated attempts

That is the path toward agent evaluation that is more reproducible than
marketing terms and more useful than pass/fail theater.

One more important distinction emerged: a single failed qualification run should
not be confused with incapability. If an agent sometimes passes and sometimes
fails, that is reliability evidence. If it never passes after repeated fair
attempts, that is a different claim. AIH needs to record both.
