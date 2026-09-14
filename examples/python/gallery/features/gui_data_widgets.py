#!/usr/bin/env python3
"""Native Python adaptation of the retained tree and table GUI example."""

from __future__ import annotations

import numpy as np

import datoviz as dvz
from examples.python.gallery import common as ex

TREE_KEYS = np.array([997, 8, 567, 688, 315, 1089, 623, 343, 1129, 549], dtype=np.uint64)
TREE_PARENTS = np.array([2**32 - 1, 0, 1, 2, 3, 2, 2, 1, 7, 8], dtype=np.uint32)
TREE_LABELS = ('root', 'grey', 'CH', 'CTX', 'Isocortex', 'HPF', 'CNU', 'BS', 'IB', 'TH')
TREE_SECONDARY = (
    'root',
    'Basic cell groups and regions',
    'Cerebrum',
    'Cerebral cortex',
    'Isocortex',
    'Hippocampal formation',
    'Cerebral nuclei',
    'Brain stem',
    'Interbrain',
    'Thalamus',
)


def _build_widgets():
    tree = dvz.dvz_gui_tree(b'python_gui_data_widgets_tree', dvz.DVZ_GUI_DATA_WIDGET_FLAGS_FILTER)
    if not tree:
        raise RuntimeError('dvz_gui_tree() failed')
    if dvz.dvz_gui_tree_set_rows(tree, TREE_KEYS, TREE_PARENTS, TREE_LABELS, TREE_SECONDARY) != 0:
        raise RuntimeError('dvz_gui_tree_set_rows() failed')
    swatches = np.array(
        [
            [242, 242, 242, 255],
            [166, 219, 232, 255],
            [166, 219, 232, 255],
            [140, 224, 153, 255],
            [89, 242, 110, 255],
            [122, 209, 107, 255],
            [122, 189, 222, 255],
            [250, 94, 122, 255],
            [250, 94, 122, 255],
            [250, 110, 128, 255],
        ],
        dtype=np.uint8,
    )
    if dvz.dvz_gui_tree_set_swatches(tree, swatches) != 0:
        raise RuntimeError('dvz_gui_tree_set_swatches() failed')
    style = dvz.dvz_gui_data_style()
    style.row_key = 315
    style.flags = dvz.DVZ_GUI_DATA_STYLE_FLAGS_ACCENT
    style.accent = ex.CYAN
    if dvz.dvz_gui_tree_set_styles(tree, [style]) != 0:
        raise RuntimeError('dvz_gui_tree_set_styles() failed')
    if dvz.dvz_gui_tree_expand_all(tree) != 0:
        raise RuntimeError('dvz_gui_tree_expand_all() failed')

    columns = [
        {
            'column_id': 1,
            'type': dvz.DVZ_GUI_TABLE_COLUMN_TEXT,
            'flags': dvz.DVZ_GUI_TABLE_COLUMN_FLAGS_SEARCHABLE
            | dvz.DVZ_GUI_TABLE_COLUMN_FLAGS_STRETCH,
            'title': 'Region',
        },
        {
            'column_id': 2,
            'type': dvz.DVZ_GUI_TABLE_COLUMN_DOUBLE,
            'flags': dvz.DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE,
            'title': 'Signal',
            'format': '%.2f',
        },
        {
            'column_id': 3,
            'type': dvz.DVZ_GUI_TABLE_COLUMN_INT64,
            'flags': dvz.DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE,
            'title': 'Count',
        },
        {'column_id': 4, 'type': dvz.DVZ_GUI_TABLE_COLUMN_COLOR, 'title': 'Color'},
    ]
    table = dvz.dvz_gui_table(
        b'python_gui_data_widgets_table',
        columns,
        dvz.DVZ_GUI_DATA_WIDGET_FLAGS_MULTI_SELECT | dvz.DVZ_GUI_DATA_WIDGET_FLAGS_FILTER,
    )
    if not table:
        raise RuntimeError('dvz_gui_table() failed')
    keys = np.arange(1001, 1007, dtype=np.uint64)
    if dvz.dvz_gui_table_set_rows(table, keys) != 0:
        raise RuntimeError('dvz_gui_table_set_rows() failed')
    if dvz.dvz_gui_table_set_column_text(table, 1, ('VISp', 'MOp', 'CA1', 'TH', 'HY', 'SCs')) != 0:
        raise RuntimeError('dvz_gui_table_set_column_text() failed')
    if (
        dvz.dvz_gui_table_set_column_double(
            table, 2, np.array([0.18, 0.92, 0.47, 0.31, 0.76, 0.59], dtype=np.float64)
        )
        != 0
    ):
        raise RuntimeError('dvz_gui_table_set_column_double() failed')
    if (
        dvz.dvz_gui_table_set_column_int64(
            table, 3, np.array([120, 480, 256, 192, 384, 320], dtype=np.int64)
        )
        != 0
    ):
        raise RuntimeError('dvz_gui_table_set_column_int64() failed')
    colors = np.array(
        [
            [64, 179, 242, 255],
            [242, 115, 89, 255],
            [115, 217, 140, 255],
            [179, 140, 242, 255],
            [242, 191, 64, 255],
            [89, 217, 217, 255],
        ],
        dtype=np.uint8,
    )
    if dvz.dvz_gui_table_set_column_color(table, 4, colors) != 0:
        raise RuntimeError('dvz_gui_table_set_column_color() failed')
    return tree, table


def _gui_callback_factory(tree, table):
    status = {'text': 'Select regions or sort a column'}

    def callback(gui, _view, _user_data):
        if dvz.dvz_gui_begin(gui, b'Retained data widgets', None, 0):
            dvz.dvz_gui_text(gui, b'Allen-inspired hierarchy and typed region table')
            dvz.dvz_gui_separator_text(gui, b'Atlas hierarchy')
            _, tree_events, dropped = dvz.dvz_gui_tree_draw(gui, tree)
            dvz.dvz_gui_separator_text(gui, b'Mapped regions')
            _, table_events, dropped_table = dvz.dvz_gui_table_draw(gui, table)
            events = tree_events + table_events
            if dropped or dropped_table:
                status['text'] = 'Events dropped; resynchronize widget state'
            elif events:
                event = events[-1]
                status['text'] = f'Last event: {event.type} (key {event.row_key})'
            dvz.dvz_gui_text(gui, status['text'].encode())
        dvz.dvz_gui_end(gui)

    return callback


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    tree = table = None
    try:
        tree, table = _build_widgets()
        app = dvz.dvz_app(scene)
        if not app:
            raise RuntimeError('dvz_app() failed')
        view = dvz.dvz_view_window(app, figure, ex.WIDTH, ex.HEIGHT, b'GUI Data Widgets')
        if not view:
            raise RuntimeError('dvz_view_window() failed')
        if not dvz.dvz_view_gui(view, None):
            raise RuntimeError('dvz_view_gui() failed')
        if dvz.dvz_view_set_gui_callback(view, _gui_callback_factory(tree, table), None) != 0:
            raise RuntimeError('dvz_view_set_gui_callback() failed')
        ex.run_app(app, view)
    finally:
        if tree:
            dvz.dvz_gui_tree_destroy(tree)
        if table:
            dvz.dvz_gui_table_destroy(table)
        if 'app' in locals() and app:
            dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


if __name__ == '__main__':
    main()
