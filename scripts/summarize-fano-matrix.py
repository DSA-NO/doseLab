#!/usr/bin/env python3
import argparse
import json
import math
import re
import sys
from pathlib import Path

import numpy as np
import uproot

CHAMBERS = (
    "farmer",
    "roos",
    "farmer-walled",
    "roos-walled",
)

MODELS = (
    "option4",
    "livermore",
    "penelope",
)

FILE_RE = re.compile(r"^doseLab-run-fano-v1-(?P<chamber>.+)-(?P<model>option4|livermore|penelope)\\.root$")


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

    entries = int(len(dose))
    dose_mean = float(np.mean(dose))
    dose_std = float(np.std(dose))
    dose_sem = float(dose_std / math.sqrt(entries))
    dose_rel_sem = float(dose_sem / max(abs(dose_mean), 1e-30))

    return {
        "entries": entries,
        "dose": {
            "mean": dose_mean,
            "std": dose_std,
            "sem": dose_sem,
            "rel_sem": dose_rel_sem,
        },
        "edep": {
            "mean": float(np.mean(edep)),
            "std": float(np.std(edep)),
        },
        "trackl": {
            "mean": float(np.mean(trackl)),
            "std": float(np.std(trackl)),
        },
    }


def _expected_cases():
    cases = []
    for chamber in CHAMBERS:
        for model in MODELS:
            tag = f"run-fano-v1-{chamber}-{model}"
            cases.append((chamber, model, tag))
    return cases


def _load_cases(input_dir: Path):
    out = {}
    missing = []

    for chamber, model, tag in _expected_cases():
        path = input_dir / f"doseLab-{tag}.root"
        if not path.exists():
            missing.append(str(path))
            continue
        out[(chamber, model)] = {
            "tag": tag,
            "path": str(path),
            "metrics": _summarize_file(path),
        }

    return out, missing


def _compute_summary(cases, chamber_spread_tol, model_spread_tol, z_tol):
    ref_key = ("farmer", "option4")
    if ref_key not in cases:
        raise RuntimeError("Reference case farmer/option4 is missing")

    ref = cases[ref_key]
    ref_mean = ref["metrics"]["dose"]["mean"]
    ref_sem = ref["metrics"]["dose"]["sem"]

    case_rows = []
    for chamber, model, _ in _expected_cases():
        item = cases[(chamber, model)]
        mean = item["metrics"]["dose"]["mean"]
        sem = item["metrics"]["dose"]["sem"]
        ratio = mean / max(ref_mean, 1e-30)
        z = (mean - ref_mean) / max(math.sqrt(sem * sem + ref_sem * ref_sem), 1e-30)
        case_rows.append(
            {
                "chamber": chamber,
                "model": model,
                "tag": item["tag"],
                "entries": item["metrics"]["entries"],
                "dose_mean": mean,
                "dose_sem": sem,
                "dose_rel_sem": item["metrics"]["dose"]["rel_sem"],
                "ratio_to_ref": ratio,
                "z_to_ref": z,
            }
        )

    chamber_spreads = []
    for model in MODELS:
        vals = [cases[(chamber, model)]["metrics"]["dose"]["mean"] for chamber in CHAMBERS]
        spread = (max(vals) - min(vals)) / max(np.mean(vals), 1e-30)
        chamber_spreads.append(
            {
                "model": model,
                "spread_fraction": float(spread),
                "spread_percent": float(100.0 * spread),
            }
        )

    model_spreads = []
    for chamber in CHAMBERS:
        vals = [cases[(chamber, model)]["metrics"]["dose"]["mean"] for model in MODELS]
        spread = (max(vals) - min(vals)) / max(np.mean(vals), 1e-30)
        model_spreads.append(
            {
                "chamber": chamber,
                "spread_fraction": float(spread),
                "spread_percent": float(100.0 * spread),
            }
        )

    worst_chamber_spread = max(chamber_spreads, key=lambda x: x["spread_fraction"])
    worst_model_spread = max(model_spreads, key=lambda x: x["spread_fraction"])
    worst_abs_z = max(case_rows, key=lambda x: abs(x["z_to_ref"]))

    checks = {
        "chamber_spread": {
            "threshold_fraction": chamber_spread_tol,
            "worst": worst_chamber_spread,
            "pass": worst_chamber_spread["spread_fraction"] <= chamber_spread_tol,
        },
        "model_spread": {
            "threshold_fraction": model_spread_tol,
            "worst": worst_model_spread,
            "pass": worst_model_spread["spread_fraction"] <= model_spread_tol,
        },
        "z_score": {
            "threshold_abs": z_tol,
            "worst": {
                "chamber": worst_abs_z["chamber"],
                "model": worst_abs_z["model"],
                "z_to_ref": worst_abs_z["z_to_ref"],
            },
            "pass": abs(worst_abs_z["z_to_ref"]) <= z_tol,
        },
    }

    passed = all(check["pass"] for check in checks.values())

    return {
        "reference": {
            "chamber": "farmer",
            "model": "option4",
            "tag": ref["tag"],
            "dose_mean": ref_mean,
            "dose_sem": ref_sem,
        },
        "cases": case_rows,
        "spreads": {
            "by_model_across_chambers": chamber_spreads,
            "by_chamber_across_models": model_spreads,
        },
        "checks": checks,
        "pass": passed,
    }


def _format_text(summary, missing):
    lines = []
    lines.append("doseLab Fano matrix summary")
    lines.append("")

    lines.append(
        "Reference: "
        f"{summary['reference']['chamber']}/{summary['reference']['model']} "
        f"(tag={summary['reference']['tag']})"
    )
    lines.append(
        f"Reference dose mean={summary['reference']['dose_mean']:.6e}, "
        f"sem={summary['reference']['dose_sem']:.6e}"
    )
    lines.append("")

    lines.append("Cases:")
    lines.append("  chamber        model      ratio_to_ref   rel_sem(%)   z_to_ref")
    for row in summary["cases"]:
        lines.append(
            f"  {row['chamber']:<13} {row['model']:<10} {row['ratio_to_ref']:>12.6f}"
            f" {100.0 * row['dose_rel_sem']:>11.4f} {row['z_to_ref']:>10.4f}"
        )

    lines.append("")
    lines.append("Checks:")
    lines.append(
        "  chamber spread (across chambers per model): "
        f"{'PASS' if summary['checks']['chamber_spread']['pass'] else 'FAIL'} "
        f"(worst={summary['checks']['chamber_spread']['worst']['spread_percent']:.3f}%, "
        f"tol={100.0 * summary['checks']['chamber_spread']['threshold_fraction']:.3f}%)"
    )
    lines.append(
        "  model spread (across models per chamber): "
        f"{'PASS' if summary['checks']['model_spread']['pass'] else 'FAIL'} "
        f"(worst={summary['checks']['model_spread']['worst']['spread_percent']:.3f}%, "
        f"tol={100.0 * summary['checks']['model_spread']['threshold_fraction']:.3f}%)"
    )
    lines.append(
        "  z-score vs reference: "
        f"{'PASS' if summary['checks']['z_score']['pass'] else 'FAIL'} "
        f"(worst={summary['checks']['z_score']['worst']['z_to_ref']:.3f}, "
        f"tol={summary['checks']['z_score']['threshold_abs']:.3f})"
    )

    if missing:
        lines.append("")
        lines.append("Missing files:")
        for path in missing:
            lines.append(f"  {path}")

    lines.append("")
    lines.append(f"Overall: {'PASS' if summary['pass'] and not missing else 'FAIL'}")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description="Summarize doseLab Fano chamber/EM matrix outputs")
    parser.add_argument("--input-dir", default="analysis/fano/latest")
    parser.add_argument("--output-json", default="analysis/fano/latest/summary.json")
    parser.add_argument("--output-text", default="analysis/fano/latest/summary.txt")
    parser.add_argument("--chamber-spread-tol", type=float, default=0.01)
    parser.add_argument("--model-spread-tol", type=float, default=0.015)
    parser.add_argument("--z-tol", type=float, default=3.0)
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    output_json = Path(args.output_json)
    output_text = Path(args.output_text)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_text.parent.mkdir(parents=True, exist_ok=True)

    cases, missing = _load_cases(input_dir)

    expected_count = len(CHAMBERS) * len(MODELS)
    if len(cases) != expected_count:
        if args.strict:
            print("Missing required Fano outputs:", file=sys.stderr)
            for path in missing:
                print(f"- {path}", file=sys.stderr)
            return 2

    if not cases:
        print("No Fano matrix outputs found", file=sys.stderr)
        return 2

    summary = _compute_summary(
        cases,
        chamber_spread_tol=args.chamber_spread_tol,
        model_spread_tol=args.model_spread_tol,
        z_tol=args.z_tol,
    )

    report = {
        "input_dir": str(input_dir),
        "expected_cases": expected_count,
        "found_cases": len(cases),
        "missing_files": missing,
        "summary": summary,
        "overall_pass": bool(summary["pass"] and not missing),
    }

    output_json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    text = _format_text(summary, missing)
    output_text.write_text(text, encoding="utf-8")

    print(text, end="")
    print(f"Wrote JSON summary: {output_json}")
    print(f"Wrote text summary: {output_text}")

    if args.strict and (missing or not summary["pass"]):
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())