import itertools
from pathlib import Path
import csv
import re
import sys
from textwrap import dedent
import io
import base64

from matplotlib import cm, image
import numpy as np
import bokeh.palettes
import colorcet
import webcolors
from PIL import Image

"""
References:

https://www.kennethmoreland.com/color-advice/

The CSV files come from https://www.kennethmoreland.com/color-advice/

"""


COLORCET_NAMES = set(colorcet.palette.keys())
COLORCET_NAMES = {n.lower(): n for n in COLORCET_NAMES}

MATPLOTLIB_NAMES = set(_ for _ in dir(cm) if not _.startswith('_'))
MATPLOTLIB_NAMES = {n.lower(): n for n in MATPLOTLIB_NAMES}

BOKEH_NAMES = bokeh.palettes.__palettes__
BOKEH_NAMES = {n.lower(): n for n in BOKEH_NAMES}
# HACK: fix bug in bokeh name
BOKEH_NAMES["category20b_20"] = "Category20b_20"
BOKEH_NAMES["category20c_20"] = "Category20c_20"

MAX_PALETTE_SIZE = 32

ROOT_DIR = Path(__file__).parent.parent
path = ROOT_DIR / "include/datoviz/colormaps.h"
texture_path = ROOT_DIR / "data/textures/color_texture.img"

# Ensure the subdirectorys exist.
texture_path.parent.mkdir(exist_ok=True, parents=True)


def group(n, iterable):
    it = iter(iterable)
    while True:
        chunk_it = itertools.islice(it, n)
        try:
            first_el = next(chunk_it)
        except StopIteration:
            return
        yield itertools.chain((first_el,), chunk_it)


def cmap_lines(l):
    cmap = ['{:3d}, {:3d}, {:3d},   0,'.format(*tuple(_)[:3]) for _ in l]
    if len(cmap) != 256:
        assert len(cmap) < MAX_PALETTE_SIZE
        cmap += ['  0,   0,   0,   0,'] * (MAX_PALETTE_SIZE - len(cmap))
    return ["    ".join(grp) for grp in group(8, cmap)]


def get_colorcet_palette(name):
    colors = colorcet.palette.get(name, None)
    return [webcolors.hex_to_rgb(h) for h in colors]


def get_matplotlib_palette(name):
    arr = (getattr(cm, name)(np.arange(256)))[:, :3]
    arr = (arr * 255).astype(np.uint8)
    # HACK: detect palette
    if np.all(arr[-100:-50, :] == arr[-50:, :]):
        i = 0
        while not np.all(arr[i] == arr[-1]):
            i += 1
        arr = arr[:i + 1, :]
    return arr.tolist()


def get_bokeh_palette(name):
    d = getattr(bokeh.palettes, name, None)
    if isinstance(d, tuple):
        colors = d
    elif isinstance(d, dict):
        k = max(d.keys())
        colors = d[k]
    else:
        print("error with bokeh palette %s" % name)
        return None
    return [webcolors.hex_to_rgb(h) for h in colors]


def get_cmap(n):
    name = MATPLOTLIB_NAMES.get(n.lower(), None)
    if name:
        return get_matplotlib_palette(name)
    name = COLORCET_NAMES.get(n.lower(), None)
    if name:
        return get_colorcet_palette(name)
    name = BOKEH_NAMES.get(n.lower(), None)
    if name:
        return get_bokeh_palette(name)
    # special colormaps in CSV files
    path = Path(__file__).parent / ("%s-table-byte-0256.csv" % n.lower())
    if path.exists():
        return [list(map(int, l.split(','))) for l in path.read_text().strip().splitlines()]
    print("name %s not found!" % n)


def _remove_comments(text):
    return '\n'.join([l.split('//')[0].rstrip() for l in text.splitlines()])


def parse_defines(text):
    text = _remove_comments(text)
    defines = re.findall(
        r"#define (C[A-Z\_0-9]+)\s+([^\n]+)", text, re.MULTILINE)
    defines = dict(defines)
    defines = {k: v.replace('(', '').replace(')', '')
               for k, v in defines.items()}
    defines = {k: v.strip()
               for k, v in defines.items()
               if k.startswith('CMAP') or
               k.startswith('CPAL') or
               k == 'CONTROL_MAX'}
    for k, v in defines.items():
        if v.isdigit():
            defines[k] = int(v)
    for k, v in defines.items():
        if isinstance(v, str) and '+' not in v and '-' not in v:
            defines[k] = defines[v]
    for k, v in defines.items():
        if isinstance(v, str) and '+' in v:
            defines[k] = defines[v.split(' + ')[0]] + defines[v.split(' + ')[1]]
        if isinstance(v, str) and '-' in v:
            defines[k] = defines[v.split(' - ')[0]] - defines[v.split(' - ')[1]]
    return defines


def generate_binary():
    colormaps = path.read_text()
    defines = parse_defines(colormaps)
    groups = re.findall(
        r"^\s+(DVZ\_C[PM][A-Z0-9]+\_)([^\,\=\s]+)", colormaps, re.MULTILINE)
    texture = np.zeros((256, 256, 4), dtype=np.uint8)
    texture[..., -1] = 255

    last_prefix = None
    row = 0
    col = 0
    out = []
    for prefix, name in groups:
        l = get_cmap(name)
        assert l is not None
        lines = np.array(l, dtype=np.uint8)
        # lines = np.hstack((l, np.ones((lines.shape[0], 1), dtype=np.uint8)))
        if prefix != "DVZ_CPAL032_":
            assert lines.shape == (256, 3)

        if prefix == "DVZ_CPAL256_" and last_prefix != prefix:
            row = defines["CPAL256_OFS"]
        elif prefix == "DVZ_CPAL032_" and last_prefix != prefix:
            row = defines["CPAL032_OFS"]

        out.append(','.join((name, str(row), str(col), str(lines.shape[0]))))
        texture[row, col:col + lines.shape[0], :3] = lines

        if prefix in ("DVZ_CMAP_", "DVZ_CPAL256_"):
            row += 1
        elif prefix == "DVZ_CPAL032_":
            col += 32
            col = col % 256
            if col == 0:
                row += 1
        last_prefix = prefix

    # Save the binary file and also PNG for debugging purposes.
    texture.tofile(texture_path)
    print("%s created." % texture_path)

    image.imsave(texture_path.with_suffix(".png"), texture / 255.)
    print("%s created." % texture_path.with_suffix(".png"))

    csv_path = Path(__file__).parent.parent / \
        "data/textures/color_texture.csv"
    print(csv_path.resolve())
    with open(csv_path, 'w') as f:
        f.write('name,row,col,size\n')
        f.write('\n'.join(out) + '\n')


def img2b64(x):
    assert x.dtype == np.dtype(np.uint8)
    im = Image.fromarray(x)
    rawBytes = io.BytesIO()
    im.save(rawBytes, "PNG")
    rawBytes.seek(0)
    return base64.b64encode(rawBytes.read()).decode('ascii')


def img2md(row, col, name, arr):
    return f"| `{name.lower()}` | {row}, {col} | ![{name}](data:image/png;base64,{img2b64(arr)}) |"


def load_colormap_texture():
    return np.fromfile(
        ROOT_DIR / "data/textures/color_texture.img", dtype=np.uint8).reshape((256, 256, 4))


def generate_colormaps_doc():
    arr = load_colormap_texture()
    out = "| Name | Row, Col | Colormap |\n| ---- | ---- | ---- |\n"
    with open(ROOT_DIR / "data/textures/color_texture.csv", 'r') as f:
        r = csv.reader(f, delimiter=',')
        next(r)
        for line in r:
            name = line[0]
            row = int(line[1])
            col = int(line[2])
            size = int(line[3])
            cmap = arr[[row], col:col + size, :]
            cmap = np.repeat(cmap, 24, axis=0)
            cmap = np.repeat(cmap, 512 // cmap.shape[1], axis=1)
            # print(cmap.shape)
            out += img2md(row, col, name, cmap) + '\n'
    return out


if __name__ == '__main__':
    generate_binary()
    generate_colormaps_doc()
    pass
