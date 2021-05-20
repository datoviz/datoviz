import os
from pathlib import Path
from setuptools import Extension, setup
import distutils.cygwinccompiler
import numpy as np
from Cython.Build import cythonize

distutils.cygwinccompiler.get_msvcr = lambda: []

CYTHON_DIR = Path(__file__).parent
ROOT_DIR = (CYTHON_DIR / '../../').resolve()
INCLUDE_DIR = ROOT_DIR / 'include'
BUILD_DIR = ROOT_DIR / 'build'
VULKAN_DIR = Path(os.environ.get('VULKAN_SDK', '.'))

with open('requirements.txt') as f:
    require = [x.strip() for x in f.readlines() if not x.startswith('git+')]

# NOTE: build with dynamic linking of datoviz. Need to add to LD_LIBRARY_PATH env variable
# the path to the datoviz library (in <root>/build/).
setup(
    name='datoviz',
    version='0.0.0a0',
    description='Scientific visualization',
    author='Cyrille Rossant',
    author_email='rossant@users.noreply.github.com',
    url='https://datoviz.org',
    long_description='''Scientific visualization''',
    packages=['datoviz'],
    install_requires=require,
    ext_modules=cythonize(
        [Extension(
            'datoviz.pydatoviz', ['datoviz/pydatoviz.pyx'],
            libraries=['datoviz'],
            include_dirs=[
                np.get_include(),
                str(INCLUDE_DIR),
                str(VULKAN_DIR / 'Include'),
                str(ROOT_DIR / 'external/cglm/include'),
                str(BUILD_DIR / '_deps/glfw-src/include'),
            ],
            library_dirs=[str(BUILD_DIR)],
            extra_compile_args=['-w'],
        )],
        compiler_directives={'language_level': '3'}),
)
