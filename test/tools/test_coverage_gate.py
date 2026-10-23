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
    def __init__(self, tracefile_text, baseline_json, *flags):
        self.dir = tempfile.TemporaryDirectory()
        self.trace = os.path.join(self.dir.name, "lcov.info")
        self.base = os.path.join(self.dir.name, "baseline.json")
        with open(self.trace, "w") as fh:
            fh.write(tracefile_text)
        with open(self.base, "w") as fh:
            json.dump(baseline_json, fh)
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            self.status = coverage_gate.main([self.trace, self.base, *flags])
        self.output = buf.getvalue()
        with open(self.base) as fh:
            self.final_baseline = json.load(fh)
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
        run = GateRun(tracefile(swapped), base)
        self.assertEqual(run.status, 1)
        self.assertIn("src/B.cpp", run.output)

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
        run = GateRun(tracefile(elsewhere), baseline(1, 1, {"src/Unit.cpp": {"lines_missed": 1, "branches_missed": 1}}))
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


if __name__ == "__main__":
    unittest.main()
