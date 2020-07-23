#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Create a multi-channel signed distance field map.
Use https://github.com/Chlumsky/msdfgen/

You need in this directory:

* msdfgen executable

Usage example:

    python utils/make_font_texture.py data/fonts/Inconsolata-Regular.ttf data/textures/font_inconsolata.png

TODO: properly integrate msdfgen cmake project and automate the creation of the font texture, the user choosing a ttf file directly

"""


import gzip
import os
from pathlib import Path
import sys

import imageio
import numpy as np


FONT_MAP_SIZE = (6, 16)
GLYPH_SIZE = (34, 64)
FONT_MAP_CHARS = ''.join(chr(i) for i in range(32, 32 + FONT_MAP_SIZE[0] * FONT_MAP_SIZE[1]))

TTF_PATH = Path(sys.argv[1])
OUTPUT = Path(sys.argv[2])


class FontMapGenerator(object):
    """Generate a SDF font map for a monospace font, with a given uniform glyph size."""
    def __init__(self):
        self.glyph_output = Path("tmp.png")
        self.font = TTF_PATH
        self.output = OUTPUT

        self.rows, self.cols = FONT_MAP_SIZE
        self.msdfgen_path = Path(__file__).parent / 'msdfgen'
        self.width, self.height = GLYPH_SIZE
        self.chars = FONT_MAP_CHARS

    def _iter_table(self):
        for i in range(self.rows):
            for j in range(self.cols):
                yield i, j

    def _get_char_number(self, i, j):
        return 32 + i * 16 + j

    def _get_glyph_range(self, i, j):
        x = j * self.width
        y = i * self.height
        return x, x + self.width, y, y + self.height

    def _get_cmd(self, char_number):
        """Command that generates a glyph signed distance field PNG to be used in the font map."""
        return (
            f'{self.msdfgen_path} msdf -font {self.font} {char_number} -o {self.glyph_output} '
            f'-size {self.width} {self.height} '
            '-pxrange 4 -scale 3.75 -translate 0.35 4')

    def _get_glyph_array(self, char_number):
        """Return the NumPy array with a glyph, by calling the msdfgen tool."""
        cmd = self._get_cmd(char_number)
        os.system(cmd)
        assert self.glyph_output.exists()
        return imageio.imread(self.glyph_output)

    def _make_font_map(self):
        """Create the font map by putting together all used glyphs."""
        Z = np.zeros((self.height * self.rows, self.width * self.cols, 4), dtype=np.uint8)
        Z[..., -1] = 255
        for i, j in self._iter_table():
            char_number = self._get_char_number(i, j)
            x0, x1, y0, y1 = self._get_glyph_range(i, j)
            glyph = self._get_glyph_array(char_number)
            Z[y0:y1, x0:x1, :3] = np.atleast_3d(glyph)[:, :, :3]
        return Z

    def make(self):
        Z = self._make_font_map()
        imageio.imwrite(self.output, Z)
        os.remove(self.glyph_output)


if __name__ == '__main__':
    fmg = FontMapGenerator()
    fmg.make()
