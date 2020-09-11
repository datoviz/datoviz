from pathlib import Path

import numpy as np

import visky
from visky.wrap import viskylib as vl, make_vertices, upload_data, array_pointer, pointer
from visky import _constants as const
from visky import _types as tp


def _memmap_flat(path, dtype=None, n_channels=None, offset=0):
    path = Path(path)
    # Find the number of samples.
    assert n_channels > 0
    fsize = path.stat().st_size
    item_size = np.dtype(dtype).itemsize
    n_samples = (fsize - offset) // (item_size * n_channels)
    shape = (n_samples, n_channels)
    return np.memmap(path, dtype=dtype, offset=offset, shape=shape)


def multi_path(scene, panel, raw):
    n_samples = raw.shape[0]
    max_paths = int(const.RAW_PATH_MAX_PATHS)

    y_offsets = np.zeros(max_paths, dtype=np.float32)
    y_offsets[:n_channels] = np.linspace(-1, 1, n_channels)

    colors = np.zeros((max_paths, 4), dtype=np.float32)
    colors[:, 0] = 1
    colors[:n_channels, 1] = np.linspace(0, 1, n_channels)
    colors[:, 3] = 1

    # Visual parameters.
    params = tp.T_MULTI_RAW_PATH_PARAMS(
        (n_channels, n_samples, 0.0001),
        np.ctypeslib.as_ctypes(y_offsets.reshape((-1, 4))),
        np.ctypeslib.as_ctypes(colors))

    visual = vl.vky_visual(
        scene, const.VISUAL_PATH_RAW_MULTI, pointer(params), None)
    vl.vky_add_visual_to_panel(
        visual, panel, const.VIEWPORT_INNER, const.VISUAL_PRIORITY_NONE)

    raw -= np.median(raw, axis=0).astype(np.int16)
    upload_data(visual, raw)


def ephys_view(path, n_channels, dtype):
    raw = _memmap_flat(path, dtype=dtype, n_channels=n_channels)
    raw = raw[:10000, :]

    assert raw.ndim == 2
    assert raw.shape[1] == n_channels

    vl.log_set_level_env()

    app = vl.vky_create_app(const.BACKEND_GLFW, None)
    canvas = vl.vky_create_canvas(app, 800, 600)
    scene = vl.vky_create_scene(canvas, const.WHITE, 1, 1)
    panel = vl.vky_get_panel(scene, 0, 0)
    vl.vky_set_controller(panel, const.CONTROLLER_AXES_2D, None)

    multi_path(scene, panel, raw)

    vl.vky_run_app(app)
    vl.vky_destroy_scene(scene)
    vl.vky_destroy_app(app)


if __name__ == '__main__':
    path = Path(
        "/data/spikesorting/probe_left/_iblrig_ephysData.raw_g0_t0.imec.ap.bin")
    n_channels = 385
    dtype = np.int16
    ephys_view(path, n_channels, dtype)
