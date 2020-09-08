import ctypes
from pathlib import Path
from platform import system


import numpy as np
from numpy.ctypeslib import ndpointer


def load_library():
    if system() == 'Linux':
        lib_path = (Path(__file__).parent / '../build/libvisky.so').resolve()
        return ctypes.cdll.LoadLibrary(lib_path)


visky = load_library()


def demo():
    visky.vky_demo_blank()
