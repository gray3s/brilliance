#!/usr/bin/env python3
"""Run all three canonical AIH v1 classes against one local Ollama stack."""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import time
import urllib.error
import urllib.request
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parent
OUT_DIR = ROOT / "runs"

PROBES = [
    {
        "test_class": "class_1",
        "test_class_name": "rule_bound_state_game_action_hallucination",
        "family": "AIchess",
        "path": ROOT / "AIchess" / "v1" / "run_aih_chess_probe_v1.py",
    },
    {
        "test_class": "class_2",
        "test_class_name": "provenance_workflow_history_hallucination",
        "family": "provenance_attribution",
        "path": ROOT / "AIhistory" / "v1" / "run_aihistory_provenance_probe_v1.py",
    },
    {
        "test_class": "class_3",
        "test_class_name": "source_bound_knowledge_education_ladder_hallucination",
        "family": "k-phd",
        "path": ROOT / "k-phd" / "v1" / "run_kphd_wikipedia_probe_v1.py",
    },
]


def load_module(path: Path) -> ModuleType:
    spec = importlib.util.spec_from_file_location(path.stem, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load probe module: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def ollama_generate(endpoint: str, model: str, prompt: str, timeout_s: int) -> tuple[str, float]:
    payload = {
        "model": model,
        "prompt": prompt,
        "stream": False,
        "options": {
            "temperature": 0,
            "num_predict": 256,
        },
    }
    data = json.dumps(payload).encode("utf-8")
    request = urllib.request.Request(
        endpoint.rstrip("/") + "/api/generate",
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    start = time.time()
    with urllib.request.urlopen(request, timeout=timeout_s) as response:
        body = json.loads(response.read().decode("utf-8"))
    elapsed_ms = round((time.time() - start) * 1000, 3)
    return str(body.get("response", "")), elapsed_ms


def stack_id_from_model(model: str) -> str:
    normalized = model.lower().replace(":", "").replace(".", "").replace("/", "_")
    normalized = re.sub(r"[^a-z0-9_]+", "", normalized)
    return normalized + "_ollama_host"


def run(model: str, endpoint: str, timeout_s: int) -> dict[str, object]:
    stack_id = stack_id_from_model(model)
    results: list[dict[str, object]] = []
    errors: list[str] = []
    for probe in PROBES:
        module = load_module(probe["path"])
        fixture_result = module.run(None)
        prompt = fixture_result["prompt"]
        try:
            raw_response, model_wall_ms = ollama_generate(endpoint, model, prompt, timeout_s)
            result = module.run(raw_response)
            result["agent_id"] = stack_id
            result["agent_stack"] = {
                "stack_id": stack_id,
                "model": model,
                "backend": "ollama",
                "endpoint": endpoint,
                "execution_context": "host",
                "temperature": 0,
                "num_predict": 256,
            }
            result["model_wall_ms"] = model_wall_ms
        except (urllib.error.URLError, TimeoutError, RuntimeError) as exc:
            errors.append(f"{probe['family']}: {exc}")
            result = {
                "test_class": probe["test_class"],
                "test_family": probe["family"],
                "agent_id": stack_id,
                "score": 0,
                "max_score": 1,
                "errors": [str(exc)],
            }
        results.append(result)
    total_score = sum(int(result.get("score", 0)) for result in results)
    total_max = sum(int(result.get("max_score", 0)) for result in results)
    return {
        "aggregate_id": "aih_v1_three_family_stack_probe_20260713",
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "stack_id": stack_id,
        "model": model,
        "backend": "ollama",
        "endpoint": endpoint,
        "test_classes": [
            "class_1_rule_bound_state",
            "class_2_provenance_workflow",
            "class_3_source_bound_knowledge",
        ],
        "score": total_score,
        "max_score": total_max,
        "pass_rate": round(total_score / total_max, 4) if total_max else 0,
        "results": results,
        "errors": errors,
        "notes": [
            "All three canonical AIH v1 classes are run against the same local stack.",
            "This aggregate tests one agent stack under normal local operating conditions.",
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", default="qwen2.5-coder:3b")
    parser.add_argument("--endpoint", default="http://127.0.0.1:11434")
    parser.add_argument("--timeout-s", type=int, default=120)
    args = parser.parse_args()

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    result = run(args.model, args.endpoint, args.timeout_s)
    stamp = time.strftime("%Y%m%d_%H%M%S")
    out = OUT_DIR / f"aih_v1_three_family_stack_{result['stack_id']}_{stamp}.json"
    out.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    print(out)


if __name__ == "__main__":
    main()
