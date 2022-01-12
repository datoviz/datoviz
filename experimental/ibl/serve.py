# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import asyncio
import base64
import sys
from pathlib import Path
import json
import io
from pprint import pprint
import urllib.request

import websockets

from joblib import Memory
import numpy as np

from datoviz import Requester, Renderer


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


# -------------------------------------------------------------------------------------------------
# Renderer
# -------------------------------------------------------------------------------------------------


MEMORY = Memory(".")


@MEMORY.cache
def download(url):
    with urllib.request.urlopen(url) as f:
        return io.BytesIO(f.read())


def normalize(x):
    m = x.min()
    M = x.max()
    return -1 + 2 * (x - m) / (M - m)


def get_array(data):
    if data.mode == 'base64':
        r = base64.decodebytes(data.buffer.encode('ascii'))
        return np.frombuffer(r, dtype=np.uint8)
    elif data.mode == 'random':
        n = data.count
        assert n > 0
        arr = np.zeros(
            n, dtype=[('pos', np.float32, 3), ('color', np.uint8, 4)])
        assert arr.itemsize == 16

        if data.dist == 'uniform2D':
            pos = np.random.uniform(size=(n, 2))
        elif data.dist == 'normal2D':
            pos = .25 * np.random.normal(size=(n, 2))

        arr['pos'][:, :2] = pos

        color = np.random.uniform(size=(n, 3), low=128, high=240)
        arr['color'][:, :3] = color
        arr['color'][:, 3] = 128
        return arr
    elif data.mode == 'ibl_ephys':

        spike_times = np.load(download(data.spike_times))
        # spike_clusters = np.load(download(data.spike_clusters))
        spike_depths = np.load(download(data.spike_depths))

        n = data.count
        assert n > 0
        assert n <= spike_times.size

        arr = np.zeros(
            n, dtype=[('pos', np.float32, 3), ('color', np.uint8, 4), ('size', np.float32)])

        x = normalize(spike_times[:n])
        y = normalize(spike_depths[:n])

        arr["pos"][:, 0] = x
        arr["pos"][:, 1] = y

        arr["size"][:] = 1

        v = 64
        a = 32
        arr['color'][:, 0] = v
        arr['color'][:, 1] = v
        arr['color'][:, 2] = v
        arr['color'][:, 3] = a

        return arr

    raise Exception(f"Data upload mode '{data.mode}' unsupported")


ROUTER = {
    ('create', 'board'): lambda r, req: r.create_board(int(req.content.width), int(req.content.height), id=int(req.id), background=req.content.background, flags=int(req.flags)),
    ('create', 'graphics'): lambda r, req: r.create_graphics(int(req.content.board), int(req.content.type), id=int(req.id), flags=int(req.flags)),
    ('create', 'dat'): lambda r, req: r.create_dat(int(req.content.type), int(req.content.size), id=int(req.id), flags=int(req.flags)),
    ('set', 'vertex'): lambda r, req: r.set_vertex(int(req.id), int(req.content.dat)),
    ('upload', 'dat'): lambda r, req: r.upload_dat(int(req.id), int(req.content.offset), get_array(Bunch(req.content.data))),
    ('set', 'begin'): lambda r, req: r.set_begin(int(req.id)),
    ('set', 'viewport'): lambda r, req: r.record_viewport(int(req.id), int(req.content.offset[0]), int(req.content.offset[1]), int(req.content.shape[0]), int(req.content.shape[0])),
    ('set', 'draw'): lambda r, req: r.record_draw(int(req.id), int(req.content.graphics), int(req.content.first_vertex), int(req.content.vertex_count)),
    ('set', 'end'): lambda r, req: r.set_end(int(req.id)),
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


if __name__ == "__main__":
    asyncio.run(main())