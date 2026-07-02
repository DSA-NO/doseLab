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


def _compute_summary(cases, chamber_spread_tol, model_spread_tol, z_tol, precision_threshold=0.03):
    case_rows = []
    for chamber, model, _ in _expected_cases():
        item = cases[(chamber, model)]
        mean = item["metrics"]["dose"]["mean"]
        sem = item["metrics"]["dose"]["sem"]
        case_rows.append(
            {
                "chamber": chamber,
                "model": model,
                "tag": item["tag"],
                "entries": item["metrics"]["entries"],
                "dose_mean": mean,
                "dose_sem": sem,
                "dose_rel_sem": item["metrics"]["dose"]["rel_sem"],
            }
        )

    max_rel_sem = max(row["dose_rel_sem"] for row in case_rows)
    precision_ok = max_rel_sem <= precision_threshold
    precision_status = "PASS" if precision_ok else "INCONCLUSIVE"

    physics_checks_per_chamber = []
    for chamber in CHAMBERS:
        opt4_row = next((r for r in case_rows if r["chamber"] == chamber and r["model"] == "option4"), None)
        if not opt4_row:
            continue

        opt4_mean = opt4_row["dose_mean"]
        opt4_sem = opt4_row["dose_sem"]

        for model in ("livermore", "penelope"):
            model_row = next((r for r in case_rows if r["chamber"] == chamber and r["model"] == model), None)
            if not model_row:
                continue

            model_mean = model_row["dose_mean"]
            model_sem = model_row["dose_sem"]
            ratio = model_mean / max(opt4_mean, 1e-30)
            z = (model_mean - opt4_mean) / max(math.sqrt(model_sem * model_sem + opt4_sem * opt4_sem), 1e-30)

            physics_checks_per_chamber.append(
                {
                    "chamber": chamber,
                    "model": model,
                    "vs_reference": "option4",
                    "ratio": float(ratio),
                    "z": float(z),
                    "pass": abs(z) <= z_tol,
                }
            )

    physics_pass = all(check["pass"] for check in physics_checks_per_chamber)
    worst_physics_z = max(physics_checks_per_chamber, key=lambda x: abs(x["z"]), default={})

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

    checks = {
        "precision": {
            "max_rel_sem": float(max_rel_sem),
            "threshold": precision_threshold,
            "status": precision_status,
            "pass": precision_ok,
        },
        "physics_agreement": {
            "threshold_z": z_tol,
            "worst": worst_physics_z if worst_physics_z else {},
            "pass": physics_pass,
        },
        "diagnostics": {
            "chamber_spread": {
                "threshold_fraction": chamber_spread_tol,
                "worst": worst_chamber_spread,
            },
            "model_spread": {
                "threshold_fraction": model_spread_tol,
                "worst": worst_model_spread,
            },
        },
    }

    primary_pass = precision_ok and physics_pass

    status = "PASS" if primary_pass else ("INCONCLUSIVE" if not precision_ok else "FAIL")

    return {
        "max_rel_sem": float(max_rel_sem),
        "cases": case_rows,
        "physics_checks_per_chamber": physics_checks_per_chamber,
        "spreads": {
            "by_model_across_chambers": chamber_spreads,
            "by_chamber_across_models": model_spreads,
        },
        "checks": checks,
        "pass": primary_pass,
        "status": status,
    }


def _format_text(summary, missing):
    lines = []
    lines.append("doseLab Fano matrix summary")
    lines.append(f"Status: {summary['status']}")
    lines.append("")

    lines.append("Precision Check:")
    lines.append(
        f"  max relative uncertainty: {100.0 * summary['max_rel_sem']:.2f}% "
        f"(threshold: {100.0 * summary['checks']['precision']['threshold']:.2f}%)"
    )
    lines.append(f"  {summary['checks']['precision']['status']}")
    lines.append("")

    if not summary["checks"]["precision"]["pass"]:
        lines.append("NOTE: Statistical uncertainty is too large to assess physics agreement reliably.")
        lines.append("      Consider running with more primaries for meaningful model comparisons.")
        lines.append("")
    else:
        lines.append("Physics Agreement (vs option4 per chamber):")
        lines.append("  chamber        livermore ratio  z-score  penelope ratio  z-score")
        for chamber in CHAMBERS:
            liv_check = next((c for c in summary["physics_checks_per_chamber"]
                             if c["chamber"] == chamber and c["model"] == "livermore"), None)
            pen_check = next((c for c in summary["physics_checks_per_chamber"]
                             if c["chamber"] == chamber and c["model"] == "penelope"), None)
            if liv_check and pen_check:
                lines.append(
                    f"  {chamber:<13} {liv_check['ratio']:>12.6f} {liv_check['z']:>9.4f}  "
                    f"{pen_check['ratio']:>12.6f} {pen_check['z']:>9.4f}"
                )

        lines.append("")
        lines.append("Physics Agreement Check:")
        if summary["checks"]["physics_agreement"]["worst"]:
            worst = summary["checks"]["physics_agreement"]["worst"]
            lines.append(
                f"  Worst: {worst.get('chamber', 'N/A')}/{worst.get('model', 'N/A')} "
                f"z={worst.get('z', 0):.4f} (tol={summary['checks']['physics_agreement']['threshold_z']:.2f})"
            )
        lines.append(
            f"  {'PASS' if summary['checks']['physics_agreement']['pass'] else 'FAIL'}"
        )
        lines.append("")

    lines.append("Geometry Diagnostics (informational):")
    lines.append("  Chamber spread across models per chamber:")
    for item in summary["spreads"]["by_chamber_across_models"]:
        lines.append(
            f"    {item['chamber']:<13}: {item['spread_percent']:>7.3f}% "
            f"(tol={100.0 * summary['checks']['diagnostics']['model_spread']['threshold_fraction']:.3f}%)"
        )

    lines.append("  Model spread across chambers per model:")
    for item in summary["spreads"]["by_model_across_chambers"]:
        lines.append(
            f"    {item['model']:<13}: {item['spread_percent']:>7.3f}% "
            f"(tol={100.0 * summary['checks']['diagnostics']['chamber_spread']['threshold_fraction']:.3f}%)"
        )

    if missing:
        lines.append("")
        lines.append("Missing files:")
        for path in missing:
            lines.append(f"  {path}")

    lines.append("")
    lines.append(f"Overall: {'PASS' if summary['pass'] and not missing else summary['status']}")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description="Summarize doseLab Fano chamber/EM matrix outputs")
    parser.add_argument("--input-dir", default="analysis/fano/latest")
    parser.add_argument("--output-json", default="analysis/fano/latest/summary.json")
    parser.add_argument("--output-text", default="analysis/fano/latest/summary.txt")
    parser.add_argument("--chamber-spread-tol", type=float, default=0.01)
    parser.add_argument("--model-spread-tol", type=float, default=0.015)
    parser.add_argument("--z-tol", type=float, default=3.0)
    parser.add_argument("--precision-threshold", type=float, default=0.03, help="Max rel uncertainty for PASS (default 3%%)")
    parser.add_argument(
        "--ci-physics-gate",
        action="store_true",
        help="In strict mode, fail only when precision passes but physics agreement fails",
    )
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
        precision_threshold=args.precision_threshold,
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

    if args.strict:
        if missing:
            return 1

        if args.ci_physics_gate:
            precision_pass = summary["checks"]["precision"]["pass"]
            physics_pass = summary["checks"]["physics_agreement"]["pass"]
            if precision_pass and not physics_pass:
                return 1
        elif not summary["pass"]:
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())