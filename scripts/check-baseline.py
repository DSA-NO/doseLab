#!/usr/bin/env python3
import argparse
import json
import math
import os
import sys
from pathlib import Path

import numpy as np
import uproot

TAGS = [
    "run-ref-10x10-d5cm-6mv-farmer",
    "run-ref-10x10-d5cm-6mv-roos",
    "run-ref-10x10-d5cm-6mv-farmer-walled",
    "run-ref-10x10-d5cm-6mv-roos-walled",
]


def _find_cavity_tree(root_file: uproot.ReadOnlyDirectory):
    if "cavity" in root_file:
        return root_file["cavity"]

    for _, obj in root_file.items(recursive=True):
        if isinstance(obj, uproot.behaviors.TTree.TTree):
            names = set(obj.keys())
            if {"Dose", "Edep", "TrackL"}.issubset(names):
                return obj

    raise RuntimeError("Could not find cavity TTree with Dose/Edep/TrackL branches")


def _summarize_file(path: Path):
    with uproot.open(path) as f:
        tree = _find_cavity_tree(f)
        dose = tree["Dose"].array(library="np")
        edep = tree["Edep"].array(library="np")
        trackl = tree["TrackL"].array(library="np")

    if len(dose) == 0:
        raise RuntimeError(f"No cavity entries found in {path}")

    def stats(values):
        return {
            "mean": float(np.mean(values)),
            "std": float(np.std(values)),
            "min": float(np.min(values)),
            "max": float(np.max(values)),
        }

    return {
        "entries": int(len(dose)),
        "dose": stats(dose),
        "edep": stats(edep),
        "trackl": stats(trackl),
    }


def _load_metrics(build_dir: Path):
    out = {"scenarios": {}}
    for tag in TAGS:
        root_path = build_dir / f"doseLab-{tag}.root"
        if not root_path.exists():
            raise FileNotFoundError(f"Missing expected ROOT output: {root_path}")
        out["scenarios"][tag] = _summarize_file(root_path)
    return out


def _relative_diff(current: float, baseline: float):
    denom = max(abs(baseline), 1e-30)
    return abs(current - baseline) / denom


def _compare(current, baseline, rel_tol: float):
    failures = []
    for tag in TAGS:
        if tag not in baseline.get("scenarios", {}):
            failures.append(f"baseline missing scenario: {tag}")
            continue

        cur = current["scenarios"][tag]
        base = baseline["scenarios"][tag]

        if cur["entries"] != base["entries"]:
            failures.append(
                f"{tag}: entries changed (current={cur['entries']}, baseline={base['entries']})"
            )

        for metric in ("dose", "edep", "trackl"):
            for field in ("mean", "std"):
                cval = cur[metric][field]
                bval = base[metric][field]
                rd = _relative_diff(cval, bval)
                if rd > rel_tol:
                    failures.append(
                        f"{tag}: {metric}.{field} rel diff {rd:.4f} exceeds tol {rel_tol:.4f}"
                    )

    return failures


def main():
    parser = argparse.ArgumentParser(description="Compare doseLab production references against baseline")
    parser.add_argument("--build-dir", default="build-production")
    parser.add_argument("--baseline", default="analysis/baseline/reference_metrics.json")
    parser.add_argument("--report", default="analysis/production/latest/metrics.json")
    parser.add_argument("--relative-tolerance", type=float, default=0.10)
    parser.add_argument("--write-baseline", action="store_true")
    args = parser.parse_args()

    build_dir = Path(args.build_dir)
    baseline_path = Path(args.baseline)
    report_path = Path(args.report)
    report_path.parent.mkdir(parents=True, exist_ok=True)

    current = _load_metrics(build_dir)
    report_path.write_text(json.dumps(current, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote current metrics report: {report_path}")

    if args.write_baseline:
        baseline_path.parent.mkdir(parents=True, exist_ok=True)
        baseline_path.write_text(json.dumps(current, indent=2) + "\n", encoding="utf-8")
        print(f"Wrote baseline: {baseline_path}")
        return 0

    if not baseline_path.exists():
        print(f"Baseline file not found: {baseline_path}", file=sys.stderr)
        print("Run with --write-baseline to initialize it.", file=sys.stderr)
        return 2

    baseline = json.loads(baseline_path.read_text(encoding="utf-8"))
    failures = _compare(current, baseline, args.relative_tolerance)

    if failures:
        print("Baseline check failed:", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1

    print("Baseline check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
