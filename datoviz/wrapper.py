"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# Python wrapper

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

import typing as tp
import numpy as np
import datoviz as dvz


# -------------------------------------------------------------------------------------------------
# Globals
# -------------------------------------------------------------------------------------------------

DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 600
PROPS = {
    'pixel': {
        'position': {'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'dtype': np.uint8, 'shape': (-1, 4)},
    },
}


# -------------------------------------------------------------------------------------------------
# Utils
# -------------------------------------------------------------------------------------------------

def parse_index(idx, value):
    if value is None:
        return 0, 0
    assert value.size > 0
    assert value.ndim >= 1
    offset = size = 0
    if isinstance(idx, slice):
        offset = idx.start or 0
        step = idx.step or 1
        size = (idx.stop or value.shape[0] - offset) // step
    return offset, size


# -------------------------------------------------------------------------------------------------
# App
# -------------------------------------------------------------------------------------------------

class App:
    _flags: int = 0
    c_app: dvz.DvzApp = None
    c_batch: dvz.DvzBatch = None
    c_scene: dvz.DvzScene = None

    def __init__(self, flags: int = 0):
        self._flags = flags
        self.c_app = dvz.app(flags)
        self.c_batch = dvz.app_batch(self.c_app)
        self.c_scene = dvz.scene(self.c_batch)

    def figure(self, width: int = DEFAULT_WIDTH, height: int = DEFAULT_HEIGHT):
        c_figure = dvz.figure(self.c_scene, width, height, 0)
        return Figure(c_figure)

    def pixel(self, position: np.ndarray = None, color: np.ndarray = None):
        c_visual = dvz.pixel(self.c_batch, 0)
        visual = Visual(c_visual, 'pixel')
        visual.position[:] = position
        visual.color[:] = color
        return visual

    def on_timer(self):
        pass

    def on_frame(self):
        pass

    def run(self, frame_count: int = 0):
        dvz.scene_run(self.c_scene, self.c_app, frame_count)

    def __del__(self):
        self.destroy()

    def destroy(self):
        dvz.scene_destroy(self.c_scene)
        dvz.app_destroy(self.c_app)


# -------------------------------------------------------------------------------------------------
# Figure
# -------------------------------------------------------------------------------------------------

class Figure:
    c_figure: dvz.DvzFigure = None

    def __init__(self, c_figure: dvz.DvzFigure):
        assert c_figure
        self.c_figure = c_figure

    def panel(self):
        c_panel = dvz.panel_default(self.c_figure)
        return Panel(c_panel)

    def on_mouse(self):
        pass

    def on_keyboard(self):
        pass

    def on_gui(self):
        pass


# -------------------------------------------------------------------------------------------------
# Visual
# -------------------------------------------------------------------------------------------------

class Visual:
    c_visual: dvz.DvzVisual = None
    visual_name: str = ''

    def __init__(self, c_visual: dvz.DvzVisual, visual_name: str):
        assert visual_name
        assert visual_name in PROPS
        assert c_visual

        self.visual_name = visual_name
        self.c_visual = c_visual

    def __getattr__(self, prop_name: str):
        if prop_name in PROPS[self.visual_name]:
            return Prop(self, prop_name)
        else:
            raise Exception(f"Prop '{prop_name}' is not a recognized property for visual {self}")


class Prop:
    visual: Visual = None
    prop_name: str = ''
    _fn: tp.Callable = None
    _fn_alloc: tp.Callable = None

    def __init__(self, visual: Visual, prop_name: str):
        assert visual
        assert prop_name

        self.visual = visual
        self.visual_name = visual.visual_name
        self.prop_name = prop_name

        self._fn = getattr(dvz, f"{visual.visual_name}_{prop_name}")
        self._fn_alloc = getattr(dvz, f"{visual.visual_name}_alloc")

    def prepare_data(self, value):
        info = PROPS[self.visual_name][self.prop_name]
        dtype = info['dtype']
        shape = info['shape']
        ndim = len(shape)
        pvalue = np.asanyarray(value, dtype=dtype)
        if pvalue.ndim < ndim:
            if pvalue.ndim == 2:
                pvalue = np.atleast_2d(pvalue)
            elif pvalue.ndim == 3:
                pvalue = np.atleast_3d(pvalue)
        elif pvalue.ndim > ndim:
            raise ValueError(
                f"Visual property {self.visual_name}.{self.prop_name} should have shape "
                f"{shape} instead of {pvalue.shape}")
        return pvalue

    def __setitem__(self, idx, value):
        if value is None:
            return
        offset, length = parse_index(idx, value)
        pvalue = self.prepare_data(value)

        assert offset >= 0
        assert length > 0

        # Allocation
        if value is not None:
            count = offset + length
            self._fn_alloc(self.visual.c_visual, count)

        # Property setter
        if value is not None:
            self._fn(self.visual.c_visual, offset, length, pvalue, 0)


# -------------------------------------------------------------------------------------------------
# Panel
# -------------------------------------------------------------------------------------------------

class Panzoom:
    c_panzoom: dvz.DvzPanzoom = None

    def __init__(self, c_panzoom: dvz.DvzPanzoom):
        assert c_panzoom
        self.c_panzoom = c_panzoom


class Panel:
    c_panel: dvz.DvzPanel = None

    def __init__(self, c_panel: dvz.DvzPanel):
        assert c_panel
        self.c_panel = c_panel

    def add(self, visual: Visual):
        assert visual
        dvz.panel_visual(self.c_panel, visual.c_visual, 0)

    def panzoom(self, flags: int = None):
        c_panzoom = dvz.panel_panzoom(self.c_panel, flags)
        return Panzoom(c_panzoom)


if __name__ == '__main__':
    app = App()
    fig = app.figure()
    panel = fig.panel()
    panzoom = panel.panzoom()

    n = 10_000
    position = np.random.normal(size=(n, 3), scale=.25)
    color = np.random.randint(size=(n, 4), low=100, high=255)
    pixel = app.pixel(position=position, color=color)
    panel.add(pixel)

    app.run()
