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
    'basic': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'group': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1,)},
        'size': {'type': float},
    },

    'pixel': {
        'position': {'type': np.ndarray, 'dtype': np.float32, 'shape': (-1, 3)},
        'color': {'type': np.ndarray, 'dtype': np.uint8, 'shape': (-1, 4)},
        'size': {'type': float},
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

    def basic(
            self, topology: str = None, position: np.ndarray = None, color: np.ndarray = None,
            group: np.ndarray = None, size: float = None):
        c_topology = dvz.to_enum(f'primitive_topology_{topology}')
        c_visual = dvz.basic(self.c_batch, c_topology, 0)
        visual = Visual(c_visual, 'basic')
        visual.position[:] = position
        visual.color[:] = color
        visual.group[:] = group
        visual.size = size
        return visual

    def pixel(self, position: np.ndarray = None, color: np.ndarray = None, size: float = None):
        c_visual = dvz.pixel(self.c_batch, 0)
        visual = Visual(c_visual, 'pixel')
        visual.position[:] = position
        visual.color[:] = color
        visual.size = size
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
    _prop_classes: dict = None

    def __init__(self, c_visual: dvz.DvzVisual, visual_name: str):
        assert visual_name
        assert visual_name in PROPS
        assert c_visual

        # UGLY HACK: we override __setattr__() which only works AFTER self.visual_name has been
        # set, but we can't set self.visual_name the usual way because it calls __setattr__()
        # which requires self.visual_name! So we directly manipulate the __dict__ instead.
        self.__dict__['visual_name'] = visual_name
        self.c_visual = c_visual
        self._prop_classes = {}

    def set_prop_class(self, prop_name: str, prop_cls: tp.Type):
        self._prop_classes[prop_name] = prop_cls

    def __getattr__(self, prop_name: str):
        prop_type = PROPS[self.visual_name].get(prop_name, {}).get('type', None)
        if prop_type == np.ndarray:
            prop_cls = self._prop_classes.get(prop_name, Prop)
            return prop_cls(self, prop_name)
        else:
            raise Exception(f"Prop '{prop_name}' is not a valid array property for visual {self}")

    def __setattr__(self, prop_name: str, value: object):
        prop_type = PROPS[self.visual_name].get(prop_name, {}).get('type', None)
        if not prop_type:
            return super().__setattr__(prop_name, value)
        elif prop_type != np.ndarray:
            prop_cls = self._prop_classes.get(prop_name, Prop)
            prop = prop_cls(self, prop_name)
            if value is not None:
                value = prop_type(value)
                prop.call(self.c_visual, value)
        else:
            raise Exception(f"Prop '{prop_name}' is not a valid scalar property for visual {self}")


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
        assert ndim == pvalue.ndim
        for dim in range(ndim):
            if shape[dim] > 0 and pvalue.shape[dim] != shape[dim]:
                raise ValueError(f"Incorrect shape {pvalue.shape[dim]} != {shape[dim]}")
        return pvalue

    def call(self, *args):
        return self._fn(*args)

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

    visual = app.basic('line_list', position=position, color=color, size=2)

    panel.add(visual)
    app.run()
