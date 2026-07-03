#!/usr/bin/env python3
import argparse
import json
import math
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

BEAMS = (
    "co60",
    "6mv",
    "10mv",
)

MEDIA = (
    "cavity",
    "water",
)


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

    if len(dose) == 0:
        raise RuntimeError(f"No cavity entries found in {path}")

    entries = int(len(dose))
    mean = float(np.mean(dose))
    std = float(np.std(dose))
    sem = float(std / math.sqrt(entries))
    rel_sem = float(sem / max(abs(mean), 1e-30))

    return {
        "entries": entries,
        "dose": {
            "mean": mean,
            "std": std,
            "sem": sem,
            "rel_sem": rel_sem,
        },
    }


def _expected_cases():
    out = []
    for beam in BEAMS:
        for chamber in CHAMBERS:
            for model in MODELS:
                for medium in MEDIA:
                    tag = f"run-calib-v1-{beam}-{chamber}-{model}-{medium}"
                    out.append((beam, chamber, model, medium, tag))
    return out


def _load_cases(input_dir: Path):
    cases = {}
    missing = []

    for beam, chamber, model, medium, tag in _expected_cases():
        path = input_dir / f"doseLab-{tag}.root"
        if not path.exists():
            missing.append(str(path))
            continue

        cases[(beam, chamber, model, medium)] = {
            "tag": tag,
            "path": str(path),
            "metrics": _summarize_file(path),
        }

    return cases, missing


def _ratio_with_uncertainty(num_mean, num_sem, den_mean, den_sem):
    ratio = num_mean / max(den_mean, 1e-30)

    num_rel = num_sem / max(abs(num_mean), 1e-30)
    den_rel = den_sem / max(abs(den_mean), 1e-30)
    ratio_rel_sem = math.sqrt(num_rel * num_rel + den_rel * den_rel)
    ratio_sem = abs(ratio) * ratio_rel_sem

    return float(ratio), float(ratio_sem), float(ratio_rel_sem)


def _compute_summary(cases, precision_threshold):
    case_rows = []
    ratio_rows = []
    kq_rows = []

    complete_triplets = []
    for beam in BEAMS:
        for chamber in CHAMBERS:
            for model in MODELS:
                if (
                    (beam, chamber, model, "cavity") in cases
                    and (beam, chamber, model, "water") in cases
                ):
                    complete_triplets.append((beam, chamber, model))

    for beam, chamber, model in complete_triplets:
        cav = cases[(beam, chamber, model, "cavity")]
        wat = cases[(beam, chamber, model, "water")]

        cav_mean = cav["metrics"]["dose"]["mean"]
        cav_sem = cav["metrics"]["dose"]["sem"]
        wat_mean = wat["metrics"]["dose"]["mean"]
        wat_sem = wat["metrics"]["dose"]["sem"]

        case_rows.append(
            {
                "beam": beam,
                "chamber": chamber,
                "model": model,
                "medium": "cavity",
                "tag": cav["tag"],
                "entries": cav["metrics"]["entries"],
                "dose_mean": cav_mean,
                "dose_sem": cav_sem,
                "dose_rel_sem": cav["metrics"]["dose"]["rel_sem"],
            }
        )
        case_rows.append(
            {
                "beam": beam,
                "chamber": chamber,
                "model": model,
                "medium": "water",
                "tag": wat["tag"],
                "entries": wat["metrics"]["entries"],
                "dose_mean": wat_mean,
                "dose_sem": wat_sem,
                "dose_rel_sem": wat["metrics"]["dose"]["rel_sem"],
            }
        )

        r_q, r_q_sem, r_q_rel_sem = _ratio_with_uncertainty(wat_mean, wat_sem, cav_mean, cav_sem)
        ratio_rows.append(
            {
                "beam": beam,
                "chamber": chamber,
                "model": model,
                "ratio_dw_over_dc": r_q,
                "ratio_sem": r_q_sem,
                "ratio_rel_sem": r_q_rel_sem,
            }
        )

    ratio_index = {(r["beam"], r["chamber"], r["model"]): r for r in ratio_rows}

    for beam, chamber, model in complete_triplets:
        r_case = ratio_index.get((beam, chamber, model))
        r_ref = ratio_index.get(("co60", chamber, model))
        if not r_case or not r_ref:
            continue

        k_q, k_q_sem, k_q_rel_sem = _ratio_with_uncertainty(
            r_case["ratio_dw_over_dc"],
            r_case["ratio_sem"],
            r_ref["ratio_dw_over_dc"],
            r_ref["ratio_sem"],
        )

        kq_rows.append(
            {
                "beam": beam,
                "chamber": chamber,
                "model": model,
                "kq_vs_co60": k_q,
                "kq_sem": k_q_sem,
                "kq_rel_sem": k_q_rel_sem,
            }
        )

    max_rel_sem = max((row["dose_rel_sem"] for row in case_rows), default=float("inf"))
    max_ratio_rel_sem = max((row["ratio_rel_sem"] for row in ratio_rows), default=float("inf"))
    max_kq_rel_sem = max((row["kq_rel_sem"] for row in kq_rows), default=float("inf"))

    precision_ok = (
        max_rel_sem <= precision_threshold
        and max_ratio_rel_sem <= precision_threshold
        and max_kq_rel_sem <= precision_threshold
        and len(kq_rows) > 0
    )

    total_expected_triplets = len(BEAMS) * len(CHAMBERS) * len(MODELS)
    complete_triplet_count = len(complete_triplets)
    skipped_triplet_count = total_expected_triplets - complete_triplet_count

    return {
        "cases": case_rows,
        "ratios": ratio_rows,
        "kq": kq_rows,
        "coverage": {
            "expected_triplets": total_expected_triplets,
            "complete_triplets": complete_triplet_count,
            "skipped_triplets": skipped_triplet_count,
        },
        "checks": {
            "precision_threshold": precision_threshold,
            "max_case_rel_sem": float(max_rel_sem),
            "max_ratio_rel_sem": float(max_ratio_rel_sem),
            "max_kq_rel_sem": float(max_kq_rel_sem),
            "pass": bool(precision_ok),
        },
        "status": "PASS" if precision_ok else "INCONCLUSIVE",
        "pass": bool(precision_ok),
    }


def _format_text(summary, missing):
    lines = []
    lines.append("doseLab calibration matrix summary")
    lines.append(f"Status: {summary['status']}")
    lines.append("")

    checks = summary["checks"]
    coverage = summary["coverage"]
    lines.append("Coverage:")
    lines.append(
        f"  complete triplets: {coverage['complete_triplets']}/{coverage['expected_triplets']}"
    )
    lines.append(f"  skipped triplets:  {coverage['skipped_triplets']}")
    lines.append("")

    lines.append("Precision check:")
    lines.append(
        f"  max case rel sem:  {100.0 * checks['max_case_rel_sem']:.2f}%"
    )
    lines.append(
        f"  max R_Q rel sem:   {100.0 * checks['max_ratio_rel_sem']:.2f}%"
    )
    lines.append(
        f"  max k_Q rel sem:   {100.0 * checks['max_kq_rel_sem']:.2f}%"
    )
    lines.append(
        f"  threshold:         {100.0 * checks['precision_threshold']:.2f}%"
    )
    lines.append(f"  {'PASS' if checks['pass'] else 'INCONCLUSIVE'}")
    lines.append("")

    lines.append("k_Q = (D_w/D_c)_Q / (D_w/D_c)_Co60")
    lines.append("  beam   chamber         model      k_Q        rel sem")
    for row in summary["kq"]:
        lines.append(
            f"  {row['beam']:<6} {row['chamber']:<15} {row['model']:<10} "
            f"{row['kq_vs_co60']:>10.6f}  {100.0 * row['kq_rel_sem']:>7.3f}%"
        )

    if missing:
        lines.append("")
        lines.append("Missing files:")
        for path in missing:
            lines.append(f"  {path}")

    lines.append("")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description="Summarize doseLab calibration matrix outputs")
    parser.add_argument("--input-dir", default="analysis/calibration/latest")
    parser.add_argument("--output-json", default="analysis/calibration/latest/summary.json")
    parser.add_argument("--output-text", default="analysis/calibration/latest/summary.txt")
    parser.add_argument(
        "--precision-threshold",
        type=float,
        default=0.03,
        help="Max rel uncertainty for PASS (default 3%%)",
    )
    parser.add_argument("--strict", action="store_true")
    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    output_json = Path(args.output_json)
    output_text = Path(args.output_text)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_text.parent.mkdir(parents=True, exist_ok=True)

    cases, missing = _load_cases(input_dir)

    if missing:
        message = "Missing required ROOT files:\n" + "\n".join(missing)
        if args.strict:
            raise SystemExit(message)
        print(message)

    summary = _compute_summary(cases, args.precision_threshold)

    output_json.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    output_text.write_text(_format_text(summary, missing), encoding="utf-8")

    print(f"Wrote calibration summary JSON: {output_json}")
    print(f"Wrote calibration summary text: {output_text}")

    if args.strict and (missing or not summary["pass"]):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
