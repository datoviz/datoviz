"""Focused tests for retained GUI widget adaptation in the generated facade."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz._array_facade as facade


def test_tree_rows_encodes_string_sequences_and_infers_count(monkeypatch):
    calls = []

    def raw(*args):
        calls.append(args)
        return 0

    monkeypatch.setattr(facade._raw, 'dvz_gui_tree_set_rows', raw)
    keys = np.array([0, 1], dtype=np.uint64)
    parents = np.array([2**32 - 1, 0], dtype=np.uint32)
    facade.dvz_gui_tree_set_rows(1, keys, parents, ['root', 'é'], [None, 'child'])

    _, count, key_ptr, parent_ptr, labels, secondary, _ = calls[0]
    assert count == 2
    assert key_ptr[0] == 0 and parent_ptr[1] == 0
    assert labels[0] == b'root' and labels[1] == 'é'.encode()
    assert secondary[0] is None and secondary[1] == b'child'

    facade.dvz_gui_tree_set_rows(1, keys, parents, ['root', 'é'], None)
    assert calls[-1][5] is None


def test_tree_rows_rejects_string_count_mismatch():
    keys = np.array([0, 1], dtype=np.uint64)
    parents = np.array([2**32 - 1, 0], dtype=np.uint32)
    try:
        facade.dvz_gui_tree_set_rows(1, keys, parents, ['root'])
    except ValueError as exc:
        assert 'labels length' in str(exc)
    else:
        raise AssertionError('expected label count validation')


def test_table_text_and_draw_are_single_raw_calls(monkeypatch):
    calls = []

    def set_text(*args):
        calls.append(('text', args))
        return 0

    def draw(*args):
        calls.append(('draw', args))
        args[4]._obj.value = 1
        args[5]._obj.value = 0
        args[2][0].row_key = 42
        return 0

    monkeypatch.setattr(facade._raw, 'dvz_gui_table_set_column_text', set_text)
    monkeypatch.setattr(facade._raw, 'dvz_gui_table_draw', draw)
    facade.dvz_gui_table_set_column_text(1, 7, ['a', 'b'])
    result, events, dropped = facade.dvz_gui_table_draw(1, 2, capacity=4)

    assert result == 0
    assert len([call for kind, call in calls if kind == 'text']) == 1
    assert len([call for kind, call in calls if kind == 'draw']) == 1
    assert events[0].row_key == 42
    assert dropped == 0


def test_table_descriptor_mapping_initializes_size_and_encodes_strings(monkeypatch):
    calls = []

    def raw(*args):
        calls.append(args)
        return None

    monkeypatch.setattr(facade._raw, 'dvz_gui_table', raw)
    facade.dvz_gui_table('widget', [{'column_id': 3, 'type': 0, 'title': 'é'}])

    widget_id, count, columns, flags = calls[0]
    assert widget_id == b'widget'
    assert count == 1 and flags == 0
    assert columns[0].struct_size == ctypes.sizeof(facade._raw.DvzGuiTableColumnDesc)
    assert columns[0].title == 'é'.encode()


def test_table_descriptor_rejects_unknown_fields_and_count_mismatch():
    for columns, message in [
        ([{'column_id': 1, 'unknown': 2}], 'unknown fields'),
        ([{'column_id': 1}], 'column_count'),
    ]:
        try:
            facade.dvz_gui_table('widget', columns, column_count=2 if message == 'column_count' else None)
        except (TypeError, ValueError) as exc:
            assert message in str(exc)
        else:
            raise AssertionError('expected descriptor validation')


def test_draw_accepts_ctypes_array_and_rejects_python_list(monkeypatch):
    def draw(*args):
        args[4]._obj.value = 0
        args[5]._obj.value = 0
        return 0

    monkeypatch.setattr(facade._raw, 'dvz_gui_tree_draw', draw)
    events = (facade._raw.DvzGuiDataEvent * 2)()
    assert facade.dvz_gui_tree_draw(1, 2, events)[1] == []
    try:
        facade.dvz_gui_tree_draw(1, 2, [])
    except TypeError as exc:
        assert 'ctypes array' in str(exc)
    else:
        raise AssertionError('expected Python-list rejection')


def test_color_columns_require_rgba8_shape(monkeypatch):
    calls = []

    def raw(*args):
        calls.append(args)
        return 0

    monkeypatch.setattr(facade._raw, 'dvz_gui_table_set_column_color', raw)
    colors = np.zeros((2, 4), dtype=np.uint8)
    facade.dvz_gui_table_set_column_color(1, 3, colors)
    assert len(calls) == 1
    for invalid in (np.zeros((2, 3), dtype=np.uint8), np.zeros((2, 4), dtype=np.int32)):
        try:
            facade.dvz_gui_table_set_column_color(1, 3, invalid)
        except ValueError:
            pass
        else:
            raise AssertionError('expected strict RGBA8 validation')


def test_retained_widget_queries_allocate_and_decode(monkeypatch):
    selection_calls = []

    def get_selection(widget, capacity, output):
        selection_calls.append(capacity)
        if capacity == 0:
            return 2
        output[0], output[1] = 11, 22
        return 2

    def empty_selection(widget, capacity, output):
        return 0

    def get_filter(widget, capacity, output):
        if capacity == 0:
            return 5
        output.value = 'é'.encode() + b'\0'
        return 5

    def get_empty_filter(widget, capacity, output):
        return 0

    monkeypatch.setattr(facade._raw, 'dvz_gui_tree_get_selection', get_selection)
    monkeypatch.setattr(facade._raw, 'dvz_gui_tree_get_expanded', empty_selection)
    monkeypatch.setattr(facade._raw, 'dvz_gui_table_get_selection', empty_selection)
    monkeypatch.setattr(facade._raw, 'dvz_gui_tree_get_filter', get_filter)
    monkeypatch.setattr(facade._raw, 'dvz_gui_table_get_filter', get_empty_filter)

    assert facade.dvz_gui_tree_get_selection(1) == [11, 22]
    assert selection_calls == [0, 2]
    assert facade.dvz_gui_tree_get_expanded(1) == []
    assert facade.dvz_gui_table_get_selection(1) == []
    assert facade.dvz_gui_tree_get_filter(1) == 'é'
    assert facade.dvz_gui_table_get_filter(1) == ''


def test_table_sort_query_returns_result_and_outputs(monkeypatch):
    def get_sort(table, column_id, direction):
        column_id._obj.value = 7
        direction._obj.value = -1
        return 0

    monkeypatch.setattr(facade._raw, 'dvz_gui_table_get_sort', get_sort)
    assert facade.dvz_gui_table_get_sort(1) == (0, 7, -1)
