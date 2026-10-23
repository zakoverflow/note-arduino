#!/usr/bin/env python3
"""Generate docs/spec/*.md from the unit tests.

The spec is DERIVED from the tests. Do not edit docs/spec/*.md by hand.
Change a test name or tag, then run this script again.

What the script reads (test/src/*_test.cpp):
  TEST_CASE("<name>", "[<Unit>][<tag>]...")   one requirement per test case
  SECTION("<name>")                            sub-requirement of a test case
  #if / #ifdef guards around a test case       build variant of the requirement
  // IMPL-CHECK-EXCEPTION: <reason>            comment directly above a test case

Naming convention that the script relies on:
  test_<unit>_<subject>_<verb phrase that states the requirement>
  The first tag is the unit (module). It must equal the source file stem.
  Extra tags are optional group keys, for example [receive] or [wire].

Usage:
  scripts/extract_spec.py            write docs/spec/
  scripts/extract_spec.py --check    exit 1 if docs/spec/ is out of date (for CI)
"""
import argparse, collections, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEST_DIR = os.path.join(ROOT, "test", "src")
SRC_DIR = os.path.join(ROOT, "src")
SPEC_DIR = os.path.join(ROOT, "docs", "spec")

TC_RE = re.compile(r'^\s*TEST_CASE\(\s*"([^"]+)"\s*,\s*"([^"]*)"\s*\)')
SEC_RE = re.compile(r'^\s*SECTION\(\s*"([^"]+)"')
IF_RE = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b(.*)$')
ELSE_RE = re.compile(r'^\s*#\s*(else|elif)\b')
ENDIF_RE = re.compile(r'^\s*#\s*endif\b')
EXC_RE = re.compile(r'^\s*//\s*IMPL-CHECK-EXCEPTION:\s*(.*)$')

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

def parse_tests():
    tests = []
    for fn in sorted(os.listdir(TEST_DIR)):
        if not fn.endswith('_test.cpp'):
            continue
        guards, lines = [], open(os.path.join(TEST_DIR, fn)).read().splitlines()
        pending_exc = None
        for i, line in enumerate(lines, 1):
            m = IF_RE.match(line)
            if m:
                guards.append(((m.group(1) + m.group(2)).strip(), False)); continue
            if ELSE_RE.match(line) and guards:
                g, _ = guards[-1]; guards[-1] = (g, True); continue
            if ENDIF_RE.match(line) and guards:
                guards.pop(); continue
            m = EXC_RE.match(line)
            if m:
                pending_exc = m.group(1).strip(); continue
            m = TC_RE.match(line)
            if m:
                name, tagstr = m.groups()
                tags = re.findall(r'\[([^\]]+)\]', tagstr)
                variant = ''
                for g, neg in guards:
                    if 'WIRE_HAS_END' in g:
                        variant = ('WIRE_HAS_END defined' if ('not' in g or 'ifndef' in g) == neg
                                   else 'WIRE_HAS_END not defined')
                tests.append(dict(file=fn, line=i, name=name, tags=tags,
                                  unit=tags[0] if tags else fn[:-9],
                                  variant=variant, exception=pending_exc,
                                  sections=[]))
                pending_exc = None
                continue
            m = SEC_RE.match(line)
            if m and tests and tests[-1]['file'] == fn:
                tests[-1]['sections'].append(m.group(1))
            if line.strip() and not line.strip().startswith('//'):
                pending_exc = None
    return tests

def src_units():
    return sorted(os.path.splitext(f)[0] for f in os.listdir(SRC_DIR)
                  if f.endswith('.cpp'))

def render(tests):
    out = {}
    by_unit = collections.defaultdict(list)
    for t in tests:
        by_unit[t['unit']].append(t)
    units = sorted(set(src_units()) | set(by_unit))
    GEN = ('<!-- GENERATED by scripts/extract_spec.py from test/src/*_test.cpp. '
           'Do not edit by hand. -->\n')
    for u in units:
        ts = by_unit.get(u, [])
        L = [GEN, '# Spec: %s\n' % u]
        if not ts:
            L += ['Status: **no tests**. No requirement is locked for this unit.\n',
                  'Source: `src/%s.cpp`. A requirement needs a test first.\n' % u]
            out[u + '.md'] = '\n'.join(L); continue
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
                    L.append('  - IMPL-CHECK-EXCEPTION: %s (pending decision)' % t['exception'])
        out[u + '.md'] = '\n'.join(L) + '\n'
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
    I += ['', '## How to read this spec', '',
          '- `behavior`: the test states an outcome that a caller can see.',
          '- `interaction`: the test name says that a call was made, or that a value was passed on',
          '  unchanged. This kind can be an implementation check.',
          '  The flag comes from a guess on the test NAME. It is ADVISORY. CI never fails because of it.',
          '  A person must decide (see `IMPL-CHECK-EXCEPTION`).',
          '- A line with `variant` applies to one build option only.',
          '', '## How to change the spec', '',
          '1. Change or add a test (name and tags).',
          '2. Run `scripts/extract_spec.py`.',
          '3. Commit the test and the changed spec file together.',
          '4. CI runs `scripts/extract_spec.py --check`. It fails only if the spec is not up to date.']
    out['README.md'] = '\n'.join(I) + '\n'
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    a = ap.parse_args()
    tests = parse_tests()
    if not tests:
        sys.exit('no TEST_CASE found')
    bad = [t for t in tests if t['tags'] and t['tags'][0] + '_test.cpp' != t['file']]
    for t in bad:
        print('warning: first tag [%s] does not match file %s' % (t['tags'][0], t['file']))
    out = render(tests)
    if a.check:
        stale = []
        for fn, txt in out.items():
            p = os.path.join(SPEC_DIR, fn)
            if not os.path.exists(p) or open(p).read() != txt:
                stale.append(fn)
        extra = set(os.listdir(SPEC_DIR)) - set(out) if os.path.isdir(SPEC_DIR) else set()
        if stale or extra:
            print('spec is out of date: %s %s' % (sorted(stale), sorted(extra)))
            sys.exit(1)
        print('spec is up to date (%d tests, %d files)' % (len(tests), len(out)))
        return
    os.makedirs(SPEC_DIR, exist_ok=True)
    for fn, txt in out.items():
        open(os.path.join(SPEC_DIR, fn), 'w').write(txt)
    print('wrote %d files for %d tests to %s' % (len(out), len(tests), SPEC_DIR))

if __name__ == '__main__':
    main()
