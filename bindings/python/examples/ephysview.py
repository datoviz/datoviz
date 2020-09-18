from pathlib import Path

import numpy as np

from visky.wrap import viskylib as vl, upload_data, pointer
from visky import _constants as const
from visky import _types as tp
from visky import api


def _memmap_flat(path, dtype=None, n_channels=None, offset=0):
    path = Path(path)
    # Find the number of samples.
    assert n_channels > 0
    fsize = path.stat().st_size
    item_size = np.dtype(dtype).itemsize
    n_samples = (fsize - offset) // (item_size * n_channels)
    if item_size * n_channels * n_samples != (fsize - offset):
        raise IOError("n_channels incorrect or binary file truncated")
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


def create_image(shape):
    image = np.zeros((shape[1], shape[0], 4), dtype=np.uint8)
    image[..., 3] = 255
    return image


def get_scale(x):
    return np.median(x, axis=0), x.std()


def normalize(x, scale):
    m, s = scale
    out = np.empty_like(x, dtype=np.float32)
    out[...] = x
    out -= m
    out *= (1.0 / s)
    out += 1
    out *= 255 * .5
    out[out < 0] = 0
    out[out > 255] = 255
    return out.astype(np.uint8)


def get_data(raw, sample, buffer):
    return raw[sample:sample + buffer, :]


class DataScroller:
    def __init__(self, visual, raw, sample_rate, buffer):
        self.visual = visual
        self.panel = visual.panel
        self.raw = raw
        self.sample_rate = float(sample_rate)
        self.image = create_image((buffer, raw.shape[1]))
        self.sample = int(10 * 3e4)
        self.buffer = buffer
        self.scale = None
        self.data = None

    def load_data(self):
        self.sample = np.clip(self.sample, 0, self.raw.shape[0] - self.buffer)
        self.data = get_data(self.raw, self.sample, self.buffer)

    def upload(self):
        if self.data is None:
            self.load_data()
        self.scale = scale = self.scale or get_scale(self.data)
        self.image[..., :3] = normalize(self.data, scale).T[:, :, np.newaxis]
        self.visual.upload_image(self.image)
        self.panel.axes_range(
            self.sample / self.sample_rate,
            (self.sample + self.buffer) / self.sample_rate,
            0, self.data.shape[1])


def ephys_view(path, n_channels, sample_rate, dtype, buffer):
    raw = _memmap_flat(path, dtype=dtype, n_channels=n_channels)

    assert raw.ndim == 2
    assert raw.shape[1] == n_channels

    vl.log_set_level_env()

    app = api.App()
    canvas = app.canvas(shape=(2, 1))
    canvas.set_heights([2, 8])

    n = 1000
    t = np.linspace(-1, 1, n)

    points = np.zeros((n, 3), dtype=np.float32)
    points[:, 0] = t
    points[:, 1] = .5 * np.cos(20 * t)

    colors = api.get_color('jet', np.linspace(0, 1, n))

    v_plot = canvas[0, 0].plot(points, colors=colors, lw=5)
    v_image = canvas[1, 0].image(
        np.empty((n_channels, buffer, 4), dtype=np.uint8))

    ds = DataScroller(v_image, raw, sample_rate, buffer)
    ds.upload()

    @canvas.on_key
    def on_key(key, modifiers=None):
        if key == 'left':
            ds.sample -= 500
            ds.load_data()
            ds.upload()
        if key == 'right':
            ds.sample += 500
            ds.load_data()
            ds.upload()
        if key == 'kp_add':
            ds.scale = (ds.scale[0], ds.scale[1] / 1.1)
            ds.upload()
        if key == 'kp_subtract':
            ds.scale = (ds.scale[0], ds.scale[1] * 1.1)
            ds.upload()

    @canvas.on_mouse
    def on_mouse(pos):
        pass
        # TODO: button, modifiers
        # print(pos)
        pick = vl.vky_pick(canvas._scene, tp.T_VEC2(pos[0], pos[1]))
        print(pick.data_coords[0], pick.data_coords[1])

    app.run()


if __name__ == '__main__':
    path = Path(__file__).parent / "raw_ephys.bin"
    n_channels = 385
    dtype = np.int16
    buffer = 10_000
    sample_rate = 30_000
    ephys_view(path, n_channels, sample_rate, dtype, buffer)
