#!/usr/bin/env python3
"""AIH AIchess v1 one-move probe.

This prototype tests the harness shape, not chess strength. The referee owns a
single starting-position state and accepts only one legal UCI move.
"""

from __future__ import annotations

import argparse
import json
import re
import time
from pathlib import Path


TEST_ID = "aih_chess_one_move_probe_v1_20260713"
ROOT = Path(__file__).resolve().parents[4]
OUT_DIR = Path(__file__).resolve().parent / "runs"

START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
LEGAL_START_MOVES = {
    "a2a3", "a2a4", "b2b3", "b2b4", "c2c3", "c2c4", "d2d3", "d2d4",
    "e2e3", "e2e4", "f2f3", "f2f4", "g2g3", "g2g4", "h2h3", "h2h4",
    "b1a3", "b1c3", "g1f3", "g1h3",
}


def fixture_agent(prompt: str) -> str:
    _ = prompt
    return "e2e4"


def parse_uci(raw_response: str) -> str | None:
    matches = re.findall(r"\b[a-h][1-8][a-h][1-8][qrbn]?\b", raw_response.lower())
    return matches[0] if matches else None


def run(agent_response: str | None) -> dict[str, object]:
    prompt = (
        "Return exactly one legal UCI chess move for White from the starting "
        f"position. FEN: {START_FEN}. Legal moves: {sorted(LEGAL_START_MOVES)}"
    )
    start = time.time()
    raw_response = agent_response if agent_response is not None else fixture_agent(prompt)
    elapsed_ms = round((time.time() - start) * 1000, 3)
    parsed_move = parse_uci(raw_response)
    legal = parsed_move in LEGAL_START_MOVES if parsed_move else False
    return {
        "test_id": TEST_ID,
        "test_class": "class_1",
        "test_class_name": "rule_bound_state_game_action_hallucination",
        "test_family": "AIchess",
        "prototype_level": "v1",
        "agent_id": "fixture_agent" if agent_response is None else "provided_response",
        "prompt": prompt,
        "raw_response": raw_response,
        "parsed_response": {"uci_move": parsed_move},
        "score": 1 if legal else 0,
        "max_score": 1,
        "timing_ms": elapsed_ms,
        "errors": [] if legal else ["illegal_or_unparseable_move"],
        "referee": {
            "owns_state": True,
            "fen_before": START_FEN,
            "legal_move_count": len(LEGAL_START_MOVES),
        },
        "notes": [
            "Prototype validates one legal move from a fixed position.",
            "Full v1 should replace the fixed legal list with python-chess or another referee.",
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--agent-response", help="Raw response to score instead of fixture response.")
    args = parser.parse_args()

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    result = run(args.agent_response)
    stamp = time.strftime("%Y%m%d_%H%M%S")
    out = OUT_DIR / f"{TEST_ID}_{stamp}.json"
    out.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    print(out)


if __name__ == "__main__":
    main()
