#!/usr/bin/env python3
"""Fail when the number of MISSED lines or branches rises.

usage: coverage_gate.py <lcov.info> <baseline.json> [--update] [--strict]

The gate compares missed counts, not percentages. A percentage can stay
the same, or even rise, when new code is added without tests. A missed
count rises in that case.

Rules:
  * The total of missed lines must not rise.
  * The total of missed branches must not rise.
  * The missed lines and branches of each file under src/ must not rise.
    A new file with a missed line or branch fails.
  * Exception branches (block id starting with "e") are not counted.
  * src/note-c/, test/ and build/_deps/ are not counted.
  * A baseline that is looser than the measurement gives a notice.
    With --strict it fails. Run with --update to tighten it.

--update writes the measured counts to the baseline file. It refuses to
write counts that are worse than the baseline. To accept worse counts,
edit the baseline file by hand and explain why in the PR.

Exit status: 0 pass, 1 a missed count rose (or is stale with --strict),
2 bad input.
"""
import argparse
import json
import sys


def tracked_path(path):
    """Return the src/-relative name of a measured file, or None."""
    norm = path.replace("\\", "/")
    for skip in ("/_deps/", "/test/src/", "/src/note-c/"):
        if skip in norm:
            return None
    cut = norm.rfind("/src/")
    if cut >= 0:
        return norm[cut + 1:]
    return norm if norm.startswith("src/") else None


def missed_counts(tracefile):
    """Return {file: {"lines_missed": n, "branches_missed": n}}."""
    lines, branches, cur = {}, {}, None
    with open(tracefile) as fh:
        for raw in fh:
            raw = raw.strip()
            if raw.startswith("SF:"):
                cur = tracked_path(raw[3:])
                if cur is not None:
                    lines.setdefault(cur, {})
                    branches.setdefault(cur, {})
            elif cur is None:
                continue
            elif raw.startswith("DA:"):
                number, hits = raw[3:].split(",")[:2]
                lines[cur][number] = lines[cur].get(number, 0) + int(hits)
            elif raw.startswith("BRDA:"):
                number, block, branch, taken = raw[5:].split(",")
                if block.startswith("e"):
                    continue
                key = (number, block, branch)
                branches[cur][key] = branches[cur].get(key, 0) + (0 if taken == "-" else int(taken))
    result = {}
    for name in lines:
        result[name] = {
            "lines_missed": sum(1 for v in lines[name].values() if v == 0),
            "branches_missed": sum(1 for v in branches[name].values() if v == 0),
        }
    return result


def totals(per_file):
    return {key: sum(f[key] for f in per_file.values()) for key in ("lines_missed", "branches_missed")}


def compare(measured, baseline):
    """Return (errors, notices) as lists of text lines."""
    errors, notices = [], []
    now, then = totals(measured), baseline["totals"]
    for key, label in (("lines_missed", "lines"), ("branches_missed", "branches")):
        if now[key] > then[key]:
            errors.append("Missed %s rose from %d to %d." % (label, then[key], now[key]))
        elif now[key] < then[key]:
            notices.append("Missed %s fell from %d to %d. Tighten the baseline." % (label, then[key], now[key]))
    for name in sorted(set(measured) | set(baseline["files"])):
        have = measured.get(name, {"lines_missed": 0, "branches_missed": 0})
        base = baseline["files"].get(name, {"lines_missed": 0, "branches_missed": 0})
        for key, label in (("lines_missed", "lines"), ("branches_missed", "branches")):
            if have[key] > base[key]:
                errors.append("  %s: missed %s rose from %d to %d." % (name, label, base[key], have[key]))
            elif have[key] < base[key]:
                notices.append("  %s: missed %s fell from %d to %d." % (name, label, base[key], have[key]))
    return errors, notices


def baseline_from(per_file):
    kept = {k: v for k, v in sorted(per_file.items()) if v["lines_missed"] or v["branches_missed"]}
    return {"scope": "src/ except src/note-c/; exception branches not counted",
            "totals": totals(per_file), "files": kept}


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("tracefile")
    ap.add_argument("baseline")
    ap.add_argument("--update", action="store_true", help="write the measured counts to the baseline")
    ap.add_argument("--strict", action="store_true", help="fail when the baseline is looser than the measurement")
    args = ap.parse_args(argv)
    try:
        measured = missed_counts(args.tracefile)
        with open(args.baseline) as fh:
            baseline = json.load(fh)
    except (OSError, ValueError, KeyError) as err:
        print("ERROR: cannot read input: %s" % err)
        return 2
    if not measured:
        print("ERROR: the tracefile has no file under src/. Stop.")
        return 2
    errors, notices = compare(measured, baseline)
    now = totals(measured)
    print("Missed lines: %d (baseline %d). Missed branches: %d (baseline %d)." % (
        now["lines_missed"], baseline["totals"]["lines_missed"],
        now["branches_missed"], baseline["totals"]["branches_missed"]))
    if args.update:
        if errors:
            print("ERROR: refusing to update. These counts are worse than the baseline:")
            print("\n".join(errors))
            return 1
        with open(args.baseline, "w") as fh:
            json.dump(baseline_from(measured), fh, indent=2)
            fh.write("\n")
        print("Baseline updated.")
        return 0
    if notices:
        print("Notice: the baseline is looser than the measurement.")
        print("\n".join(notices))
        print("Run: python3 scripts/coverage_gate.py <lcov.info> <baseline.json> --update")
    if errors:
        print("FAIL: a missed count rose. Add tests for the new or changed code.")
        print("\n".join(errors))
        return 1
    if notices and args.strict:
        print("FAIL: the baseline is stale (--strict).")
        return 1
    print("PASS: no missed count rose.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
