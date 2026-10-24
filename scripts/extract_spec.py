#!/usr/bin/env python3
"""Generate docs/spec/*.md from the unit tests.

The spec is DERIVED from the tests. Do not edit docs/spec/*.md by hand.
Change a test name or tag, then run this script again.

What the script reads (test/src/*_test.cpp):
  TEST_CASE("<name>", "[<Unit>][<tag>]...")   one requirement per test case
                                               (it can be on more than one line)
  SECTION("<name>")                            sub-requirement of a test case
  #if / #ifdef / #ifndef / #else / #endif      build variant of the requirement. The
                                               spec shows the condition of each guard
                                               around the test case, for example
                                               "X defined", "X not defined", "N > 1"
                                               or "not (N > 1)" after #else. #elif and
                                               an #else or #endif with no #if are errors.
  // IMPL-CHECK-EXCEPTION: <name>: <reason>   comment directly above a test case
  // SPEC-GAP: <text>                          comment anywhere in a test file. It names
                                               a behavior that the docs do not specify and
                                               that no test locks. The text goes into the
                                               section "Open spec gaps" at the end of the
                                               spec of the unit (the stem of the test file).

Naming convention that the script relies on:
  test_<unit>_<subject>_<verb phrase that states the requirement>
  The first tag is the unit (module). It must equal the source file stem.
  Extra tags are optional group keys, for example [receive] or [wire].

Comments and string literals: the script removes // and /* */ comments before
it looks for TEST_CASE. A "TEST_CASE(" in a comment or in a string literal is
not a test case. (A raw string literal R"(...)" is not supported.)

Cross-check: the script counts each "TEST_CASE(" in the code of a test file
(also two on one line) and the test cases that it parsed. A difference is an
error, so a test case cannot drop out of the spec without a message. A marker
that has no name or no reason is an error too. A TEST_CASE( ... ) can be on
up to MAX_TEST_CASE_LINES (10) lines. A longer one is an error.

Usage:
  scripts/extract_spec.py            write docs/spec/
  scripts/extract_spec.py --check    exit 1 if docs/spec/ is out of date (for CI)
Exit status: 0 ok, 1 spec out of date (--check), 2 the tests cannot be parsed.
"""
import argparse
import collections
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEST_DIR = os.path.join(ROOT, "test", "src")
SRC_DIR = os.path.join(ROOT, "src")
SPEC_DIR = os.path.join(ROOT, "docs", "spec")

TC_START_RE = re.compile(r'\b\w*TEST_CASE\w*\(')
TC_RE = re.compile(r'\bTEST_CASE\(\s*"([^"]+)"\s*,\s*"([^"]*)"\s*\)')
SEC_RE = re.compile(r'^\s*SECTION\(\s*"([^"]+)"')
IF_RE = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b(.*)$')
ELSE_RE = re.compile(r'^\s*#\s*else\b')
ELIF_RE = re.compile(r'^\s*#\s*elif\b')
DEFINED_RE = re.compile(r'^(!|not\s+)?\s*defined\s*(?:\(\s*(\w+)\s*\)|\s(\w+))$')
ENDIF_RE = re.compile(r'^\s*#\s*endif\b')
EXC_RE = re.compile(r'^\s*//\s*IMPL-CHECK-EXCEPTION:(.*)$')
# The one form of the marker. The error message and docs/spec/README.md use
# this text, so the code and the docs cannot disagree.
MARKER_FORM = '// IMPL-CHECK-EXCEPTION: <name>: <reason>'
# <name>: <reason>. The name is one word (letters, digits, "_" and "-").
EXC_BODY_RE = re.compile(r'^\s*([A-Za-z0-9_-]+)\s*:\s*(\S.*)$')
GAP_RE = re.compile(r'^\s*//\s*SPEC-GAP:(.*)$')

# The longest TEST_CASE( ... ) that the script joins from several lines.
MAX_TEST_CASE_LINES = 10


class SpecError(Exception):
    """The tests cannot be parsed. main() prints the message and returns 2."""


# A test is an INTERACTION CHECK when it can only say "this call was made" or
# "this value was passed on unchanged". Rule 1 says tests lock behavior. These
# tests may be implementation checks. The flag is a guess from the NAME, so it is
# advisory. The script only flags them. A person decides. CI must not gate on it.
INTERACTION_RE = re.compile(
    r'(does_not_modify_|_invokes_|_shares_|_provides_|does_not_call_|does_not_invoke_|'
    r'_floats_|_configures_|_sets_.*_pin_)')

def humanize(s):
    s = re.sub(r'_if(n?)def_\w+$', '', s)   # the build variant comes from the #if guard
    s = re.sub(r'_?(\d+)$', lambda m: ' (case %s)' % m.group(1), s)
    return s.replace('_', ' ').strip()

def strip_prefix(name, unit):
    n = name[5:] if name.startswith('test_') else name
    stem = unit.lower()
    for p in (stem + '_', stem.replace('_arduino', '') + '_arduino_',
              stem.replace('_arduino', '') + '_'):
        if n.startswith(p):
            return n[len(p):]
    return n

def subject_of(name, unit):
    n = name[5:] if name.startswith('test_') else name
    m = re.match(r'make_(note_\w+?)_(instantiates|enforces|deletes)', n)
    if m:
        return 'make_' + m.group(1)
    m = re.match(r'static_callback_(note_\w+?)_(invokes|does_not|returns)', n)
    if m:
        return 'callback ' + m.group(1)
    body = strip_prefix(name, unit)
    m = re.match(r'(constructor|deconstructor|destructor|begin_i2c|begin_serial|'
                 r'receive|transmit|reset|available|print|start|stop|end|'
                 r'[A-Za-z]+(?:[A-Z][a-z]+)+)(?:_|$)', body)
    return m.group(1) if m else 'general'

def read_text(path):
    with open(path, encoding='utf-8') as fh:
        return fh.read()


def code_lines(text):
    """Return two lists, one item for each line of text.

    The first list has the code with the comments replaced by spaces. The
    second list also has the contents of string and character literals
    replaced by spaces (the quotes stay). Line numbers do not change.
    """
    code = []
    state = 'code'      # code, block (/* */), line (//), str ("), chr (')
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        two = text[i:i + 2]
        if c == '\n':
            if state == 'line':
                state = 'code'
            code.append(('\n', '\n'))
        elif state == 'block':
            if two == '*/':
                state = 'code'
                code.append(('  ', '  '))
                i += 1
            else:
                code.append((' ', ' '))
        elif state == 'line':
            code.append((' ', ' '))
        elif state in ('str', 'chr'):
            quote = '"' if state == 'str' else "'"
            if c == '\\' and i + 1 < n and text[i + 1] != '\n':
                code.append((two, '  '))
                i += 1
            elif c == quote:
                state = 'code'
                code.append((c, c))
            else:
                code.append((c, ' '))
        elif two == '/*':
            state = 'block'
            code.append(('  ', '  '))
            i += 1
        elif two == '//':
            state = 'line'
            code.append(('  ', '  '))
            i += 1
        else:
            if c == '"':
                state = 'str'
            elif c == "'" and not (i > 0 and (text[i - 1].isalnum())):
                state = 'chr'       # not a C++14 digit separator (1'000)
            code.append((c, c))
        i += 1
    with_literals = ''.join(a for a, _ in code).split('\n')
    no_literals = ''.join(b for _, b in code).split('\n')
    return with_literals, no_literals


def guard_condition(directive, expr, in_else):
    """Return the text of the condition of one guard, for example "X defined"."""
    expr = ' '.join(expr.split())
    if directive in ('ifdef', 'ifndef'):
        cond, negative = expr + ' defined', directive == 'ifndef'
        name = expr
    else:
        m = DEFINED_RE.match(expr)
        if not m:
            return 'not (%s)' % expr if in_else else expr
        name, negative = m.group(2) or m.group(3), bool(m.group(1))
    return '%s %s' % (name, 'defined' if negative == in_else else 'not defined')


def join_test_case(lines, start):
    """Return the TEST_CASE( ... ) text that begins on lines[start], or None."""
    text = ''
    for line in lines[start:start + MAX_TEST_CASE_LINES]:
        text += line + '\n'
        if text.count('(') <= text.count(')'):
            return text
    return None


def parse_tests(test_dir=TEST_DIR, gaps=None):
    """Return the test cases. Put the SPEC-GAP texts in gaps (unit -> list)."""
    if gaps is None:
        gaps = {}
    tests = []
    for fn in sorted(os.listdir(test_dir)):
        if not fn.endswith('_test.cpp'):
            continue
        guards = []
        text = read_text(os.path.join(test_dir, fn)).replace('\r\n', '\n')
        lines = text.split('\n')
        code, bare = code_lines(text)
        pending_exc = None
        seen = 0        # each "TEST_CASE(" in code (not in a comment or a string)
        parsed = 0
        for i, raw in enumerate(lines, 1):
            line = code[i - 1]
            m = IF_RE.match(line)
            if m:
                guards.append([m.group(1), m.group(2), False, i])
                continue
            if ELIF_RE.match(line):
                raise SpecError('%s:%d: #elif is not supported. Use #if, #else and #endif'
                                % (fn, i))
            if ELSE_RE.match(line) or ENDIF_RE.match(line):
                if not guards:
                    raise SpecError('%s:%d: this #else or #endif has no #if' % (fn, i))
                if ELSE_RE.match(line):
                    guards[-1][2] = True
                else:
                    guards.pop()
                continue
            m = GAP_RE.match(raw) if not line.strip() else None
            if m:
                if not m.group(1).strip():
                    raise SpecError('%s:%d: SPEC-GAP needs a text: "// SPEC-GAP: <text>"'
                                    % (fn, i))
                gaps.setdefault(fn[:-len('_test.cpp')], []).append(m.group(1).strip())
                continue
            m = EXC_RE.match(raw) if not line.strip() else None
            if m:
                body = EXC_BODY_RE.match(m.group(1))
                if not body:
                    raise SpecError('%s:%d: IMPL-CHECK-EXCEPTION needs a name and a reason: '
                                    '"%s"' % (fn, i, MARKER_FORM))
                pending_exc = (body.group(1), body.group(2).strip())
                continue
            if not line.strip():
                continue
            starts = len(TC_START_RE.findall(bare[i - 1]))
            if starts:
                seen += starts
                joined = join_test_case(code, i - 1)
                m = TC_RE.search(joined) if joined else None
                if not m:
                    raise SpecError('%s:%d: cannot parse this TEST_CASE. Use '
                                    'TEST_CASE("<name>", "[<Unit>]...") with two string '
                                    'literals' % (fn, i))
                name, tagstr = m.groups()
                tags = re.findall(r'\[([^\]]+)\]', tagstr)
                variant = ' and '.join(guard_condition(d, e, el) for d, e, el, _ in guards)
                tests.append(dict(file=fn, line=i, name=name, tags=tags,
                                  unit=tags[0] if tags else fn[:-9],
                                  variant=variant, exception=pending_exc,
                                  sections=[]))
                parsed += 1
                pending_exc = None
                continue
            m = SEC_RE.match(line)
            if m and tests and tests[-1]['file'] == fn:
                tests[-1]['sections'].append(m.group(1))
            if line.strip():
                if pending_exc:
                    raise SpecError('%s: an IMPL-CHECK-EXCEPTION comment is not directly above a '
                                    'TEST_CASE (it is above line %d)' % (fn, i))
                pending_exc = None
        if guards:
            raise SpecError('%s:%d: this #%s has no #endif' % (fn, guards[-1][3], guards[-1][0]))
        if seen != parsed:
            raise SpecError('%s: %d "TEST_CASE(" found but %d test cases were parsed'
                            % (fn, seen, parsed))
    return tests

def src_units(src_dir=SRC_DIR):
    return sorted(os.path.splitext(f)[0] for f in os.listdir(src_dir)
                  if f.endswith('.cpp'))

def gap_section(texts):
    """Return the lines of the "Open spec gaps" section (none when there is no gap)."""
    if not texts:
        return []
    return (['', '## Open spec gaps', '',
             'The docs do not specify these behaviors. No test locks them.', '']
            + ['- gap: %s' % x for x in texts])


def render(tests, units_in_src=None, gaps=None):
    gaps = gaps or {}
    out = {}
    by_unit = collections.defaultdict(list)
    for t in tests:
        by_unit[t['unit']].append(t)
    units = sorted(set(units_in_src if units_in_src is not None else src_units())
                   | set(by_unit) | set(gaps))
    GEN = ('<!-- GENERATED by scripts/extract_spec.py from test/src/*_test.cpp. '
           'Do not edit by hand. -->\n')
    for u in units:
        ts = by_unit.get(u, [])
        L = [GEN, '# Spec: %s\n' % u]
        if not ts:
            L += ['Status: **no tests**. No requirement is locked for this unit.\n',
                  'Source: `src/%s.cpp`. A requirement needs a test first.\n' % u]
            out[u + '.md'] = '\n'.join(L + gap_section(gaps.get(u)))
            continue
        inter = sum(1 for t in ts if INTERACTION_RE.search(t['name']))
        L += ['Source: `src/%s.cpp`. Tests: %d. Interaction checks (flagged): %d.\n'
              % (u, len(ts), inter)]
        groups = collections.OrderedDict()
        for t in ts:
            groups.setdefault(subject_of(t['name'], u), []).append(t)
        for g, gts in groups.items():
            L.append('\n## %s\n' % g.replace('_', ' '))
            for t in gts:
                body = humanize(strip_prefix(t['name'], u))
                kind = 'interaction' if INTERACTION_RE.search(t['name']) else 'behavior'
                extra = ''
                if t['variant']:
                    extra += ' _[variant: %s]_' % t['variant']
                L.append('- `%s` — %s%s  \n  <sub>`%s` `%s`</sub>'
                         % (kind, body, extra, t['file'], t['name']))
                for s in t['sections']:
                    L.append('  - %s' % s)
                if t['exception']:
                    L.append('  - IMPL-CHECK-EXCEPTION `%s`: %s (pending decision)' % t['exception'])
        out[u + '.md'] = '\n'.join(L + gap_section(gaps.get(u))) + '\n'
    # index
    n_int = sum(1 for t in tests if INTERACTION_RE.search(t['name']))
    I = [GEN, '# SDK spec (draft, derived from unit tests)\n',
         'Every line in this folder comes from a test name. No test means no requirement.\n',
         '| Unit | Tests | Interaction checks (flagged) | File |', '|---|---|---|---|']
    for u in units:
        ts = by_unit.get(u, [])
        ni = sum(1 for t in ts if INTERACTION_RE.search(t['name']))
        I.append('| %s | %d | %d | [%s.md](%s.md) |' % (u, len(ts), ni, u, u))
    I.append('| **Total** | **%d** | **%d** | |' % (len(tests), n_int))
    n_gap = sum(len(v) for v in gaps.values())
    I += ['', 'Open spec gaps (the docs do not specify the behavior and no test locks it): '
          '%d. See the section "Open spec gaps" in the unit file.' % n_gap]
    I += ['', '## How to read this spec', '',
          '- `behavior`: the test states an outcome that a caller can see.',
          '- `interaction`: the test name says that a call was made, or that a value was passed on',
          '  unchanged. This kind can be an implementation check.',
          '  The flag comes from a guess on the test NAME. It is ADVISORY. CI never fails because of it.',
          '  A person must decide. A test that is kept as an exception has the comment',
          '  `%s` above it. The comment shows in this spec.' % MARKER_FORM,
          '- A line with `variant` applies to one build option only.',
          '- `gap`: a behavior that the docs do not specify. No test locks it. The source is a',
          '  `// SPEC-GAP: <text>` comment in a test file.',
          '', '## How to change the spec', '',
          '1. Change or add a test (name and tags).',
          '2. Run `scripts/extract_spec.py`.',
          '3. Commit the test and the changed spec file together.',
          '4. CI runs `scripts/extract_spec.py --check`. It fails only if the spec is not up to date.']
    out['README.md'] = '\n'.join(I) + '\n'
    return out

def main(argv=None, root=ROOT):
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    a = ap.parse_args(argv)
    test_dir = os.path.join(root, 'test', 'src')
    src_dir = os.path.join(root, 'src')
    spec_dir = os.path.join(root, 'docs', 'spec')
    gaps = {}
    try:
        tests = parse_tests(test_dir, gaps)
    except (SpecError, OSError, UnicodeDecodeError) as e:
        print('ERROR: %s' % e)
        return 2
    if not tests:
        print('ERROR: no TEST_CASE found in %s' % test_dir)
        return 2
    bad = [t for t in tests if t['tags'] and t['tags'][0] + '_test.cpp' != t['file']]
    for t in bad:
        print('warning: first tag [%s] does not match file %s' % (t['tags'][0], t['file']))
    out = render(tests, src_units(src_dir), gaps)
    if a.check:
        stale = []
        for fn, txt in out.items():
            p = os.path.join(spec_dir, fn)
            if not os.path.exists(p) or read_text(p) != txt:
                stale.append(fn)
        extra = set(os.listdir(spec_dir)) - set(out) if os.path.isdir(spec_dir) else set()
        if stale or extra:
            print('spec is out of date: %s %s' % (sorted(stale), sorted(extra)))
            return 1
        print('spec is up to date (%d tests, %d files)' % (len(tests), len(out)))
        return 0
    os.makedirs(spec_dir, exist_ok=True)
    for fn, txt in out.items():
        with open(os.path.join(spec_dir, fn), 'w', encoding='utf-8', newline='\n') as fh:
            fh.write(txt)
    print('wrote %d files for %d tests to %s' % (len(out), len(tests), spec_dir))
    return 0


if __name__ == '__main__':
    sys.exit(main())
