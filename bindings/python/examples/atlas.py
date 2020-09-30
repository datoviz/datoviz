from pathlib import Path

import numpy as np

from visky.wrap import viskylib as vl, upload_data, pointer
from visky import _constants as const
from visky import _types as tp
from visky import api as vk

from ibllib.atlas import AllenAtlas

res = 25
atlas = AllenAtlas(res)

c = vk.canvas(shape=(1, 3))


def make_image(im):
    return vk.get_color(
        'gray', im, vmin=np.nanmin(im), vmax=np.nanmax(im), alpha=1)


c[0, 0].axes_range(atlas.bc.xlim[0], atlas.bc.ylim[1],
                   atlas.bc.xlim[1], atlas.bc.ylim[0])
c[0, 1].axes_range(atlas.bc.ylim[1], atlas.bc.zlim[1],
                   atlas.bc.ylim[0], atlas.bc.zlim[0])
c[0, 2].axes_range(atlas.bc.xlim[0], atlas.bc.zlim[1],
                   atlas.bc.xlim[1], atlas.bc.zlim[0])

v_imtop = c[0, 0].imshow(make_image(atlas.top))
v_im1 = c[0, 1].imshow(make_image(atlas.image[:, 0, :].T))
v_im2 = c[0, 2].imshow(make_image(atlas.image[0, :, :].T))


@ c.on_mouse
def on_mouse(button, pos, ev=None):
    if button is None:
        return
    pick = vl.vky_pick(c._scene, tp.T_VEC2(pos[0], pos[1]))
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
