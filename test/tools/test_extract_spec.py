"""Requirements of scripts/extract_spec.py.

Run: python3 -m unittest discover -s test/tools -p 'test_*.py'
Each test builds a small repository in a temporary directory (test/src, src,
docs/spec), runs the script on it, and checks the exit status, the printed
message and the spec files. The tests do not read script internals.
"""
import ast
import contextlib
import io
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import unittest.mock

SCRIPT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "scripts")
sys.path.insert(0, SCRIPT_DIR)
import extract_spec  # noqa: E402

SIMPLE = '''#include <catch2/catch_test_macros.hpp>

TEST_CASE("test_Unit_begin_returns_true", "[Unit][begin]")
{
    REQUIRE(true);
}

TEST_CASE("test_Unit_end_clears_the_buffer", "[Unit]")
{
    SECTION("when the buffer is empty") { REQUIRE(true); }
}
'''


class SpecRepo:
    """A temporary repository. Use as a context manager."""

    def __init__(self, tests=None, src_units=("Unit",)):
        self.dir = tempfile.TemporaryDirectory()
        self.root = self.dir.name
        for sub in ("test/src", "src", "docs/spec", "scripts"):
            os.makedirs(os.path.join(self.root, sub))
        for unit in src_units:
            self.write("src/%s.cpp" % unit, "// source\n")
        for name, text in (tests or {"Unit_test.cpp": SIMPLE}).items():
            self.write("test/src/" + name, text)

    def write(self, rel, text):
        with open(os.path.join(self.root, rel), "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)

    def read(self, rel):
        with open(os.path.join(self.root, rel), encoding="utf-8") as fh:
            return fh.read()

    def run(self, *args):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            status = extract_spec.main(list(args), root=self.root)
        return status, buf.getvalue()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.dir.cleanup()


class ExtractSpecTest(unittest.TestCase):
    def test_script_writes_one_spec_file_per_unit_and_an_index(self):
        with SpecRepo() as repo:
            status, out = repo.run()
            self.assertEqual(status, 0, out)
            self.assertEqual(sorted(os.listdir(os.path.join(repo.root, "docs/spec"))),
                             ["README.md", "Unit.md"])
            self.assertIn("returns true", repo.read("docs/spec/Unit.md"))
            self.assertIn("| **Total** | **2** |", repo.read("docs/spec/README.md"))

    def test_script_finds_a_test_case_that_is_written_on_several_lines(self):
        split = SIMPLE + '''
TEST_CASE(
    "test_Unit_reset_is_found_on_three_lines",
    "[Unit][reset]"
)
{
    REQUIRE(true);
}
'''
        with SpecRepo({"Unit_test.cpp": split}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 0, out)
            self.assertIn("reset is found on three lines", repo.read("docs/spec/Unit.md"))
            self.assertIn("| **Total** | **3** |", repo.read("docs/spec/README.md"))

    def test_script_finds_a_test_case_whose_name_and_tags_are_on_separate_lines(self):
        split = 'TEST_CASE("test_Unit_a_is_found",\n          "[Unit]") {\n}\n'
        with SpecRepo({"Unit_test.cpp": split}) as repo:
            self.assertEqual(repo.run()[0], 0)
            self.assertIn("a is found", repo.read("docs/spec/Unit.md"))

    def test_script_stops_with_status_2_when_a_test_case_cannot_be_parsed(self):
        for label, text in {
            "name from a macro": 'TEST_CASE(NAME, "[Unit]") {}\n',
            "tags from a macro": 'TEST_CASE("test_Unit_a_b", TAGS) {}\n',
            "fixture macro": 'TEST_CASE_METHOD(Fixture, "test_Unit_a_b", "[Unit]") {}\n',
            "template macro": 'TEMPLATE_TEST_CASE("test_Unit_a_b", "[Unit]", int) {}\n',
            "never closed": 'TEST_CASE("test_Unit_a_b", "[Unit]"\n' + "{\n" * 12,
        }.items():
            with self.subTest(label), SpecRepo({"Unit_test.cpp": SIMPLE + text}) as repo:
                status, out = repo.run()
                self.assertEqual(status, 2)
                self.assertIn("ERROR: Unit_test.cpp:", out)
                self.assertIn("TEST_CASE", out)

    def test_script_ignores_a_test_case_that_is_only_in_a_comment(self):
        text = SIMPLE + '// TEST_CASE("test_Unit_old_is_not_a_test", "[Unit]")\n'
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 0, out)
            self.assertNotIn("old is not a test", repo.read("docs/spec/Unit.md"))

    def test_script_ignores_a_test_case_that_is_in_a_block_comment(self):
        text = SIMPLE + ('/*\nTEST_CASE("test_Unit_old_is_not_a_test", "[Unit]")\n{\n}\n*/\n'
                         '/* TEST_CASE("test_Unit_inline_is_not_a_test", "[Unit]") */\n')
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 0, out)
            spec = repo.read("docs/spec/Unit.md")
            self.assertNotIn("is not a test", spec)
            self.assertIn("Tests: 2.", spec)

    def test_script_ignores_test_case_text_in_a_string_or_after_a_trailing_comment(self):
        text = SIMPLE + ('TEST_CASE("test_Unit_print_writes_text", "[Unit]")  // was TEST_CASE("x"\n'
                         '{\n    const char *s = "TEST_CASE(\\"y\\", ";  /* TEST_CASE( */\n}\n')
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 0, out)
            spec = repo.read("docs/spec/Unit.md")
            self.assertIn("Tests: 3.", spec)
            self.assertIn("print writes text", spec)

    def test_script_keeps_the_line_number_after_a_block_comment(self):
        text = '/* one\n two */\nTEST_CASE("test_Unit_a_b", "[Unit]") {} TEST_CASE("test_Unit_c_d", "[Unit]") {}\n'
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 2, out)
            self.assertIn('Unit_test.cpp: 2 "TEST_CASE(" found but 1 test cases were parsed', out)

    def test_script_stops_with_status_2_when_there_is_no_test_case(self):
        with SpecRepo({"Unit_test.cpp": "// nothing\n"}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 2)
            self.assertIn("no TEST_CASE found", out)

    def test_script_lists_the_sections_and_the_build_variant_of_a_test(self):
        text = ('#ifdef WIRE_HAS_END\nTEST_CASE("test_Unit_end_calls_wire_end", "[Unit]")\n'
                '{\n    SECTION("first") {}\n}\n#endif\n')
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            self.assertEqual(repo.run()[0], 0)
            spec = repo.read("docs/spec/Unit.md")
            self.assertIn("variant: WIRE_HAS_END", spec)
            self.assertIn("  - first", spec)

    def test_script_shows_the_condition_of_every_guard_around_a_test(self):
        def tc(n):
            return 'TEST_CASE("test_Unit_%s_works", "[Unit]") {}\n' % n
        text = ('#ifndef ARDUINO_ARCH_ESP32\n' + tc('a') + '#else\n' + tc('b') + '#endif\n'
                '#if !defined(USE_SPI) // comment\n' + tc('c') + '#else\n' + tc('d') + '#endif\n'
                '#if NOTE_LEVEL > 1\n' + tc('e') + '#else\n'
                '# ifdef USE_I2C\n' + tc('f') + '# endif\n#endif\n' + tc('g'))
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 0, out)
            spec = repo.read("docs/spec/Unit.md")
            for n, v in (("a", "ARDUINO_ARCH_ESP32 not defined"), ("b", "ARDUINO_ARCH_ESP32 defined"),
                         ("c", "USE_SPI not defined"), ("d", "USE_SPI defined"),
                         ("e", "NOTE_LEVEL > 1"), ("f", "not (NOTE_LEVEL > 1) and USE_I2C defined")):
                self.assertIn("%s works _[variant: %s]_" % (n, v), spec)
            self.assertIn("g works  \n", spec)

    def test_script_stops_with_status_2_on_a_guard_that_it_cannot_follow(self):
        tc = 'TEST_CASE("test_Unit_a_b", "[Unit]") {}\n'
        cases = (("#if A\n" + tc + "#elif B\n#endif\n", "Unit_test.cpp:3: #elif is not supported"),
                 (tc + "#endif\n", "Unit_test.cpp:2: this #else or #endif has no #if"),
                 (tc + "#else\n", "Unit_test.cpp:2: this #else or #endif has no #if"),
                 ("#ifdef A\n" + tc, "Unit_test.cpp:1: this #ifdef has no #endif"))
        for text, message in cases:
            with SpecRepo({"Unit_test.cpp": text}) as repo:
                status, out = repo.run()
                self.assertEqual(status, 2, out)
                self.assertIn(message, out)

    def test_script_shows_an_exception_marker_with_its_name_and_reason(self):
        text = ('// IMPL-CHECK-EXCEPTION: end_calls_wire_end: the call is the only visible effect\n'
                'TEST_CASE("test_Unit_end_calls_wire_end", "[Unit]") {}\n')
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 0, out)
            self.assertIn("IMPL-CHECK-EXCEPTION `end_calls_wire_end`: the call is the only visible effect",
                          repo.read("docs/spec/Unit.md"))

    def test_script_stops_with_status_2_when_an_exception_marker_has_no_name_or_no_reason(self):
        for label, marker in {
            "reason only": "// IMPL-CHECK-EXCEPTION: the call is the only visible effect",
            "name only": "// IMPL-CHECK-EXCEPTION: end_calls_wire_end:",
            "nothing": "// IMPL-CHECK-EXCEPTION:",
        }.items():
            with self.subTest(label):
                text = marker + '\nTEST_CASE("test_Unit_a_b", "[Unit]") {}\n'
                with SpecRepo({"Unit_test.cpp": text}) as repo:
                    status, out = repo.run()
                    self.assertEqual(status, 2)
                    self.assertIn("IMPL-CHECK-EXCEPTION needs a name and a reason", out)

    def test_script_stops_with_status_2_when_an_exception_marker_is_not_above_a_test_case(self):
        text = ('// IMPL-CHECK-EXCEPTION: name: reason\nint x = 1;\n'
                'TEST_CASE("test_Unit_a_b", "[Unit]") {}\n')
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 2)
            self.assertIn("not directly above a TEST_CASE", out)

    def test_script_reads_a_test_file_that_has_utf_8_text_when_the_locale_is_not_utf_8(self):
        text = SIMPLE + '// Zeichen: \u00e4\u00f6\u00fc \u2014 end\nTEST_CASE("test_Unit_x_is_found", "[Unit]") {}\n'
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            shutil.copy(os.path.join(SCRIPT_DIR, "extract_spec.py"),
                        os.path.join(repo.root, "scripts", "extract_spec.py"))
            env = dict(os.environ, LC_ALL="C", LANG="C", PYTHONUTF8="0", PYTHONCOERCECLOCALE="0")
            env.pop("PYTHONIOENCODING", None)
            proc = subprocess.run([sys.executable, os.path.join(repo.root, "scripts", "extract_spec.py")],
                                  env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  universal_newlines=True)
            self.assertEqual(proc.returncode, 0, proc.stdout)
            self.assertIn("x is found", repo.read("docs/spec/Unit.md"))

    def test_check_passes_when_the_spec_was_just_written(self):
        with SpecRepo() as repo:
            repo.run()
            status, out = repo.run("--check")
            self.assertEqual(status, 0, out)
            self.assertIn("spec is up to date (2 tests, 2 files)", out)

    def test_check_fails_when_a_test_was_added_and_the_spec_was_not_written_again(self):
        with SpecRepo() as repo:
            repo.run()
            repo.write("test/src/Unit_test.cpp", SIMPLE + 'TEST_CASE("test_Unit_new_is_added", "[Unit]") {}\n')
            status, out = repo.run("--check")
            self.assertEqual(status, 1)
            self.assertIn("spec is out of date", out)

    def test_check_fails_when_a_spec_file_is_missing_or_not_from_a_unit(self):
        with SpecRepo() as repo:
            repo.run()
            os.remove(os.path.join(repo.root, "docs/spec/Unit.md"))
            self.assertEqual(repo.run("--check")[0], 1)
            repo.run()
            repo.write("docs/spec/Old.md", "old\n")
            self.assertEqual(repo.run("--check")[0], 1)

    def test_check_passes_when_only_the_body_of_a_test_changes(self):
        with SpecRepo() as repo:
            repo.run()
            repo.write("test/src/Unit_test.cpp", SIMPLE.replace("REQUIRE(true);\n}", "REQUIRE(1 == 1);\n    REQUIRE(2 == 2);\n}", 1))
            self.assertEqual(repo.run("--check")[0], 0)

    def test_check_stops_with_status_2_and_does_not_report_a_stale_spec_when_a_test_cannot_be_parsed(self):
        with SpecRepo() as repo:
            repo.run()
            repo.write("test/src/Unit_test.cpp", SIMPLE + 'TEST_CASE(NAME, "[Unit]") {}\n')
            status, out = repo.run("--check")
            self.assertEqual(status, 2)
            self.assertNotIn("out of date", out)

    def test_script_lists_a_source_unit_that_has_no_test_as_having_no_tests(self):
        with SpecRepo(src_units=("Unit", "Lonely")) as repo:
            self.assertEqual(repo.run()[0], 0)
            self.assertIn("no tests", repo.read("docs/spec/Lonely.md"))

    def test_script_warns_when_the_first_tag_is_not_the_unit_of_the_file(self):
        with SpecRepo({"Unit_test.cpp": 'TEST_CASE("test_Other_a_b", "[Other]") {}\n'}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 0)
            self.assertIn("warning: first tag [Other] does not match file Unit_test.cpp", out)

    def test_script_counts_every_test_case_in_the_repository(self):
        """The cross-check on the real tests: each TEST_CASE( is a parsed test."""
        repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
        test_dir = os.path.join(repo_root, "test", "src")
        expected = 0
        for fn in os.listdir(test_dir):
            if fn.endswith("_test.cpp"):
                with open(os.path.join(test_dir, fn), encoding="utf-8") as fh:
                    expected += sum(line.count("TEST_CASE(") for line in fh
                                    if not line.lstrip().startswith("//"))
        self.assertEqual(len(extract_spec.parse_tests(test_dir)), expected)

    def test_script_stops_with_status_2_when_two_test_cases_are_on_one_line(self):
        text = SIMPLE + 'TEST_CASE("test_Unit_a_b", "[Unit]") {} TEST_CASE("test_Unit_c_d", "[Unit]") {}\n'
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 2, out)
            self.assertIn('Unit_test.cpp: 4 "TEST_CASE(" found but 3 test cases were parsed', out)

    def test_the_marker_form_in_the_readme_is_the_form_that_the_script_accepts(self):
        """Code and docs agree: the README shows one form, and the script takes that form."""
        with SpecRepo() as repo:
            self.assertEqual(repo.run()[0], 0)
            readme = repo.read("docs/spec/README.md")
            self.assertIn("`// IMPL-CHECK-EXCEPTION: <name>: <reason>`", readme)
            form = "// IMPL-CHECK-EXCEPTION: <name>: <reason>"
            marker = form.replace("<name>", "end_calls_wire_end").replace("<reason>", "only effect")
            repo.write("test/src/Unit_test.cpp",
                       marker + '\nTEST_CASE("test_Unit_end_calls_wire_end", "[Unit]") {}\n')
            status, out = repo.run()
            self.assertEqual(status, 0, out)
            self.assertIn("`end_calls_wire_end`: only effect", repo.read("docs/spec/Unit.md"))

    def test_script_flags_a_test_name_that_states_a_call_as_interaction(self):
        text = ('TEST_CASE("test_unit_begin_invokes_wire_begin", "[Unit]") {}\n'
                'TEST_CASE("test_unit_begin_returns_true", "[Unit]") {}\n')
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            self.assertEqual(repo.run()[0], 0)
            spec = repo.read("docs/spec/Unit.md")
            self.assertIn("- `interaction` — begin invokes wire begin", spec)
            self.assertIn("- `behavior` — begin returns true", spec)
            self.assertIn("Interaction checks (flagged): 1.", spec)


class OutputOrderTest(unittest.TestCase):
    """The output does not depend on the directory order or on the hash seed."""

    UNITS = ("Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot", "Golf", "Hotel")

    def make_repo(self):
        tests = {}
        for u in self.UNITS[:6]:
            tests[u + "_test.cpp"] = ('// SPEC-GAP: gap of %s\n'
                                      'TEST_CASE("test_%s_begin_works", "[%s]") {}\n'
                                      'TEST_CASE("test_Shared_%s_works", "[Shared]") {}\n'
                                      % (u, u, u, u))
        return SpecRepo(tests, src_units=self.UNITS)

    @staticmethod
    def spec_files(repo):
        d = os.path.join(repo.root, "docs/spec")
        return {fn: repo.read("docs/spec/" + fn) for fn in os.listdir(d)}

    def test_output_is_byte_identical_when_the_directory_order_changes(self):
        real_listdir = os.listdir
        results = []
        for order in (sorted, lambda x: sorted(x, reverse=True),
                      lambda x: sorted(x, key=lambda n: n[::-1])):
            def listdir(path, order=order):
                return order(real_listdir(path))
            with self.make_repo() as repo:
                with unittest.mock.patch.object(extract_spec.os, "listdir", listdir):
                    self.assertEqual(repo.run()[0], 0)
                results.append(self.spec_files(repo))
        self.assertEqual(results[0], results[1])
        self.assertEqual(results[0], results[2])
        self.assertIn("Alpha", results[0]["README.md"])

    def test_output_is_byte_identical_for_each_hash_seed(self):
        code = ("import sys; sys.path.insert(0, sys.argv[1]); import extract_spec; "
                "sys.exit(extract_spec.main([], root=sys.argv[2]))")
        results = []
        for seed in ("0", "1", "2", "3", "4", "5"):
            with self.make_repo() as repo:
                env = dict(os.environ, PYTHONHASHSEED=seed)
                r = subprocess.run([sys.executable, "-c", code, SCRIPT_DIR, repo.root],
                                   env=env, capture_output=True, text=True)
                self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
                results.append(self.spec_files(repo))
        for other in results[1:]:
            self.assertEqual(results[0], other)


class SpecGapTest(unittest.TestCase):
    GAP = "// SPEC-GAP: start(0) is not specified. No test locks it.\n"

    def test_a_spec_gap_is_the_last_section_of_the_unit_spec(self):
        with SpecRepo({"Unit_test.cpp": self.GAP + SIMPLE}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 0, out)
            spec = repo.read("docs/spec/Unit.md")
            self.assertTrue(spec.endswith(
                "## Open spec gaps\n\n"
                "The docs do not specify these behaviors. No test locks them.\n\n"
                "- gap: start(0) is not specified. No test locks it.\n"), spec)
            self.assertEqual(spec.count("## Open spec gaps"), 1)

    def test_the_readme_shows_the_gap_count(self):
        text = self.GAP + SIMPLE + "// SPEC-GAP: second gap\n"
        with SpecRepo({"Unit_test.cpp": text}) as repo:
            self.assertEqual(repo.run()[0], 0)
            self.assertIn("no test locks it): 2.", repo.read("docs/spec/README.md"))
            self.assertIn("- gap: second gap", repo.read("docs/spec/Unit.md"))

    def test_a_unit_with_no_gap_has_no_gap_section_and_the_readme_shows_0(self):
        with SpecRepo() as repo:
            self.assertEqual(repo.run()[0], 0)
            self.assertNotIn("Open spec gaps", repo.read("docs/spec/Unit.md"))
            self.assertIn("no test locks it): 0.", repo.read("docs/spec/README.md"))

    def test_a_gap_goes_to_the_unit_of_its_test_file_only(self):
        other = 'TEST_CASE("test_Other_a_b", "[Other]") {}\n'
        with SpecRepo({"Unit_test.cpp": SIMPLE, "Other_test.cpp": self.GAP + other},
                      src_units=("Unit", "Other")) as repo:
            self.assertEqual(repo.run()[0], 0)
            self.assertIn("- gap: start(0)", repo.read("docs/spec/Other.md"))
            self.assertNotIn("gap", repo.read("docs/spec/Unit.md"))

    def test_a_gap_is_not_a_test_and_does_not_change_the_test_count(self):
        with SpecRepo({"Unit_test.cpp": self.GAP + SIMPLE}) as repo:
            self.assertEqual(repo.run()[0], 0)
            self.assertIn("| **Total** | **2** |", repo.read("docs/spec/README.md"))

    def test_check_fails_when_a_gap_was_added_and_the_spec_was_not_written_again(self):
        with SpecRepo() as repo:
            repo.run()
            repo.write("test/src/Unit_test.cpp", self.GAP + SIMPLE)
            self.assertEqual(repo.run("--check")[0], 1)

    def test_script_stops_with_status_2_when_a_gap_has_no_text(self):
        with SpecRepo({"Unit_test.cpp": "// SPEC-GAP:   \n" + SIMPLE}) as repo:
            status, out = repo.run()
            self.assertEqual(status, 2)
            self.assertIn("SPEC-GAP needs a text", out)


class EncodingTest(unittest.TestCase):
    """Every open() in the script and in its tests names an encoding.

    Without encoding=, Python uses the locale of the machine. The check reads
    the source with ast, so it needs no special Python flag.
    """
    FILES = (extract_spec.__file__, __file__)

    def test_every_open_call_names_an_encoding(self):
        bad = []
        for path in self.FILES:
            with open(path, encoding="utf-8") as fh:
                tree = ast.parse(fh.read())
            for node in ast.walk(tree):
                if isinstance(node, ast.Call) and getattr(node.func, "id", None) == "open":
                    if "encoding" not in [kw.arg for kw in node.keywords]:
                        bad.append("%s:%d" % (os.path.basename(path), node.lineno))
        self.assertEqual(bad, [], "open() without encoding=")


if __name__ == "__main__":
    unittest.main()
