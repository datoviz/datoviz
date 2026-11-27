"""Stack-report fixtures exercise coverage and failure modes without a compiler."""

import contextlib
import io
import json
import os
import tempfile
import unittest
from pathlib import Path

from tools.check_stack_usage import audit, gcc_compiler, main, parse_report


class StackUsageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.commands = []

    def write(self, name, content=''):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding='utf-8')
        os.utime(path, ns=(1_000_000_000, 1_000_000_000))
        return path

    def unit(self, source='src/demo.c', flags=('-fstack-usage',), report=True):
        self.write(source)
        output = 'build/' + Path(source).name + '.o'
        self.write(output)
        if report:
            self.write(str(Path(output).with_suffix('.su')), f'{source}:2:1:demo\t32\tstatic\n')
        command = {'directory': str(self.root), 'file': source,
                   'arguments': ['gcc', *flags, '-c', source, '-o', output]}
        self.commands.append(command)
        return command

    def database(self):
        return self.write('build/compile_commands.json', json.dumps(self.commands))

    def test_parser_preserves_small_bounded_dynamic_and_cpp_names(self):
        path = self.write('sample.su',
                          'demo.c:1:2:tiny\t8\tstatic\n'
                          'demo.cpp:3:4:void Thing::run(int)\t128\tdynamic,bounded\n'
                          'demo.c:5:6:unknown\t64\tdynamic\n')
        frames, errors = parse_report(path)
        self.assertEqual(errors, [])
        self.assertEqual([f['bytes'] for f in frames], [8, 128, 64])
        self.assertEqual([f['unbounded'] for f in frames], [False, False, True])
        self.assertEqual(frames[1]['function'], 'void Thing::run(int)')

    def test_parser_rejects_unknown_and_malformed_records(self):
        path = self.write('sample.su', 'broken\ndemo.c:1:1:f\t8\tmystery\n')
        frames, errors = parse_report(path)
        self.assertEqual(frames, [])
        self.assertEqual(len(errors), 2)

    def test_object_suffix_and_missing_report_are_covered(self):
        self.unit(report=False)
        result = audit(self.database(), self.root)
        self.assertFalse(result['ok'])
        self.assertTrue(result['units'][0]['report'].endswith('demo.c.su'))
        self.assertIn('missing report:', '\n'.join(result['units'][0]['errors']))

    def test_missing_stack_flag_and_disabled_flag_fail_coverage(self):
        self.unit('src/absent.c', flags=())
        self.unit('src/disabled.cpp', flags=('-fstack-usage', '-fno-stack-usage'))
        result = audit(self.database(), self.root)
        self.assertEqual(result['summary']['complete_units'], 0)
        self.assertTrue(all('enable -fstack-usage' in unit['errors'][0]
                            for unit in result['units']))

    def test_source_not_object_path_controls_inventory(self):
        self.unit()
        external = self.unit('external/third.cpp', flags=(), report=False)
        external['arguments'][-1] = 'build/src/third.cpp.o'
        self.unit('src/demo/tests/test_demo.c', flags=(), report=False)
        self.write('src/optional/disabled.c')
        result = audit(self.database(), self.root)
        self.assertTrue(result['ok'])
        self.assertEqual(result['summary']['configured_units'], 1)
        self.assertEqual(len(result['excluded_commands']), 2)
        self.assertEqual(result['uncompiled_sources'],
                         [str(self.root / 'src/optional/disabled.c')])

    def test_stale_source_and_depfile_header_are_not_passes(self):
        self.unit(flags=('-fstack-usage', '-MF', 'build/demo.c.o.d'))
        header = self.write('include/a header.h')
        self.write('build/demo.c.o.d', 'build/demo.c.o: src/demo.c \\\n include/a\\ header.h\n')
        os.utime(header, ns=(2_000_000_000, 2_000_000_000))
        result = audit(self.database(), self.root)
        unit = result['units'][0]
        self.assertEqual(unit['header_freshness'], 'depfile')
        self.assertEqual(sum('stale' in error for error in unit['errors']), 2)
        self.assertFalse(result['ok'])

    def test_source_freshness_without_depfile(self):
        self.unit()
        os.utime(self.root / 'src/demo.c', ns=(2_000_000_000, 2_000_000_000))
        result = audit(self.database(), self.root)
        self.assertEqual(result['units'][0]['header_freshness'], 'unverified')
        self.assertEqual(sum('stale' in error for error in result['units'][0]['errors']), 2)

    def test_threshold_and_unknown_with_explicit_check(self):
        self.unit()
        self.write('build/demo.c.su',
                   'demo.c:1:1:at_limit\t16384\tdynamic,bounded\n'
                   'demo.c:2:1:large\t16385\tstatic\n'
                   'demo.c:3:1:unknown\t16\tdynamic\n'
                   'demo.c:4:1:tiny\t8\tstatic\n')
        database = self.database()
        result = audit(database, self.root)
        self.assertEqual([f['function'] for f in result['violations']], ['unknown', 'large'])
        argv = ['--root', str(self.root), '--compile-commands', str(database), '--json']
        with contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(main(argv), 0)
        self.assertEqual(json.loads(output.getvalue())['summary']['frames'], 4)
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(main(argv + ['--check']), 1)

    def test_bounded_dynamic_is_not_automatically_failure(self):
        self.unit()
        self.write('build/demo.c.su', 'demo.c:1:1:bounded\t128\tdynamic,bounded\n')
        self.assertTrue(audit(self.database(), self.root)['ok'])

    def test_empty_inventory_does_not_pass(self):
        self.assertFalse(audit(self.database(), self.root)['ok'])

    def test_clang_response_file_and_missing_object_are_incomplete(self):
        command = self.unit()
        command['arguments'][0] = 'clang'
        command['arguments'].insert(1, '@flags.rsp')
        (self.root / 'build/demo.c.o').unlink()
        result = audit(self.database(), self.root)
        errors = '\n'.join(result['units'][0]['errors'])
        self.assertIn('not identifiable as GCC', errors)
        self.assertIn('response files', errors)
        self.assertIn('missing object', errors)
        self.assertFalse(result['ok'])

    def test_compiler_names_and_launchers(self):
        self.assertTrue(gcc_compiler(['ccache', '/usr/bin/x86_64-linux-gnu-g++-12']))
        self.assertTrue(gcc_compiler(['/opt/compiler/gcc']))
        self.assertFalse(gcc_compiler(['clang++']))
        self.assertFalse(gcc_compiler(['unknown-compiler']))

    def test_shell_command_joined_output_and_duplicate_commands(self):
        command = self.unit()
        command['command'] = 'ccache gcc -fstack-usage -c src/demo.c -obuild/demo.c.o'
        del command['arguments']
        self.commands.append(command.copy())
        result = audit(self.database(), self.root)
        self.assertTrue(result['ok'])
        self.assertEqual(result['summary']['configured_units'], 1)

    def test_distinct_compile_commands_each_need_evidence(self):
        self.unit()
        command = self.commands[0].copy()
        command['arguments'] = [*command['arguments'][:-1], 'build/other/demo.c.o']
        self.commands.append(command)
        result = audit(self.database(), self.root)
        self.assertEqual(result['summary']['configured_units'], 2)
        self.assertEqual(result['summary']['complete_units'], 1)
        self.assertFalse(result['ok'])

    def test_empty_report_covers_preprocessor_disabled_translation_unit(self):
        self.unit()
        self.write('build/demo.c.su', '')
        result = audit(self.database(), self.root)
        self.assertEqual(result['summary']['empty_reports'], 1)
        self.assertTrue(result['units'][0]['empty_report'])
        self.assertTrue(result['ok'])


if __name__ == '__main__':
    unittest.main()
