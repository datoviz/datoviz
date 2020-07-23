import ctypes
from pathlib import Path

import numpy as np
from numpy.ctypeslib import ndpointer


path = (Path(__file__).parent / '../../build/libvisky.so').resolve()
visky = ctypes.cdll.LoadLibrary(path)


DEMO = 3


# Blank demo.
if DEMO == 0:
    visky.vky_demo_blank()


# Ray tracing demo.
if DEMO == 1:
    visky.vky_demo_raytracing()


# Param demo: we pass a color as a 4-tuple of float32.
Color = ctypes.c_float * 4
visky.vky_demo_param.restype = None
visky.vky_demo_param.argtypes = [Color]
if DEMO == 2:
    visky.vky_demo_param(Color(1, 1, 0, 1))


# Scatter demo: pass data as NumPy array.
visky.vky_demo_scatter.restype = None
visky.vky_demo_scatter.argtypes = [ctypes.c_size_t, ndpointer(
    ctypes.c_double, ndim=2, flags="C_CONTIGUOUS")]
if DEMO == 3:
    # NOTE: any higher value causes a segfault atm
    data = .25 * np.random.randn(417788, 2)
    visky.vky_demo_scatter(data.shape[0], data.astype(np.float64))
