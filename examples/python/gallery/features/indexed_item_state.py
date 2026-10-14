#!/usr/bin/env python3
"""Two mesh groups share a fixed state bank; hover changes two four-byte words."""
from __future__ import annotations

import ctypes

import numpy as np
import datoviz as dvz
from examples.python.gallery import common as ex


def main():
    scene, figure, panel = ex.scene_panel()
    mesh = dvz.dvz_mesh(scene, 0)
    positions = np.array([[-.85,-.65,0],[-.05,-.65,0],[-.45,.65,0],
                          [.05,-.65,0],[.85,-.65,0],[.45,.65,0]], dtype=np.float32)
    dvz.dvz_visual_set_data(mesh, 'position', positions)
    dvz.dvz_visual_set_data(mesh, 'normal', np.tile(np.array([0,0,1], np.float32), (6,1)))
    dvz.dvz_visual_set_data(mesh, 'color', np.array([[38,132,167,255]]*3 +
                                                   [[42,176,142,255]]*3, np.uint8))
    dvz.dvz_visual_set_index_data(mesh, np.arange(6, dtype=np.uint32))
    desc = dvz.dvz_item_state_buffer_desc()
    desc.slot_count = 4096
    bank = dvz.dvz_item_state_buffer(scene, ctypes.byref(desc))
    if not bank:
        raise RuntimeError('state bank creation failed')
    indices = np.array([11]*3 + [4000]*3, np.uint32)
    if dvz.dvz_visual_set_item_state_buffer(mesh, bank, indices) != 0:
        raise RuntimeError('state binding failed')
    selection = dvz.dvz_selection_visual_style()
    selection.unselected_alpha = .35
    hovered = dvz.dvz_item_state_visual_style()
    hovered.visual_flags = dvz.DVZ_ITEM_STATE_VISUAL_TINT
    hovered.tint = dvz.DvzColor(255,255,255,255)
    hovered.tint_mix = .65
    dvz.dvz_item_state_buffer_style(bank, ctypes.byref(selection), ctypes.byref(hovered), True)
    slots = np.array([11,4000], np.uint32)
    words = np.array([dvz.DVZ_ITEM_STATE_SELECTED | dvz.DVZ_ITEM_STATE_HOVERED,0], np.uint32)
    dvz.dvz_item_state_buffer_update(bank, slots, words)
    dvz.dvz_visual_set_alpha_mode(mesh, dvz.DVZ_ALPHA_WBOIT)
    ex.add_visual(panel, mesh)
    previous = [0]

    def on_frame(_view, _index, elapsed):
        active = int(elapsed) % 2
        if active == previous[0]:
            return
        words[:] = [dvz.DVZ_ITEM_STATE_SELECTED,0]
        words[active] |= dvz.DVZ_ITEM_STATE_HOVERED
        dvz.dvz_item_state_buffer_update(bank, slots, words)
        previous[0] = active

    ex.run_with_frame_callback(scene, figure, 'Indexed Item State', on_frame)


if __name__ == '__main__':
    main()
