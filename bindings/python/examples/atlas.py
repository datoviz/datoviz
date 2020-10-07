from pathlib import Path

import numpy as np

from visky.wrap import viskylib as vl, upload_data, pointer, get_const
from visky import _constants as const
from visky import _types as tp
from visky import api as vk

from ibllib.atlas import AllenAtlas

res = 25
atlas = AllenAtlas(res)

c = vk.canvas(shape=(1, 3), background=get_const('black'))


def make_image(im):
    return vk.get_color(
        'gray', im, vmin=np.nanmin(im), vmax=np.nanmax(im), alpha=1)


xmin, xmax = atlas.bc.xlim
ymin, ymax = atlas.bc.ylim
zmin, zmax = atlas.bc.zlim

c[0, 0].axes_range(xmin, ymax, xmax, ymin)
c[0, 1].axes_range(ymax, zmax, ymin, zmin)
c[0, 2].axes_range(xmin, zmax, xmax, zmin)

v_imtop = c[0, 0].imshow(make_image(atlas.top))
v_im1 = c[0, 1].imshow(make_image(atlas.image[:, 0, :].T))
v_im2 = c[0, 2].imshow(make_image(atlas.image[0, :, :].T))

xmin, .5 * (ymin + ymax), 0
xmax, .5 * (ymin + ymax), 0

p0 = np.array([[-1, -1, 0]], dtype=np.float32)
p1 = np.array([[+1, +1, 0]], dtype=np.float32)
col = np.array([[255, 255, 0, 255]], dtype=np.uint8)
c[0, 0].segments(p0, p1, color=col, lw=10, cap='round', transform_mode=None)


@ c.on_mouse
def on_mouse(button, pos, ev=None):
    if button is None:
        return
    pick = vl.vky_pick(c._scene, tp.T_VEC2(pos[0], pos[1]), None)
    idx = vl.vky_get_panel_index(pick.panel)
    if (idx.row, idx.col) != (0, 0):
        return
    x, y = pick.pos_data

    j = atlas.bc.x2i(x)
    i = atlas.bc.y2i(y)

    i = np.clip(i, 0, atlas.bc.nxyz[1] - 1)
    j = np.clip(j, 0, atlas.bc.nxyz[0] - 1)

    v_im1.set_image(make_image(atlas.image[::-1, j, :].T))
    v_im2.set_image(make_image(atlas.image[i, ::-1, :].T))


vk.run()
