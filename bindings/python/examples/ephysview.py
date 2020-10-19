"""
Python example of an interactive raw ephys data viewer.

TODO:
- top panel with another file
- sort by different words
- apply different filters

"""

import math
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


class RawEphysViewer:
    def __init__(self, n_channels, sample_rate, dtype, buffer_size=30_000):
        self.n_channels = n_channels
        self.sample_rate = sample_rate
        self.dtype = dtype
        self.buffer_size = buffer_size
        self.sample = 0  # current sample
        self.arr_buf = None
        self.scale = None

    def memmap_file(self, path):
        self.mmap_array = _memmap_flat(
            path, dtype=self.dtype, n_channels=self.n_channels)
        assert self.mmap_array.ndim == 2
        assert self.mmap_array.shape[1] == n_channels
        self.n_samples = self.mmap_array.shape[0]

    def load_session(self, eid):
        from oneibl.one import ONE
        self.one = ONE()
        # TODO
        self.url_cbin = ""
        self.url_ch = ""
        self.n_samples = 0  # TODO

    def _load_from_file(self):
        return self.mmap_array[self.sample:self.sample + self.buffer_size, :]

    def _load_from_web(self):
        # WARNING: this assumes that 1 chunk = 1 second
        chunk_idx = int(self.time)  # chunk number
        return self.one.download_raw_partial(
            self.url_cbin, self.url_ch,
            first_chunk=chunk_idx, last_chunk=chunk_idx)

    def load_data(self):
        self.sample = np.clip(
            self.sample, 0, self.n_samples - self.buffer_size)
        if hasattr(self, 'mmap_array'):
            self.arr_buf = self._load_from_file()
        elif hasattr(self, 'one'):
            self.arr_buf = self._load_from_web()
        assert self.arr_buf.shape == (self.buffer_size, self.n_channels)

    def update_view(self):
        self.scale = scale = self.scale or get_scale(self.arr_buf)
        self.image[..., :3] = normalize(
            self.arr_buf, scale).T[:, :, np.newaxis]
        self.v_image.set_image(self.image)
        self.panel.axes_range(
            self.sample / self.sample_rate,
            0,
            (self.sample + self.buffer_size) / self.sample_rate,
            self.n_channels)

    def create(self):
        # Create the Visky view.
        vl.log_set_level_env()
        self.canvas = api.canvas(shape=(1, 1))
        self.v_image = self.canvas[0, 0].imshow(
            np.empty((self.n_channels, self.buffer_size, 4), dtype=np.uint8))
        self.panel = self.v_image.panel
        self.image = create_image((self.buffer_size, self.n_channels))

        # Load the data and put it on the GPU.
        self.load_data()
        self.update_view()

        # Interactivity bindings.
        self.canvas.on_key(self.on_key)
        self.canvas.on_mouse(self.on_mouse)

    @property
    def duration(self):
        return self.n_samples / float(self.sample_rate)

    @property
    def time(self):
        return self.sample / float(self.sample_rate)

    def goto(self, time):
        self.sample = int(round(time * self.sample_rate))
        self.load_data()
        self.update_view()

    def on_key(self, key, modifiers=None):
        if key == 'left':
            self.goto(self.time - 1)
        if key == 'right':
            self.goto(self.time + 1)
        if key == 'kp_add':
            self.scale = (self.scale[0], self.scale[1] / 1.1)
            self.update_view()
        if key == 'kp_subtract':
            self.scale = (self.scale[0], self.scale[1] * 1.1)
            self.update_view()
        if key == 'home':
            self.goto(0)
        if key == 'end':
            self.goto(self.duration)

    def on_mouse(self, button, pos, ev=None):
        if ev.state == 'click':
            pick = vl.vky_pick(
                self.canvas._scene, tp.T_VEC2(pos[0], pos[1]), None)
            x, y = pick.pos_data
            i = math.floor(
                (x - self.sample / self.sample_rate) /
                (self.buffer_size / self.sample_rate) *
                self.buffer_size)
            j = math.floor(y)
            j = self.n_channels - 1 - j
            i = np.clip(i, 0, self.n_samples - 1)
            j = np.clip(j, 0, self.n_channels - 1)
            print(
                f"Picked {x}, {y} : {self.arr_buf[i, j]}")

    def show(self):
        api.run()


if __name__ == '__main__':
    path = Path(__file__).parent / "raw_ephys.bin"
    n_channels = 385
    dtype = np.int16
    sample_rate = 30_000

    viewer = RawEphysViewer(n_channels, sample_rate, dtype)
    viewer.memmap_file(path)
    viewer.create()
    viewer.show()
