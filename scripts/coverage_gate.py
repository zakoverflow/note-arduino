#!/usr/bin/env python3
"""Fail when the number of MISSED lines or branches rises.

usage: coverage_gate.py <lcov.info> <baseline.json> [--root DIR] [--src-tree DIR] [--update] [--strict]

The gate compares missed counts, not percentages. A percentage can stay
the same, or even rise, when new code is added without tests. A missed
count rises in that case.

Rules:
  * The total of missed lines must not rise.
  * The total of missed branches must not rise.
  * The missed lines and branches of each file under src/ must not rise.
    A new file with a missed line or branch fails.
  * Exception branches (block id starting with "e") are not counted.
  * Only files under <root>/src/ are counted. src/note-c/ is not counted.
    <root> is the directory that holds src/, as written in the tracefile.
    The default is the repository that holds this script. In CI the
    tracefile comes from the Docker image, so CI passes --root /note-arduino.
  * A baseline that is looser than the measurement gives a notice.
    With --strict it fails. Run with --update to tighten it.
  * Every source file under src/ must have a record in the tracefile. A source
    file is a .c or .cpp file under <src-tree>/src/, except src/note-c/. The
    gate reads the list from the checkout at run time, so the list cannot go
    stale. A file with no record fails (exit 1) and the gate names it. This
    holds without --strict too, because the measurement is then incomplete.
    The check also catches a file with 0 missed lines and 0 missed branches.
    Header files are not listed: a header that holds no code has no record.
  * A baseline file that has no record in the tracefile, and that is not a
    source file of the checkout, gives a notice. With --strict it fails.
    Run with --update if the file was deleted.

--update writes the measured counts to the baseline file. It refuses to
write counts that are worse than the baseline. To accept worse counts,
edit the baseline file by hand and explain why in the PR.

Exit status:
  0  The gate passes. No missed count rose and every source file has a record.
     A notice alone does not fail without --strict. --update wrote the baseline.
  1  A missed line count or a missed branch count rose, in total or in one
     file (each is compared alone). Or a source file has no record in the
     tracefile. Or --update refused to write a baseline (a count is worse, or a
     source file has no record). Or --strict found a notice (a count fell, or a
     baseline file has no record and is not in the checkout).
  2  Bad input: the tracefile or the baseline is missing, empty or malformed,
     the tracefile has no file under <root>/src/, or the checkout has no .c or
     .cpp file under <src-tree>/src/. A wrong option also exits 2 (argparse).
"""
import argparse
import json
import os
import sys


def normalize_root(root):
    """Return root with "/" separators and no trailing "/"."""
    return root.replace("\\", "/").rstrip("/")


def tracked_path(path, root):
    """Return the name of a measured file relative to root, or None.

    A file counts only when it is under <root>/src/ and not under
    <root>/src/note-c/. The name is "src/<file>". The function does not search
    the path for "/src/", so a checkout in a directory named "src" is safe.
    """
    norm = path.replace("\\", "/")
    prefix = normalize_root(root) + "/"
    if not norm.startswith(prefix):
        return None
    rel = norm[len(prefix):]
    if rel.startswith("src/") and not rel.startswith("src/note-c/"):
        return rel
    return None


def source_files(tree):
    """Return the sorted names "src/<file>" of the .c and .cpp files under <tree>/src/.

    src/note-c/ is not listed. The names use "/" as separator.
    """
    base = os.path.join(tree, "src")
    names = []
    for folder, dirs, files in os.walk(base):
        rel = os.path.relpath(folder, base).replace(os.sep, "/")
        if rel == "." and "note-c" in dirs:
            dirs.remove("note-c")
        for name in files:
            if name.endswith((".c", ".cpp")):
                names.append("src/" + name if rel == "." else "src/%s/%s" % (rel, name))
    return sorted(names)


def missed_counts(tracefile, root):
    """Return {file: {"lines_missed": n, "branches_missed": n}}."""
    lines, branches, cur = {}, {}, None
    with open(tracefile, encoding="utf-8") as fh:
        for raw in fh:
            raw = raw.strip()
            if raw.startswith("SF:"):
                cur = tracked_path(raw[3:], root)
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


class BaselineError(ValueError):
    """The baseline file does not have the expected shape."""


def _count(value, where):
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise BaselineError("%s must be a whole number of 0 or more" % where)
    return value


def load_baseline(path):
    """Read and check the baseline file. Raise BaselineError or OSError or ValueError."""
    with open(path, encoding="utf-8") as fh:
        data = json.load(fh)
    if not isinstance(data, dict):
        raise BaselineError("the top level must be an object")
    for key in ("totals", "files"):
        if not isinstance(data.get(key), dict):
            raise BaselineError('"%s" must be an object' % key)
    for key in ("lines_missed", "branches_missed"):
        _count(data["totals"].get(key), 'totals.%s' % key)
    for name, entry in data["files"].items():
        if not isinstance(entry, dict):
            raise BaselineError('files["%s"] must be an object' % name)
        for key in ("lines_missed", "branches_missed"):
            _count(entry.get(key), 'files["%s"].%s' % (name, key))
    return data


def totals(per_file):
    return {key: sum(f[key] for f in per_file.values()) for key in ("lines_missed", "branches_missed")}


def compare(measured, baseline):
    """Return (errors, notices, dropped). errors and notices are lists of text lines.

    dropped lists the baseline files that have no record in the tracefile.
    """
    errors, notices = [], []
    dropped = sorted(set(baseline["files"]) - set(measured))
    now, then = totals(measured), baseline["totals"]
    for key, label in (("lines_missed", "lines"), ("branches_missed", "branches")):
        if now[key] > then[key]:
            errors.append("Missed %s rose from %d to %d." % (label, then[key], now[key]))
        elif now[key] < then[key]:
            notices.append("Missed %s fell from %d to %d. Tighten the baseline." % (label, then[key], now[key]))
    for name in sorted(set(measured) | set(baseline["files"])):
        if name in dropped:
            continue
        have = measured.get(name, {"lines_missed": 0, "branches_missed": 0})
        base = baseline["files"].get(name, {"lines_missed": 0, "branches_missed": 0})
        for key, label in (("lines_missed", "lines"), ("branches_missed", "branches")):
            if have[key] > base[key]:
                errors.append("  %s: missed %s rose from %d to %d." % (name, label, base[key], have[key]))
            elif have[key] < base[key]:
                notices.append("  %s: missed %s fell from %d to %d." % (name, label, base[key], have[key]))
    return errors, notices, dropped


def baseline_from(per_file):
    kept = {k: v for k, v in sorted(per_file.items()) if v["lines_missed"] or v["branches_missed"]}
    return {"scope": "src/ except src/note-c/; exception branches not counted",
            "totals": totals(per_file), "files": kept}


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("tracefile")
    ap.add_argument("baseline")
    ap.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                    help="directory that holds src/, as written in the tracefile "
                         "(default: the repository of this script)")
    ap.add_argument("--src-tree", default=os.path.dirname(os.path.dirname(os.path.realpath(__file__))),
                    help="directory that holds src/ on this machine, to list the source files "
                         "(default: the repository of this script)")
    ap.add_argument("--update", action="store_true", help="write the measured counts to the baseline")
    ap.add_argument("--strict", action="store_true", help="fail when the baseline is looser than the measurement")
    args = ap.parse_args(argv)
    try:
        measured = missed_counts(args.tracefile, args.root)
    except (OSError, ValueError) as err:
        print("ERROR: cannot read the tracefile %s: %s" % (args.tracefile, err))
        return 2
    if not measured:
        print("ERROR: the tracefile %s has no file under %s/src/ (it is empty, or --root is wrong). Stop."
              % (args.tracefile, normalize_root(args.root)))
        return 2
    try:
        baseline = load_baseline(args.baseline)
    except (OSError, ValueError) as err:
        print("ERROR: cannot use the baseline %s: %s" % (args.baseline, err))
        return 2
    listed = source_files(args.src_tree)
    if not listed:
        print("ERROR: no .c or .cpp file under %s/src/ (--src-tree is wrong). Stop." % normalize_root(args.src_tree))
        return 2
    missing = [name for name in listed if name not in measured]
    errors, notices, dropped = compare(measured, baseline)
    now = totals(measured)
    print("Missed lines: %d (baseline %d). Missed branches: %d (baseline %d)." % (
        now["lines_missed"], baseline["totals"]["lines_missed"],
        now["branches_missed"], baseline["totals"]["branches_missed"]))
    if missing:
        print("FAIL: the tracefile has no record of a source file. Do not ignore this: the measurement is incomplete.")
        print("\n".join("  %s: the source file has no record in the tracefile." % name for name in missing))
        if errors:
            print("A missed count also rose:")
            print("\n".join(errors))
        return 1
    if args.update:
        if errors:
            print("ERROR: refusing to update. These counts are worse than the baseline:")
            print("\n".join(errors))
            return 1
        with open(args.baseline, "w", encoding="utf-8") as fh:
            json.dump(baseline_from(measured), fh, indent=2)
            fh.write("\n")
        print("Baseline updated.")
        return 0
    for name in dropped:
        notices.append("  %s: the baseline lists it, but the tracefile has no record of it." % name)
    if notices:
        print("Notice: the baseline is looser than the measurement, or a file is missing.")
        print("\n".join(notices))
        print("Run: python3 scripts/coverage_gate.py <lcov.info> <baseline.json> --update")
    if errors:
        print("FAIL: a missed count rose. Add tests for the new or changed code.")
        print("\n".join(errors))
        return 1
    if notices and args.strict:
        print("FAIL: the baseline is stale or a file is missing (--strict).")
        return 1
    print("PASS: no missed count rose.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
