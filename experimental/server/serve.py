# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import base64
import sys
from pathlib import Path
import json
import io
from pprint import pprint

import numpy as np
from flask import Flask, request, send_file, session, render_template

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


RENDERER = Renderer()


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


def process(requests):
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
    requester.submit(RENDERER)


# -------------------------------------------------------------------------------------------------
# Server
# -------------------------------------------------------------------------------------------------

app = Flask(__name__)


@app.route('/', methods=['GET'])
def main():
    with open(CURDIR / '../../datoviz/tests/triangle.json', 'r') as f:
        json_contents = f.read().replace('\n', '')
    return render_template(
        'index.html',
        json_contents=json_contents,
    )


@app.route('/request', methods=['POST'])
def post_request():
    d = request.json
    process(d['requests'])
    return ''


@app.route('/render', methods=['GET'])
def render():
    # TODO: find board id?
    board_id = 1

    # Trigger a redraw.
    requester = Requester()
    with requester.requests():
        requester.update_board(board_id)
    requester.submit(RENDERER)

    # Get the image.
    img = RENDERER.get_png(1)

    # Return as PNG
    output = io.BytesIO(img)
    return send_file(output, mimetype='image/png', as_attachment=False)
