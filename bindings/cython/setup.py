from pathlib import Path
from setuptools import Extension, setup
import numpy as np
from Cython.Build import cythonize

CYTHON_DIR = Path(__file__).parent
ROOT_DIR = (CYTHON_DIR / '../../').resolve()
INCLUDE_DIR = ROOT_DIR / 'include'
BUILD_DIR = ROOT_DIR / 'build'

# NOTE: build with dynamic linking of datoviz. Need to add to LD_LIBRARY_PATH env variable
# the path to the datoviz library (in <root>/build/).
setup(
    name='datoviz',
    version='0.0.0a0',
    description='Scientific visualization',
    author='Cyrille Rossant',
    author_email='rossant@users.noreply.github.com',
    url='https://datoviz.dev',
    long_description='''Scientific visualization''',
    packages=['datoviz'],
    ext_modules=cythonize(
        [Extension(
            'datoviz.pydatoviz', ['datoviz/pydatoviz.pyx'],
            libraries=['datoviz'],
            include_dirs=[
                np.get_include(),
                str(INCLUDE_DIR),
                str(ROOT_DIR / 'external/cglm/include'),
                str(BUILD_DIR / '_deps/glfw-src/include'),
            ],
            library_dirs=[str(BUILD_DIR)],
            extra_compile_args=['-w'],
        )],
        compiler_directives={'language_level': '3'}),
)
