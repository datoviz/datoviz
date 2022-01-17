# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import asyncio
import base64
import sys
import logging
import traceback
from pathlib import Path
import json
import io
from pprint import pprint
import urllib.request

from joblib import Memory
import numpy as np
import websockets
from PIL import Image

from datoviz import Requester, Renderer


logger = logging.getLogger('datoviz')


class Bunch(dict):
    def __init__(self, *args, **kwargs):
        self.__dict__ = self
        super().__init__(*args, **kwargs)


# -------------------------------------------------------------------------------------------------
# CONSTANTS
# -------------------------------------------------------------------------------------------------

CURDIR = Path(__file__).parent.resolve()
WIDTH = 800
HEIGHT = 600
MAX_COUNT = 10_000_000
ALPHA = 128
SIZE = 5
COLOR = 128

DEFAULT_URI = "https://ibl.flatironinstitute.org/public/cortexlab/Subjects/KS023/2019-12-10/001/alf/probe01/"
SESSIONS = {
    "https://ibl.flatironinstitute.org/public/cortexlab/Subjects/KS023/2019-12-10/001/alf/probe01/": {
        "times": "spikes.times.99ed1a2d-bb18-4f86-92f3-23a15b7d9972.npy",
        "clusters": "spikes.clusters.2e3a207d-6b7a-41b3-80b9-d759a6557b55.npy",
        "depths": "spikes.depths.9cbed712-3d4e-44d4-8ec8-f915a884e667.npy"
    },

    "https://ibl.flatironinstitute.org/public/churchlandlab/Subjects/CSHL047/2020-01-20/001/alf/probe00/": {
        "times": "spikes.times.32609666-84f6-4b20-8450-afc311b3e64e.npy",
        "clusters": "spikes.clusters.a44513ba-069e-443a-bcb6-1de2a5071615.npy",
        "depths": "spikes.depths.879aedc4-227d-4234-828e-9d9f78b7323e.npy",
    },

    "https://ibl.flatironinstitute.org/public/mainenlab/Subjects/ZM_2240/2020-01-21/001/alf/probe00/": {
        "times": "spikes.times.be8c4562-f83d-45c3-ba4d-7ac9eec94b85.npy",
        "clusters": "spikes.clusters.33e2b747-8cf9-48d7-9814-69c6aa30cd59.npy",
        "depths": "spikes.depths.7ba042a8-cc40-416f-8390-12b0c8f4009f.npy",
    }
}


# -------------------------------------------------------------------------------------------------
# Data access
# -------------------------------------------------------------------------------------------------

MEMORY = Memory(".")


@MEMORY.cache
def download(url):
    with urllib.request.urlopen(url) as f:
        return io.BytesIO(f.read())


def normalize(x):
    m = x.min()
    M = x.max()
    if m == M:
        logger.warning("degenerate values")
        M = m + 1
    return -1 + 2 * (x - m) / (M - m)


def from_base64(buf):
    r = base64.decodebytes(buf.encode('ascii'))
    return np.frombuffer(r, dtype=np.uint8)


# -------------------------------------------------------------------------------------------------
# Renderer
# -------------------------------------------------------------------------------------------------

MVP_BUFFER = "AACAPwAAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAAAAAACAPwAAgD8AAAAAAAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAAAAAAAAgD8AAIA/AAAAAAAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAAAAAA=="
VIEWPORT_BUFFER = "AAAAAAAAAAAAAEhEAAAWRAAAAAAAAIA/AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgAwAAWAIAAAAAAAAAAAAAIAMAAFgCAAAAAAAAAAAAAAAAAAAAAAAA"

board_id = 1
graphics_id = 2
mvp_id = 10
viewport_id = 11
vertex_id = 3


def get_data(uri, st_name, sd_name, t0=0, t1=1):
    spike_times = np.load(download(uri + st_name))
    spike_depths = np.load(download(uri + sd_name))
    spike_depths[np.isnan(spike_depths)] = 0

    assert 0 <= t0 and t0 < t1

    # HACK: will need resize_dat request to lift this limitation
    assert spike_times.shape[0] <= MAX_COUNT

    # # DEBUG
    # spike_times = spike_times[:MAX_COUNT]
    # spike_depths = spike_depths[:MAX_COUNT]

    # Select spikes in the time range.
    imin = np.searchsorted(spike_times, t0)
    imax = np.searchsorted(spike_times, t1)

    logger.debug(f"downloaded {len(spike_times)} spikes, keep {imax - imin}")

    spike_times = spike_times[imin:imax]
    spike_depths = spike_depths[imin:imax]

    x = normalize(spike_times)
    y = normalize(spike_depths)

    n = spike_times.shape[0]
    arr = np.zeros(
        n, dtype=[('pos', np.float32, 3), ('color', np.uint8, 4), ('size', np.float32)])

    arr["pos"][:, 0] = x
    arr["pos"][:, 1] = y

    arr["size"][:] = SIZE

    v = COLOR
    a = ALPHA
    arr['color'][:, 0] = v
    arr['color'][:, 1] = v
    arr['color'][:, 2] = v
    arr['color'][:, 3] = a

    return arr


def record(rqr, count):
    logger.debug(f"draw {count} spikes")

    rqr.record_begin(board_id)
    rqr.record_viewport(board_id, 0, 0, 0, 0)
    rqr.record_draw(board_id, graphics_id, 0, count)
    rqr.record_end(board_id)


def create(rqr, rnd, uri, st_name, sd_name, t0=0, t1=1):
    with rqr.requests():
        rqr.create_board(WIDTH, HEIGHT, background=(
            255, 255, 255), id=board_id)
        rqr.create_graphics(board_id, 1, id=graphics_id)

        rqr.create_dat(5, 200, id=mvp_id)
        rqr.bind_dat(graphics_id, 0, mvp_id)
        rqr.upload_dat(mvp_id, 0, from_base64(MVP_BUFFER))

        rqr.create_dat(5, 80, id=viewport_id)
        rqr.bind_dat(graphics_id, 1, viewport_id)
        rqr.upload_dat(viewport_id, 0, from_base64(VIEWPORT_BUFFER))

        rqr.create_dat(2, 20 * MAX_COUNT, id=vertex_id)
        rqr.set_vertex(graphics_id, vertex_id)
        data = get_data(uri, st_name, sd_name, t0=t0, t1=t1)
        rqr.upload_dat(vertex_id, 0, data)

        record(rqr, data.shape[0])

    rqr.submit(rnd)


def update(rqr, rnd, uri, st_name, sd_name, t0=0, t1=1):
    with rqr.requests():
        data = get_data(uri, st_name, sd_name, t0=t0, t1=t1)
        rqr.upload_dat(vertex_id, 0, data)
        record(rqr, data.shape[0])
    rqr.submit(rnd)


def render(rqr, rnd, board_id=1):
    # Trigger a redraw.
    with rqr.requests():
        rqr.update_board(board_id)
    rqr.submit(rnd)

    # Get the image.
    img = rnd.get_png(board_id)
    logger.debug(f"retrieved the PNG image with {len(img)} bytes")

    # Return as PNG
    output = io.BytesIO(img)
    return output


# -------------------------------------------------------------------------------------------------
# Server
# -------------------------------------------------------------------------------------------------

async def process_requests(websocket):
    rqr = Requester()
    rnd = Renderer()
    created = False

    while True:
        try:
            msg = await websocket.recv()
            logger.debug("new message")
            msg = json.loads(msg)
            if 'set_session' in msg:
                uri = msg['set_session']['uri']
                st_name = msg['set_session']['spike_times']
                sd_name = msg['set_session']['spike_depths']
                t0 = msg['set_session'].get('start_time', 0)
                t1 = msg['set_session'].get('stop_time', 1)

                create(rqr, rnd, uri, st_name, sd_name, t0=t0, t1=t1)

            elif 'update_time' in msg:
                t0 = msg['update_time'].get('start_time', 0)
                t1 = msg['update_time'].get('stop_time', 1)

                update(rqr, rnd, uri, st_name, sd_name, t0=t0, t1=t1)

            img = render(rqr, rnd, 1)
            logger.debug(
                f"sending back the image with {img.getbuffer().nbytes} bytes...")
            img.seek(0)
            await websocket.send(img)
        except (websockets.ConnectionClosedOK, websockets.ConnectionClosedError):
            logger.debug("server connection closed")
            break
        except Exception as e:
            logger.error(traceback.format_exc())
            break
    logger.debug("server stopped")


async def server():
    async with websockets.serve(process_requests, "localhost", 1234, max_size=None):
        logger.info("Starting the websockets server")
        await asyncio.Future()  # run forever


# -------------------------------------------------------------------------------------------------
# Client
# -------------------------------------------------------------------------------------------------

async def set_session(websocket, uri, t0=0, t1=1):
    msg = {
        'set_session': {
            'uri': uri,
            'spike_times': SESSIONS[uri]['times'],
            'spike_depths': SESSIONS[uri]['depths'],
            'start_time': t0,
            'stop_time': t1,
        },
    }
    await websocket.send(json.dumps(msg))


async def update_time(websocket, t0=0, t1=1):
    msg = {
        'update_time': {
            'start_time': t0,
            'stop_time': t1,
        },
    }
    await websocket.send(json.dumps(msg))


async def recv_img(websocket, filename):
    logger.debug("client waiting for an incoming message...")
    buf = await websocket.recv()
    logger.info(f"received buffer with {len(buf)} bytes")
    with open(filename, 'wb') as f:
        f.write(buf)


async def client():
    async with websockets.connect("ws://localhost:1234", max_size=None) as websocket:
        try:
            logger.info("starting the websockets client")

            await set_session(websocket, DEFAULT_URI, t0=0, t1=100)
            await recv_img(websocket, 'img0.png')

            await update_time(websocket, t0=1000, t1=1010)
            await recv_img(websocket, 'img1.png')

            logger.info("stopping the websockets client")

        except Exception as e:
            logger.error(traceback.format_exc())


# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

if __name__ == "__main__":
    if len(sys.argv) <= 1 or sys.argv[-1] == 'server':
        asyncio.run(server())
    elif sys.argv[1] == 'client':
        asyncio.run(client())
