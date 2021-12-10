# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import base64
import sys
import json
import io
from pprint import pprint

from imageio import v3 as iio
import numpy as np
from flask import Flask, request, send_file, session

from datoviz import Renderer


class Bunch(dict):
    def __init__(self, *args, **kwargs):
        self.__dict__ = self
        super().__init__(*args, **kwargs)


# -------------------------------------------------------------------------------------------------
# CONSTANTS
# -------------------------------------------------------------------------------------------------

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
    ('create', 'board'): lambda r, req: r._create_board(req.content.width, req.content.height, id=req.id),
    ('create', 'graphics'): lambda r, req: r._create_graphics(req.content.board, req.content.type, id=req.id),
    ('create', 'dat'): lambda r, req: r._create_dat(req.content.type, req.content.size, id=req.id),
    ('set', 'vertex'): lambda r, req: r._set_vertex(req.id, req.content.dat),
    ('upload', 'dat'): lambda r, req: r._upload_dat(req.id, req.content.offset, get_array(Bunch(req.content.data))),
    ('set', 'begin'): lambda r, req: r._set_begin(req.id),
    ('set', 'viewport'): lambda r, req: r._set_viewport(req.id, req.content.offset[0], req.content.offset[1], req.content.shape[0], req.content.shape[0]),
    ('set', 'draw'): lambda r, req: r._set_draw(req.id, req.content.graphics, req.content.first_vertex, req.content.vertex_count),
    ('set', 'end'): lambda r, req: r._set_end(req.id),
    ('update', 'board'): lambda r, req: r._update_board(req.id),
}

def process(requests):
    # Process all requests.
    for req in requests:
        req = Bunch(req)
        if 'content' in req:
            req.content = Bunch(req.content)
        ROUTER[req.action, req.type](RENDERER, req)


# -------------------------------------------------------------------------------------------------
# Server
# -------------------------------------------------------------------------------------------------

app = Flask(__name__)

@app.route('/', methods=['POST'])
def main():
    d = json.loads(request.form['json'])
    process(d['requests'])
    return ''

@app.route('/', methods=['GET'])
def get_image():
    # Save the image.
    # TODO: find board id?

    # RENDERER.save_image(1, "a.png")
    # with open("a.png", "rb") as f:
    #     img = f.read()

    img = RENDERER.get_image(1)
    output = io.BytesIO()
    iio.imwrite(output, img, plugin="pillow", format="PNG")
    output.seek(0)
    return send_file(
        output,
        mimetype='image/png',
        as_attachment=False)
