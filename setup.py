import os
import shutil
import sys
from pathlib import Path
from setuptools import Extension, setup
import distutils.cygwinccompiler

import numpy as np
from Cython.Build import cythonize

distutils.cygwinccompiler.get_msvcr = lambda: []

ROOT_DIR = Path(__file__).resolve().parent
INCLUDE_DIR = ROOT_DIR / 'include'
BUILD_DIR = ROOT_DIR / 'build'
VULKAN_DIR = Path(os.environ.get('VULKAN_SDK', '.')).resolve()

DESCRIPTION = 'High-performance interactive scientific visualization with Vulkan'

with open('requirements.txt') as f:
    require = [x.strip() for x in f.readlines() if not x.startswith('git+')]

# On Windows, copy libdatoviz.dll alonside the Cython module and bundle it in the wheel.
package_data = {}
if sys.platform == 'win32':
    shutil.copy(BUILD_DIR / 'libdatoviz.dll',
                ROOT_DIR / 'datoviz/libdatoviz.dll')
    package_data = {'datoviz': ['*.dll']}

# NOTE: build with dynamic linking of datoviz. Need to add to LD_LIBRARY_PATH env variable
# the path to the datoviz library (in <root>/build/).
setup(
    name='datoviz',
    version='0.2.0a1',
    description=DESCRIPTION,
    author='Cyrille Rossant, International Brain Laboratory',
    author_email='rossant@users.noreply.github.com',
    url='https://datoviz.org',
    long_description=DESCRIPTION,
    packages=['datoviz'],
    package_data=package_data,
    install_requires=require,
    ext_modules=cythonize(
        [
            Extension(
                'datoviz.app', ['datoviz/app.pyx'],
                libraries=['datoviz'],
                include_dirs=[
                    np.get_include(),
                    str(INCLUDE_DIR),
                    str(VULKAN_DIR / 'include'),
                    str(ROOT_DIR / 'external/'),
                    str(BUILD_DIR / '_deps/cglm-src/include'),
                ],
                library_dirs=[str(BUILD_DIR)],
                undef_macros=['NDEBUG'],
                # extra_compile_args=['-w'],
            ),
        ],
        compiler_directives={'language_level': '3'}),
)
