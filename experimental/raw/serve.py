# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import asyncio
import base64
import sys
from pathlib import Path
import json
import logging
import io
from pprint import pprint
import urllib.request

from joblib import Memory
import numpy as np
import websockets

from brainbox.io.spikeglx import stream
from brainbox.io.one import (
    load_channels_from_insertion, load_spike_sorting_with_channel, load_spike_sorting_fast,)
from one.api import ONE
from ibllib.atlas import AllenAtlas, BrainRegions
from ibllib.dsp import voltage
from ibllib.pipes.ephys_alignment import EphysAlignment
from ibllib.ephys.neuropixel import SITES_COORDINATES

from datoviz import Requester, Renderer


logger = logging.getLogger('datoviz')
logger.setLevel("DEBUG")


# -------------------------------------------------------------------------------------------------
# CONSTANTS
# -------------------------------------------------------------------------------------------------

CURDIR = Path(__file__).parent.resolve()
WIDTH = 800
HEIGHT = 600

SAMPLE_SKIP = 0  # DEBUG. 200  # Skip beginning for show, otherwise blurry due to filter
CMIN = -.0001
CMAX = .00008


# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------

class Bunch(dict):
    def __init__(self, *args, **kwargs):
        self.__dict__ = self
        super().__init__(*args, **kwargs)


class SpikeData(Bunch):
    def __init__(self, spike_times, spike_clusters, spike_depths, spike_colors):
        self.spike_times = spike_times
        self.spike_clusters = spike_clusters
        self.spike_depths = spike_depths
        self.spike_colors = spike_colors


def _index_of(arr, lookup):
    lookup = np.asarray(lookup, dtype=np.int32)
    m = (lookup.max() if len(lookup) else 0) + 1
    tmp = np.zeros(m + 1, dtype=np.int)
    # Ensure that -1 values are kept.
    tmp[-1] = -1
    if len(lookup):
        tmp[lookup] = np.arange(len(lookup))
    return tmp[arr]


def get_data_urls(eid, probe_idx=0, one=None):
    # Find URL to .cbin file
    dsets = one.alyx.rest(
        'datasets', 'list', session=eid,
        django='name__icontains,ap.cbin,collection__endswith,probe%02d' % probe_idx)
    for fr in dsets[0]['file_records']:
        if fr['data_url']:
            url_cbin = fr['data_url']

    # Find URL to .ch file
    dsets = one.alyx.rest(
        'datasets', 'list', session=eid,
        django='name__icontains,ap.ch,collection__endswith,probe%02d' % probe_idx)
    for fr in dsets[0]['file_records']:
        if fr['data_url']:
            url_ch = fr['data_url']

    return url_cbin, url_ch


def normalize(x):
    m = x.min()
    M = x.max()
    if m == M:
        logger.warning("degenerate values")
        M = m + 1
    return -1 + 2 * (x - m) / (M - m)


# -------------------------------------------------------------------------------------------------
# Data access
# -------------------------------------------------------------------------------------------------

MEMORY = Memory(".")


@MEMORY.cache
def download(url):
    with urllib.request.urlopen(url) as f:
        return io.BytesIO(f.read())


@MEMORY.cache
def _load_spikes(eid, probe_idx=0, fs=3e4):
    one = ONE()
    br = BrainRegions()

    probe_name = 'probe%02d' % probe_idx
    spikes, clusters, channels = load_spike_sorting_fast(
        eid=eid, one=one, probe=probe_name,
        # spike_sorter='pykilosort',
        dataset_types=['spikes.samples', 'spikes.amps', 'spikes.depths'],
        brain_regions=br)

    st = spikes[probe_name]['samples'] / fs
    sc = spikes[probe_name]['clusters']
    sa = spikes[probe_name]['amps']
    sd = spikes[probe_name]['depths']
    n = len(st)

    sd[np.isnan(sd)] = sd[~np.isnan(sd)].min()

    # Colored or gray spikes?
    # color = colorpal(sc.astype(np.int32), cpal='glasbey')
    color = np.tile(np.array([127, 127, 127, 32]), (n, 1))

    # assert 100 < len(cr) < 1000
    # # Brain region colors
    # atlas = AllenAtlas(25)
    # n = len(atlas.regions.rgb)
    # alpha = 255 * np.ones((n, 1))
    # rgb = np.hstack((atlas.regions.rgb, alpha)).astype(np.uint8)
    # spike_regions = cr[sc]
    # # HACK: spurious values
    # spike_regions[spike_regions > 2000] = 0
    # color = rgb[spike_regions]

    return SpikeData(st, sc, sd, color)


@MEMORY.cache
def _load_brain_regions(eid, probe_idx=0):
    one = ONE()
    ba = AllenAtlas()

    probe = 'probe0%d' % probe_idx
    ins = one.alyx.rest('insertions', 'list', session=eid, name=probe)[0]

    xyz_chans = load_channels_from_insertion(
        ins, depths=SITES_COORDINATES[:, 1], one=one, ba=ba)
    region, region_label, region_color, _ = EphysAlignment.get_histology_regions(
        xyz_chans, SITES_COORDINATES[:, 1], brain_atlas=ba)
    return region, region_label, region_color


@MEMORY.cache
def _download_chunk(probe_id, chunk_idx):
    try:
        logger.info(
            f"Starting data download of probe {probe_id}, chunk {chunk_idx}...")
        sr, t0 = stream(probe_id, chunk_idx, nsecs=1, one=ONE())
    except BaseException as e:
        raise e
        logger.error(
            f'PID {probe_id} : recording shorter than {int(chunk_idx / 60.0)} min')
        return None, None
    raw = sr[:, :-1].T
    destripe = voltage.destripe(raw, fs=sr.fs)
    X = destripe[:, :].T  # :int(DISPLAY_TIME * sr.fs)].T
    Xs = X[SAMPLE_SKIP:]  # Remove artifact at begining
    # Tplot = Xs.shape[1] / sr.fs
    info = sr._raw.cmeta
    info.fs = sr.fs
    return info, Xs


# -------------------------------------------------------------------------------------------------
# Model
# -------------------------------------------------------------------------------------------------

class Model:
    vmin = None
    vmax = None

    def __init__(self, eid, probe_id, probe_idx=0):
        self.eid = eid
        self.probe_id = probe_id
        self.probe_idx = probe_idx

        # Ephys data
        logger.info(
            f"Downloading first chunk of ephys data {eid}, probe #{probe_idx}")
        info, arr = _download_chunk(self.probe_id, 0)
        # HACK: this sets self.fs, the sample rate
        self.fs = info.fs
        assert info
        assert arr.size

        self.n_samples = info.chopped_total_samples
        assert self.n_samples > 0

        self.n_channels = arr.shape[1]
        assert self.n_channels > 0

        self.sample_rate = float(info.sample_rate)
        assert self.sample_rate > 1000

        self.duration = self.n_samples / self.sample_rate
        assert self.duration > 0
        logger.info(
            f"Downloaded first chunk of ephys data "
            f"{self.n_samples=}, {self.n_channels=}, {self.duration=}")

        # Spike data.
        self.d = _load_spikes(eid, probe_idx, fs=self.fs)
        self.depth_min = self.d.spike_depths.min()
        self.depth_max = self.d.spike_depths.max()
        assert self.depth_min < self.depth_max
        logger.info(f"Loaded {len(self.d.spike_times)} spikes")

        # Brain regions.
        r, rl, rc = _load_brain_regions(eid, probe_idx)
        self.regions = Bunch(r=r, rl=rl, rc=rc)

    def get_chunk(self, chunk_idx):
        return _download_chunk(self.probe_id, chunk_idx)[1]

    def _get_range_chunks(self, t0, t1):
        # Chunk idxs, assuming 1 second chunk
        i0 = int(t0)  # in seconds
        i1 = int(t1)  # in seconds

        assert i0 >= 0
        assert i0 <= t0
        assert i1 <= t1
        assert i1 < self.n_samples

        return i0, i1

    def get_data(self, t0, t1, filter=None):  # float32
        t0 = np.clip(t0, 0, self.duration)
        t1 = np.clip(t1, 0, self.duration)
        assert t0 < t1
        expected_samples = int(round((t1 - t0) * self.sample_rate))

        # Find the chunks.
        i0, i1 = self._get_range_chunks(t0, t1)

        # Download the chunks.
        arr = np.vstack([self.get_chunk(i) for i in range(i0, i1 + 1)])
        assert arr.ndim == 2
        assert arr.shape[1] == self.n_channels, (arr.shape, self.n_channels)

        # Offset within the array.
        s0 = int(round((t0 - i0) * self.sample_rate))
        assert 0 <= s0 < self.n_samples
        s1 = int(round((t1 - i0) * self.sample_rate))
        assert s0 < s1, (s0, s1)
        assert 0 < s1 <= self.n_samples, (s1, self.n_samples)
        assert s1 - s0 == expected_samples, (s0, s1, expected_samples)
        out = arr[s0:s1, :]
        assert out.shape == (expected_samples, self.n_channels)

        # HACK: the last column seems corrupted
        out[:, -1] = out[:, -2]
        return out

    def to_image(self, data):
        # CAR
        data -= data.mean(axis=0)

        # Vrange
        # self.vmin = data.min() if self.vmin is None else self.vmin
        # self.vmax = data.max() if self.vmax is None else self.vmax
        self.vmin = CMIN if self.vmin is None else self.vmin
        self.vmax = CMAX if self.vmax is None else self.vmax

        data = 255 * np.clip((data - self.vmin) /
                             (self.vmax - self.vmin), 0, 1)

        img = np.dstack(
            (data, data, data, 255 * np.ones_like(data))).astype(np.uint8)
        # # Colormap
        # img = colormap(data.ravel().astype(np.double),
        #                vmin=self.vmin, vmax=self.vmax, cmap='gray')
        # img = img.reshape(data.shape + (-1,))
        # assert img.shape == data.shape[:2] + (4,)

        return img

    def spikes_in_range(self, t0, t1):
        imin = np.searchsorted(self.d.spike_times, t0)
        imax = np.searchsorted(self.d.spike_times, t1)
        return imin, imax

    def get_cluster_spikes(self, cl, t0_t1=None):
        # Select spikes in the given time range, or all spikes.
        if t0_t1 is not None:
            i0, i1 = self.spikes_in_range(*t0_t1)
            s = slice(i0, i1, 1)
        else:
            s = slice(None, None, None)

        # Select the spikes from the requested cluster within the time range.
        sc = self.d.spike_clusters[s]
        idx = sc == cl
        return s, idx

    def get_spike_pos_colors(self, s, idx):
        if np.sum(idx) == 0:
            return

        # x and y coordinates of the spikes.
        x = self.d.spike_times[s][idx]
        y = self.d.spike_depths[s][idx]

        # Color of the first spike.
        i = np.nonzero(idx)[0][0]
        color = self.d.spike_colors[s][i]

        return x, y, color


def get_array(data):
    if data.mode == 'base64':
        r = base64.decodebytes(data.buffer.encode('ascii'))
        return np.frombuffer(r, dtype=np.uint8)
    elif data.mode == 'ibl_raw_ephys':
        m = Model(data.eid, data.probe_id, probe_idx=data.probe_idx)
        arr = m.get_data(data.t0, data.t1)
        img = m.to_image(arr)
        return img

    raise Exception(f"Data upload mode '{data.mode}' unsupported")


# -------------------------------------------------------------------------------------------------
# Renderer
# -------------------------------------------------------------------------------------------------

ROUTER = {
    ('create', 'board'): lambda r, req: r.create_board(int(req.content.width), int(req.content.height), id=int(req.id), background=req.content.background, flags=int(req.flags)),
    ('create', 'graphics'): lambda r, req: r.create_graphics(int(req.content.board), int(req.content.type), id=int(req.id), flags=int(req.flags)),
    ('create', 'dat'): lambda r, req: r.create_dat(int(req.content.type), int(req.content.size), id=int(req.id), flags=int(req.flags)),
    ('create', 'tex'): lambda r, req: r.create_tex(int(req.content.dims), int(req.content.format), width=int(req.content.shape[0]), height=int(req.content.shape[1]), depth=int(req.content.shape[2]), id=int(req.id), flags=int(req.flags)),
    ('create', 'sampler'): lambda r, req: r.create_sampler(int(req.content.filter), int(req.content.address_mode), id=int(req.id)),
    ('bind', 'dat'): lambda r, req: r.bind_dat(int(req.id), int(req.content.slot_idx), int(req.content.dat)),
    ('bind', 'tex'): lambda r, req: r.bind_tex(int(req.id), int(req.content.slot_idx), int(req.content.tex), int(req.content.sampler)),
    ('set', 'vertex'): lambda r, req: r.set_vertex(int(req.id), int(req.content.dat)),
    ('upload', 'dat'): lambda r, req: r.upload_dat(int(req.id), int(req.content.offset), get_array(Bunch(req.content.data))),
    ('upload', 'tex'): lambda r, req: r.upload_tex(int(req.id), get_array(Bunch(req.content.data)), w=int(req.content.shape[0]), h=int(req.content.shape[1]), d=int(req.content.shape[2])),
    ('record', 'begin'): lambda r, req: r.record_begin(int(req.id)),
    ('record', 'viewport'): lambda r, req: r.record_viewport(int(req.id), int(req.content.offset[0]), int(req.content.offset[1]), int(req.content.shape[0]), int(req.content.shape[0])),
    ('record', 'draw'): lambda r, req: r.record_draw(int(req.id), int(req.content.graphics), int(req.content.first_vertex), int(req.content.vertex_count)),
    ('record', 'end'): lambda r, req: r.record_end(int(req.id)),
    ('update', 'board'): lambda r, req: r.update_board(int(req.id)),
}


def process(rnd, requests):
    requester = Requester()
    with requester.requests():
        # Process all requests.
        for req in requests:
            req = Bunch(req)
            if 'flags' not in req:
                req.flags = 0
            if 'content' in req:
                req.content = Bunch(req.content)
            ROUTER[req.action, req.type](requester, req)
    requester.submit(rnd)


def render(rnd, board_id=1):
    # Trigger a redraw.
    requester = Requester()
    with requester.requests():
        requester.update_board(board_id)
    requester.submit(rnd)

    # Get the image.
    img = rnd.get_png(board_id)

    # Return as PNG
    output = io.BytesIO(img)
    return output


# -------------------------------------------------------------------------------------------------
# Server
# -------------------------------------------------------------------------------------------------

async def process_requests(websocket):
    rnd = Renderer()

    while True:
        try:
            msg = await websocket.recv()
            msg = json.loads(msg)
            process(rnd, msg['requests'])
            # TODO: board id
            img = render(rnd, 1)  # msg['render'])
            await websocket.send(img)
        except websockets.ConnectionClosedOK:
            break


async def main():
    async with websockets.serve(process_requests, "localhost", 1234):
        await asyncio.Future()  # run forever


# -------------------------------------------------------------------------------------------------
# Entry point
# -------------------------------------------------------------------------------------------------

def get_eid_default():
    # eid, probeid
    # return 'f25642c6-27a5-4a97-9ea0-06652db79fbd', 'bebe7c8f-0f34-4c3a-8fbb-d2a5119d2961'
    # return '15948667-747b-4702-9d53-354ac70e9119', '4e6dfe08-cab0-4a05-903b-94283cb9f8e7'
    # return '15763234-d21e-491f-a01b-1238eb96d389', '8ca1a850-26ef-42be-8b28-c2e2d12f06d6'

    # eid, pname = one.pid2eid(pid)

    return '41872d7f-75cb-4445-bb1a-132b354c44f0', '8b7c808f-763b-44c8-b273-63c6afbc6aae'


def get_eid_one(probe_idx=0):
    one = ONE()
    insertions = one.alyx.rest(
        'insertions', 'list', dataset_type='channels.mlapdv')
    probe_id = insertions[probe_idx]['id']
    eid = insertions[probe_idx]['session_info']['id']
    return eid, probe_id


def get_eid_argv():
    if len(sys.argv) <= 1:
        return get_eid_default()
    eid = sys.argv[1]
    probe_idx = int(sys.argv[2]) if len(sys.argv) == 3 else 0
    probe = 'probe0%d' % probe_idx
    logger.info("Finding insertion id #%d for eid %s...", probe_idx, eid)
    one = ONE()
    ins = one.alyx.rest('insertions', 'list', session=eid, name=probe)[0]
    probe_id = ins['id']
    logger.info("Found insertion id: %s", probe_id)
    return eid, probe_id


def plot_brain_regions(panel, regions):
    r = regions.r
    rc = regions.rc

    n = r.shape[0]
    p0 = np.zeros((n, 3))
    p0[:, 0] = 0
    p0[:, 1] = r[:, 0]

    p1 = np.zeros((n, 3))
    p1[:, 0] = 1
    p1[:, 1] = r[:, 1]

    v = panel.visual('rectangle')
    v.data('pos', p0, idx=0)
    v.data('pos', p1, idx=1)
    v.data('color', np.c_[rc, 255 * np.ones(n)].astype(np.uint8))


if __name__ == '__main__':
    # eid, probe_id = get_eid_default()
    # # print(eid)
    # # print(probe_id)
    # m = Model(eid, probe_id, probe_idx=0)
    # arr = m.get_data(0, 1)
    # img = m.to_image(arr)
    # print(img)

    asyncio.run(main())
