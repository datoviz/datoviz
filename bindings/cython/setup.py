from pathlib import Path
from skbuild import setup

ROOT_DIR = (Path(__file__).parent / '../../').resolve()

setup(
    name='visky',
    version='0.0.0a0',
    description='Scientific visualization',
    author='Cyrille Rossant',
    author_email='rossant@users.noreply.github.com',
    url='https://visky.dev',
    long_description='''Scientific visualization''',
    packages=['visky'],
    cmake_args=[
        '-DVISKY_WITH_EXAMPLES=0',
    ],
    cmake_source_dir=ROOT_DIR,
)
