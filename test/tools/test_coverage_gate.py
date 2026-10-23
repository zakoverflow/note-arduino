"""Requirements of scripts/coverage_gate.py.

Run: python3 -m unittest discover -s test/tools -p 'test_*.py'
Every test feeds a small lcov tracefile and a baseline to the gate and checks
the exit status and the printed verdict. The tests do not read gate internals.
"""
import contextlib
import io
import json
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "scripts"))
import coverage_gate  # noqa: E402


def tracefile(files):
    """files: {path: (lines, branches)}. lines: [hits]. branches: [(block, taken)]."""
    out = []
    for path, (lines, branches) in files.items():
        out.append("SF:" + path)
        out += ["DA:%d,%d" % (n + 1, hits) for n, hits in enumerate(lines)]
        out += ["BRDA:%d,%s,%d,%s" % (n + 1, blk, n, taken) for n, (blk, taken) in enumerate(branches)]
        out.append("end_of_record")
    return "\n".join(out) + "\n"


def baseline(lines_missed, branches_missed, files=None):
    return {"totals": {"lines_missed": lines_missed, "branches_missed": branches_missed},
            "files": files or {}}


class GateRun:
    """Run the gate once. root is the --root value ("" = leave the option out).

    baseline_json is written as JSON. baseline_text is written as it is (use it
    for a baseline that is not valid JSON). None for either leaves no file.

    tree lists the source files of the checkout, for example ["src/A.cpp"]. The
    gate reads them from a temporary directory (--src-tree). The default is the
    .c and .cpp files that the tracefile lists under <root>/src/ (not src/note-c/),
    so a test that does not care about the list sees a complete tracefile.
    """
    def __init__(self, tracefile_text, baseline_json, *flags, root="/note-arduino", baseline_text=None, tree=None):
        self.dir = tempfile.TemporaryDirectory()
        self.trace = os.path.join(self.dir.name, "lcov.info")
        self.base = os.path.join(self.dir.name, "baseline.json")
        if tracefile_text is not None:
            with open(self.trace, "w") as fh:
                fh.write(tracefile_text)
        if baseline_text is not None:
            with open(self.base, "w") as fh:
                fh.write(baseline_text)
        elif baseline_json is not None:
            with open(self.base, "w") as fh:
                json.dump(baseline_json, fh)
        if tree is None:
            top = root or os.path.dirname(os.path.dirname(os.path.realpath(coverage_gate.__file__)))
            prefix = top + "/src/"
            listed = [line[3:].strip() for line in (tracefile_text or "").splitlines() if line.startswith("SF:")]
            tree = [name[len(top) + 1:] for name in listed
                    if name.startswith(prefix) and not name.startswith(prefix + "note-c/")
                    and name.endswith((".c", ".cpp"))]
        src_tree = os.path.join(self.dir.name, "checkout")
        for name in tree:
            path = os.path.join(src_tree, *name.split("/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            open(path, "w").close()
        os.makedirs(os.path.join(src_tree, "src"), exist_ok=True)
        args = ([self.trace, self.base] + (["--root", root] if root else []) +
                ["--src-tree", src_tree] + list(flags))
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            self.status = coverage_gate.main(args)
        self.output = buf.getvalue()
        try:
            with open(self.base) as fh:
                self.final_baseline = json.load(fh)
        except (OSError, ValueError):
            self.final_baseline = None
        self.dir.cleanup()


SRC = "/note-arduino/src/Unit.cpp"
ONE_MISS = {SRC: ([1, 1, 0], [("0", "1"), ("0", "-")])}


class CoverageGateTest(unittest.TestCase):
    def test_gate_passes_when_missed_counts_equal_the_baseline(self):
        run = GateRun(tracefile(ONE_MISS), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}))
        self.assertEqual(run.status, 0)
        self.assertIn("PASS", run.output)

    def test_gate_fails_when_missed_lines_rise(self):
        worse = {SRC: ([1, 0, 0], [("0", "1"), ("0", "-")])}
        run = GateRun(tracefile(worse), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}))
        self.assertEqual(run.status, 1)
        self.assertIn("Missed lines rose from 1 to 2", run.output)

    def test_gate_fails_when_missed_branches_rise(self):
        worse = {SRC: ([1, 1, 0], [("0", "-"), ("0", "0")])}
        run = GateRun(tracefile(worse), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}))
        self.assertEqual(run.status, 1)
        self.assertIn("Missed branches rose from 1 to 2", run.output)

    def test_gate_fails_when_code_is_added_without_tests_even_if_the_percentage_holds(self):
        # 1 of 3 lines missed (33%) becomes 2 of 6 missed (33%).
        same_percent = {SRC: ([1, 1, 0, 1, 1, 0], [("0", "1"), ("0", "-")])}
        run = GateRun(tracefile(same_percent), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}))
        self.assertEqual(run.status, 1)

    def test_gate_fails_when_a_new_file_has_a_missed_line(self):
        added = dict(ONE_MISS)
        added["/note-arduino/src/New.cpp"] = ([0], [])
        run = GateRun(tracefile(added), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}))
        self.assertEqual(run.status, 1)
        self.assertIn("src/New.cpp", run.output)

    def test_gate_fails_when_one_file_gets_worse_and_another_gets_better(self):
        base = baseline(1, 0, {"src/A.cpp": {"lines_missed": 1, "branches_missed": 0}})
        swapped = {"/n/src/A.cpp": ([1], []), "/n/src/B.cpp": ([0], [])}
        run = GateRun(tracefile(swapped), base, root="/n")
        self.assertEqual(run.status, 1)
        self.assertIn("src/B.cpp", run.output)

    # Lines and branches are compared apart, per file and in total. A rise in one
    # kind fails even when the other kind falls by the same amount.
    @staticmethod
    def misses(lines, branches):
        """One file record with this many missed lines and missed branches (and one covered of each)."""
        return ([0] * lines + [1], [("0", "-")] * branches + [("0", "1")])

    SWAP_BASELINE = {"src/A.cpp": {"lines_missed": 1, "branches_missed": 1}}

    def test_gate_fails_when_a_file_trades_a_missed_branch_for_a_missed_line(self):
        # File A: 1 line and 1 branch missed in the baseline. Now 2 lines and 0 branches. 2 = 2, but lines rose.
        run = GateRun(tracefile({"/n/src/A.cpp": self.misses(2, 0)}), baseline(1, 1, self.SWAP_BASELINE), root="/n")
        self.assertEqual(run.status, 1, run.output)
        self.assertIn("src/A.cpp: missed lines rose from 1 to 2", run.output)

    def test_gate_fails_when_a_file_trades_a_missed_line_for_a_missed_branch(self):
        run = GateRun(tracefile({"/n/src/A.cpp": self.misses(0, 2)}), baseline(1, 1, self.SWAP_BASELINE), root="/n")
        self.assertEqual(run.status, 1, run.output)
        self.assertIn("src/A.cpp: missed branches rose from 1 to 2", run.output)

    def test_gate_fails_on_a_per_file_rise_in_lines_when_the_total_does_not_rise(self):
        # B improves, so the totals fall or stay. A still gets worse.
        base = baseline(2, 2, {"src/A.cpp": {"lines_missed": 1, "branches_missed": 1},
                               "src/B.cpp": {"lines_missed": 1, "branches_missed": 1}})
        files = {"/n/src/A.cpp": self.misses(2, 0), "/n/src/B.cpp": self.misses(0, 1)}   # totals 2 and 1
        run = GateRun(tracefile(files), base, root="/n")
        self.assertEqual(run.status, 1, run.output)
        self.assertNotIn("Missed lines rose", run.output)
        self.assertIn("src/A.cpp: missed lines rose from 1 to 2", run.output)

    def test_gate_fails_on_a_per_file_rise_in_branches_when_the_total_does_not_rise(self):
        base = baseline(2, 2, {"src/A.cpp": {"lines_missed": 1, "branches_missed": 1},
                               "src/B.cpp": {"lines_missed": 1, "branches_missed": 1}})
        files = {"/n/src/A.cpp": self.misses(0, 2), "/n/src/B.cpp": self.misses(1, 0)}   # totals 1 and 2
        run = GateRun(tracefile(files), base, root="/n")
        self.assertEqual(run.status, 1, run.output)
        self.assertNotIn("Missed branches rose", run.output)
        self.assertIn("src/A.cpp: missed branches rose from 1 to 2", run.output)

    def test_gate_fails_when_the_total_rises_in_one_kind_and_falls_in_the_other(self):
        # Each file keeps its own count. The baseline totals are tighter than the sum of the files.
        base = baseline(0, 2, {"src/A.cpp": {"lines_missed": 1, "branches_missed": 1}})
        run = GateRun(tracefile({"/n/src/A.cpp": self.misses(1, 1)}), base, root="/n")
        self.assertEqual(run.status, 1, run.output)
        self.assertIn("Missed lines rose from 0 to 1", run.output)

    def test_gate_does_not_count_exception_branches(self):
        only_exception_misses = {SRC: ([1, 1, 0], [("0", "1"), ("e0", "0"), ("e1", "-")])}
        run = GateRun(tracefile(only_exception_misses), baseline(1, 0, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 0}}))
        self.assertEqual(run.status, 0)

    def test_gate_does_not_count_note_c_or_test_files(self):
        extra = dict(ONE_MISS)
        extra["/note-arduino/src/note-c/note.c"] = ([0, 0], [("0", "-")])
        extra["/note-arduino/test/src/arduino_fakes.cpp"] = ([0], [])
        extra["/note-arduino/build/_deps/catch2-src/src/catch2/a.cpp"] = ([0], [])
        run = GateRun(tracefile(extra), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}))
        self.assertEqual(run.status, 0)

    def test_gate_passes_with_a_notice_when_missed_counts_fall(self):
        better = {SRC: ([1, 1, 1], [("0", "1"), ("0", "1")])}
        run = GateRun(tracefile(better), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}))
        self.assertEqual(run.status, 0)
        self.assertIn("Tighten the baseline", run.output)

    def test_gate_fails_on_a_stale_baseline_when_strict(self):
        better = {SRC: ([1, 1, 1], [("0", "1"), ("0", "1")])}
        run = GateRun(tracefile(better), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}), "--strict")
        self.assertEqual(run.status, 1)

    def test_gate_reads_the_same_counts_from_a_path_in_another_checkout_directory(self):
        elsewhere = {"/home/someone/work/note-arduino/src/Unit.cpp": ONE_MISS[SRC]}
        run = GateRun(tracefile(elsewhere), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}),
                      root="/home/someone/work/note-arduino")
        self.assertEqual(run.status, 0)

    def test_update_writes_better_counts_to_the_baseline(self):
        better = {SRC: ([1, 1, 1], [("0", "1"), ("0", "-")])}
        run = GateRun(tracefile(better), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}), "--update")
        self.assertEqual(run.status, 0)
        self.assertEqual(run.final_baseline["totals"], {"lines_missed": 0, "branches_missed": 1})

    def test_update_refuses_to_write_worse_counts(self):
        worse = {SRC: ([0, 0, 0], [("0", "-")])}
        original = baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}})
        run = GateRun(tracefile(worse), original, "--update")
        self.assertEqual(run.status, 1)
        self.assertEqual(run.final_baseline, original)

    def test_gate_stops_with_status_2_when_the_tracefile_has_no_src_file(self):
        run = GateRun(tracefile({"/note-arduino/other/x.cpp": ([0], [])}), baseline(0, 0))
        self.assertEqual(run.status, 2)

    # --- review fixes -------------------------------------------------------

    def test_gate_stops_with_status_2_and_a_message_when_the_baseline_is_malformed(self):
        good = {"lines_missed": 1, "branches_missed": 1}
        bad_baselines = {
            "not json": '{"totals": ',
            "top level is a list": "[1, 2]",
            "totals missing": json.dumps({"files": {}}),
            "totals is a string": json.dumps({"totals": "x", "files": {}}),
            "count is a string": json.dumps({"totals": {"lines_missed": "1", "branches_missed": 1}, "files": {}}),
            "count is negative": json.dumps({"totals": {"lines_missed": -1, "branches_missed": 1}, "files": {}}),
            "count is a boolean": json.dumps({"totals": {"lines_missed": True, "branches_missed": 1}, "files": {}}),
            "branches key missing": json.dumps({"totals": {"lines_missed": 1}, "files": {}}),
            "files missing": json.dumps({"totals": good}),
            "file entry is a list": json.dumps({"totals": good, "files": {"src/Unit.cpp": [1, 1]}}),
            "file entry has no count": json.dumps({"totals": good, "files": {"src/Unit.cpp": {"lines_missed": 1}}}),
        }
        for label, text in bad_baselines.items():
            with self.subTest(label):
                run = GateRun(tracefile(ONE_MISS), None, baseline_text=text)
                self.assertEqual(run.status, 2)
                self.assertIn("ERROR: cannot use the baseline", run.output)

    def test_gate_stops_with_status_2_when_the_baseline_file_does_not_exist(self):
        run = GateRun(tracefile(ONE_MISS), None)
        self.assertEqual(run.status, 2)
        self.assertIn("ERROR: cannot use the baseline", run.output)

    def test_gate_stops_with_status_2_when_the_tracefile_does_not_exist(self):
        run = GateRun(None, baseline(1, 1))
        self.assertEqual(run.status, 2)
        self.assertIn("ERROR: cannot read the tracefile", run.output)

    def test_gate_stops_with_status_2_when_the_tracefile_is_empty(self):
        run = GateRun("", baseline(1, 1))
        self.assertEqual(run.status, 2)
        self.assertIn("has no file under", run.output)

    def test_gate_stops_with_status_2_when_a_tracefile_line_is_malformed(self):
        run = GateRun("SF:/note-arduino/src/Unit.cpp\nDA:1\nend_of_record\n", baseline(1, 1))
        self.assertEqual(run.status, 2)
        self.assertIn("ERROR: cannot read the tracefile", run.output)

    def test_gate_stops_with_status_2_and_names_the_root_option_when_the_root_does_not_match(self):
        run = GateRun(tracefile(ONE_MISS), baseline(1, 1), root="/somewhere/else")
        self.assertEqual(run.status, 2)
        self.assertIn("--root", run.output)

    def test_gate_fails_when_strict_and_the_tracefile_has_no_record_of_a_baseline_file(self):
        base = baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1},
                               "src/Gone.cpp": {"lines_missed": 0, "branches_missed": 0}})
        run = GateRun(tracefile(ONE_MISS), base, "--strict")
        self.assertEqual(run.status, 1)
        self.assertIn("src/Gone.cpp: the baseline lists it, but the tracefile has no record of it", run.output)

    def test_gate_passes_with_a_notice_when_not_strict_and_the_tracefile_has_no_record_of_a_baseline_file(self):
        base = baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1},
                               "src/Gone.cpp": {"lines_missed": 0, "branches_missed": 0}})
        run = GateRun(tracefile(ONE_MISS), base)
        self.assertEqual(run.status, 0)
        self.assertIn("src/Gone.cpp: the baseline lists it", run.output)

    def test_update_removes_a_baseline_file_that_has_no_record_in_the_tracefile(self):
        base = baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1},
                               "src/Gone.cpp": {"lines_missed": 0, "branches_missed": 0}})
        run = GateRun(tracefile(ONE_MISS), base, "--update")
        self.assertEqual(run.status, 0)
        self.assertEqual(list(run.final_baseline["files"]), ["src/Unit.cpp"])

    def test_gate_names_files_relative_to_the_root_when_the_repository_is_in_a_directory_named_src(self):
        for root in ("/home/me/src/note-arduino", "/src/note-arduino", "/src"):
            with self.subTest(root):
                files = {
                    root + "/src/Unit.cpp": ONE_MISS[SRC],
                    root + "/test/include/fake.cpp": ([0], []),   # not under src/: not counted
                    root + "/build/generated.cpp": ([0], []),     # not under src/: not counted
                    root + "/src/note-c/note.c": ([0], [("0", "-")]),
                }
                run = GateRun(tracefile(files), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}),
                              "--strict", root=root)
                self.assertEqual(run.status, 0, run.output)
                self.assertIn("PASS", run.output)

    def test_gate_counts_a_file_in_a_sub_directory_named_src_under_the_src_directory_by_its_path_from_the_root(self):
        files = {"/n/src/Unit.cpp": ([1], []), "/n/src/src/Inner.cpp": ([0], [])}
        run = GateRun(tracefile(files), baseline(0, 0), root="/n")
        self.assertEqual(run.status, 1)
        self.assertIn("src/src/Inner.cpp", run.output)

    def test_gate_counts_a_line_as_hit_when_any_record_of_the_same_file_hits_it(self):
        # lcov can write two records for one file. A line or branch that one record
        # misses and another record hits is not missed.
        misses = tracefile({SRC: ([0, 1], [("0", "-")])})
        hits = tracefile({SRC: ([1, 1], [("0", "1")])})
        for label, text in (("miss first", misses + hits), ("hit first", hits + misses)):
            with self.subTest(label):
                run = GateRun(text, baseline(0, 0), "--strict")
                self.assertEqual(run.status, 0, run.output)
                self.assertIn("Missed lines: 0 (baseline 0). Missed branches: 0 (baseline 0).", run.output)

    # --- the list of source files ------------------------------------------

    COVERED = ([1, 1], [("0", "1"), ("0", "1")])   # 0 missed lines, 0 missed branches
    FULL_TREE = ["src/Unit.cpp", "src/Covered.cpp"]
    ONE_MISS_BASELINE = {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}

    def test_gate_fails_when_strict_and_the_tracefile_drops_a_fully_covered_source_file(self):
        dropped = {SRC: ONE_MISS[SRC]}   # src/Covered.cpp has no record
        run = GateRun(tracefile(dropped), baseline(1, 1, self.ONE_MISS_BASELINE), "--strict", tree=self.FULL_TREE)
        self.assertEqual(run.status, 1, run.output)
        self.assertIn("src/Covered.cpp: the source file has no record in the tracefile", run.output)

    def test_gate_fails_when_not_strict_and_the_tracefile_drops_a_fully_covered_source_file(self):
        dropped = {SRC: ONE_MISS[SRC]}
        run = GateRun(tracefile(dropped), baseline(1, 1, self.ONE_MISS_BASELINE), tree=self.FULL_TREE)
        self.assertEqual(run.status, 1, run.output)
        self.assertIn("src/Covered.cpp", run.output)

    def test_gate_names_every_source_file_that_the_tracefile_drops(self):
        run = GateRun(tracefile({SRC: ONE_MISS[SRC]}), baseline(1, 1, self.ONE_MISS_BASELINE),
                      tree=self.FULL_TREE + ["src/sub/Deep.cpp", "src/Plain.c"])
        self.assertEqual(run.status, 1)
        for name in ("src/Covered.cpp", "src/sub/Deep.cpp", "src/Plain.c"):
            self.assertIn(name, run.output)

    def test_gate_names_a_dropped_source_file_once_when_the_baseline_lists_it_too(self):
        listed = dict(self.ONE_MISS_BASELINE)
        listed["src/Covered.cpp"] = {"lines_missed": 0, "branches_missed": 0}
        run = GateRun(tracefile({SRC: ONE_MISS[SRC]}), baseline(1, 1, listed), tree=self.FULL_TREE)
        self.assertEqual(run.status, 1)
        self.assertEqual(run.output.count("src/Covered.cpp"), 1, run.output)

    def test_gate_passes_when_the_tracefile_has_a_record_for_every_source_file(self):
        files = {SRC: ONE_MISS[SRC], "/note-arduino/src/Covered.cpp": self.COVERED}
        run = GateRun(tracefile(files), baseline(1, 1, self.ONE_MISS_BASELINE), "--strict", tree=self.FULL_TREE)
        self.assertEqual(run.status, 0, run.output)

    def test_gate_does_not_need_a_record_for_header_files_or_for_note_c(self):
        run = GateRun(tracefile({SRC: ONE_MISS[SRC]}), baseline(1, 1, self.ONE_MISS_BASELINE), "--strict",
                      tree=["src/Unit.cpp", "src/Unit.hpp", "src/Defines.h", "src/note-c/note.c", "src/note-c/deep/x.cpp"])
        self.assertEqual(run.status, 0, run.output)

    def test_update_refuses_to_write_a_baseline_when_the_tracefile_drops_a_source_file(self):
        original = baseline(1, 1, self.ONE_MISS_BASELINE)
        run = GateRun(tracefile({SRC: ONE_MISS[SRC]}), original, "--update", tree=self.FULL_TREE)
        self.assertEqual(run.status, 1)
        self.assertEqual(run.final_baseline, original)

    def test_gate_stops_with_status_2_when_the_checkout_has_no_source_file(self):
        run = GateRun(tracefile(ONE_MISS), baseline(1, 1, self.ONE_MISS_BASELINE), tree=[])
        self.assertEqual(run.status, 2)
        self.assertIn("--src-tree", run.output)

    def test_gate_reads_the_source_files_from_the_repository_of_the_script_by_default(self):
        repo = os.path.dirname(os.path.dirname(os.path.realpath(coverage_gate.__file__)))
        listed = coverage_gate.source_files(repo)
        self.assertTrue(listed)
        self.assertTrue(all(name.endswith((".c", ".cpp")) for name in listed))
        self.assertTrue(all(name.startswith("src/") and not name.startswith("src/note-c/") for name in listed))

    def test_gate_uses_the_repository_of_the_script_as_the_default_root(self):
        repo = os.path.dirname(os.path.dirname(os.path.realpath(coverage_gate.__file__)))
        path = repo + "/src/Unit.cpp"
        run = GateRun(tracefile({path: ONE_MISS[SRC]}),
                      baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}), root="")
        self.assertEqual(run.status, 0)


if __name__ == "__main__":
    unittest.main()
