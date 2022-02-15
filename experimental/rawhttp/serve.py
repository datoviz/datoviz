import io
from pathlib import Path

import numpy as np
import png
from flask import Flask, send_file
from flask_cors import CORS, cross_origin

from mtscomp.lossy import decompress_lossy


app = Flask(__name__)
CORS(app, support_credentials=True)


TIME_HALF_WINDOW = 0.1  # in seconds


def to_png(arr):
    p = png.from_array(arr, mode="L")
    b = io.BytesIO()
    p.write(b)
    b.seek(0)
    return b


def send_image(img):
    return send_file(to_png(img), mimetype='image/png')


def get_img(eid, time=0):
    path_lossy = Path(__file__).parent / 'raw.lossy.npy'
    lossy = decompress_lossy(path_lossy)
    duration = lossy.duration
    assert duration > 0

    dt = TIME_HALF_WINDOW
    t = float(time)
    t = np.clip(t, dt, duration - dt)

    arr = lossy.get(t - dt, t + dt, cast_to_uint8=True).T
    return arr


@app.route('/<eid>/')
@cross_origin(supports_credentials=True)
def serve_default(eid):
    img = get_img(eid)
    return send_image(img)


@app.route('/<eid>/<time>')
@cross_origin(supports_credentials=True)
def serve_time_float(eid, time=0):
    img = get_img(eid, time=float(time))
    return send_image(img)
