#!/usr/bin/env python3
"""Summarize an lcov tracefile for the unit-test coverage report.

usage: coverage_summary.py <lcov.info> <summary.json> [--repo DIR]

Scope: every file under src/ except src/note-c/ (vendored). A file that has
no data in the tracefile is listed with zero lines, so it cannot hide.

Branches whose block id starts with "e" are compiler-made exception
(unwinding) branches. This script reports them apart and keeps them out of
the branch numbers. Report them with: lcov --rc no_exception_branch=0.
"""
import argparse
import json
import os
import sys


def repo_relative(path, repo):
    """Return the path relative to the repo, or from its last '/src/'."""
    norm = os.path.normpath(path)
    if norm.startswith(repo + os.sep):
        return os.path.relpath(norm, repo)
    cut = norm.rfind(os.sep + "src" + os.sep)
    return norm[cut + 1:] if cut >= 0 else norm


def parse_tracefile(info, repo):
    files, cur = {}, None
    with open(info) as fh:
        for raw in fh:
            raw = raw.strip()
            if raw.startswith("SF:"):
                cur = repo_relative(raw[3:], repo)
                files.setdefault(cur, dict(lines={}, br={}, fn={}))
            elif cur is None:
                continue
            elif raw.startswith("DA:"):
                ln, hits = raw[3:].split(",")[:2]
                files[cur]["lines"][int(ln)] = files[cur]["lines"].get(int(ln), 0) + int(hits)
            elif raw.startswith("BRDA:"):
                ln, blk, br, taken = raw[5:].split(",")
                key = (int(ln), blk, br)
                files[cur]["br"][key] = files[cur]["br"].get(key, 0) + (0 if taken == "-" else int(taken))
            elif raw.startswith("FNA:"):  # lcov 2: FNA:<id>,<count>,<name>
                _id, count, name = raw[4:].split(",", 2)
                files[cur]["fn"][name] = files[cur]["fn"].get(name, 0) + int(count)
            elif raw.startswith("FN:"):  # lcov 1: FN:<line>,<name>
                files[cur]["fn"].setdefault(raw[3:].split(",", 1)[1], 0)
            elif raw.startswith("FNDA:"):  # lcov 1: FNDA:<count>,<name>
                count, name = raw[5:].split(",", 1)
                files[cur]["fn"][name] = files[cur]["fn"].get(name, 0) + int(count)
    return files


def source_universe(repo):
    src = os.path.join(repo, "src")
    found = []
    for d, _dirs, names in os.walk(src):
        if os.path.relpath(d, src).split(os.sep)[0] == "note-c":
            continue
        found += [os.path.relpath(os.path.join(d, n), repo) for n in names if n.endswith((".c", ".cc", ".cpp", ".h", ".hpp"))]
    return sorted(found)


def pct(hit, total):
    return None if total == 0 else round(100.0 * hit / total, 1)


def summarize(files, universe):
    rows = []
    tot = dict(lh=0, lt=0, bh=0, bt=0, eh=0, et=0, fh=0, ft=0)
    for name in universe:
        d = files.get(name, dict(lines={}, br={}, fn={}))
        nbr = {k: v for k, v in d["br"].items() if not k[1].startswith("e")}
        ebr = {k: v for k, v in d["br"].items() if k[1].startswith("e")}
        ft, fh = len(d["fn"]), sum(1 for v in d["fn"].values() if v > 0)
        lt, lh = len(d["lines"]), sum(1 for v in d["lines"].values() if v > 0)
        bt, bh = len(nbr), sum(1 for v in nbr.values() if v > 0)
        et, eh = len(ebr), sum(1 for v in ebr.values() if v > 0)
        rows.append(dict(
            file=name,
            lines_hit=lh, lines_total=lt, lines_missed=lt - lh, line_pct=pct(lh, lt),
            branches_hit=bh, branches_total=bt, branches_missed=bt - bh, branch_pct=pct(bh, bt),
            exception_branches_hit=eh, exception_branches_total=et,
            functions_hit=fh, functions_total=ft,
            missed_lines=sorted(k for k, v in d["lines"].items() if v == 0),
            missed_branches=sorted("%d:%s:%s" % k for k, v in nbr.items() if v == 0)))
        for key, val in (("lh", lh), ("lt", lt), ("bh", bh), ("bt", bt), ("eh", eh),
                         ("et", et), ("fh", fh), ("ft", ft)):
            tot[key] += val
    totals = dict(
        lines_hit=tot["lh"], lines_total=tot["lt"], lines_missed=tot["lt"] - tot["lh"],
        line_pct=pct(tot["lh"], tot["lt"]),
        branches_hit=tot["bh"], branches_total=tot["bt"], branches_missed=tot["bt"] - tot["bh"],
        branch_pct=pct(tot["bh"], tot["bt"]),
        exception_branches_hit=tot["eh"], exception_branches_total=tot["et"],
        functions_hit=tot["fh"], functions_total=tot["ft"])
    return dict(scope="src/ except src/note-c/", totals=totals, files=rows)


def cell(hit, total, percent):
    return "n/a" if total == 0 else "%d/%d (%.1f%%)" % (hit, total, percent)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("tracefile")
    ap.add_argument("summary_json")
    ap.add_argument("--repo", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    args = ap.parse_args()
    repo = os.path.realpath(args.repo)
    summary = summarize(parse_tracefile(args.tracefile, repo), source_universe(repo))
    with open(args.summary_json, "w") as out:
        json.dump(summary, out, indent=2)
        out.write("\n")
    print("%-28s %-16s %-16s" % ("file", "lines", "branches"))
    for r in summary["files"]:
        print("%-28s %-16s %-16s" % (os.path.basename(r["file"]),
              cell(r["lines_hit"], r["lines_total"], r["line_pct"] or 0),
              cell(r["branches_hit"], r["branches_total"], r["branch_pct"] or 0)))
    t = summary["totals"]
    print("%-28s %-16s %-16s" % ("TOTAL", cell(t["lines_hit"], t["lines_total"], t["line_pct"]),
          cell(t["branches_hit"], t["branches_total"], t["branch_pct"])))
    print("missed lines=%d  missed branches=%d  (exception branches counted apart: %d/%d)" % (
        t["lines_missed"], t["branches_missed"], t["exception_branches_hit"], t["exception_branches_total"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
