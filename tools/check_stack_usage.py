#!/usr/bin/env python3
"""Inventory GCC stack-usage reports for configured Datoviz production sources."""

import argparse
import json
import re
import shlex
import shutil
import sys
from pathlib import Path


SOURCE_SUFFIXES = {'.c', '.cc', '.cpp', '.cxx', '.C'}
EXCLUDED_PARTS = {'tests', 'external', 'vendor', 'vendored', 'third_party', 'third-party'}
LIMITATION = (
    'Freshness checks use source and available GCC depfile timestamps. Without a depfile, '
    'header freshness is unverified. Compiler flags, compiler binaries, environment changes, '
    'and transitive call-stack depth are not verified; use a clean build for exact evidence.'
)


def production_source(path, root):
    """Identify owned production sources by source path, never object path."""
    try:
        relative = path.resolve().relative_to((root / 'src').resolve())
    except ValueError:
        return False
    return path.suffix in SOURCE_SUFFIXES and not EXCLUDED_PARTS.intersection(relative.parts)


def resolve_path(value, directory):
    """Resolve a compile-command path relative to its working directory."""
    path = Path(value)
    return (directory / path).resolve() if not path.is_absolute() else path.resolve()


def option_value(arguments, option):
    """Return the last separate or joined GCC option value."""
    value = None
    for index, arg in enumerate(arguments):
        if arg == option:
            value = arguments[index + 1] if index + 1 < len(arguments) else None
        elif arg.startswith(option) and len(arg) > len(option):
            value = arg[len(option):]
    return value


def gcc_compiler(arguments):
    """Identify GCC commands, allowing common launchers and resolved cc aliases."""
    index = 0
    while index < len(arguments) and Path(arguments[index]).name in {'ccache', 'sccache', 'distcc'}:
        index += 1
    if index == len(arguments):
        return False
    compiler = Path(arguments[index])
    name = compiler.name
    if name in {'cc', 'c++'}:
        executable = shutil.which(str(compiler))
        if executable:
            name = Path(executable).resolve().name
    return re.fullmatch(r'(?:.*-)?(?:gcc|g\+\+)(?:-\d+(?:\.\d+)*)?', name) is not None


def parse_report(path):
    """Read GCC tab-separated records without dropping small or dynamic frames."""
    frames = []
    errors = []
    for number, line in enumerate(path.read_text(encoding='utf-8').splitlines(), 1):
        if not line.strip():
            continue
        fields = line.rsplit('\t', 2)
        if len(fields) != 3 or not fields[1].isdigit():
            errors.append(f'{path}:{number}: malformed stack-usage record')
            continue
        location, size, qualifier = fields
        match = re.match(r'^(.*):(\d+):(\d+):(.*)$', location)
        qualifiers = {part.strip() for part in qualifier.split(',')}
        if not match or qualifiers not in ({'static'}, {'dynamic'}, {'dynamic', 'bounded'}):
            errors.append(f'{path}:{number}: unsupported stack-usage record')
            continue
        frames.append({
            'file': match[1], 'line': int(match[2]), 'column': int(match[3]),
            'function': match[4], 'bytes': int(size), 'qualifier': qualifier,
            'unbounded': qualifiers == {'dynamic'},
        })
    return frames, errors


def dependencies(path, directory):
    """Read dependencies from GCC's first Make rule, including escaped spaces."""
    content = path.read_text(encoding='utf-8').replace('\\\n', '')
    first_rule = content.splitlines()[0]
    # GCC emits a target followed by a colon and whitespace, then prerequisites.
    match = re.search(r':\s', first_rule)
    if match is None:
        raise ValueError('missing dependency rule')
    return [resolve_path(item, directory) for item in shlex.split(first_rule[match.end():])]


def audit(database, root, limit=16384):
    """Collect complete configured-unit coverage and frame evidence deterministically."""
    database = database.resolve()
    root = root.resolve()
    commands = json.loads(database.read_text(encoding='utf-8'))
    if not isinstance(commands, list):
        raise ValueError('compilation database must contain an array')
    units = []
    excluded_commands = []
    configured = set()
    seen = set()
    for command in commands:
        directory = resolve_path(command['directory'], database.parent)
        source = resolve_path(command['file'], directory)
        if not production_source(source, root):
            excluded_commands.append(str(source))
            continue
        configured.add(source)
        arguments = command.get('arguments')
        if arguments is None:
            arguments = shlex.split(command['command'])
        identity = (str(directory), str(source), tuple(arguments))
        if identity in seen:
            continue
        seen.add(identity)
        errors = []
        unit = {'source': str(source), 'object': None, 'report': None,
                'header_freshness': 'unverified', 'empty_report': False,
                'errors': errors, 'frames': []}
        units.append(unit)
        if any(arg.startswith('@') for arg in arguments):
            errors.append('response files are unsupported; expand the compile command')
        stack_flags = [arg for arg in arguments if arg in ('-fstack-usage', '-fno-stack-usage')]
        if not stack_flags or stack_flags[-1] != '-fstack-usage':
            errors.append('compile command does not enable -fstack-usage')
        if not gcc_compiler(arguments):
            errors.append('compiler is not identifiable as GCC; no GCC stack-usage coverage')
        output = option_value(arguments, '-o')
        if output is None:
            errors.append('compile command has no -o object output')
            continue
        obj = resolve_path(output, directory)
        report = obj.with_suffix('.su')
        unit.update(object=str(obj), report=str(report))
        inputs = [source]
        dep_option = option_value(arguments, '-MF')
        depfile = resolve_path(dep_option, directory) if dep_option else Path(str(obj) + '.d')
        if depfile.is_file():
            try:
                inputs.extend(dependencies(depfile, directory))
                unit['header_freshness'] = 'depfile'
            except (OSError, ValueError, IndexError) as exc:
                errors.append(f'cannot read depfile {depfile}: {exc}')
        elif dep_option:
            errors.append(f'missing depfile: {depfile}')
        newest = 0
        for path in sorted(set(inputs)):
            if not path.is_file():
                errors.append(f'missing input: {path}')
            else:
                newest = max(newest, path.stat().st_mtime_ns)
        for label, path in (('object', obj), ('report', report)):
            if not path.is_file():
                errors.append(f'missing {label}: {path}')
            elif path.stat().st_mtime_ns < newest:
                errors.append(f'stale {label}: {path}')
        if report.is_file():
            frames, parse_errors = parse_report(report)
            unit['frames'] = frames
            errors.extend(parse_errors)
            # Preprocessor-disabled translation units legitimately contain no functions.
            unit['empty_report'] = not frames and not parse_errors
        unit['frames'].sort(key=lambda frame: (
            -frame['bytes'], frame['file'], frame['line'], frame['function']))
    units.sort(key=lambda unit: (unit['source'], unit['object'] or ''))
    uncompiled = sorted(str(path.resolve()) for path in (root / 'src').rglob('*')
                        if path.is_file() and production_source(path, root)
                        and path.resolve() not in configured)
    violations = []
    for unit in units:
        for frame in unit['frames']:
            if frame['unbounded'] or frame['bytes'] > limit:
                violations.append(dict(frame, source=unit['source'], report=unit['report']))
    violations.sort(key=lambda frame: (
        not frame['unbounded'], -frame['bytes'], frame['source'], frame['function']))
    return {
        'compile_commands': str(database), 'root': str(root), 'limit': limit,
        'limitation': LIMITATION, 'units': units, 'uncompiled_sources': uncompiled,
        'excluded_commands': sorted(set(excluded_commands)), 'violations': violations,
        'summary': {
            'configured_units': len(units),
            'complete_units': sum(not unit['errors'] for unit in units),
            'empty_reports': sum(unit['empty_report'] for unit in units),
            'frames': sum(len(unit['frames']) for unit in units),
            'violations': len(violations), 'uncompiled_sources': len(uncompiled),
        },
        'ok': bool(units) and not violations and all(not unit['errors'] for unit in units),
    }


def main(argv=None):
    """Print inventory or enforce the explicitly requested per-function frame gate."""
    parser = argparse.ArgumentParser(description=__doc__, epilog=LIMITATION)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--compile-commands', type=Path, default=Path('build/compile_commands.json'))
    parser.add_argument('--limit', type=int, default=16384, help='maximum bounded frame bytes')
    parser.add_argument('--top', type=int, default=20, help='number of frames printed (JSON includes all)')
    parser.add_argument('--json', action='store_true', help='write the complete inventory as JSON')
    parser.add_argument('--check', action='store_true', help='fail for oversized/unbounded frames or incomplete configured-unit evidence')
    args = parser.parse_args(argv)
    if args.limit < 0 or args.top < 0:
        parser.error('--limit and --top must be nonnegative')
    try:
        result = audit(args.compile_commands, args.root, args.limit)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f'stack-usage inventory failed: {exc}', file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        summary = result['summary']
        print(f"Stack evidence: {summary['complete_units']}/{summary['configured_units']} units; "
              f"{summary['frames']} frames ({summary['empty_reports']} empty reports); "
              f"{summary['violations']} frame violations; "
              f"{summary['uncompiled_sources']} uncompiled/excluded production sources")
        frames = [dict(frame, source=unit['source'])
                  for unit in result['units'] for frame in unit['frames']]
        frames.sort(key=lambda frame: (
            not frame['unbounded'], -frame['bytes'], frame['source'], frame['function']))
        for frame in frames[:args.top]:
            size = f"{frame['bytes']}+ unknown" if frame['unbounded'] else str(frame['bytes'])
            print(f"{size:>16}  {frame['qualifier']:<16} "
                  f"{frame['file']}:{frame['line']}:{frame['function']}")
        for unit in result['units']:
            for error in unit['errors']:
                print(f"ERROR {unit['source']}: {error}")
        for source in result['uncompiled_sources'][:args.top]:
            print(f'UNCOMPILED/EXCLUDED {source}')
        omitted = len(result['uncompiled_sources']) - args.top
        if omitted > 0:
            print(f'{omitted} more uncompiled/excluded sources (see --json)')
        print(LIMITATION)
    return 1 if args.check and not result['ok'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
