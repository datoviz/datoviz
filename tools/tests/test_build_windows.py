"""Exercise Windows build policy using fixtures and mocked subprocesses."""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from tasks import build_windows


class BuildWindowsTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.build = self.root / 'build'

    def write(self, name, text=''):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding='utf-8')
        return path

    def cache(self, generator='Ninja', compiler='C:/VS/bin/cl.exe', source=None, extras=''):
        return self.write('build/CMakeCache.txt',
                          f'CMAKE_GENERATOR:INTERNAL={generator}\n'
                          f'CMAKE_C_COMPILER:FILEPATH={compiler}\n'
                          f'CMAKE_CXX_COMPILER:FILEPATH={compiler}\n'
                          f'CMAKE_HOME_DIRECTORY:INTERNAL={source or self.root}\n{extras}')

    def test_cache_accepts_only_matching_ninja_msvc_checkout(self):
        self.cache()
        build_windows.guard_cache(self.build, self.root)
        for options in ({'generator': 'Visual Studio 17 2022'},
                        {'compiler': 'C:/LLVM/clang-cl.exe'},
                        {'source': self.root / 'another-checkout'}):
            with self.subTest(options=options):
                path = self.cache(**options)
                original = path.read_bytes()
                with self.assertRaisesRegex(RuntimeError, 'Refusing existing cache'):
                    build_windows.guard_cache(self.build, self.root)
                self.assertEqual(path.read_bytes(), original)

    def test_fresh_build_needs_no_cache(self):
        build_windows.guard_cache(self.build, self.root)

    def test_posix_quoting_keeps_spaces_and_never_evaluates_shell_text(self):
        arguments = build_windows.cmake_arguments(
            '-DMEDIA="C:/Program Files/assets" \'-DTEXT=$(whoami); literal\'')
        self.assertEqual(arguments, ['-DMEDIA=C:/Program Files/assets', '-DTEXT=$(whoami); literal'])
        with self.assertRaisesRegex(RuntimeError, 'cannot override'):
            build_windows.cmake_arguments('-Belsewhere')

    def test_configured_runtime_directory_and_configuration_are_respected(self):
        installed = self.root / 'packages outside build'
        self.cache(extras=f'VCPKG_INSTALLED_DIR:PATH={installed}\n'
                   'VCPKG_TARGET_TRIPLET:STRING=x64-windows\n')
        self.write('packages outside build/x64-windows/debug/bin/codec.dll', 'debug')
        self.write('packages outside build/x64-windows/bin/codec.dll', 'release')
        self.write('packages outside build/x64-windows/bin/ignored.txt', 'ignore')
        self.write('build/src/datoviz.dll', 'datoviz')
        self.write('build/testing/test.exe')
        self.write('build/examples/nested/demo.exe')
        unrelated = self.write('build/testing/unrelated.dll', 'keep')
        for configuration, expected in [('Debug', 'debug'), ('Release', 'release')]:
            with self.subTest(configuration=configuration):
                copies = build_windows.stage_runtime(self.build, configuration)
                self.assertEqual(copies, 5)
                for directory in ('src', 'testing', 'examples/nested'):
                    self.assertEqual((self.build / directory / 'codec.dll').read_text(), expected)
                    self.assertFalse((self.build / directory / 'ignored.txt').exists())
                for directory in ('testing', 'examples/nested'):
                    self.assertEqual((self.build / directory / 'datoviz.dll').read_text(), 'datoviz')
                self.assertEqual(unrelated.read_text(), 'keep')

    def test_existing_developer_environment_needs_no_subprocess(self):
        environment = {'Path': 'developer-tools', 'VSINSTALLDIR': 'C:/VS', 'SECRET': 'hidden'}
        with mock.patch.object(build_windows.shutil, 'which', return_value='cl.exe') as which:
            with mock.patch.object(build_windows.subprocess, 'run') as run:
                result = build_windows.initialize_msvc(environment)
        self.assertEqual(result, environment)
        which.assert_called_once_with('cl', path='developer-tools')
        run.assert_not_called()

    def test_vswhere_initializes_and_preserves_environment_without_printing_it(self):
        self.write('Program Files/Microsoft Visual Studio/Installer/vswhere.exe')
        self.write('VS/Common7/Tools/VsDevCmd.bat')
        environment = {'ProgramFiles(x86)': str(self.root / 'Program Files'),
                       'PATH': 'old', 'SECRET': 'sensitive'}
        results = [SimpleNamespace(returncode=0, stdout=str(self.root / 'VS') + '\n'),
                   SimpleNamespace(returncode=0, stdout='Path=new\nVSINSTALLDIR=C:/VS\nX=a=b\n')]
        with mock.patch.object(build_windows.shutil, 'which', side_effect=[None, 'cl.exe']):
            with mock.patch.object(build_windows.subprocess, 'run', side_effect=results) as run:
                with contextlib.redirect_stdout(io.StringIO()) as output:
                    result = build_windows.initialize_msvc(environment)
        self.assertEqual(output.getvalue(), '')
        self.assertEqual(result['Path'], 'new')
        self.assertNotIn('PATH', result)
        self.assertEqual(result['SECRET'], 'sensitive')
        self.assertEqual(result['X'], 'a=b')
        self.assertIn('-arch=x64', run.call_args_list[1].args[0])
        self.assertIn('/s /c "call "', run.call_args_list[1].args[0])
        self.assertTrue(run.call_args_list[1].kwargs['capture_output'])

    def test_vcpkg_explicit_root_and_visual_studio_fallback(self):
        root = self.root / 'vcpkg'
        self.write('vcpkg/scripts/buildsystems/vcpkg.cmake')
        self.assertEqual(build_windows.find_vcpkg({'VCPKG_ROOT': str(root)})[0], root)
        bundled = self.root / 'VS/VC/vcpkg'
        self.write('VS/VC/vcpkg/scripts/buildsystems/vcpkg.cmake')
        self.assertEqual(build_windows.find_vcpkg({'VSINSTALLDIR': str(self.root / 'VS')})[0], bundled)
        with self.assertRaisesRegex(RuntimeError, 'Set VCPKG_ROOT'):
            build_windows.find_vcpkg({})

    def test_non_windows_error_is_clear(self):
        with mock.patch.object(build_windows, 'IS_WINDOWS', False):
            with contextlib.redirect_stderr(io.StringIO()) as error:
                self.assertEqual(build_windows.main(['Debug']), 1)
        self.assertIn('requires Windows', error.getvalue())

    def test_missing_yaml_stops_before_environment_or_configure(self):
        with mock.patch.object(build_windows, 'IS_WINDOWS', True):
            with mock.patch.object(build_windows.importlib.util, 'find_spec', return_value=None):
                with mock.patch.object(build_windows, 'initialize_msvc') as initialize:
                    with contextlib.redirect_stderr(io.StringIO()) as error:
                        self.assertEqual(build_windows.main(['Release']), 1)
        initialize.assert_not_called()
        self.assertIn('needs PyYAML', error.getvalue())

    def test_configure_uses_selected_python_toolchain_and_extra_options(self):
        self.cache()
        toolchain = self.write('vcpkg/scripts/buildsystems/vcpkg.cmake')
        environment = {'DVZ_CMAKE_ARGS': '-DFEATURE=ON \'-DASSETS=C:/Some Assets\'',
                       'DVZ_BUILD_JOBS': '3'}
        with contextlib.ExitStack() as stack:
            stack.enter_context(mock.patch.object(build_windows, 'IS_WINDOWS', True))
            stack.enter_context(mock.patch.object(build_windows, '__file__',
                                                  str(self.root / 'tasks/build_windows.py')))
            stack.enter_context(mock.patch.object(build_windows.importlib.util, 'find_spec',
                                                  return_value=object()))
            stack.enter_context(mock.patch.object(build_windows, 'initialize_msvc',
                                                  return_value=environment))
            stack.enter_context(mock.patch.object(build_windows, 'find_vcpkg',
                                                  return_value=(toolchain.parents[2], toolchain)))
            run = stack.enter_context(mock.patch.object(build_windows.subprocess, 'run'))
            stage = stack.enter_context(mock.patch.object(build_windows, 'stage_runtime'))
            self.assertEqual(build_windows.main(['Debug', '--configure-only']), 0)
        stage.assert_not_called()
        run.assert_called_once()
        arguments = run.call_args.args[0]
        self.assertEqual(arguments[:3], ['cmake', '--preset', 'msvc-ninja'])
        self.assertIn('-DCMAKE_BUILD_TYPE=Debug', arguments)
        self.assertIn(f'-DPython3_EXECUTABLE={build_windows.sys.executable}', arguments)
        self.assertIn(f'-DCMAKE_TOOLCHAIN_FILE={toolchain}', arguments)
        self.assertIn('-DVCPKG_TARGET_TRIPLET=x64-windows', arguments)
        self.assertIn('-DASSETS=C:/Some Assets', arguments)
        self.assertEqual(run.call_args.kwargs['cwd'], self.root)

    def test_build_honors_job_count_then_stages_selected_configuration(self):
        self.cache()
        toolchain = self.write('vcpkg/scripts/buildsystems/vcpkg.cmake')
        with contextlib.ExitStack() as stack:
            stack.enter_context(mock.patch.object(build_windows, 'IS_WINDOWS', True))
            stack.enter_context(mock.patch.object(build_windows, '__file__',
                                                  str(self.root / 'tasks/build_windows.py')))
            stack.enter_context(mock.patch.object(build_windows.importlib.util, 'find_spec',
                                                  return_value=object()))
            stack.enter_context(mock.patch.object(build_windows, 'initialize_msvc',
                                                  return_value={'DVZ_BUILD_JOBS': '5'}))
            stack.enter_context(mock.patch.object(build_windows, 'find_vcpkg',
                                                  return_value=(toolchain.parents[2], toolchain)))
            run = stack.enter_context(mock.patch.object(build_windows.subprocess, 'run'))
            stage = stack.enter_context(mock.patch.object(build_windows, 'stage_runtime', return_value=4))
            stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
            self.assertEqual(build_windows.main(['Release']), 0)
        self.assertEqual(run.call_args_list[1].args[0],
                         ['cmake', '--build', str(self.build), '--parallel', '5'])
        stage.assert_called_once_with(self.build, 'Release')

    def test_missing_vswhere_and_failed_developer_command_are_clear_errors(self):
        with mock.patch.object(build_windows.shutil, 'which', return_value=None):
            with self.assertRaisesRegex(RuntimeError, 'vswhere.exe was not found'):
                build_windows.initialize_msvc({})
        self.write('Program Files/Microsoft Visual Studio/Installer/vswhere.exe')
        self.write('VS/Common7/Tools/VsDevCmd.bat')
        results = [SimpleNamespace(returncode=0, stdout=str(self.root / 'VS')),
                   SimpleNamespace(returncode=1, stdout='SECRET=must-not-be-printed')]
        with mock.patch.object(build_windows.shutil, 'which', return_value=None):
            with mock.patch.object(build_windows.subprocess, 'run', side_effect=results):
                with self.assertRaisesRegex(RuntimeError, 'failed to initialize') as error:
                    build_windows.initialize_msvc({'ProgramFiles(x86)': str(self.root / 'Program Files')})
        self.assertNotIn('SECRET', str(error.exception))


if __name__ == '__main__':
    unittest.main()
