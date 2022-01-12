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

import websockets

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

def get_array(data):
    if data.mode == 'base64':
        r = base64.decodebytes(data.buffer.encode('ascii'))
        return np.frombuffer(r, dtype=np.uint8)


ROUTER = {
    ('create', 'board'): lambda r, req: r.create_board(req.content.width, req.content.height, id=req.id, flags=req.flags),
    ('create', 'graphics'): lambda r, req: r.create_graphics(req.content.board, req.content.type, id=req.id, flags=req.flags),
    ('create', 'dat'): lambda r, req: r.create_dat(req.content.type, req.content.size, id=req.id, flags=req.flags),
    ('set', 'vertex'): lambda r, req: r.set_vertex(req.id, req.content.dat),
    ('upload', 'dat'): lambda r, req: r.upload_dat(req.id, req.content.offset, get_array(Bunch(req.content.data))),
    ('record', 'begin'): lambda r, req: r.record_begin(req.id),
    ('record', 'viewport'): lambda r, req: r.record_viewport(req.id, req.content.offset[0], req.content.offset[1], req.content.shape[0], req.content.shape[0]),
    ('record', 'draw'): lambda r, req: r.record_draw(req.id, req.content.graphics, req.content.first_vertex, req.content.vertex_count),
    ('record', 'end'): lambda r, req: r.record_end(req.id),
    ('update', 'board'): lambda r, req: r.update_board(req.id),
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
            print(req, file=sys.stderr)
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
            # if 'render' in msg:
            #     img = render(rnd, msg['render'])
            #     await websocket.send(img)
            # else:
            #     process(rnd, msg['requests'])
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
