/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  Retained tree and table data widgets                                                         */
/*************************************************************************************************/

#include "datoviz/gui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits.h>

#include "_alloc.h"
#include "_gui.h"
#include "imgui.h"
#include "imgui_internal.h"

#define WIDGET_FLAGS (DVZ_GUI_DATA_WIDGET_FLAGS_MULTI_SELECT | DVZ_GUI_DATA_WIDGET_FLAGS_FILTER)
#define STYLE_FLAGS                                                                               \
    (DVZ_GUI_DATA_STYLE_FLAGS_FOREGROUND | DVZ_GUI_DATA_STYLE_FLAGS_BACKGROUND |                  \
     DVZ_GUI_DATA_STYLE_FLAGS_ACCENT | DVZ_GUI_DATA_STYLE_FLAGS_DISABLED)
#define COLUMN_FLAGS                                                                              \
    (DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE | DVZ_GUI_TABLE_COLUMN_FLAGS_SEARCHABLE |                \
     DVZ_GUI_TABLE_COLUMN_FLAGS_STRETCH)
#define TREE_LAYOUT_KNOWN_FLAGS                                                                   \
    (DVZ_GUI_TREE_LAYOUT_FLAGS_EXACT_SPACING | DVZ_GUI_TREE_LAYOUT_FLAGS_READABLE_SECONDARY)

typedef struct _KeyIndex
{
    uint64_t key;
    uint32_t index;
} _KeyIndex;

typedef struct _Rows
{
    uint32_t count;
    uint64_t* keys;
    _KeyIndex* index;
    bool* selected;
    bool* visible;
    bool* matches;
    uint32_t* order;
    DvzGuiDataStyle* styles;
    uint32_t style_count;
} _Rows;

typedef struct _Core
{
    char* id;
    uint32_t flags;
    uint32_t count;
    uint64_t* keys;
    _KeyIndex* index;
    bool* selected;
    bool* visible;
    bool* matches;
    bool matches_active;
    uint32_t* order;
    uint32_t order_count;
    bool dirty;
    uint64_t rebuild_count;
    DvzGuiDataStyle* styles;
    uint32_t style_count;
    char* filter;
    size_t filter_capacity;
    uint64_t revision;
    uint64_t anchor;
    bool has_anchor;
    DvzGui* gui;
    uint64_t frame;
} _Core;

struct DvzGuiTree
{
    _Core core;
    uint32_t* parents;
    uint32_t* depths;
    bool* branches;
    char** labels;
    char** secondary;
    DvzColor* swatches;
    bool* expanded;
    DvzGuiTreeLayout layout;
    uint64_t reveal;
    bool reveal_pending;
    uint32_t* display_depths;
    bool* display_branches;
    bool* display_selectable;
    DvzGuiTreeAnnotation* annotations;
    uint32_t annotation_count;
    uint64_t focus;
    bool focus_pending;
    uint64_t hovered;
    bool has_hovered;
    uint64_t focused;
    bool has_focused;
};

typedef struct _Column
{
    DvzGuiTableColumnDesc desc;
    char* title;
    char* format;
    void* values;
} _Column;

struct DvzGuiTable
{
    _Core core;
    uint32_t column_count;
    _Column* columns;
    uint32_t sort_column;
    int32_t sort_direction;
};

static int _tree_projected(const DvzGuiTree* tree, uint64_t key);


/*************************************************************************************************/
/*  Common model helpers                                                                         */
/*************************************************************************************************/

static bool _utf8(const char* s)
{
    if (s == NULL)
        return false;
    const unsigned char* p = (const unsigned char*)s;
    while (*p)
    {
        uint32_t cp = 0, n = 0;
        if (*p <= 0x7f)
        {
            p++;
            continue;
        }
        if ((*p & 0xe0) == 0xc0)
        {
            cp = *p & 0x1f;
            n = 2;
            if (cp < 2)
                return false;
        }
        else if ((*p & 0xf0) == 0xe0)
        {
            cp = *p & 0x0f;
            n = 3;
        }
        else if ((*p & 0xf8) == 0xf0)
        {
            cp = *p & 0x07;
            n = 4;
        }
        else
            return false;
        for (uint32_t i = 1; i < n; i++)
        {
            if (p[i] == '\0' || (p[i] & 0xc0) != 0x80)
                return false;
            cp = (cp << 6) | (p[i] & 0x3f);
        }
        if ((n == 3 && cp < 0x800) || (n == 4 && cp < 0x10000) || cp > 0x10ffff ||
            (cp >= 0xd800 && cp <= 0xdfff))
            return false;
        p += n;
    }
    return true;
}

static int _key_cmp(const void* pa, const void* pb)
{
    const _KeyIndex* a = (const _KeyIndex*)pa;
    const _KeyIndex* b = (const _KeyIndex*)pb;
    return (a->key > b->key) - (a->key < b->key);
}

static bool _find_index(const _KeyIndex* index, uint32_t count, uint64_t key, uint32_t* out)
{
    uint32_t lo = 0, hi = count;
    while (lo < hi)
    {
        uint32_t mid = lo + (hi - lo) / 2;
        if (index[mid].key < key)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo >= count || index[lo].key != key)
        return false;
    if (out != NULL)
        *out = index[lo].index;
    return true;
}

static bool _find(const _Core* core, uint64_t key, uint32_t* out)
{
    return core != NULL && _find_index(core->index, core->count, key, out);
}

static void _rows_free(_Rows* rows)
{
    dvz_free(rows->keys);
    dvz_free(rows->index);
    dvz_free(rows->selected);
    dvz_free(rows->visible);
    dvz_free(rows->matches);
    dvz_free(rows->order);
    dvz_free(rows->styles);
    *rows = {};
}

static bool
_rows(const _Core* old, uint32_t count, const uint64_t* keys, uint32_t flags, _Rows* out)
{
    if (out == NULL || count > INT_MAX || (count > 0 && keys == NULL) ||
        (flags & ~(uint32_t)DVZ_GUI_DATA_SET_FLAGS_RESET_STATE) != 0)
        return false;
    *out = {};
    out->count = count;
    if (count == 0)
        return true;
    out->keys = (uint64_t*)dvz_calloc(count, sizeof(uint64_t));
    out->index = (_KeyIndex*)dvz_calloc(count, sizeof(_KeyIndex));
    out->selected = (bool*)dvz_calloc(count, sizeof(bool));
    out->visible = (bool*)dvz_calloc(count, sizeof(bool));
    out->matches = (bool*)dvz_calloc(count, sizeof(bool));
    out->order = (uint32_t*)dvz_calloc(count, sizeof(uint32_t));
    if (out->keys == NULL || out->index == NULL || out->selected == NULL || out->visible == NULL ||
        out->matches == NULL || out->order == NULL)
    {
        _rows_free(out);
        return false;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        out->keys[i] = keys[i];
        out->index[i] = {keys[i], i};
        out->visible[i] = out->matches[i] = true;
    }
    std::qsort(out->index, count, sizeof(_KeyIndex), _key_cmp);
    for (uint32_t i = 1; i < count; i++)
        if (out->index[i - 1].key == out->index[i].key)
        {
            _rows_free(out);
            return false;
        }
    if (old == NULL || (flags & DVZ_GUI_DATA_SET_FLAGS_RESET_STATE) != 0)
        return true;
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t previous = 0;
        if (_find(old, keys[i], &previous))
            out->selected[i] = old->selected[previous];
    }
    for (uint32_t i = 0; i < old->style_count; i++)
        if (_find_index(out->index, count, old->styles[i].row_key, NULL))
            out->style_count++;
    if (out->style_count > 0)
    {
        out->styles = (DvzGuiDataStyle*)dvz_calloc(out->style_count, sizeof(DvzGuiDataStyle));
        if (out->styles == NULL)
        {
            _rows_free(out);
            return false;
        }
        uint32_t j = 0;
        for (uint32_t i = 0; i < old->style_count; i++)
            if (_find_index(out->index, count, old->styles[i].row_key, NULL))
                out->styles[j++] = old->styles[i];
    }
    return true;
}

static void _commit(_Core* core, _Rows* rows, uint32_t flags)
{
    dvz_free(core->keys);
    dvz_free(core->index);
    dvz_free(core->selected);
    dvz_free(core->visible);
    dvz_free(core->matches);
    dvz_free(core->order);
    dvz_free(core->styles);
    core->count = rows->count;
    core->keys = rows->keys;
    core->index = rows->index;
    core->selected = rows->selected;
    core->visible = rows->visible;
    core->matches = rows->matches;
    core->order = rows->order;
    core->styles = rows->styles;
    core->style_count = rows->style_count;
    core->matches_active = false;
    core->order_count = 0;
    core->dirty = true;
    if ((flags & DVZ_GUI_DATA_SET_FLAGS_RESET_STATE) != 0)
        core->has_anchor = false;
    *rows = {};
}

static bool _core_init(_Core* core, const char* id, uint32_t flags)
{
    if (core == NULL || id == NULL || id[0] == '\0' || !_utf8(id) ||
        (flags & ~(uint32_t)WIDGET_FLAGS) != 0)
        return false;
    *core = {};
    core->id = dvz_strdup(id);
    core->filter_capacity = 64;
    core->filter = (char*)dvz_calloc(core->filter_capacity, sizeof(char));
    if (core->id == NULL || core->filter == NULL)
    {
        dvz_free(core->id);
        dvz_free(core->filter);
        *core = {};
        return false;
    }
    core->flags = flags;
    core->dirty = true;
    core->frame = UINT64_MAX;
    return true;
}

static void _core_free(_Core* core)
{
    dvz_free(core->id);
    dvz_free(core->keys);
    dvz_free(core->index);
    dvz_free(core->selected);
    dvz_free(core->visible);
    dvz_free(core->matches);
    dvz_free(core->order);
    dvz_free(core->styles);
    dvz_free(core->filter);
    *core = {};
}

static bool _contains(const char* text, const char* query)
{
    if (query[0] == '\0')
        return true;
    if (text == NULL)
        return false;
    for (; *text; text++)
    {
        const unsigned char* a = (const unsigned char*)text;
        const unsigned char* b = (const unsigned char*)query;
        while (*a && *b)
        {
            unsigned char ca = *a, cb = *b;
            if (ca >= 'A' && ca <= 'Z')
                ca += 'a' - 'A';
            if (cb >= 'A' && cb <= 'Z')
                cb += 'a' - 'A';
            if (ca != cb)
                break;
            a++;
            b++;
        }
        if (*b == '\0')
            return true;
    }
    return false;
}

static int _text_cmp(const char* lhs, const char* rhs)
{
    const unsigned char* a = (const unsigned char*)lhs;
    const unsigned char* b = (const unsigned char*)rhs;
    while (*a && *b)
    {
        unsigned char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z')
            ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z')
            cb += 'a' - 'A';
        if (ca != cb)
            return (ca > cb) - (ca < cb);
        a++;
        b++;
    }
    return (*a > *b) - (*a < *b);
}

static DvzResult _mask(_Core* core, uint32_t count, const bool* values, bool strict)
{
    if (core == NULL)
        return DVZ_ERROR;
    if (count == 0 && values == NULL)
    {
        for (uint32_t i = 0; i < core->count; i++)
            (strict ? core->visible : core->matches)[i] = true;
        if (!strict)
            core->matches_active = false;
        core->dirty = true;
        return DVZ_OK;
    }
    if (count != core->count || (count > 0 && values == NULL))
        return DVZ_ERROR;
    bool* copy = count ? (bool*)dvz_calloc(count, sizeof(bool)) : NULL;
    if (count && copy == NULL)
        return DVZ_ERROR;
    if (count)
        dvz_memcpy(copy, count * sizeof(bool), values, count * sizeof(bool));
    if (count)
        dvz_memcpy(
            strict ? core->visible : core->matches, count * sizeof(bool), copy,
            count * sizeof(bool));
    dvz_free(copy);
    if (!strict)
        core->matches_active = true;
    core->dirty = true;
    return DVZ_OK;
}

static DvzResult _filter(_Core* core, const char* value)
{
    if (core == NULL || !_utf8(value))
        return DVZ_ERROR;
    size_t n = strlen(value) + 1, capacity = 64;
    while (capacity < n)
    {
        if (capacity > SIZE_MAX / 2)
            return DVZ_ERROR;
        capacity *= 2;
    }
    char* copy = (char*)dvz_calloc(capacity, sizeof(char));
    if (copy == NULL)
        return DVZ_ERROR;
    dvz_memcpy(copy, capacity, value, n);
    dvz_free(core->filter);
    core->filter = copy;
    core->filter_capacity = capacity;
    core->dirty = true;
    return DVZ_OK;
}

static uint32_t _get_filter(const _Core* core, uint32_t capacity, char* out)
{
    if (core == NULL || (capacity > 0 && out == NULL))
        return 0;
    size_t n = strlen(core->filter) + 1;
    if (n > UINT32_MAX)
        return 0;
    if (capacity)
    {
        size_t copy = std::min(n - 1, (size_t)capacity - 1);
        if (copy)
            dvz_memcpy(out, capacity, core->filter, copy);
        out[copy] = '\0';
    }
    return (uint32_t)n;
}

static DvzResult _selection(_Core* core, uint32_t count, const uint64_t* keys)
{
    if (core == NULL || (count && keys == NULL))
        return DVZ_ERROR;
    for (uint32_t i = 0; i < count; i++)
    {
        if (!_find(core, keys[i], NULL))
            return DVZ_ERROR;
        for (uint32_t j = 0; j < i; j++)
            if (keys[i] == keys[j])
                return DVZ_ERROR;
    }
    bool* selected = core->count ? (bool*)dvz_calloc(core->count, sizeof(bool)) : NULL;
    if (core->count && selected == NULL)
        return DVZ_ERROR;
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t row = 0;
        _find(core, keys[i], &row);
        selected[row] = true;
    }
    dvz_free(core->selected);
    core->selected = selected;
    core->has_anchor = count > 0;
    if (count)
        core->anchor = keys[count - 1];
    return DVZ_OK;
}

static uint32_t _get_selection(const _Core* core, uint32_t capacity, uint64_t* keys)
{
    if (core == NULL || (capacity && keys == NULL))
        return 0;
    uint32_t n = 0;
    for (uint32_t i = 0; i < core->count; i++)
        if (core->selected[i])
        {
            if (n < capacity)
                keys[n] = core->keys[i];
            n++;
        }
    return n;
}

static const DvzGuiDataStyle* _style(const _Core* core, uint64_t key)
{
    for (uint32_t i = 0; i < core->style_count; i++)
        if (core->styles[i].row_key == key)
            return &core->styles[i];
    return NULL;
}

static DvzResult _styles(_Core* core, uint32_t count, const DvzGuiDataStyle* styles)
{
    if (core == NULL || (count && styles == NULL))
        return DVZ_ERROR;
    for (uint32_t i = 0; i < count; i++)
    {
        if (styles[i].struct_size != sizeof(DvzGuiDataStyle) ||
            (styles[i].flags & ~(uint32_t)STYLE_FLAGS) != 0 ||
            !_find(core, styles[i].row_key, NULL))
            return DVZ_ERROR;
        for (uint32_t k = 0; k < 4; k++)
            if (styles[i].reserved[k] != 0)
                return DVZ_ERROR;
        for (uint32_t j = 0; j < i; j++)
            if (styles[i].row_key == styles[j].row_key)
                return DVZ_ERROR;
    }
    DvzGuiDataStyle* copy = count ? (DvzGuiDataStyle*)dvz_calloc(count, sizeof(*copy)) : NULL;
    if (count && copy == NULL)
        return DVZ_ERROR;
    if (count)
        dvz_memcpy(copy, count * sizeof(*copy), styles, count * sizeof(*copy));
    dvz_free(core->styles);
    core->styles = copy;
    core->style_count = count;
    return DVZ_OK;
}


/*************************************************************************************************/
/*  Interaction helpers                                                                          */
/*************************************************************************************************/

static uint32_t _mods(void)
{
    const ImGuiIO& io = ImGui::GetIO();
    uint32_t mods = 0;
    if (io.KeyShift)
        mods |= DVZ_KEY_MODIFIER_SHIFT;
    if (io.KeyCtrl)
        mods |= DVZ_KEY_MODIFIER_CONTROL;
    if (io.KeyAlt)
        mods |= DVZ_KEY_MODIFIER_ALT;
    if (io.KeySuper)
        mods |= DVZ_KEY_MODIFIER_SUPER;
    return mods;
}

static void _event(
    _Core* core, DvzGuiDataEvent* events, uint32_t capacity, uint32_t* written, uint32_t* dropped,
    uint32_t type, uint64_t key, uint32_t column, int32_t detail, uint32_t flags)
{
    core->revision++;
    if (*written < capacity)
        events[(*written)++] = {type, flags, key, column, detail, core->revision};
    else
        (*dropped)++;
}

static void _select(_Core* core, uint32_t display, uint32_t mods)
{
    const uint32_t row = core->order[display];
    const bool multi = (core->flags & DVZ_GUI_DATA_WIDGET_FLAGS_MULTI_SELECT) != 0;
    const bool toggle = multi && (mods & (DVZ_KEY_MODIFIER_CONTROL | DVZ_KEY_MODIFIER_SUPER));
    if (multi && (mods & DVZ_KEY_MODIFIER_SHIFT) && core->has_anchor)
    {
        uint32_t anchor_row = 0, anchor_display = UINT32_MAX;
        if (_find(core, core->anchor, &anchor_row))
            for (uint32_t i = 0; i < core->order_count; i++)
                if (core->order[i] == anchor_row)
                    anchor_display = i;
        if (anchor_display != UINT32_MAX)
        {
            memset(core->selected, 0, core->count * sizeof(bool));
            uint32_t lo = std::min(display, anchor_display),
                     hi = std::max(display, anchor_display);
            for (uint32_t i = lo; i <= hi; i++)
                core->selected[core->order[i]] = true;
            return;
        }
    }
    if (!toggle)
        memset(core->selected, 0, core->count * sizeof(bool));
    core->selected[row] = toggle ? !core->selected[row] : true;
    core->anchor = core->keys[row];
    core->has_anchor = true;
}

static int _resize_filter(ImGuiInputTextCallbackData* data)
{
    _Core* core = (_Core*)data->UserData;
    if (data->EventFlag != ImGuiInputTextFlags_CallbackResize || data->BufSize <= 0)
        return 0;
    char* resized = (char*)dvz_realloc(core->filter, (size_t)data->BufSize);
    if (resized == NULL)
        return 1;
    core->filter = resized;
    core->filter_capacity = (size_t)data->BufSize;
    data->Buf = resized;
    return 0;
}

static DvzResult _draw_start(
    DvzGui* gui, _Core* core, DvzGuiDataEvent* events, uint32_t capacity, uint32_t* written,
    uint32_t* dropped)
{
    if (gui == NULL || core == NULL || written == NULL || dropped == NULL ||
        (capacity && events == NULL))
        return DVZ_ERROR;
    *written = *dropped = 0;
    uint64_t frame = 0;
    if (!_dvz_gui_data_draw_context(gui, &frame))
        return DVZ_ERROR;
    if (core->gui == NULL)
        core->gui = gui;
    if (core->gui != gui || core->frame == frame)
        return DVZ_ERROR;
    core->frame = frame;
    return DVZ_OK;
}

static bool _draw_filter(_Core* core)
{
    return ImGui::InputText(
        "Filter", core->filter, core->filter_capacity, ImGuiInputTextFlags_CallbackResize,
        _resize_filter, core);
}

static ImVec4 _color(DvzColor c)
{
    return ImVec4(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f);
}

static void _push_id(const _Core* core, uint32_t row)
{
    ImGui::PushID((int)(core->keys[row] >> 32));
    ImGui::PushID((int)core->keys[row]);
}

static void _pop_id(void)
{
    ImGui::PopID();
    ImGui::PopID();
}


/*************************************************************************************************/
/*  Tree                                                                                         */
/*************************************************************************************************/

static void _tree_arrays_free(DvzGuiTree* tree)
{
    dvz_free(tree->parents);
    dvz_free(tree->depths);
    dvz_free(tree->branches);
    dvz_free_strings(tree->core.count, tree->labels);
    dvz_free(tree->labels);
    dvz_free_strings(tree->core.count, tree->secondary);
    dvz_free(tree->secondary);
    dvz_free(tree->swatches);
    dvz_free(tree->expanded);
    dvz_free(tree->display_depths);
    dvz_free(tree->display_branches);
    dvz_free(tree->display_selectable);
    tree->display_depths = NULL;
    tree->display_branches = tree->display_selectable = NULL;
    for (uint32_t i = 0; i < tree->annotation_count; i++)
    {
        dvz_free(const_cast<char*>(tree->annotations[i].value_text));
        dvz_free(const_cast<char*>(tree->annotations[i].tooltip));
        dvz_free(const_cast<DvzGuiTreeTextSpan*>(tree->annotations[i].label_spans));
        dvz_free(const_cast<DvzGuiTreeTextSpan*>(tree->annotations[i].secondary_spans));
    }
    dvz_free(tree->annotations);
    tree->annotations = NULL;
    tree->annotation_count = 0;
}

static bool _tree_layout(uint32_t n, const uint32_t* parents, uint32_t* depths, bool* branches)
{
    for (uint32_t i = 0; i < n; i++)
    {
        if (parents[i] == UINT32_MAX)
            depths[i] = 0;
        else
        {
            if (parents[i] >= i)
                return false;
            depths[i] = depths[parents[i]] + 1;
            branches[parents[i]] = true;
        }
        if (i && depths[i] > depths[i - 1] + 1)
            return false;
        if (depths[i])
        {
            uint32_t candidate = i - 1;
            while (depths[candidate] >= depths[i])
            {
                if (candidate == 0)
                    return false;
                candidate--;
            }
            if (parents[i] != candidate)
                return false;
        }
    }
    return true;
}

static void _tree_rebuild(DvzGuiTree* tree)
{
    _Core* c = &tree->core;
    if (c->flags & DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED)
    {
        c->dirty = false;
        return;
    }
    if (!c->dirty)
        return;
    c->order_count = 0;
    bool* path = c->count ? (bool*)dvz_calloc(c->count, sizeof(bool)) : NULL;
    bool* match = c->count ? (bool*)dvz_calloc(c->count, sizeof(bool)) : NULL;
    if (c->count && (path == NULL || match == NULL))
    {
        dvz_free(path);
        dvz_free(match);
        return;
    }
    bool filtering = c->matches_active || c->filter[0] != '\0';
    for (uint32_t i = 0; i < c->count; i++)
    {
        uint32_t p = tree->parents[i];
        path[i] = c->visible[i] && (p == UINT32_MAX || path[p]);
        match[i] =
            path[i] && c->matches[i] &&
            (_contains(tree->labels[i], c->filter) || _contains(tree->secondary[i], c->filter));
    }
    for (uint32_t i = c->count; i > 0; i--)
    {
        uint32_t row = i - 1, p = tree->parents[row];
        if (match[row] && p != UINT32_MAX && path[p])
            match[p] = true;
    }
    for (uint32_t i = 0; i < c->count; i++)
    {
        bool shown = path[i] && (!filtering || match[i]);
        if (shown && !filtering)
            for (uint32_t p = tree->parents[i]; p != UINT32_MAX; p = tree->parents[p])
                shown &= tree->expanded[p];
        if (shown)
            c->order[c->order_count++] = i;
    }
    dvz_free(path);
    dvz_free(match);
    c->dirty = false;
    c->rebuild_count++;
}

DvzGuiTableColumnDesc dvz_gui_table_column_desc(void)
{
    DvzGuiTableColumnDesc desc = {};
    desc.struct_size = sizeof(desc);
    return desc;
}

DvzGuiDataStyle dvz_gui_data_style(void)
{
    DvzGuiDataStyle style = {};
    style.struct_size = sizeof(style);
    return style;
}

DvzGuiTree* dvz_gui_tree(const char* id, uint32_t flags)
{
    DvzGuiTree* tree = (DvzGuiTree*)dvz_calloc(1, sizeof(*tree));
    if (tree == NULL ||
        !_core_init(&tree->core, id, flags & ~(uint32_t)DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED))
    {
        dvz_free(tree);
        return NULL;
    }
    tree->layout = dvz_gui_tree_layout();
    tree->core.flags = flags;
    return tree;
}

DvzGuiTreeLayout dvz_gui_tree_layout(void)
{
    DvzGuiTreeLayout layout = {};
    layout.struct_size = DVZ_STRUCT_SIZE(DvzGuiTreeLayout);
    layout.indent_em = 0.70f;
    layout.row_padding_em = 0.0625f;
    layout.item_spacing_em = 0.0625f;
    layout.disclosure_gap_em = 0.20f;
    layout.swatch_gap_em = 0.40f;
    layout.secondary_gap_em = 0.625f;
    return layout;
}

DvzResult dvz_gui_tree_set_layout(DvzGuiTree* tree, const DvzGuiTreeLayout* layout)
{
    if (tree == NULL || layout == NULL ||
        !DVZ_STRUCT_VALID(layout, DvzGuiTreeLayout, TREE_LAYOUT_KNOWN_FLAGS) ||
        !isfinite(layout->indent_em) ||
        !isfinite(layout->row_padding_em) || !isfinite(layout->item_spacing_em) ||
        !isfinite(layout->disclosure_gap_em) || !isfinite(layout->swatch_gap_em) ||
        !isfinite(layout->secondary_gap_em) || layout->indent_em < 0.0f ||
        layout->row_padding_em < 0.0f || layout->item_spacing_em < 0.0f ||
        layout->disclosure_gap_em < 0.0f || layout->swatch_gap_em < 0.0f ||
        layout->secondary_gap_em < 0.0f || layout->reserved[0] != 0 || layout->reserved[1] != 0)
        return DVZ_ERROR;
    tree->layout = *layout;
    return DVZ_OK;
}

DvzResult dvz_gui_tree_set_rows(
    DvzGuiTree* tree, uint32_t n, const uint64_t* keys, const uint32_t* parents,
    const char* const* labels, const char* const* secondary, uint32_t flags)
{
    if (tree == NULL || (n && (parents == NULL || labels == NULL)))
        return DVZ_ERROR;
    _Rows rows = {};
    if (!_rows(&tree->core, n, keys, flags, &rows))
        return DVZ_ERROR;
    uint32_t* ps = n ? (uint32_t*)dvz_calloc(n, sizeof(uint32_t)) : NULL;
    uint32_t* ds = n ? (uint32_t*)dvz_calloc(n, sizeof(uint32_t)) : NULL;
    bool* bs = n ? (bool*)dvz_calloc(n, sizeof(bool)) : NULL;
    char** ls = n ? (char**)dvz_calloc(n, sizeof(char*)) : NULL;
    char** ss = n ? (char**)dvz_calloc(n, sizeof(char*)) : NULL;
    bool* es = n ? (bool*)dvz_calloc(n, sizeof(bool)) : NULL;
    bool ok = !n || (ps && ds && bs && ls && ss && es);
    for (uint32_t i = 0; ok && i < n; i++)
    {
        ok =
            _utf8(labels[i]) && (secondary == NULL || secondary[i] == NULL || _utf8(secondary[i]));
        if (!ok)
            break;
        ps[i] = parents[i];
        ls[i] = dvz_strdup(labels[i]);
        ss[i] = dvz_strdup(secondary && secondary[i] ? secondary[i] : "");
        ok = ls[i] && ss[i];
        uint32_t old = 0;
        if (!(flags & DVZ_GUI_DATA_SET_FLAGS_RESET_STATE) && _find(&tree->core, keys[i], &old))
            es[i] = tree->expanded[old];
    }
    ok = ok && _tree_layout(n, ps, ds, bs);
    if (!ok)
    {
        _rows_free(&rows);
        dvz_free(ps);
        dvz_free(ds);
        dvz_free(bs);
        dvz_free_strings(n, ls);
        dvz_free(ls);
        dvz_free_strings(n, ss);
        dvz_free(ss);
        dvz_free(es);
        return DVZ_ERROR;
    }
    _tree_arrays_free(tree);
    _commit(&tree->core, &rows, flags);
    tree->parents = ps;
    tree->depths = ds;
    tree->branches = bs;
    tree->labels = ls;
    tree->secondary = ss;
    tree->expanded = es;
    tree->reveal_pending = false;
    tree->focus_pending = false;
    return DVZ_OK;
}

/**
 * Copy a host-owned display projection without deriving semantic ancestry.
 *
 * @param tree controlled tree
 * @param n projected row count
 * @param keys ordered existing row keys
 * @param depths display depths
 * @param branches disclosure capability
 * @param selectable selection eligibility
 * @return result; failure leaves the previous projection intact
 */
DvzResult dvz_gui_tree_set_projection(
    DvzGuiTree* tree, uint32_t n, const uint64_t* keys, const uint32_t* depths,
    const bool* branches, const bool* selectable)
{
    if (tree == NULL || !(tree->core.flags & DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED) ||
        n > tree->core.count || n > SIZE_MAX / sizeof(uint32_t) ||
        (n && (!keys || !depths || !branches || !selectable)))
        return DVZ_ERROR;
    uint32_t* order = n ? (uint32_t*)dvz_calloc(n, sizeof(uint32_t)) : NULL;
    uint32_t* ds = n ? (uint32_t*)dvz_calloc(n, sizeof(uint32_t)) : NULL;
    bool* bs = n ? (bool*)dvz_calloc(n, sizeof(bool)) : NULL;
    bool* ss = n ? (bool*)dvz_calloc(n, sizeof(bool)) : NULL;
    bool* seen = tree->core.count ? (bool*)dvz_calloc(tree->core.count, sizeof(bool)) : NULL;
    bool ok = (!n || (order && ds && bs && ss)) && (!tree->core.count || seen);
    for (uint32_t i = 0; ok && i < n; i++)
    {
        ok = _find(&tree->core, keys[i], &order[i]) && !seen[order[i]] && depths[i] <= INT_MAX;
        if (!ok)
            break;
        seen[order[i]] = true;
        ds[i] = depths[i];
        bs[i] = branches[i];
        ss[i] = selectable[i];
    }
    dvz_free(seen);
    if (!ok)
    {
        dvz_free(order);
        dvz_free(ds);
        dvz_free(bs);
        dvz_free(ss);
        return DVZ_ERROR;
    }
    dvz_free(tree->core.order);
    dvz_free(tree->display_depths);
    dvz_free(tree->display_branches);
    dvz_free(tree->display_selectable);
    tree->core.order = order;
    tree->core.order_count = n;
    tree->display_depths = ds;
    tree->display_branches = bs;
    tree->display_selectable = ss;
    tree->core.dirty = false;
    tree->core.rebuild_count++;
    if (tree->reveal_pending && _tree_projected(tree, tree->reveal) < 0)
        tree->reveal_pending = false;
    if (tree->focus_pending && _tree_projected(tree, tree->focus) < 0)
        tree->focus_pending = false;
    return DVZ_OK;
}

/**
 * Return a default copied row annotation descriptor.
 *
 * @return size-versioned annotation
 */
DvzGuiTreeAnnotation dvz_gui_tree_annotation(void)
{
    DvzGuiTreeAnnotation annotation = {};
    annotation.struct_size = sizeof(annotation);
    return annotation;
}

/**
 * Validate ordered UTF-8 highlight spans without interpreting their text.
 *
 * @param text UTF-8 text
 * @param n span count
 * @param spans byte ranges
 * @return whether ranges are sorted, nonoverlapping codepoint ranges
 */
static bool _tree_spans(const char* text, uint32_t n, const DvzGuiTreeTextSpan* spans)
{
    if (n > SIZE_MAX / sizeof(DvzGuiTreeTextSpan) || (n && spans == NULL))
        return false;
    const size_t size = strlen(text);
    size_t previous = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        size_t start = spans[i].start, length = spans[i].length;
        if (length == 0 || start < previous || start > size || length > size - start)
            return false;
        size_t end = start + length;
        if (((unsigned char)text[start] & 0xc0) == 0x80 ||
            ((unsigned char)text[end] & 0xc0) == 0x80)
            return false;
        previous = end;
    }
    return true;
}

/**
 * Replace host-provided row text and highlight decorations transactionally.
 *
 * @param tree tree
 * @param n descriptor count
 * @param annotations descriptors, whose strings and spans are copied
 * @return result; failure preserves the previous annotations
 */
DvzResult
dvz_gui_tree_set_annotations(DvzGuiTree* tree, uint32_t n, const DvzGuiTreeAnnotation* annotations)
{
    if (tree == NULL || n > tree->core.count || n > SIZE_MAX / sizeof(DvzGuiTreeAnnotation) ||
        (n && annotations == NULL))
        return DVZ_ERROR;
    DvzGuiTreeAnnotation* copy = n ? (DvzGuiTreeAnnotation*)dvz_calloc(n, sizeof(*copy)) : NULL;
    bool ok = !n || copy;
    for (uint32_t i = 0; ok && i < n; i++)
    {
        const DvzGuiTreeAnnotation* a = &annotations[i];
        uint32_t row = 0;
        ok = a->struct_size == sizeof(*a) &&
             !(a->flags & ~(uint32_t)DVZ_GUI_TREE_ANNOTATION_FLAGS_MISSING) &&
             a->reserved[0] == 0 && a->reserved[1] == 0 &&
             (!a->value_text || _utf8(a->value_text)) && (!a->tooltip || _utf8(a->tooltip)) &&
             _find(&tree->core, a->row_key, &row);
        for (uint32_t j = 0; ok && j < i; j++)
            ok = annotations[j].row_key != a->row_key;
        ok = ok && _tree_spans(tree->labels[row], a->label_span_count, a->label_spans) &&
             _tree_spans(tree->secondary[row], a->secondary_span_count, a->secondary_spans);
        if (!ok)
            break;
        copy[i] = *a;
        copy[i].value_text = a->value_text ? dvz_strdup(a->value_text) : NULL;
        copy[i].tooltip = a->tooltip ? dvz_strdup(a->tooltip) : NULL;
        copy[i].label_spans =
            a->label_span_count
                ? (DvzGuiTreeTextSpan*)dvz_calloc(a->label_span_count, sizeof(DvzGuiTreeTextSpan))
                : NULL;
        copy[i].secondary_spans = a->secondary_span_count
                                      ? (DvzGuiTreeTextSpan*)dvz_calloc(
                                            a->secondary_span_count, sizeof(DvzGuiTreeTextSpan))
                                      : NULL;
        ok = (!a->value_text || copy[i].value_text) && (!a->tooltip || copy[i].tooltip) &&
             (!a->label_span_count || copy[i].label_spans) &&
             (!a->secondary_span_count || copy[i].secondary_spans);
        if (a->label_span_count && copy[i].label_spans)
            dvz_memcpy(
                const_cast<DvzGuiTreeTextSpan*>(copy[i].label_spans),
                a->label_span_count * sizeof(DvzGuiTreeTextSpan), a->label_spans,
                a->label_span_count * sizeof(DvzGuiTreeTextSpan));
        if (a->secondary_span_count && copy[i].secondary_spans)
            dvz_memcpy(
                const_cast<DvzGuiTreeTextSpan*>(copy[i].secondary_spans),
                a->secondary_span_count * sizeof(DvzGuiTreeTextSpan), a->secondary_spans,
                a->secondary_span_count * sizeof(DvzGuiTreeTextSpan));
    }
    if (ok)
    {
        std::swap(copy, tree->annotations);
        std::swap(n, tree->annotation_count);
    }
    for (uint32_t i = 0; i < n && copy; i++)
    {
        dvz_free(const_cast<char*>(copy[i].value_text));
        dvz_free(const_cast<char*>(copy[i].tooltip));
        dvz_free(const_cast<DvzGuiTreeTextSpan*>(copy[i].label_spans));
        dvz_free(const_cast<DvzGuiTreeTextSpan*>(copy[i].secondary_spans));
    }
    dvz_free(copy);
    return ok ? DVZ_OK : DVZ_ERROR;
}

/**
 * Find a key in the host-controlled displayed order.
 *
 * @param tree tree
 * @param key row key
 * @return display index, or -1 when absent
 */
static int _tree_projected(const DvzGuiTree* tree, uint64_t key)
{
    for (uint32_t i = 0; i < tree->core.order_count; i++)
        if (tree->core.keys[tree->core.order[i]] == key)
            return (int)i;
    return -1;
}

/**
 * Request focus and scrolling without changing the host's expansion state.
 *
 * @param tree controlled tree
 * @param key projected row key
 * @return result
 */
DvzResult dvz_gui_tree_set_focus(DvzGuiTree* tree, uint64_t key)
{
    if (tree == NULL || !(tree->core.flags & DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED) ||
        _tree_projected(tree, key) < 0)
        return DVZ_ERROR;
    tree->focus = key;
    tree->focus_pending = true;
    return dvz_gui_tree_reveal(tree, key);
}

DvzResult dvz_gui_tree_set_swatches(DvzGuiTree* tree, uint32_t n, const DvzColor* colors)
{
    if (tree == NULL)
        return DVZ_ERROR;
    if (n == 0 && colors == NULL)
    {
        dvz_free(tree->swatches);
        tree->swatches = NULL;
        return DVZ_OK;
    }
    if (n != tree->core.count || (n && colors == NULL))
        return DVZ_ERROR;
    DvzColor* copy = n ? (DvzColor*)dvz_calloc(n, sizeof(*copy)) : NULL;
    if (n && copy == NULL)
        return DVZ_ERROR;
    if (n)
        dvz_memcpy(copy, n * sizeof(*copy), colors, n * sizeof(*copy));
    dvz_free(tree->swatches);
    tree->swatches = copy;
    return DVZ_OK;
}

DvzResult dvz_gui_tree_set_styles(DvzGuiTree* t, uint32_t n, const DvzGuiDataStyle* s)
{
    return t ? _styles(&t->core, n, s) : DVZ_ERROR;
}
DvzResult dvz_gui_tree_set_selection(DvzGuiTree* t, uint32_t n, const uint64_t* k)
{
    return t ? _selection(&t->core, n, k) : DVZ_ERROR;
}
uint32_t dvz_gui_tree_get_selection(const DvzGuiTree* t, uint32_t n, uint64_t* k)
{
    return t ? _get_selection(&t->core, n, k) : 0;
}
DvzResult dvz_gui_tree_set_visible(DvzGuiTree* t, uint32_t n, const bool* v)
{
    return t ? _mask(&t->core, n, v, true) : DVZ_ERROR;
}
DvzResult dvz_gui_tree_set_matches(DvzGuiTree* t, uint32_t n, const bool* v)
{
    return t ? _mask(&t->core, n, v, false) : DVZ_ERROR;
}
DvzResult dvz_gui_tree_set_filter(DvzGuiTree* t, const char* f)
{
    return t ? _filter(&t->core, f) : DVZ_ERROR;
}
uint32_t dvz_gui_tree_get_filter(const DvzGuiTree* t, uint32_t n, char* f)
{
    return t ? _get_filter(&t->core, n, f) : 0;
}

DvzResult dvz_gui_tree_set_expanded(DvzGuiTree* tree, uint32_t n, const uint64_t* keys)
{
    if (tree == NULL || (n && keys == NULL))
        return DVZ_ERROR;
    for (uint32_t i = 0; i < n; i++)
        if (!_find(&tree->core, keys[i], NULL))
            return DVZ_ERROR;
    bool* copy = tree->core.count ? (bool*)dvz_calloc(tree->core.count, sizeof(bool)) : NULL;
    if (tree->core.count && copy == NULL)
        return DVZ_ERROR;
    for (uint32_t i = 0; i < n; i++)
    {
        uint32_t row = 0;
        _find(&tree->core, keys[i], &row);
        copy[row] = true;
    }
    dvz_free(tree->expanded);
    tree->expanded = copy;
    tree->core.dirty = true;
    return DVZ_OK;
}

uint32_t dvz_gui_tree_get_expanded(const DvzGuiTree* tree, uint32_t cap, uint64_t* keys)
{
    if (tree == NULL || (cap && keys == NULL))
        return 0;
    uint32_t n = 0;
    for (uint32_t i = 0; i < tree->core.count; i++)
        if (tree->expanded[i])
        {
            if (n < cap)
                keys[n] = tree->core.keys[i];
            n++;
        }
    return n;
}

DvzResult dvz_gui_tree_expand_to_depth(DvzGuiTree* tree, uint32_t depth)
{
    if (tree == NULL)
        return DVZ_ERROR;
    for (uint32_t i = 0; i < tree->core.count; i++)
        tree->expanded[i] = tree->depths[i] < depth;
    tree->core.dirty = true;
    return DVZ_OK;
}
DvzResult dvz_gui_tree_expand_all(DvzGuiTree* tree)
{
    return dvz_gui_tree_expand_to_depth(tree, UINT32_MAX);
}
DvzResult dvz_gui_tree_collapse_all(DvzGuiTree* tree)
{
    return dvz_gui_tree_expand_to_depth(tree, 0);
}
DvzResult dvz_gui_tree_reveal(DvzGuiTree* tree, uint64_t key)
{
    uint32_t row = 0;
    if (tree == NULL || !_find(&tree->core, key, &row))
        return DVZ_ERROR;
    if (tree->core.flags & DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED)
    {
        if (_tree_projected(tree, key) < 0)
            return DVZ_ERROR;
    }
    else
        for (uint32_t p = tree->parents[row]; p != UINT32_MAX; p = tree->parents[p])
            tree->expanded[p] = true;
    tree->reveal = key;
    tree->reveal_pending = tree->core.dirty = true;
    return DVZ_OK;
}

/**
 * Draw host-supplied highlights behind text using its active font metrics.
 *
 * @param text UTF-8 text
 * @param n highlight count
 * @param spans validated byte ranges
 */
static void _tree_text(const char* text, uint32_t n, const DvzGuiTreeTextSpan* spans)
{
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    for (uint32_t i = 0; i < n; i++)
    {
        const char* start = text + spans[i].start;
        const char* end = start + spans[i].length;
        const float x = ImGui::CalcTextSize(text, start).x;
        const float width = ImGui::CalcTextSize(start, end).x;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(origin.x + x, origin.y),
            ImVec2(origin.x + x + width, origin.y + ImGui::GetFontSize()),
            ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
    }
    ImGui::TextUnformatted(text);
}

/**
 * Render the tree rows in the active ImGui window.
 *
 * @param gui GUI, or NULL for CPU-only ImGui regression tests
 * @param tree tree
 * @param events event destination
 * @param cap capacity
 * @param written event count
 * @param dropped dropped event count
 * @return result
 */
static DvzResult _tree_draw(
    DvzGui* gui, DvzGuiTree* tree, DvzGuiDataEvent* events, uint32_t cap, uint32_t* written,
    uint32_t* dropped)
{
    const bool controlled = tree->core.flags & DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED;
    if (controlled && tree->has_focused && ImGui::IsWindowFocused())
    {
        const ImGuiKey imgui_keys[] = {ImGuiKey_UpArrow,    ImGuiKey_DownArrow, ImGuiKey_LeftArrow,
                                       ImGuiKey_RightArrow, ImGuiKey_Home,      ImGuiKey_End,
                                       ImGuiKey_PageUp,     ImGuiKey_PageDown,  ImGuiKey_Enter,
                                       ImGuiKey_Space,      ImGuiKey_Escape};
        const int32_t keys[] = {DVZ_KEY_UP,    DVZ_KEY_DOWN,  DVZ_KEY_LEFT,    DVZ_KEY_RIGHT,
                                DVZ_KEY_HOME,  DVZ_KEY_END,   DVZ_KEY_PAGE_UP, DVZ_KEY_PAGE_DOWN,
                                DVZ_KEY_ENTER, DVZ_KEY_SPACE, DVZ_KEY_ESCAPE};
        // Capture keys for the previously focused row before Escape or activation changes focus.
        // Only directional requests are canceled; Tab still leaves the widget normally.
        for (uint32_t i = 0; i < 11; i++)
            if (ImGui::IsKeyPressed(imgui_keys[i]))
            {
                if (i < 8)
                    ImGui::NavMoveRequestCancel();
                _event(
                    &tree->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_KEY_PRESSED,
                    tree->focused, UINT32_MAX, keys[i], _mods());
            }
    }
    bool has_hovered = false, has_focused = false;
    uint64_t hovered = 0, focused = 0;
    // Bound row input intents independently of the number of clipped rows.
    bool click_emitted = false, disclosure_emitted = false, activation_emitted = false;
    ImGui::PushID(tree->core.id);
    if (!controlled && (tree->core.flags & DVZ_GUI_DATA_WIDGET_FLAGS_FILTER) &&
        _draw_filter(&tree->core))
    {
        tree->core.dirty = true;
        _event(
            &tree->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_FILTER_CHANGED, 0,
            UINT32_MAX, 0, _mods());
    }
    _tree_rebuild(tree);
    int reveal_display = -1;
    if (tree->reveal_pending)
        for (uint32_t i = 0; i < tree->core.order_count; i++)
            if (tree->core.keys[tree->core.order[i]] == tree->reveal)
                reveal_display = (int)i;
    const ImGuiStyle& imgui_style = ImGui::GetStyle();
    const float font_size = ImGui::GetFontSize();
    // Keep deep retained trees compact while deriving distances from the active ImGui font size.
    const float indent_spacing = font_size * tree->layout.indent_em;
    const bool exact_spacing = (tree->layout.flags & DVZ_GUI_TREE_LAYOUT_FLAGS_EXACT_SPACING) != 0;
    const bool readable_secondary =
        (tree->layout.flags & DVZ_GUI_TREE_LAYOUT_FLAGS_READABLE_SECONDARY) != 0;
    const float row_padding =
        exact_spacing
            ? font_size * tree->layout.row_padding_em
            : std::max(imgui_style.FramePadding.y, font_size * tree->layout.row_padding_em);
    const float item_spacing_y =
        exact_spacing
            ? font_size * tree->layout.item_spacing_em
            : std::max(imgui_style.ItemSpacing.y, font_size * tree->layout.item_spacing_em);
    const float disclosure_gap = font_size * tree->layout.disclosure_gap_em;
    const float swatch_gap = font_size * tree->layout.swatch_gap_em;
    const float secondary_gap = font_size * tree->layout.secondary_gap_em;
    ImGui::PushStyleVar(
        ImGuiStyleVar_FramePadding, ImVec2(imgui_style.FramePadding.x, row_padding));
    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing, ImVec2(imgui_style.ItemSpacing.x, item_spacing_y));
    ImGuiListClipper clip;
    clip.Begin((int)tree->core.order_count);
    if (reveal_display >= 0)
        clip.IncludeItemByIndex(reveal_display);
    if (controlled && tree->focus_pending)
    {
        const int focus_display = _tree_projected(tree, tree->focus);
        if (focus_display >= 0)
            clip.IncludeItemByIndex(focus_display);
    }
    if (controlled && tree->has_focused)
    {
        const int focus_display = _tree_projected(tree, tree->focused);
        if (focus_display >= 0)
            clip.IncludeItemByIndex(focus_display);
    }
    while (clip.Step())
        for (int q = clip.DisplayStart; q < clip.DisplayEnd; q++)
        {
            uint32_t row = tree->core.order[q];
            const DvzGuiDataStyle* style = _style(&tree->core, tree->core.keys[row]);
            bool disabled = style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_DISABLED);
            const DvzGuiTreeAnnotation* annotation = NULL;
            for (uint32_t i = 0; i < tree->annotation_count; i++)
                if (tree->annotations[i].row_key == tree->core.keys[row])
                    annotation = &tree->annotations[i];
            _push_id(&tree->core, row);
            const float indent =
                (controlled ? tree->display_depths[q] : tree->depths[row]) * indent_spacing;
            const ImVec2 row_start = ImGui::GetCursorScreenPos();
            const float row_height = font_size + 2.0f * row_padding;
            if (style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_BACKGROUND))
            {
                ImVec2 b(
                    ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x,
                    row_start.y + row_height);
                ImGui::GetWindowDrawList()->AddRectFilled(
                    row_start, b,
                    ImGui::ColorConvertFloat4ToU32(_color(style->background)));
            }
            if (style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_FOREGROUND))
                ImGui::PushStyleColor(ImGuiCol_Text, _color(style->foreground));
            if (disabled)
                ImGui::BeginDisabled();
            if (controlled && tree->focus_pending && tree->focus == tree->core.keys[row])
            {
                ImGui::SetKeyboardFocusHere();
                tree->focus_pending = false;
            }
            ImGui::Selectable(
                "##selection", tree->core.selected[row],
                ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_AllowDoubleClick,
                ImVec2(0, row_height));
            bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
            bool row_hovered = ImGui::IsItemHovered();
            bool activated = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0);
            if (controlled && row_hovered && !disabled)
            {
                has_hovered = true;
                hovered = tree->core.keys[row];
            }
            if (controlled && ImGui::IsItemFocused() && !disabled)
            {
                has_focused = true;
                focused = tree->core.keys[row];
            }
            const ImVec2 next_row = ImGui::GetCursorScreenPos();

            if (style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_ACCENT))
                ImGui::GetWindowDrawList()->AddRectFilled(
                    row_start, ImVec2(row_start.x + 3, row_start.y + row_height),
                    ImGui::ColorConvertFloat4ToU32(_color(style->accent)));

            ImGui::SetCursorScreenPos(row_start);
            if (indent > 0)
                ImGui::Indent(indent);
            bool toggled = false;
            if (controlled ? tree->display_branches[q] : tree->branches[row])
            {
                ImGui::SetNextItemOpen(tree->expanded[row], ImGuiCond_Always);
                if (controlled)
                    ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
                ImGui::TreeNodeEx(
                    "##disclosure",
                    ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow);
                if (controlled)
                    ImGui::PopItemFlag();
                toggled = ImGui::IsItemToggledOpen();
                row_hovered |= ImGui::IsItemHovered();
                if (controlled && row_hovered && !disabled)
                {
                    has_hovered = true;
                    hovered = tree->core.keys[row];
                }
            }
            else
                ImGui::Dummy(ImVec2(ImGui::GetTreeNodeToLabelSpacing(), row_height));
            if (toggled && !disabled)
            {
                if (controlled)
                {
                    if (!disclosure_emitted)
                        _event(
                            &tree->core, events, cap, written, dropped,
                            DVZ_GUI_DATA_EVENT_DISCLOSURE_CLICKED, tree->core.keys[row],
                            UINT32_MAX, !tree->expanded[row], _mods());
                    disclosure_emitted = true;
                }
                else
                {
                    tree->expanded[row] = !tree->expanded[row];
                    tree->core.dirty = true;
                    _event(
                        &tree->core, events, cap, written, dropped,
                        DVZ_GUI_DATA_EVENT_EXPANSION_CHANGED, tree->core.keys[row], UINT32_MAX,
                        tree->expanded[row], _mods());
                }
            }
            if (clicked && !toggled && !disabled &&
                (!controlled || tree->display_selectable[q] || tree->display_branches[q]))
            {
                uint32_t mods = _mods();
                if (controlled)
                {
                    if (!click_emitted)
                        _event(
                            &tree->core, events, cap, written, dropped,
                            DVZ_GUI_DATA_EVENT_ROW_CLICKED, tree->core.keys[row], UINT32_MAX, 0,
                            mods);
                    click_emitted = true;
                }
                else
                {
                    _select(&tree->core, (uint32_t)q, mods);
                    _event(
                        &tree->core, events, cap, written, dropped,
                        DVZ_GUI_DATA_EVENT_SELECTION_CHANGED, tree->core.keys[row], UINT32_MAX,
                        tree->core.selected[row], mods);
                }
            }
            ImGui::SameLine(0, disclosure_gap);
            if (tree->swatches)
            {
                const float swatch_size = ImGui::GetFontSize() * 0.80f;
                ImGui::ColorButton(
                    "##swatch", _color(tree->swatches[row]),
                    ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop |
                        ImGuiColorEditFlags_NoBorder,
                    ImVec2(swatch_size, swatch_size));
                ImGui::SameLine(0, swatch_gap);
            }
            ImFont* bold = gui ? (ImFont*)_dvz_gui_bold_font(gui) : NULL;
            if (bold != NULL)
                ImGui::PushFont(bold);
            _tree_text(
                tree->labels[row], annotation ? annotation->label_span_count : 0,
                annotation ? annotation->label_spans : NULL);
            if (bold != NULL)
                ImGui::PopFont();
            if (tree->secondary[row][0])
            {
                ImGui::SameLine(0, secondary_gap);
                ImGui::PushStyleColor(
                    ImGuiCol_Text,
                    ImGui::GetStyleColorVec4(
                        readable_secondary ? ImGuiCol_Text : ImGuiCol_TextDisabled));
                _tree_text(
                    tree->secondary[row], annotation ? annotation->secondary_span_count : 0,
                    annotation ? annotation->secondary_spans : NULL);
                ImGui::PopStyleColor();
            }
            if (annotation && annotation->value_text)
            {
                ImGui::SameLine(0, secondary_gap);
                if (annotation->flags & DVZ_GUI_TREE_ANNOTATION_FLAGS_MISSING)
                    ImGui::PushStyleColor(
                        ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::TextUnformatted(annotation->value_text);
                if (annotation->flags & DVZ_GUI_TREE_ANNOTATION_FLAGS_MISSING)
                    ImGui::PopStyleColor();
            }
            if (row_hovered)
            {
                ImGui::BeginTooltip();
                if (annotation && annotation->tooltip)
                {
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
                    ImGui::TextUnformatted(annotation->tooltip);
                    ImGui::PopTextWrapPos();
                }
                else
                {
                    ImGui::TextUnformatted(tree->labels[row]);
                    if (tree->secondary[row][0])
                    {
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
                        ImGui::TextUnformatted(tree->secondary[row]);
                        ImGui::PopTextWrapPos();
                    }
                }
                ImGui::EndTooltip();
            }
            if (activated && !disabled &&
                (!controlled || (tree->display_selectable[q] && !activation_emitted)))
            {
                _event(
                    &tree->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_ACTIVATED,
                    tree->core.keys[row], UINT32_MAX, 0, _mods());
                activation_emitted = true;
            }
            if (tree->reveal_pending && tree->reveal == tree->core.keys[row])
            {
                ImGui::SetScrollHereY(.5f);
                tree->reveal_pending = false;
            }
            if (disabled)
                ImGui::EndDisabled();
            if (style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_FOREGROUND))
                ImGui::PopStyleColor();
            if (indent > 0)
                ImGui::Unindent(indent);
            ImGui::SetCursorScreenPos(next_row);
            _pop_id();
        }
    if (controlled)
    {
        if (tree->has_hovered && (!has_hovered || tree->hovered != hovered))
            _event(
                &tree->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_HOVER_CHANGED,
                tree->hovered, UINT32_MAX, 0, _mods());
        if (has_hovered && (!tree->has_hovered || tree->hovered != hovered))
            _event(
                &tree->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_HOVER_CHANGED,
                hovered, UINT32_MAX, 1, _mods());
        if (tree->has_focused && (!has_focused || tree->focused != focused))
            _event(
                &tree->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_FOCUS_CHANGED,
                tree->focused, UINT32_MAX, 0, _mods());
        if (has_focused && (!tree->has_focused || tree->focused != focused))
            _event(
                &tree->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_FOCUS_CHANGED,
                focused, UINT32_MAX, 1, _mods());
        tree->has_hovered = has_hovered;
        tree->hovered = hovered;
        tree->has_focused = has_focused;
        tree->focused = focused;
    }
    ImGui::PopStyleVar(2);
    ImGui::PopID();
    return DVZ_OK;
}

/**
 * Validate event storage before consuming draw context or controlled input.
 *
 * @param tree tree
 * @param events destination
 * @param cap event capacity
 * @param written written count
 * @param dropped dropped count
 * @return whether drawing may proceed
 */
static bool _tree_draw_capacity(
    DvzGuiTree* tree, DvzGuiDataEvent* events, uint32_t cap, uint32_t* written, uint32_t* dropped)
{
    if (written)
        *written = 0;
    if (dropped)
        *dropped = 0;
    return tree && written && dropped && (!cap || events) &&
           (!(tree->core.flags & DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED) ||
            cap >= DVZ_GUI_TREE_CONTROLLED_EVENT_CAPACITY);
}

/**
 * Draw a retained tree in its owning GUI callback and window.
 *
 * @param gui GUI
 * @param tree tree
 * @param events destination
 * @param cap event capacity
 * @param written written count
 * @param dropped dropped count
 * @return result
 */
DvzResult dvz_gui_tree_draw(
    DvzGui* gui, DvzGuiTree* tree, DvzGuiDataEvent* events, uint32_t cap, uint32_t* written,
    uint32_t* dropped)
{
    if (!_tree_draw_capacity(tree, events, cap, written, dropped) ||
        _draw_start(gui, tree ? &tree->core : NULL, events, cap, written, dropped) != DVZ_OK)
        return DVZ_ERROR;
    return _tree_draw(gui, tree, events, cap, written, dropped);
}

/**
 * Exercise actual row drawing in an isolated CPU-only ImGui test context.
 *
 * @param tree tree
 * @param events destination
 * @param cap event capacity
 * @param written written count
 * @param dropped dropped count
 * @return result
 */
DvzResult _dvz_gui_tree_debug_draw(
    DvzGuiTree* tree, DvzGuiDataEvent* events, uint32_t cap, uint32_t* written, uint32_t* dropped)
{
    if (!_tree_draw_capacity(tree, events, cap, written, dropped) || !ImGui::GetCurrentContext())
        return DVZ_ERROR;
    return _tree_draw(NULL, tree, events, cap, written, dropped);
}

/**
 * Inspect copied annotation storage for ownership regression tests.
 *
 * @param tree tree
 * @param key row key
 * @return borrowed descriptor, invalidated by annotation or row replacement
 */
const DvzGuiTreeAnnotation* _dvz_gui_tree_debug_annotation(DvzGuiTree* tree, uint64_t key)
{
    if (tree)
        for (uint32_t i = 0; i < tree->annotation_count; i++)
            if (tree->annotations[i].row_key == key)
                return &tree->annotations[i];
    return NULL;
}

void dvz_gui_tree_destroy(DvzGuiTree* tree)
{
    if (tree == NULL)
        return;
    _tree_arrays_free(tree);
    _core_free(&tree->core);
    dvz_free(tree);
}


/*************************************************************************************************/
/*  Table                                                                                        */
/*************************************************************************************************/

static _Column* _column(DvzGuiTable* table, uint32_t id)
{
    if (table)
        for (uint32_t i = 0; i < table->column_count; i++)
            if (table->columns[i].desc.column_id == id)
                return &table->columns[i];
    return NULL;
}

static void _values_free(_Column* column, uint32_t count)
{
    if (column->desc.type == DVZ_GUI_TABLE_COLUMN_TEXT)
        dvz_free_strings(count, (char**)column->values);
    dvz_free(column->values);
    column->values = NULL;
}

static bool _format_valid(const char* f)
{
    if (f == NULL || f[0] == '\0')
        return true;
    const char* p = strchr(f, '%');
    if (p == NULL)
        return false;
    p++;
    while (*p != '\0' && strchr("-+ #0", *p) != NULL)
        p++;
    while (*p >= '0' && *p <= '9')
        p++;
    if (*p == '.')
    {
        p++;
        if (*p < '0' || *p > '9')
            return false;
        while (*p >= '0' && *p <= '9')
            p++;
    }
    return *p != '\0' && strchr("eEfFgG", *p) != NULL && strchr(p + 1, '%') == NULL;
}

DvzGuiTable*
dvz_gui_table(const char* id, uint32_t n, const DvzGuiTableColumnDesc* descs, uint32_t flags)
{
    if (n == 0 || n > INT_MAX || descs == NULL)
        return NULL;
    for (uint32_t i = 0; i < n; i++)
    {
        const DvzGuiTableColumnDesc* d = &descs[i];
        if (d->struct_size != sizeof(*d) || (d->flags & ~(uint32_t)COLUMN_FLAGS) ||
            d->type > DVZ_GUI_TABLE_COLUMN_COLOR || !std::isfinite(d->initial_width) ||
            d->initial_width < 0 || !_utf8(d->title ? d->title : "") ||
            !_utf8(d->format ? d->format : "") ||
            (d->type == DVZ_GUI_TABLE_COLUMN_COLOR &&
             (d->flags & DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE)) ||
            (d->type == DVZ_GUI_TABLE_COLUMN_DOUBLE && !_format_valid(d->format)) ||
            (d->type != DVZ_GUI_TABLE_COLUMN_DOUBLE && d->format && d->format[0]))
            return NULL;
        for (uint32_t k = 0; k < 4; k++)
            if (d->reserved[k])
                return NULL;
        for (uint32_t j = 0; j < i; j++)
            if (descs[j].column_id == d->column_id)
                return NULL;
    }
    DvzGuiTable* table = (DvzGuiTable*)dvz_calloc(1, sizeof(*table));
    if (table == NULL || !_core_init(&table->core, id, flags))
    {
        dvz_free(table);
        return NULL;
    }
    table->columns = (_Column*)dvz_calloc(n, sizeof(_Column));
    if (table->columns == NULL)
    {
        dvz_gui_table_destroy(table);
        return NULL;
    }
    table->column_count = n;
    table->sort_column = UINT32_MAX;
    for (uint32_t i = 0; i < n; i++)
    {
        _Column* c = &table->columns[i];
        c->desc = descs[i];
        c->title = dvz_strdup(descs[i].title ? descs[i].title : "");
        c->format = dvz_strdup(descs[i].format ? descs[i].format : "");
        if (c->title == NULL || c->format == NULL)
        {
            dvz_gui_table_destroy(table);
            return NULL;
        }
        c->desc.title = c->title;
        c->desc.format = c->format;
    }
    return table;
}

DvzResult
dvz_gui_table_set_rows(DvzGuiTable* table, uint32_t n, const uint64_t* keys, uint32_t flags)
{
    if (table == NULL)
        return DVZ_ERROR;
    _Rows rows = {};
    if (!_rows(&table->core, n, keys, flags, &rows))
        return DVZ_ERROR;
    for (uint32_t i = 0; i < table->column_count; i++)
        _values_free(&table->columns[i], table->core.count);
    _commit(&table->core, &rows, flags);
    table->sort_column = UINT32_MAX;
    table->sort_direction = 0;
    return DVZ_OK;
}

static DvzResult _set_column(
    DvzGuiTable* table, uint32_t id, uint32_t n, const void* values, uint32_t type, size_t size)
{
    _Column* c = _column(table, id);
    if (c == NULL || c->desc.type != type || n != table->core.count || (n && values == NULL))
        return DVZ_ERROR;
    void* copy = n ? dvz_calloc(n, size) : NULL;
    if (n && copy == NULL)
        return DVZ_ERROR;
    if (n)
        dvz_memcpy(copy, n * size, values, n * size);
    _values_free(c, table->core.count);
    c->values = copy;
    table->core.dirty = true;
    return DVZ_OK;
}

DvzResult dvz_gui_table_set_column_text(
    DvzGuiTable* table, uint32_t id, uint32_t n, const char* const* values)
{
    _Column* c = _column(table, id);
    if (c == NULL || c->desc.type != DVZ_GUI_TABLE_COLUMN_TEXT || n != table->core.count ||
        (n && values == NULL))
        return DVZ_ERROR;
    char** copy = n ? (char**)dvz_calloc(n, sizeof(char*)) : NULL;
    if (n && copy == NULL)
        return DVZ_ERROR;
    for (uint32_t i = 0; i < n; i++)
    {
        if (!_utf8(values[i]) || (copy[i] = dvz_strdup(values[i])) == NULL)
        {
            dvz_free_strings(n, copy);
            dvz_free(copy);
            return DVZ_ERROR;
        }
    }
    _values_free(c, table->core.count);
    c->values = copy;
    table->core.dirty = true;
    return DVZ_OK;
}

DvzResult dvz_gui_table_set_column_int64(DvzGuiTable* t, uint32_t id, uint32_t n, const int64_t* v)
{
    return _set_column(t, id, n, v, DVZ_GUI_TABLE_COLUMN_INT64, sizeof(*v));
}
DvzResult dvz_gui_table_set_column_double(DvzGuiTable* t, uint32_t id, uint32_t n, const double* v)
{
    return _set_column(t, id, n, v, DVZ_GUI_TABLE_COLUMN_DOUBLE, sizeof(*v));
}
DvzResult dvz_gui_table_set_column_bool(DvzGuiTable* t, uint32_t id, uint32_t n, const bool* v)
{
    return _set_column(t, id, n, v, DVZ_GUI_TABLE_COLUMN_BOOL, sizeof(*v));
}
DvzResult
dvz_gui_table_set_column_color(DvzGuiTable* t, uint32_t id, uint32_t n, const DvzColor* v)
{
    return _set_column(t, id, n, v, DVZ_GUI_TABLE_COLUMN_COLOR, sizeof(*v));
}
DvzResult dvz_gui_table_set_styles(DvzGuiTable* t, uint32_t n, const DvzGuiDataStyle* s)
{
    return t ? _styles(&t->core, n, s) : DVZ_ERROR;
}
DvzResult dvz_gui_table_set_selection(DvzGuiTable* t, uint32_t n, const uint64_t* k)
{
    return t ? _selection(&t->core, n, k) : DVZ_ERROR;
}
uint32_t dvz_gui_table_get_selection(const DvzGuiTable* t, uint32_t n, uint64_t* k)
{
    return t ? _get_selection(&t->core, n, k) : 0;
}
DvzResult dvz_gui_table_set_visible(DvzGuiTable* t, uint32_t n, const bool* v)
{
    return t ? _mask(&t->core, n, v, true) : DVZ_ERROR;
}
DvzResult dvz_gui_table_set_matches(DvzGuiTable* t, uint32_t n, const bool* v)
{
    return t ? _mask(&t->core, n, v, false) : DVZ_ERROR;
}
DvzResult dvz_gui_table_set_filter(DvzGuiTable* t, const char* f)
{
    return t ? _filter(&t->core, f) : DVZ_ERROR;
}
uint32_t dvz_gui_table_get_filter(const DvzGuiTable* t, uint32_t n, char* f)
{
    return t ? _get_filter(&t->core, n, f) : 0;
}
DvzResult dvz_gui_table_get_sort(const DvzGuiTable* t, uint32_t* c, int32_t* d)
{
    if (t == NULL || c == NULL || d == NULL)
        return DVZ_ERROR;
    *c = t->sort_column;
    *d = t->sort_direction;
    return DVZ_OK;
}

static int _compare(const _Column* c, uint32_t a, uint32_t b)
{
    switch (c->desc.type)
    {
    case DVZ_GUI_TABLE_COLUMN_TEXT:
        return _text_cmp(((char**)c->values)[a], ((char**)c->values)[b]);
    case DVZ_GUI_TABLE_COLUMN_INT64:
    {
        int64_t x = ((int64_t*)c->values)[a], y = ((int64_t*)c->values)[b];
        return (x > y) - (x < y);
    }
    case DVZ_GUI_TABLE_COLUMN_DOUBLE:
    {
        double x = ((double*)c->values)[a], y = ((double*)c->values)[b];
        if (std::isnan(x) || std::isnan(y))
            return std::isnan(x) == std::isnan(y) ? 0 : (std::isnan(x) ? 1 : -1);
        return (x > y) - (x < y);
    }
    case DVZ_GUI_TABLE_COLUMN_BOOL:
        return (int)((bool*)c->values)[a] - (int)((bool*)c->values)[b];
    default:
        return 0;
    }
}

static void _table_rebuild(DvzGuiTable* table)
{
    _Core* core = &table->core;
    if (!core->dirty)
        return;
    core->order_count = 0;
    for (uint32_t row = 0; row < core->count; row++)
    {
        if (!core->visible[row] || (core->matches_active && !core->matches[row]))
            continue;
        bool match = core->filter[0] == '\0';
        for (uint32_t i = 0; !match && i < table->column_count; i++)
            if ((table->columns[i].desc.flags & DVZ_GUI_TABLE_COLUMN_FLAGS_SEARCHABLE) &&
                table->columns[i].desc.type == DVZ_GUI_TABLE_COLUMN_TEXT &&
                table->columns[i].values)
                match = _contains(((char**)table->columns[i].values)[row], core->filter);
        if (match)
            core->order[core->order_count++] = row;
    }
    _Column* sort = _column(table, table->sort_column);
    if (sort && sort->values && table->sort_direction)
        std::sort(
            core->order, core->order + core->order_count, [sort, table](uint32_t a, uint32_t b) {
                if (sort->desc.type == DVZ_GUI_TABLE_COLUMN_DOUBLE)
                {
                    const bool a_nan = std::isnan(((double*)sort->values)[a]);
                    const bool b_nan = std::isnan(((double*)sort->values)[b]);
                    if (a_nan != b_nan)
                        return !a_nan;
                }
                int cmp = _compare(sort, a, b);
                return cmp == 0 ? a < b : (table->sort_direction > 0 ? cmp < 0 : cmp > 0);
            });
    core->dirty = false;
    core->rebuild_count++;
}

static void _cell(const _Column* c, uint32_t row)
{
    if (c->values == NULL)
        return;
    char s[128] = {};
    switch (c->desc.type)
    {
    case DVZ_GUI_TABLE_COLUMN_TEXT:
        ImGui::TextUnformatted(((char**)c->values)[row]);
        break;
    case DVZ_GUI_TABLE_COLUMN_INT64:
        snprintf(s, sizeof(s), "%lld", (long long)((int64_t*)c->values)[row]);
        ImGui::TextUnformatted(s);
        break;
    case DVZ_GUI_TABLE_COLUMN_DOUBLE:
        snprintf(s, sizeof(s), c->format[0] ? c->format : "%.6g", ((double*)c->values)[row]);
        ImGui::TextUnformatted(s);
        break;
    case DVZ_GUI_TABLE_COLUMN_BOOL:
        ImGui::TextUnformatted(((bool*)c->values)[row] ? "true" : "false");
        break;
    case DVZ_GUI_TABLE_COLUMN_COLOR:
        ImGui::ColorButton(
            "##color", _color(((DvzColor*)c->values)[row]), ImGuiColorEditFlags_NoTooltip);
        break;
    default:
        break;
    }
}

DvzResult dvz_gui_table_draw(
    DvzGui* gui, DvzGuiTable* table, DvzGuiDataEvent* events, uint32_t cap, uint32_t* written,
    uint32_t* dropped)
{
    if (_draw_start(gui, table ? &table->core : NULL, events, cap, written, dropped) != DVZ_OK)
        return DVZ_ERROR;
    ImGui::PushID(table->core.id);
    if ((table->core.flags & DVZ_GUI_DATA_WIDGET_FLAGS_FILTER) && _draw_filter(&table->core))
    {
        table->core.dirty = true;
        _event(
            &table->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_FILTER_CHANGED, 0,
            UINT32_MAX, 0, _mods());
    }
    ImGuiTableFlags tf = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV |
                         ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                         ImGuiTableFlags_ScrollY;
    bool sortable = false;
    for (uint32_t i = 0; i < table->column_count; i++)
        sortable |= table->columns[i].desc.flags & DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE;
    if (sortable)
        tf |= ImGuiTableFlags_Sortable;
    if (ImGui::BeginTable("##table", (int)table->column_count, tf))
    {
        for (uint32_t i = 0; i < table->column_count; i++)
        {
            const DvzGuiTableColumnDesc* d = &table->columns[i].desc;
            ImGuiTableColumnFlags cf = (d->flags & DVZ_GUI_TABLE_COLUMN_FLAGS_STRETCH)
                                           ? ImGuiTableColumnFlags_WidthStretch
                                           : ImGuiTableColumnFlags_WidthFixed;
            if (!(d->flags & DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE))
                cf |= ImGuiTableColumnFlags_NoSort;
            ImGui::TableSetupColumn(table->columns[i].title, cf, d->initial_width, d->column_id);
        }
        ImGui::TableHeadersRow();
        ImGuiTableSortSpecs* specs = sortable ? ImGui::TableGetSortSpecs() : NULL;
        if (specs && specs->SpecsDirty)
        {
            uint32_t col = UINT32_MAX;
            int32_t dir = 0;
            if (specs->SpecsCount)
            {
                col = specs->Specs[0].ColumnUserID;
                dir = specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending ? 1 : -1;
            }
            if (col != table->sort_column || dir != table->sort_direction)
            {
                table->sort_column = col;
                table->sort_direction = dir;
                table->core.dirty = true;
                _event(
                    &table->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_SORT_CHANGED,
                    0, col, dir, _mods());
            }
            specs->SpecsDirty = false;
        }
        _table_rebuild(table);
        ImGuiListClipper clip;
        clip.Begin((int)table->core.order_count);
        while (clip.Step())
            for (int q = clip.DisplayStart; q < clip.DisplayEnd; q++)
            {
                uint32_t row = table->core.order[q];
                const DvzGuiDataStyle* style = _style(&table->core, table->core.keys[row]);
                bool disabled = style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_DISABLED);
                ImGui::TableNextRow();
                if (style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_BACKGROUND))
                    ImGui::TableSetBgColor(
                        ImGuiTableBgTarget_RowBg0,
                        ImGui::ColorConvertFloat4ToU32(_color(style->background)));
                _push_id(&table->core, row);
                if (style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_FOREGROUND))
                    ImGui::PushStyleColor(ImGuiCol_Text, _color(style->foreground));
                if (disabled)
                    ImGui::BeginDisabled();
                ImGui::TableSetColumnIndex(0);
                if (style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_ACCENT))
                {
                    ImVec2 a = ImGui::GetCursorScreenPos();
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        a, ImVec2(a.x + 3, a.y + ImGui::GetFrameHeight()),
                        ImGui::ColorConvertFloat4ToU32(_color(style->accent)));
                }
                ImGui::Selectable(
                    "##row", table->core.selected[row],
                    ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap |
                        ImGuiSelectableFlags_AllowDoubleClick);
                bool clicked = ImGui::IsItemClicked(0);
                bool activated = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0);
                ImGui::SameLine(0, 0);
                _cell(&table->columns[0], row);
                for (uint32_t c = 1; c < table->column_count; c++)
                {
                    ImGui::TableSetColumnIndex((int)c);
                    ImGui::PushID((int)c);
                    _cell(&table->columns[c], row);
                    ImGui::PopID();
                }
                if (clicked && !disabled)
                {
                    uint32_t mods = _mods();
                    _select(&table->core, (uint32_t)q, mods);
                    _event(
                        &table->core, events, cap, written, dropped,
                        DVZ_GUI_DATA_EVENT_SELECTION_CHANGED, table->core.keys[row], UINT32_MAX,
                        table->core.selected[row], mods);
                }
                if (activated && !disabled)
                    _event(
                        &table->core, events, cap, written, dropped, DVZ_GUI_DATA_EVENT_ACTIVATED,
                        table->core.keys[row], UINT32_MAX, 0, _mods());
                if (disabled)
                    ImGui::EndDisabled();
                if (style && (style->flags & DVZ_GUI_DATA_STYLE_FLAGS_FOREGROUND))
                    ImGui::PopStyleColor();
                _pop_id();
            }
        ImGui::EndTable();
    }
    ImGui::PopID();
    return DVZ_OK;
}

void dvz_gui_table_destroy(DvzGuiTable* table)
{
    if (table == NULL)
        return;
    for (uint32_t i = 0; i < table->column_count; i++)
    {
        _values_free(&table->columns[i], table->core.count);
        dvz_free(table->columns[i].title);
        dvz_free(table->columns[i].format);
    }
    dvz_free(table->columns);
    _core_free(&table->core);
    dvz_free(table);
}


/*************************************************************************************************/
/*  Internal CPU test support                                                                    */
/*************************************************************************************************/

bool _dvz_gui_tree_debug_state(DvzGuiTree* tree, DvzGuiDataDebugState* out)
{
    if (tree == NULL || out == NULL)
        return false;
    _tree_rebuild(tree);
    *out = {tree->core.count, tree->core.order_count, tree->core.rebuild_count};
    return true;
}

uint64_t _dvz_gui_tree_debug_display_key(DvzGuiTree* tree, uint32_t display)
{
    if (tree == NULL)
        return UINT64_MAX;
    _tree_rebuild(tree);
    return display < tree->core.order_count ? tree->core.keys[tree->core.order[display]]
                                            : UINT64_MAX;
}

bool _dvz_gui_table_debug_state(DvzGuiTable* table, DvzGuiDataDebugState* out)
{
    if (table == NULL || out == NULL)
        return false;
    _table_rebuild(table);
    *out = {table->core.count, table->core.order_count, table->core.rebuild_count};
    return true;
}

uint64_t _dvz_gui_table_debug_display_key(DvzGuiTable* table, uint32_t display)
{
    if (table == NULL)
        return UINT64_MAX;
    _table_rebuild(table);
    return display < table->core.order_count ? table->core.keys[table->core.order[display]]
                                             : UINT64_MAX;
}

DvzResult _dvz_gui_table_debug_sort(DvzGuiTable* table, uint32_t id, int32_t direction)
{
    _Column* c = _column(table, id);
    if (table == NULL || (direction != -1 && direction != 0 && direction != 1) ||
        (direction && (c == NULL || c->values == NULL ||
                       !(c->desc.flags & DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE))))
        return DVZ_ERROR;
    table->sort_column = direction ? id : UINT32_MAX;
    table->sort_direction = direction;
    table->core.dirty = true;
    return DVZ_OK;
}
