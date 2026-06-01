#!/usr/bin/env python3
"""
ctypes smoke test for the wrap window backend API.
"""

from __future__ import annotations

import ctypes
import platform
from pathlib import Path


def _lib_path() -> Path:
    root = Path(__file__).resolve().parents[1]
    system = platform.system()
    name = {
        "Linux": "libdatoviz.so",
        "Darwin": "libdatoviz.dylib",
        "Windows": "libdatoviz.dll",
    }.get(system)
    if name is None:
        raise RuntimeError(f"unsupported platform: {system}")
    candidates = [
        root / "build" / "src" / name,
        root / "build" / name,
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise RuntimeError(f"unable to locate {name} in build tree")


class VkExtent2D(ctypes.Structure):
    _fields_ = [("width", ctypes.c_uint32), ("height", ctypes.c_uint32)]


class DvzWindowConfig(ctypes.Structure):
    _fields_ = [
        ("struct_size", ctypes.c_uint32),
        ("flags", ctypes.c_uint32),
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("title", ctypes.c_char_p),
        ("resizable", ctypes.c_bool),
        ("visible", ctypes.c_bool),
        ("user_scale", ctypes.c_float),
    ]


class DvzWindowExternalSurfaceInfo(ctypes.Structure):
    _fields_ = [
        ("instance", ctypes.c_void_p),
        ("surface", ctypes.c_void_p),
        ("extent", VkExtent2D),
        ("scale_x", ctypes.c_float),
        ("scale_y", ctypes.c_float),
        ("owned_by_datoviz", ctypes.c_bool),
    ]


DVZ_BACKEND_WRAP = 4


def main() -> int:
    lib = ctypes.cdll.LoadLibrary(str(_lib_path()))

    lib.dvz_window_host.restype = ctypes.c_void_p
    lib.dvz_window_host_destroy.argtypes = [ctypes.c_void_p]
    lib.dvz_window_config.restype = DvzWindowConfig
    lib.dvz_window_create.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.POINTER(DvzWindowConfig)]
    lib.dvz_window_create.restype = ctypes.c_void_p
    lib.dvz_window_destroy.argtypes = [ctypes.c_void_p]

    lib.dvz_window_wrap_set_required_extensions.argtypes = [
        ctypes.c_void_p,
        ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_char_p),
    ]
    lib.dvz_window_wrap_set_required_extensions.restype = ctypes.c_int32
    lib.dvz_window_host_required_extension_count.argtypes = [ctypes.c_void_p, ctypes.c_int32]
    lib.dvz_window_host_required_extension_count.restype = ctypes.c_uint32
    lib.dvz_window_host_required_extensions.argtypes = [
        ctypes.c_void_p,
        ctypes.c_int32,
        ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_char_p),
    ]
    lib.dvz_window_host_required_extensions.restype = ctypes.c_int32

    lib.dvz_window_wrap_attach_surface.argtypes = [
        ctypes.c_void_p,
        ctypes.POINTER(DvzWindowExternalSurfaceInfo),
    ]
    lib.dvz_window_wrap_attach_surface.restype = ctypes.c_int32
    lib.dvz_window_wrap_update_surface.argtypes = [
        ctypes.c_void_p,
        ctypes.POINTER(DvzWindowExternalSurfaceInfo),
    ]
    lib.dvz_window_wrap_update_surface.restype = ctypes.c_int32
    lib.dvz_window_wrap_detach_surface.argtypes = [ctypes.c_void_p]

    host = lib.dvz_window_host()
    assert host, "failed to create window host"
    try:
        ext_names = [b"VK_KHR_surface", b"VK_EXT_metal_surface"]
        ext_arr = (ctypes.c_char_p * len(ext_names))(*ext_names)
        rc = lib.dvz_window_wrap_set_required_extensions(host, len(ext_names), ext_arr)
        assert rc == 0, f"set_required_extensions failed: {rc}"

        count = lib.dvz_window_host_required_extension_count(host, DVZ_BACKEND_WRAP)
        assert count == len(ext_names), (count, len(ext_names))
        out = (ctypes.c_char_p * count)()
        wrote = lib.dvz_window_host_required_extensions(host, DVZ_BACKEND_WRAP, count, out)
        assert wrote == count, wrote
        got = [out[i] for i in range(count)]
        assert got == ext_names, (got, ext_names)

        cfg = lib.dvz_window_config()
        cfg.title = b"py-wrap-smoke"
        window = lib.dvz_window_create(host, DVZ_BACKEND_WRAP, ctypes.byref(cfg))
        assert window, "failed to create wrap window"
        try:
            info = DvzWindowExternalSurfaceInfo(
                instance=ctypes.c_void_p(0x1),
                surface=ctypes.c_void_p(0x2),
                extent=VkExtent2D(640, 480),
                scale_x=1.0,
                scale_y=1.0,
                owned_by_datoviz=False,
            )
            rc = lib.dvz_window_wrap_attach_surface(window, ctypes.byref(info))
            assert rc == 0, f"attach_surface failed: {rc}"

            info_loss = DvzWindowExternalSurfaceInfo(
                instance=ctypes.c_void_p(),
                surface=ctypes.c_void_p(),
                extent=VkExtent2D(640, 480),
                scale_x=1.0,
                scale_y=1.0,
                owned_by_datoviz=False,
            )
            rc = lib.dvz_window_wrap_update_surface(window, ctypes.byref(info_loss))
            assert rc == 0, f"update_surface failed: {rc}"

            lib.dvz_window_wrap_detach_surface(window)
        finally:
            lib.dvz_window_destroy(window)
    finally:
        lib.dvz_window_host_destroy(host)

    print("wrap ctypes smoke: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
