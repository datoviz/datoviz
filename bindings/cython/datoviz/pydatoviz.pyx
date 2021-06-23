# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from functools import wraps, partial
import logging

cimport numpy as np
import numpy as np
from cpython.ref cimport Py_INCREF
from libc.string cimport memcpy
from libc.stdio cimport printf

cimport datoviz.cydatoviz as cv

logger = logging.getLogger('datoviz')



# -------------------------------------------------------------------------------------------------
# Types
# -------------------------------------------------------------------------------------------------

ctypedef np.float32_t FLOAT
ctypedef np.double_t DOUBLE
ctypedef np.uint8_t CHAR
ctypedef np.uint8_t[4] CVEC4
ctypedef np.int16_t SHORT
ctypedef np.uint16_t USHORT
ctypedef np.int32_t INT
ctypedef np.uint32_t UINT
ctypedef np.uint32_t[3] TEX_SHAPE



# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------
# region  # folding in VSCode

DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 600

cdef TEX_SHAPE DVZ_ZERO_OFFSET = (0, 0, 0)



# TODO: add more keys
_KEYS = {
    cv.DVZ_KEY_LEFT: 'left',
    cv.DVZ_KEY_RIGHT: 'right',
    cv.DVZ_KEY_UP: 'up',
    cv.DVZ_KEY_DOWN: 'down',
    cv.DVZ_KEY_HOME: 'home',
    cv.DVZ_KEY_END: 'end',
    cv.DVZ_KEY_KP_ADD: '+',
    cv.DVZ_KEY_KP_SUBTRACT: '-',
    cv.DVZ_KEY_F: 'f',
    cv.DVZ_KEY_G: 'g',
}

# HACK: these keys do not raise a Python key event
_EXCLUDED_KEYS = (
    cv.DVZ_KEY_NONE,
    cv.DVZ_KEY_LEFT_SHIFT,
    cv.DVZ_KEY_LEFT_CONTROL,
    cv.DVZ_KEY_LEFT_ALT,
    cv.DVZ_KEY_LEFT_SUPER,
    cv.DVZ_KEY_RIGHT_SHIFT,
    cv.DVZ_KEY_RIGHT_CONTROL,
    cv.DVZ_KEY_RIGHT_ALT,
    cv.DVZ_KEY_RIGHT_SUPER,
)

_BUTTONS = {
    cv.DVZ_MOUSE_BUTTON_LEFT: 'left',
    cv.DVZ_MOUSE_BUTTON_MIDDLE: 'middle',
    cv.DVZ_MOUSE_BUTTON_RIGHT: 'right',
}

_MODIFIERS = {
    cv.DVZ_KEY_MODIFIER_SHIFT: 'shift',
    cv.DVZ_KEY_MODIFIER_CONTROL: 'control',
    cv.DVZ_KEY_MODIFIER_ALT: 'alt',
    cv.DVZ_KEY_MODIFIER_SUPER: 'super',
}

_BUTTONS_INV = {v: k for k, v in _BUTTONS.items()}

_EVENTS ={
    'mouse_press': cv.DVZ_EVENT_MOUSE_PRESS,
    'mouse_release': cv.DVZ_EVENT_MOUSE_RELEASE,
    'mouse_move': cv.DVZ_EVENT_MOUSE_MOVE,
    'mouse_wheel': cv.DVZ_EVENT_MOUSE_WHEEL,
    'mouse_drag_begin': cv.DVZ_EVENT_MOUSE_DRAG_BEGIN,
    'mouse_drag_end': cv.DVZ_EVENT_MOUSE_DRAG_END,
    'mouse_click': cv.DVZ_EVENT_MOUSE_CLICK,
    'mouse_double_click': cv.DVZ_EVENT_MOUSE_DOUBLE_CLICK,
    'key_press': cv.DVZ_EVENT_KEY_PRESS,
    'key_release': cv.DVZ_EVENT_KEY_RELEASE,
    'frame': cv.DVZ_EVENT_FRAME,
    'timer': cv.DVZ_EVENT_TIMER,
    'gui': cv.DVZ_EVENT_GUI,
}

_VISUALS = {
    'point': cv.DVZ_VISUAL_POINT,
    'marker': cv.DVZ_VISUAL_MARKER,
    'mesh': cv.DVZ_VISUAL_MESH,
    'path': cv.DVZ_VISUAL_PATH,
    'polygon': cv.DVZ_VISUAL_POLYGON,
    'image': cv.DVZ_VISUAL_IMAGE,
    'image_cmap': cv.DVZ_VISUAL_IMAGE_CMAP,
    'volume': cv.DVZ_VISUAL_VOLUME,
    'volume_slice': cv.DVZ_VISUAL_VOLUME_SLICE,
    'line_strip': cv.DVZ_VISUAL_LINE_STRIP,
}

_CONTROLLERS = {
    'panzoom': cv.DVZ_CONTROLLER_PANZOOM,
    'axes': cv.DVZ_CONTROLLER_AXES_2D,
    'arcball': cv.DVZ_CONTROLLER_ARCBALL,
    'camera': cv.DVZ_CONTROLLER_CAMERA,
}

_TRANSPOSES = {
    None: cv.DVZ_CDS_TRANSPOSE_NONE,
    'xfyrzu': cv.DVZ_CDS_TRANSPOSE_XFYRZU,
    'xbydzl': cv.DVZ_CDS_TRANSPOSE_XBYDZL,
    'xlybzd': cv.DVZ_CDS_TRANSPOSE_XLYBZD,
}

_COORDINATE_SYSTEMS = {
    'data': cv.DVZ_CDS_DATA,
    'scene': cv.DVZ_CDS_SCENE,
    'vulkan': cv.DVZ_CDS_VULKAN,
    'framebuffer': cv.DVZ_CDS_FRAMEBUFFER,
    'window': cv.DVZ_CDS_WINDOW,
}

_PROPS = {
    'pos': cv.DVZ_PROP_POS,
    'color': cv.DVZ_PROP_COLOR,
    'alpha': cv.DVZ_PROP_ALPHA,
    'ms': cv.DVZ_PROP_MARKER_SIZE,
    'marker_type': cv.DVZ_PROP_MARKER_TYPE,
    'normal': cv.DVZ_PROP_NORMAL,
    'texcoords': cv.DVZ_PROP_TEXCOORDS,
    'index': cv.DVZ_PROP_INDEX,
    'range': cv.DVZ_PROP_RANGE,
    'length': cv.DVZ_PROP_LENGTH,
    'scale': cv.DVZ_PROP_SCALE,
    'cap_type': cv.DVZ_PROP_CAP_TYPE,
    'light_params': cv.DVZ_PROP_LIGHT_PARAMS,
    'light_pos': cv.DVZ_PROP_LIGHT_POS,
    'texcoefs': cv.DVZ_PROP_TEXCOEFS,
    'linewidth': cv.DVZ_PROP_LINE_WIDTH,
    'colormap': cv.DVZ_PROP_COLORMAP,
    'transferx': cv.DVZ_PROP_TRANSFER_X,
    'transfery': cv.DVZ_PROP_TRANSFER_Y,
    'clip': cv.DVZ_PROP_CLIP,
}

_DTYPES = {
    cv.DVZ_DTYPE_CHAR: (np.uint8, 1),
    cv.DVZ_DTYPE_CVEC2: (np.uint8, 2),
    cv.DVZ_DTYPE_CVEC3: (np.uint8, 3),
    cv.DVZ_DTYPE_CVEC4: (np.uint8, 4),

    cv.DVZ_DTYPE_USHORT: (np.uint16, 1),
    cv.DVZ_DTYPE_USVEC2: (np.uint16, 2),
    cv.DVZ_DTYPE_USVEC3: (np.uint16, 3),
    cv.DVZ_DTYPE_USVEC4: (np.uint16, 4),

    cv.DVZ_DTYPE_SHORT: (np.int16, 1),
    cv.DVZ_DTYPE_SVEC2: (np.int16, 2),
    cv.DVZ_DTYPE_SVEC3: (np.int16, 3),
    cv.DVZ_DTYPE_SVEC4: (np.int16, 4),

    cv.DVZ_DTYPE_UINT: (np.uint32, 1),
    cv.DVZ_DTYPE_UVEC2: (np.uint32, 2),
    cv.DVZ_DTYPE_UVEC3: (np.uint32, 3),
    cv.DVZ_DTYPE_UVEC4: (np.uint32, 4),

    cv.DVZ_DTYPE_INT: (np.int32, 1),
    cv.DVZ_DTYPE_IVEC2: (np.int32, 2),
    cv.DVZ_DTYPE_IVEC3: (np.int32, 3),
    cv.DVZ_DTYPE_IVEC4: (np.int32, 4),

    cv.DVZ_DTYPE_FLOAT: (np.float32, 1),
    cv.DVZ_DTYPE_VEC2: (np.float32, 2),
    cv.DVZ_DTYPE_VEC3: (np.float32, 3),
    cv.DVZ_DTYPE_VEC4: (np.float32, 4),

    cv.DVZ_DTYPE_DOUBLE: (np.double, 1),
    cv.DVZ_DTYPE_DVEC2: (np.double, 2),
    cv.DVZ_DTYPE_DVEC3: (np.double, 3),
    cv.DVZ_DTYPE_DVEC4: (np.double, 4),

    cv.DVZ_DTYPE_MAT2: (np.float32, (2, 2)),
    cv.DVZ_DTYPE_MAT3: (np.float32, (3, 3)),
    cv.DVZ_DTYPE_MAT4: (np.float32, (4, 4)),
}

_TRANSFORMS = {
    'earth': cv.DVZ_TRANSFORM_EARTH_MERCATOR_WEB,
}

_CONTROLS = {
    'slider_float': cv.DVZ_GUI_CONTROL_SLIDER_FLOAT,
    'slider_float2': cv.DVZ_GUI_CONTROL_SLIDER_FLOAT2,
    'slider_int': cv.DVZ_GUI_CONTROL_SLIDER_INT,
    'input_float': cv.DVZ_GUI_CONTROL_INPUT_FLOAT,
    'checkbox': cv.DVZ_GUI_CONTROL_CHECKBOX,
    'button': cv.DVZ_GUI_CONTROL_BUTTON,
    'label': cv.DVZ_GUI_CONTROL_LABEL,
}

_COLORMAPS = {
    'binary': cv.DVZ_CMAP_BINARY,
    'hsv': cv.DVZ_CMAP_HSV,
    'cividis': cv.DVZ_CMAP_CIVIDIS,
    'inferno': cv.DVZ_CMAP_INFERNO,
    'magma': cv.DVZ_CMAP_MAGMA,
    'plasma': cv.DVZ_CMAP_PLASMA,
    'viridis': cv.DVZ_CMAP_VIRIDIS,
    'blues': cv.DVZ_CMAP_BLUES,
    'bugn': cv.DVZ_CMAP_BUGN,
    'bupu': cv.DVZ_CMAP_BUPU,
    'gnbu': cv.DVZ_CMAP_GNBU,
    'greens': cv.DVZ_CMAP_GREENS,
    'greys': cv.DVZ_CMAP_GREYS,
    'oranges': cv.DVZ_CMAP_ORANGES,
    'orrd': cv.DVZ_CMAP_ORRD,
    'pubu': cv.DVZ_CMAP_PUBU,
    'pubugn': cv.DVZ_CMAP_PUBUGN,
    'purples': cv.DVZ_CMAP_PURPLES,
    'rdpu': cv.DVZ_CMAP_RDPU,
    'reds': cv.DVZ_CMAP_REDS,
    'ylgn': cv.DVZ_CMAP_YLGN,
    'ylgnbu': cv.DVZ_CMAP_YLGNBU,
    'ylorbr': cv.DVZ_CMAP_YLORBR,
    'ylorrd': cv.DVZ_CMAP_YLORRD,
    'afmhot': cv.DVZ_CMAP_AFMHOT,
    'autumn': cv.DVZ_CMAP_AUTUMN,
    'bone': cv.DVZ_CMAP_BONE,
    'cool': cv.DVZ_CMAP_COOL,
    'copper': cv.DVZ_CMAP_COPPER,
    'gist_heat': cv.DVZ_CMAP_GIST_HEAT,
    'gray': cv.DVZ_CMAP_GRAY,
    'hot': cv.DVZ_CMAP_HOT,
    'pink': cv.DVZ_CMAP_PINK,
    'spring': cv.DVZ_CMAP_SPRING,
    'summer': cv.DVZ_CMAP_SUMMER,
    'winter': cv.DVZ_CMAP_WINTER,
    'wistia': cv.DVZ_CMAP_WISTIA,
    'brbg': cv.DVZ_CMAP_BRBG,
    'bwr': cv.DVZ_CMAP_BWR,
    'coolwarm': cv.DVZ_CMAP_COOLWARM,
    'piyg': cv.DVZ_CMAP_PIYG,
    'prgn': cv.DVZ_CMAP_PRGN,
    'puor': cv.DVZ_CMAP_PUOR,
    'rdbu': cv.DVZ_CMAP_RDBU,
    'rdgy': cv.DVZ_CMAP_RDGY,
    'rdylbu': cv.DVZ_CMAP_RDYLBU,
    'rdylgn': cv.DVZ_CMAP_RDYLGN,
    'seismic': cv.DVZ_CMAP_SEISMIC,
    'spectral': cv.DVZ_CMAP_SPECTRAL,
    'twilight_shifted': cv.DVZ_CMAP_TWILIGHT_SHIFTED,
    'twilight': cv.DVZ_CMAP_TWILIGHT,
    'brg': cv.DVZ_CMAP_BRG,
    'cmrmap': cv.DVZ_CMAP_CMRMAP,
    'cubehelix': cv.DVZ_CMAP_CUBEHELIX,
    'flag': cv.DVZ_CMAP_FLAG,
    'gist_earth': cv.DVZ_CMAP_GIST_EARTH,
    'gist_ncar': cv.DVZ_CMAP_GIST_NCAR,
    'gist_rainbow': cv.DVZ_CMAP_GIST_RAINBOW,
    'gist_stern': cv.DVZ_CMAP_GIST_STERN,
    'gnuplot2': cv.DVZ_CMAP_GNUPLOT2,
    'gnuplot': cv.DVZ_CMAP_GNUPLOT,
    'jet': cv.DVZ_CMAP_JET,
    'nipy_spectral': cv.DVZ_CMAP_NIPY_SPECTRAL,
    'ocean': cv.DVZ_CMAP_OCEAN,
    'prism': cv.DVZ_CMAP_PRISM,
    'rainbow': cv.DVZ_CMAP_RAINBOW,
    'terrain': cv.DVZ_CMAP_TERRAIN,
    'bkr': cv.DVZ_CMAP_BKR,
    'bky': cv.DVZ_CMAP_BKY,
    'cet_d10': cv.DVZ_CMAP_CET_D10,
    'cet_d11': cv.DVZ_CMAP_CET_D11,
    'cet_d8': cv.DVZ_CMAP_CET_D8,
    'cet_d13': cv.DVZ_CMAP_CET_D13,
    'cet_d3': cv.DVZ_CMAP_CET_D3,
    'cet_d1a': cv.DVZ_CMAP_CET_D1A,
    'bjy': cv.DVZ_CMAP_BJY,
    'gwv': cv.DVZ_CMAP_GWV,
    'bwy': cv.DVZ_CMAP_BWY,
    'cet_d12': cv.DVZ_CMAP_CET_D12,
    'cet_r3': cv.DVZ_CMAP_CET_R3,
    'cet_d9': cv.DVZ_CMAP_CET_D9,
    'cwr': cv.DVZ_CMAP_CWR,
    'cet_cbc1': cv.DVZ_CMAP_CET_CBC1,
    'cet_cbc2': cv.DVZ_CMAP_CET_CBC2,
    'cet_cbl1': cv.DVZ_CMAP_CET_CBL1,
    'cet_cbl2': cv.DVZ_CMAP_CET_CBL2,
    'cet_cbtc1': cv.DVZ_CMAP_CET_CBTC1,
    'cet_cbtc2': cv.DVZ_CMAP_CET_CBTC2,
    'cet_cbtl1': cv.DVZ_CMAP_CET_CBTL1,
    'bgy': cv.DVZ_CMAP_BGY,
    'bgyw': cv.DVZ_CMAP_BGYW,
    'bmw': cv.DVZ_CMAP_BMW,
    'cet_c1': cv.DVZ_CMAP_CET_C1,
    'cet_c1s': cv.DVZ_CMAP_CET_C1S,
    'cet_c2': cv.DVZ_CMAP_CET_C2,
    'cet_c4': cv.DVZ_CMAP_CET_C4,
    'cet_c4s': cv.DVZ_CMAP_CET_C4S,
    'cet_c5': cv.DVZ_CMAP_CET_C5,
    'cet_i1': cv.DVZ_CMAP_CET_I1,
    'cet_i3': cv.DVZ_CMAP_CET_I3,
    'cet_l10': cv.DVZ_CMAP_CET_L10,
    'cet_l11': cv.DVZ_CMAP_CET_L11,
    'cet_l12': cv.DVZ_CMAP_CET_L12,
    'cet_l16': cv.DVZ_CMAP_CET_L16,
    'cet_l17': cv.DVZ_CMAP_CET_L17,
    'cet_l18': cv.DVZ_CMAP_CET_L18,
    'cet_l19': cv.DVZ_CMAP_CET_L19,
    'cet_l4': cv.DVZ_CMAP_CET_L4,
    'cet_l7': cv.DVZ_CMAP_CET_L7,
    'cet_l8': cv.DVZ_CMAP_CET_L8,
    'cet_l9': cv.DVZ_CMAP_CET_L9,
    'cet_r1': cv.DVZ_CMAP_CET_R1,
    'cet_r2': cv.DVZ_CMAP_CET_R2,
    'colorwheel': cv.DVZ_CMAP_COLORWHEEL,
    'fire': cv.DVZ_CMAP_FIRE,
    'isolum': cv.DVZ_CMAP_ISOLUM,
    'kb': cv.DVZ_CMAP_KB,
    'kbc': cv.DVZ_CMAP_KBC,
    'kg': cv.DVZ_CMAP_KG,
    'kgy': cv.DVZ_CMAP_KGY,
    'kr': cv.DVZ_CMAP_KR,
    'black_body': cv.DVZ_CMAP_BLACK_BODY,
    'kindlmann': cv.DVZ_CMAP_KINDLMANN,
    'extended_kindlmann': cv.DVZ_CMAP_EXTENDED_KINDLMANN,
    'glasbey': cv.DVZ_CPAL256_GLASBEY,
    'glasbey_cool': cv.DVZ_CPAL256_GLASBEY_COOL,
    'glasbey_dark': cv.DVZ_CPAL256_GLASBEY_DARK,
    'glasbey_hv': cv.DVZ_CPAL256_GLASBEY_HV,
    'glasbey_light': cv.DVZ_CPAL256_GLASBEY_LIGHT,
    'glasbey_warm': cv.DVZ_CPAL256_GLASBEY_WARM,
    'accent': cv.DVZ_CPAL032_ACCENT,
    'dark2': cv.DVZ_CPAL032_DARK2,
    'paired': cv.DVZ_CPAL032_PAIRED,
    'pastel1': cv.DVZ_CPAL032_PASTEL1,
    'pastel2': cv.DVZ_CPAL032_PASTEL2,
    'set1': cv.DVZ_CPAL032_SET1,
    'set2': cv.DVZ_CPAL032_SET2,
    'set3': cv.DVZ_CPAL032_SET3,
    'tab10': cv.DVZ_CPAL032_TAB10,
    'tab20': cv.DVZ_CPAL032_TAB20,
    'tab20b': cv.DVZ_CPAL032_TAB20B,
    'tab20c': cv.DVZ_CPAL032_TAB20C,
    'category10_10': cv.DVZ_CPAL032_CATEGORY10_10,
    'category20_20': cv.DVZ_CPAL032_CATEGORY20_20,
    'category20b_20': cv.DVZ_CPAL032_CATEGORY20B_20,
    'category20c_20': cv.DVZ_CPAL032_CATEGORY20C_20,
    'colorblind8': cv.DVZ_CPAL032_COLORBLIND8,
}

_TEXTURE_FILTERS = {
    None: cv.VK_FILTER_NEAREST,
    'nearest': cv.VK_FILTER_NEAREST,
    'linear': cv.VK_FILTER_LINEAR,
    # 'cubic': cv.VK_FILTER_CUBIC_EXT,  # requires extension VK_EXT_filter_cubic
}

_SOURCE_TYPES = {
    1: cv.DVZ_SOURCE_TYPE_TRANSFER,
    2: cv.DVZ_SOURCE_TYPE_IMAGE,
    3: cv.DVZ_SOURCE_TYPE_VOLUME,
}

_FORMATS = {
    (np.dtype(np.uint8), 1): cv.VK_FORMAT_R8_UNORM,
    (np.dtype(np.uint8), 3): cv.VK_FORMAT_R8G8B8_UNORM,
    (np.dtype(np.uint8), 4): cv.VK_FORMAT_R8G8B8A8_UNORM,
    (np.dtype(np.uint16), 1): cv.VK_FORMAT_R16_UNORM,
    (np.dtype(np.int16), 1): cv.VK_FORMAT_R16_SNORM,
    (np.dtype(np.uint32), 1): cv.VK_FORMAT_R32_UINT,
    (np.dtype(np.int32), 1): cv.VK_FORMAT_R32_SINT,
    (np.dtype(np.float32), 1): cv.VK_FORMAT_R32_SFLOAT,
}

_MARKER_TYPES = {
    'disc': cv.DVZ_MARKER_DISC,
    'vbar': cv.DVZ_MARKER_VBAR,
}

_CUSTOM_COLORMAPS = {}

#endregion



# -------------------------------------------------------------------------------------------------
# Constant utils
# -------------------------------------------------------------------------------------------------

def _key_name(key):
    """From key code used by Datoviz to key name."""
    return _KEYS.get(key, key)

def _button_name(button):
    """From button code used by Datoviz to button name."""
    return _BUTTONS.get(button, None)

def _get_modifiers(mod):
    """From modifier flag to a tuple of strings."""
    mods_py = []
    for c_enum, name in _MODIFIERS.items():
        if mod & c_enum:
            mods_py.append(name)
    return tuple(mods_py)

def _c_modifiers(*mods):
    cdef int mod, c_enum
    mod = 0
    for c_enum, name in _MODIFIERS.items():
        if name in mods:
            mod |= c_enum
    return mod

def _get_prop(name):
    """From prop name to prop enum for Datoviz."""
    return _PROPS[name]



# -------------------------------------------------------------------------------------------------
# Python event callbacks
# -------------------------------------------------------------------------------------------------

cdef _get_event_args(cv.DvzEvent c_ev):
    """Prepare the arguments to the Python event callbacks from the Datoviz DvzEvent struct."""
    cdef float* fvalue
    cdef int* ivalue
    cdef bint* bvalue
    dt = c_ev.type

    # GUI events.
    if dt == cv.DVZ_EVENT_GUI:
        if c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_SLIDER_FLOAT:
            fvalue = <float*>c_ev.u.g.control.value
            return (fvalue[0],), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_SLIDER_FLOAT2:
            fvalue = <float*>c_ev.u.g.control.value
            return (fvalue[0], fvalue[1]), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_SLIDER_INT:
            ivalue = <int*>c_ev.u.g.control.value
            return (ivalue[0],), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_INPUT_FLOAT:
            fvalue = <float*>c_ev.u.g.control.value
            return (fvalue[0],), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_CHECKBOX:
            bvalue = <bint*>c_ev.u.g.control.value
            return (bvalue[0],), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_BUTTON:
            bvalue = <bint*>c_ev.u.g.control.value
            return (bvalue[0],), {}

    # Key events.
    elif dt == cv.DVZ_EVENT_KEY_PRESS or dt == cv.DVZ_EVENT_KEY_RELEASE:
        key = _key_name(c_ev.u.k.key_code)
        modifiers = _get_modifiers(c_ev.u.k.modifiers)
        return (key, modifiers), {}

    # Mouse button events.
    elif dt == cv.DVZ_EVENT_MOUSE_PRESS or dt == cv.DVZ_EVENT_MOUSE_RELEASE:
        button = _button_name(c_ev.u.b.button)
        modifiers = _get_modifiers(c_ev.u.b.modifiers)
        return (button, modifiers), {}

    # Mouse button events.
    elif dt == cv.DVZ_EVENT_MOUSE_CLICK or dt == cv.DVZ_EVENT_MOUSE_DOUBLE_CLICK:
        x = c_ev.u.c.pos[0]
        y = c_ev.u.c.pos[1]
        button = _button_name(c_ev.u.c.button)
        modifiers = _get_modifiers(c_ev.u.c.modifiers)
        dbl = c_ev.u.c.double_click
        return (x, y), dict(button=button, modifiers=modifiers)  #, double_click=dbl)

    # Mouse move event.
    elif dt == cv.DVZ_EVENT_MOUSE_MOVE:
        x = c_ev.u.m.pos[0]
        y = c_ev.u.m.pos[1]
        modifiers = _get_modifiers(c_ev.u.m.modifiers)
        return (x, y), dict(modifiers=modifiers)

    # Mouse wheel event.
    elif dt == cv.DVZ_EVENT_MOUSE_WHEEL:
        x = c_ev.u.w.pos[0]
        y = c_ev.u.w.pos[1]
        dx = c_ev.u.w.dir[0]
        dy = c_ev.u.w.dir[1]
        modifiers = _get_modifiers(c_ev.u.w.modifiers)
        return (x, y, dx, dy), dict(modifiers=modifiers)

    # Frame event.
    elif dt == cv.DVZ_EVENT_FRAME:
        idx = c_ev.u.f.idx
        return (idx,), {}

    return (), {}



cdef _wrapped_callback(cv.DvzCanvas* c_canvas, cv.DvzEvent c_ev):
    """C callback function that wraps a Python callback function."""

    # NOTE: this function may run in a background thread if using async callbacks
    # It should not acquire the GIL.

    cdef object tup
    if c_ev.user_data != NULL:

        # The Python function and its arguments are wrapped in this Python object.
        tup = <object>c_ev.user_data

        # For each type of event, get the arguments to the function
        ev_args, ev_kwargs = _get_event_args(c_ev)

        # Recover the Python function and arguments.
        f, args = tup

        # This is the control type the callback was registered for.
        name = args[0] if args else None

        # NOTE: we only call the callback if the raised GUI event is for that control.
        dt = c_ev.type
        if dt == cv.DVZ_EVENT_GUI:
            if c_ev.u.g.control.name != name:
                return

        # We run the registered Python function on the event arguments.
        try:
            f(*ev_args, **ev_kwargs)
        except Exception as e:
            print("Error: %s" % e)



cdef _add_event_callback(
    cv.DvzCanvas* c_canvas, cv.DvzEventType evtype, double param, f, args,
    cv.DvzEventMode mode=cv.DVZ_EVENT_MODE_SYNC):
    """Register a Python callback function using the Datoviz C API."""

    # Create a tuple with the Python function, and the arguments.
    cdef void* ptr_to_obj
    tup = (f, args)

    # IMPORTANT: need to either keep a reference of this tuple object somewhere in the class,
    # or increase the ref, otherwise this tuple will be deleted by the time we call it in the
    # C callback function.
    Py_INCREF(tup)

    # Call the Datoviz C API to register the C-wrapped callback function.
    ptr_to_obj = <void*>tup
    cv.dvz_event_callback(
        c_canvas, evtype, param, mode,
        <cv.DvzEventCallback>_wrapped_callback, ptr_to_obj)



# -------------------------------------------------------------------------------------------------
# Public functions
# -------------------------------------------------------------------------------------------------

def colormap(np.ndarray[DOUBLE, ndim=1] values, vmin=None, vmax=None, cmap=None, alpha=None):
    """Apply a colormap to a 1D array of values."""
    N = values.size
    if cmap in _COLORMAPS:
        cmap_ = _COLORMAPS[cmap]
    elif cmap in _CUSTOM_COLORMAPS:
        cmap_ = _CUSTOM_COLORMAPS[cmap]
    else:
        cmap_ = cv.DVZ_CMAP_VIRIDIS
    # TODO: ndarrays
    cdef np.ndarray out = np.zeros((N, 4), dtype=np.uint8)
    if vmin is None:
        vmin = values.min()
    if vmax is None:
        vmax = values.max()
    cv.dvz_colormap_array(cmap_, N, <double*>&values.data[0], vmin, vmax, <cv.cvec4*>&out.data[0])
    if alpha is not None:
        if not isinstance(alpha, np.ndarray):
            alpha = np.array(alpha)
        alpha = (alpha * 255).astype(np.uint8)
        out[:, 3] = alpha
    return out

def demo():
    cv.dvz_demo_standalone()



# -------------------------------------------------------------------------------------------------
# Util functions
# -------------------------------------------------------------------------------------------------

def _validate_data(dt, nc, data):
    """Ensure a NumPy array has a given dtype and shape and is contiguous."""
    data = data.astype(dt)
    if not data.flags['C_CONTIGUOUS']:
        data = np.ascontiguousarray(data)
    if not hasattr(nc, '__len__'):
        nc = (nc,)
    nd = len(nc)  # expected dimension of the data - 1
    if nc[0] == 1 and data.ndim == 1:
        data = data.reshape((-1, 1))
    if data.ndim < nd + 1:
        data = data[np.newaxis, :]
    assert data.ndim == nd + 1, f"Incorrect array dimension {data.shape}, nc={nc}"
    assert data.shape[1:] == nc, f"Incorrect array shape {data.shape} instead of {nc}"
    assert data.dtype == dt, f"Array dtype is {data.dtype} instead of {dt}"
    return data



cdef _canvas_flags(show_fps=None, pick=None, high_dpi=None, offscreen=None):
    """Make the canvas flags from the Python keyword arguments to the canvas creation function."""
    cdef int flags = 0
    flags |= cv.DVZ_CANVAS_FLAGS_IMGUI
    if show_fps:
        flags |= cv.DVZ_CANVAS_FLAGS_FPS
    if pick:
        flags |= cv.DVZ_CANVAS_FLAGS_PICK
    if high_dpi:
        flags |= cv.DVZ_CANVAS_FLAGS_DPI_SCALE_200
    if offscreen:
        flags |= cv.DVZ_CANVAS_FLAGS_OFFSCREEN
    return flags



# -------------------------------------------------------------------------------------------------
# App
# -------------------------------------------------------------------------------------------------

cdef class App:
    """Singleton object that gives access to the GPUs."""

    cdef cv.DvzApp* _c_app

    _gpus = {}

    def __cinit__(self):
        """Create a Datoviz app."""
        # TODO: selection of the backend
        self._c_app = cv.dvz_app(cv.DVZ_BACKEND_GLFW)
        if self._c_app is NULL:
            raise MemoryError()

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        """Destroy the app."""
        if self._c_app is not NULL:
            # # Destroy all GPUs.
            # for gpu in self._gpus.values():
            #     gpu.destroy()
            # self._gpus.clear()
            cv.dvz_app_destroy(self._c_app)
            self._c_app = NULL

    def gpu(self, idx=None):
        """Get a GPU, identified by its index, or the "best" one by default."""
        if idx in self._gpus:
            return self._gpus[idx]
        g = GPU()
        if idx is None:
            g.create_best(self._c_app)
        else:
            assert idx >= 0
            g.create(self._c_app, idx)
        self._gpus[idx] = g
        return g

    def run(
            self, int n_frames=0, unicode screenshot=None, unicode video=None,
            bint offscreen=False):
        """Start the rendering loop."""

        # Autorun struct.
        cdef cv.DvzAutorun autorun = [0, 0]
        if screenshot or video:
            logger.debug("Enabling autorun")
            autorun.enable = True
            autorun.n_frames = n_frames
            autorun.offscreen = offscreen
            if screenshot:
                ss = screenshot.encode('UTF-8')
                autorun.screenshot[:len(ss) + 1] = ss
                logger.debug(f"Autorun screenshot: {ss}")
                autorun.video[0] = 0
            if video:
                sv = video.encode('UTF-8')
                autorun.video[:len(sv) + 1] = sv
                autorun.screenshot[0] = 0
                logger.debug(f"Autorun video: {sv}")
            cv.dvz_autorun_setup(self._c_app, autorun)

        cv.dvz_app_run(self._c_app, n_frames)

    def next_frame(self):
        """Run a single frame for all canvases."""
        return cv.dvz_app_run(self._c_app, 1)

    def _set_running(self, bint running):
        """Manually set whether the app is running or not."""
        self._c_app.is_running = running

    def __repr__(self):
        return "<Datoviz App>"



# -------------------------------------------------------------------------------------------------
# GPU
# -------------------------------------------------------------------------------------------------

cdef class GPU:
    """The GPU object allows to create GPU objects and canvases."""

    cdef cv.DvzApp* _c_app
    cdef cv.DvzGpu* _c_gpu

    cdef object _context
    # _canvases = []

    cdef create(self, cv.DvzApp* c_app, int idx):
        """Create a GPU."""
        assert c_app is not NULL
        self._c_app = c_app
        self._c_gpu = cv.dvz_gpu(self._c_app, idx);
        if self._c_gpu is NULL:
            raise MemoryError()

    cdef create_best(self, cv.DvzApp* c_app):
        """Create the best GPU found."""
        assert c_app is not NULL
        self._c_app = c_app
        self._c_gpu = cv.dvz_gpu_best(self._c_app);
        if self._c_gpu is NULL:
            raise MemoryError()

    @property
    def name(self):
        return self._c_gpu.name

    def canvas(
            self,
            int width=DEFAULT_WIDTH,
            int height=DEFAULT_HEIGHT,
            bint show_fps=False,
            bint pick=False,
            bint high_dpi=False,
            bint offscreen=False,
            clear_color=None,
        ):
        """Create a new canvas."""

        # Canvas flags.
        cdef int flags = 0
        flags = _canvas_flags(show_fps=show_fps, pick=pick, high_dpi=high_dpi, offscreen=offscreen)

        # Create the canvas using the Datoviz C API.
        c_canvas = cv.dvz_canvas(self._c_gpu, width, height, flags)

        # Canvas clear color.
        if clear_color == 'white':
            cv.dvz_canvas_clear_color(c_canvas, 1, 1, 1)

        if c_canvas is NULL:
            raise MemoryError()

        # Create and return the Canvas Cython wrapper.
        c = Canvas()
        c.create(self, c_canvas)
        # self._canvases.append(c)
        return c

    def context(self):
        """Return the GPU context object, used to create GPU buffers and textures."""
        if self._context is not None:
            return self._context
        c = Context()

        assert self._c_gpu is not NULL

        # If the context has not been created, it means we must create the GPU in offscreen mode.
        if self._c_gpu.context is NULL:
            logger.debug("Automatically creating a GPU context with no surface (offscreen only)")
            # Create the GPU without a surface.
            cv.dvz_gpu_default(self._c_gpu, NULL)
            # Create the context.
            cv.dvz_context(self._c_gpu)
        c.create(self._c_app, self._c_gpu, self._c_gpu.context)

        self._context = c
        return c

    def __repr__(self):
        return f"<GPU \"{self.name}\">"



# -------------------------------------------------------------------------------------------------
# Context
# -------------------------------------------------------------------------------------------------

cdef class Context:
    """A Context is attached to a GPU and allows to create GPU buffers and textures."""
    cdef cv.DvzApp* _c_app
    cdef cv.DvzGpu* _c_gpu
    cdef cv.DvzContext* _c_context

    cdef create(self, cv.DvzApp* c_app, cv.DvzGpu* c_gpu, cv.DvzContext* c_context):
        assert c_app is not NULL
        assert c_gpu is not NULL
        assert c_context is not NULL

        self._c_app = c_app
        self._c_gpu = c_gpu
        self._c_context = c_context

    def texture(
            self, int height, int width=1, int depth=1,
            int ncomp=4, np.dtype dtype=None, int ndim=2):
        """Create a 1D, 2D, or 3D texture."""

        dtype = np.dtype(dtype or np.uint8)

        tex = Texture()

        # Texture shape.
        assert width > 0
        assert height > 0
        assert depth > 0
        if depth > 1:
            ndim = 3
        cdef TEX_SHAPE shape
        # NOTE: shape is in Vulkan convention.
        shape[0] = width
        shape[1] = height
        shape[2] = depth

        # Create the texture.
        tex.create(self._c_context, ndim, ncomp, shape, dtype)
        logging.debug(f"Create a {str(tex)}")
        return tex

    def colormap(self, unicode name, np.ndarray[CHAR, ndim=2] colors):
        """Create a custom colormap"""
        assert colors.shape[1] == 4
        color_count = colors.shape[0]
        assert color_count > 0
        assert color_count <= 256
        colors = colors.astype(np.uint8)
        if not colors.flags['C_CONTIGUOUS']:
            colors = np.ascontiguousarray(colors)

        # TODO: use constant CMAP_CUSTOM instead of hard-coded value
        cmap = 160 + len(_CUSTOM_COLORMAPS)
        _CUSTOM_COLORMAPS[name] = cmap
        cv.dvz_colormap_custom(cmap, color_count, <cv.cvec4*>&colors.data[0])
        cv.dvz_context_colormap(self._c_context)

    def __repr__(self):
        return f"<Context for GPU \"{self._c_gpu.name}\">"



# -------------------------------------------------------------------------------------------------
# Texture
# -------------------------------------------------------------------------------------------------

cdef class Texture:
    """A 1D, 2D, or 3D GPU texture."""

    cdef cv.DvzContext* _c_context
    cdef cv.DvzTexture* _c_texture

    cdef TEX_SHAPE _c_shape # always 3 values: width, height, depth (WARNING, reversed in NumPy)
    cdef np.dtype dtype
    cdef int ndim  # 1D, 2D, or 3D texture
    cdef int ncomp  # 1-4 (eg 3 for RGB, 4 for RGBA)

    cdef cv.DvzSourceType _c_source_type

    cdef create(self, cv.DvzContext* c_context, int ndim, int ncomp, TEX_SHAPE shape, np.dtype dtype):
        """Create a texture."""
        assert c_context is not NULL
        self._c_context = c_context

        assert 1 <= ndim <= 3
        assert 1 <= ncomp <= 4
        self.ndim = ndim
        self.ncomp = ncomp

        # Store the shape.
        for i in range(ndim):
            self._c_shape[i] = shape[i]
        for i in range(ndim, 3):
            self._c_shape[i] = 1

        # Find the source type.
        assert ndim in _SOURCE_TYPES
        self._c_source_type = _SOURCE_TYPES[ndim]

        # Find the Vulkan format.
        cdef cv.VkFormat c_format
        self.dtype = dtype
        assert (dtype, ncomp) in _FORMATS
        c_format = _FORMATS[dtype, ncomp]

        # Create the Datoviz texture.
        self._c_texture = cv.dvz_ctx_texture(self._c_context, ndim, &shape[0], c_format)

    @property
    def item_size(self):
        """Size, in bytes, of every value."""
        return np.dtype(self.dtype).itemsize

    @property
    def size(self):
        """Total number of values in the texture (including the number of color components)."""
        return np.prod(self.shape)

    @property
    def shape(self):
        """Shape (NumPy convention: height, width, depth).
        Also, the last dimension is the number of color components."""
        shape = [1, 1, 1]
        for i in range(3):
            shape[i] = self._c_shape[i]
        if self.ndim > 1:
            # NOTE: Vulkan considers textures as (width, height, depth) whereas NumPy
            # considers them as (height, width, depth), hence the need to transpose here.
            shape[0], shape[1] = shape[1], shape[0]
        return tuple(shape[:self.ndim]) + (self.ncomp,)

    def set_filter(self, name):
        """Change the filtering of the texture."""
        cv.dvz_texture_filter(self._c_texture, cv.DVZ_FILTER_MIN, _TEXTURE_FILTERS[name])
        cv.dvz_texture_filter(self._c_texture, cv.DVZ_FILTER_MAG, _TEXTURE_FILTERS[name])

    def upload(self, np.ndarray arr):
        """Set the texture data from a NumPy array."""
        assert arr.dtype == self.dtype
        for i in range(self.ndim):
            assert arr.shape[i] == self.shape[i]
        logger.debug(f"Upload NumPy array to {self}.")
        cv.dvz_upload_texture(
            self._c_context, self._c_texture, &DVZ_ZERO_OFFSET[0], &DVZ_ZERO_OFFSET[0],
            self.size * self.item_size, &arr.data[0])

    def download(self):
        """Download the texture data to a NumPy array."""
        cdef np.ndarray arr
        arr = np.empty(self.shape, dtype=self.dtype)
        logger.debug(f"Download {self}.")
        cv.dvz_download_texture(
            self._c_context, self._c_texture, &DVZ_ZERO_OFFSET[0], &DVZ_ZERO_OFFSET[0],
            self.size * self.item_size, &arr.data[0])
        cv.dvz_process_transfers(self._c_context)
        return arr

    def __repr__(self):
        """The shape axes are in the following order: height, width, depth, ncomp."""
        return f"<Texture {self.ndim}D {'x'.join(map(str, self.shape))} ({self.dtype})>"



# -------------------------------------------------------------------------------------------------
# Canvas
# -------------------------------------------------------------------------------------------------

cdef class Canvas:
    """A canvas."""

    cdef cv.DvzCanvas* _c_canvas
    cdef object _gpu
    cdef bint _video_recording
    cdef object _scene

    cdef create(self, gpu, cv.DvzCanvas* c_canvas):
        """Create a canvas."""
        self._c_canvas = c_canvas
        self._gpu = gpu
        self._scene = None

    def gpu(self):
        return self._gpu

    def scene(self, rows=1, cols=1):
        """Create a scene, which allows to use subplots, controllers, visuals, and so on."""
        if self._scene is not None:
            logger.debug("reusing existing Scene object, discarding rows and cols")
            return self._scene
        s = Scene()
        s.create(self, self._c_canvas, rows, cols)
        return s

    def screenshot(self, unicode path):
        """Make a screenshot and save it to a PNG file."""
        cdef char* _c_path = path
        cv.dvz_screenshot_file(self._c_canvas, _c_path);

    def video(self, unicode path):
        """Start a high-quality video recording."""
        # TODO: customizable video parameters.
        cdef char* _c_path = path
        cv.dvz_canvas_video(self._c_canvas, 30, 10000000, _c_path, False)
        self._video_recording = True

    def pause(self):
        """Pause the video recording."""
        self._video_recording = not self._video_recording
        cv.dvz_canvas_pause(self._c_canvas, self._video_recording)

    def stop(self):
        """Stop the video recording and save the video to disk."""
        cv.dvz_canvas_stop(self._c_canvas)

    def pick(self, cv.uint32_t x, cv.uint32_t y):
        """If the canvas was created with picking support, get the color value at a given pixel."""
        cdef cv.uvec2 xy
        cdef cv.ivec4 rgba
        xy[0] = x
        xy[1] = y
        cv.dvz_canvas_pick(self._c_canvas, xy, rgba)

        cdef cv.int32_t r, g, b, a
        r = rgba[0]
        g = rgba[1]
        b = rgba[2]
        a = rgba[3]

        return (r, g, b, a)

    def gui(self, unicode title):
        """Create a new GUI."""
        c_gui = cv.dvz_gui(self._c_canvas, title, 0)
        gui = Gui()
        gui.create(self._c_canvas, c_gui)
        return gui

    def gui_demo(self):
        """Show the Dear ImGui demo."""
        cv.dvz_imgui_demo(self._c_canvas)

    def close(self):
        if self._c_canvas is not NULL:
            cv.dvz_canvas_to_close(self._c_canvas)
            cv.dvz_app_run(self._c_canvas.app, 1)
            self._c_canvas = NULL

    def _connect(self, evtype_py, f, param=0, cv.DvzEventMode mode=cv.DVZ_EVENT_MODE_SYNC):
        # NOTE: only SYNC callbacks for now.
        cdef cv.DvzEventType evtype
        evtype = _EVENTS.get(evtype_py, 0)
        _add_event_callback(self._c_canvas, evtype, param, f, (), mode=mode)

    def connect(self, f):
        """Add an event callback function."""
        assert f.__name__.startswith('on_')
        ev_name = f.__name__[3:]
        self._connect(ev_name, f)

    def click(self, float x, float y, button='left', modifiers=()):
        """Simulate a mouse click at a given position."""
        cdef cv.vec2 pos
        cdef int mod
        cdef cv.DvzMouseButton c_button

        pos[0] = x
        pos[1] = y
        c_button = _BUTTONS_INV.get(button, 0)
        mod = _c_modifiers(*modifiers)
        cv.dvz_event_mouse_click(self._c_canvas, pos, c_button, mod)



# -------------------------------------------------------------------------------------------------
# Scene
# -------------------------------------------------------------------------------------------------

cdef class Scene:
    """The Scene is attached to a canvas, and provides high-level scientific
    plotting facilities."""

    cdef cv.DvzCanvas* _c_canvas
    cdef cv.DvzScene* _c_scene
    cdef cv.DvzGrid* _c_grid
    cdef object _canvas

    _panels = []

    cdef create(self, canvas, cv.DvzCanvas* c_canvas, int rows, int cols):
        """Create the scene."""
        self._canvas = canvas
        self._c_canvas = c_canvas
        self._c_scene = cv.dvz_scene(c_canvas, rows, cols)
        self._c_grid = &self._c_scene.grid

    def destroy(self):
        """Destroy the scene."""
        if self._c_scene is not NULL:
            cv.dvz_scene_destroy(self._c_scene)
            self._c_scene = NULL

    def panel(self, int row=0, int col=0, controller='axes', transform=None, transpose=None, **kwargs):
        """Add a new panel with a controller."""
        cdef int flags
        flags = 0
        if controller == 'axes':
            if kwargs.pop('hide_minor_ticks', False):
                flags |= cv.DVZ_AXES_FLAGS_HIDE_MINOR
            if kwargs.pop('hide_grid', False):
                flags |= cv.DVZ_AXES_FLAGS_HIDE_GRID

        ctl = _CONTROLLERS.get(controller, cv.DVZ_CONTROLLER_NONE)
        trans = _TRANSPOSES.get(transpose, cv.DVZ_CDS_TRANSPOSE_NONE)
        transf = _TRANSFORMS.get(transform, cv.DVZ_TRANSFORM_CARTESIAN)
        c_panel = cv.dvz_scene_panel(self._c_scene, row, col, ctl, flags)
        if c_panel is NULL:
            raise MemoryError()
        c_panel.data_coords.transform = transf
        cv.dvz_panel_transpose(c_panel, trans)
        p = Panel()
        p.create(self._c_scene, c_panel)
        self._panels.append(p)
        return p

    def panel_at(self, x, y):
        """Find the panel at a given pixel position."""
        cdef cv.vec2 pos
        pos[0] = x
        pos[1] = y
        c_panel = cv.dvz_panel_at(self._c_grid, pos)

        cdef Panel panel
        for p in self._panels:
            panel = p
            if panel._c_panel == c_panel:
                return panel



# -------------------------------------------------------------------------------------------------
# Panel
# -------------------------------------------------------------------------------------------------

cdef class Panel:
    """The Panel is a subplot in the Scene."""

    cdef cv.DvzScene* _c_scene
    cdef cv.DvzPanel* _c_panel

    _visuals = []

    cdef create(self, cv.DvzScene* c_scene, cv.DvzPanel* c_panel):
        """Create the panel."""
        self._c_panel = c_panel
        self._c_scene = c_scene

    @property
    def row(self):
        """Get the panel's row index."""
        return self._c_panel.row

    @property
    def col(self):
        """Get the panel's column index."""
        return self._c_panel.col

    def visual(self, vtype, depth_test=None, transform='auto'):
        """Add a visual to the panel."""
        visual_type = _VISUALS.get(vtype, 0)
        if not visual_type:
            raise ValueError("unknown visual type")
        flags = 0
        if depth_test:
            flags |= cv.DVZ_GRAPHICS_FLAGS_DEPTH_TEST
        if transform is None:
            flags |= cv.DVZ_VISUAL_FLAGS_TRANSFORM_NONE
        # This keyword means that the panel box will NOT be recomputed every time the POS prop
        # changes
        elif transform == 'init':
            flags |= cv.DVZ_VISUAL_FLAGS_TRANSFORM_BOX_INIT
        c_visual = cv.dvz_scene_visual(self._c_panel, visual_type, flags)
        if c_visual is NULL:
            raise MemoryError()
        v = Visual()
        v.create(self._c_panel, c_visual, vtype)
        self._visuals.append(v)
        return v

    def pick(self, x, y, target_cds='data'):
        """Convert a position in pixels to the data coordinate system, or another
        coordinate system."""
        cdef cv.dvec3 pos_in
        cdef cv.dvec3 pos_out
        pos_in[0] = x
        pos_in[1] = y
        pos_in[2] = 0
        source = cv.DVZ_CDS_WINDOW
        target = _COORDINATE_SYSTEMS[target_cds]
        cv.dvz_transform(self._c_panel, source, pos_in, target, pos_out)
        return pos_out[0], pos_out[1]



# -------------------------------------------------------------------------------------------------
# Visual
# -------------------------------------------------------------------------------------------------

cdef class Visual:
    """A visual is added to a given panel."""
    cdef cv.DvzPanel* _c_panel
    cdef cv.DvzVisual* _c_visual
    cdef cv.DvzContext* _c_context
    cdef unicode vtype
    _textures = {}

    cdef create(self, cv.DvzPanel* c_panel, cv.DvzVisual* c_visual, unicode vtype):
        """Create a visual."""
        self._c_panel = c_panel
        self._c_visual = c_visual
        self._c_context = c_visual.canvas.gpu.context
        self.vtype = vtype

    def data(self, name, np.ndarray value, idx=0, mode=None, range=None):
        """Set the data of the visual associated to a given property."""
        prop_type = _get_prop(name)
        c_prop = cv.dvz_prop_get(self._c_visual, prop_type, idx)
        dtype, nc = _DTYPES[c_prop.dtype]
        value = _validate_data(dtype, nc, value)
        N = value.shape[0]
        if mode == 'append':
            cv.dvz_visual_data_append(self._c_visual, prop_type, idx, N, &value.data[0])
        elif mode == 'partial' and range is not None:
            first_item, n_items = range
            assert first_item >= 0, "first item should be positive"
            assert n_items > 0, "n_items should be strictly positive"
            cv.dvz_visual_data_partial(
                self._c_visual, prop_type, idx, first_item, n_items, N, &value.data[0])
        else:
            cv.dvz_visual_data(self._c_visual, prop_type, idx, N, &value.data[0])

    def append(self, *args, **kwargs):
        """Add some data to a visual prop's data."""
        return self.data(*args, **kwargs, mode='append')

    def partial(self, *args, **kwargs):
        """Make a partial data update."""
        return self.data(*args, **kwargs, mode='partial')

    def texture(self, Texture tex, idx=0):
        """Attach a texture to a visual."""
        # Bind the texture with the visual for the specified source.
        cv.dvz_visual_texture(
            self._c_visual, tex._c_source_type, idx, tex._c_texture)

    def load_obj(self, unicode path, compute_normals=False):
        """Load a mesh from an OBJ file."""
        # TODO: move to subclass Mesh?

        cdef cv.DvzMesh mesh = cv.dvz_mesh_obj(path);

        if compute_normals:
            print("computing normals")
            cv.dvz_mesh_normals(&mesh)

        nv = mesh.vertices.item_count;
        ni = mesh.indices.item_count;

        cv.dvz_visual_data_source(self._c_visual, cv.DVZ_SOURCE_TYPE_VERTEX, 0, 0, nv, nv, mesh.vertices.data);
        cv.dvz_visual_data_source(self._c_visual, cv.DVZ_SOURCE_TYPE_INDEX, 0, 0, ni, ni, mesh.indices.data);



# -------------------------------------------------------------------------------------------------
# GUI
# -------------------------------------------------------------------------------------------------

cdef class GuiControl:
    """A GUI control."""
    cdef cv.DvzGui* _c_gui
    cdef cv.DvzCanvas* _c_canvas
    cdef cv.DvzGuiControl* _c_control
    cdef unicode name
    cdef unicode ctype
    cdef bytes str_ascii
    cdef object _callback

    cdef create(self, cv.DvzGui* c_gui, cv.DvzGuiControl* c_control, unicode name, unicode ctype):
        """Create a GUI control."""
        self._c_gui = c_gui
        self._c_canvas = c_gui.canvas
        assert self._c_canvas is not NULL
        self._c_control = c_control
        self.ctype = ctype
        self.name = name

    def get(self):
        """Get the current value."""
        cdef void* ptr
        ptr = cv.dvz_gui_value(self._c_control)
        if self.ctype == 'input_float' or self.ctype == 'slider_float':
            return (<float*>ptr)[0]

    def set(self, obj):
        """Set the control's value."""
        cdef void* ptr
        cdef char* c_str
        ptr = cv.dvz_gui_value(self._c_control)
        if self.ctype == 'input_float' or self.ctype == 'slider_float':
            (<float*>ptr)[0] = <float>float(obj)
        elif self.ctype == 'label':
            self.str_ascii = obj.encode('ascii')
            if len(self.str_ascii) >= 1024:
                self.str_ascii = self.str_ascii[:1024]
            c_str = self.str_ascii
            # HACK: +1 for string null termination
            memcpy(ptr, c_str, len(self.str_ascii) + 1)
        else:
            raise NotImplementedError(
                f"Setting the value for a GUI control `{self.ctype}` is not implemented yet.")

    def connect(self, f):
        """Bind a callback function to the control."""
        self._callback = f
        _add_event_callback(self._c_canvas, cv.DVZ_EVENT_GUI, 0, f, (self.name,))

    def press(self):
        """For buttons only: simulate a press."""
        if self.ctype == 'button' and self._callback:
            self._callback(False)

    @property
    def pos(self):
        """The x, y coordinates of the widget, in screen coordinates."""
        cdef float x, y
        x = self._c_control.pos[0]
        y = self._c_control.pos[1]
        return (x, y)

    @property
    def size(self):
        """The width and height of the widget, in screen coordinates."""
        cdef float w, h
        w = self._c_control.size[0]
        h = self._c_control.size[1]
        return (w, h)



cdef class Gui:
    """A GUI dialog."""
    cdef cv.DvzCanvas* _c_canvas
    cdef cv.DvzGui* _c_gui

    cdef create(self, cv.DvzCanvas* c_canvas, cv.DvzGui* c_gui):
        """Create a GUI."""
        self._c_canvas = c_canvas
        self._c_gui = c_gui

    def control(self, unicode ctype, unicode name, **kwargs):
        """Add a GUI control."""
        ctrl = _CONTROLS.get(ctype, 0)
        cdef char* c_name = name
        cdef cv.DvzGuiControl* c
        cdef cv.vec2 vec2_value

        if (ctype == 'slider_float'):
            c_vmin = kwargs.get('vmin', 0)
            c_vmax = kwargs.get('vmax', 1)
            c_value = kwargs.get('value', (c_vmin + c_vmax) / 2.0)
            c = cv.dvz_gui_slider_float(self._c_gui, c_name, c_vmin, c_vmax, c_value)
        elif (ctype == 'slider_float2'):
            c_vmin = kwargs.get('vmin', 0)
            c_vmax = kwargs.get('vmax', 1)
            c_value = kwargs.get('value', (c_vmin, c_vmax))
            c_force = kwargs.get('force_increasing', False)
            vec2_value[0] = c_value[0]
            vec2_value[1] = c_value[1]
            c = cv.dvz_gui_slider_float2(self._c_gui, c_name, c_vmin, c_vmax, vec2_value, c_force)
        elif (ctype == 'slider_int'):
            c_vmin = kwargs.get('vmin', 0)
            c_vmax = kwargs.get('vmax', 1)
            c_value = kwargs.get('value', c_vmin)
            c = cv.dvz_gui_slider_int(self._c_gui, c_name, c_vmin, c_vmax, c_value)
        elif (ctype == 'input_float'):
            c_step = kwargs.get('step', .1)
            c_step_fast = kwargs.get('step_fast', 1)
            c_value = kwargs.get('value', 0)
            c = cv.dvz_gui_input_float(self._c_gui, c_name, c_step, c_step_fast, c_value)
        elif (ctype == 'checkbox'):
            c_value = kwargs.get('value', 0)
            c = cv.dvz_gui_checkbox(self._c_gui, c_name, c_value)
        elif (ctype == 'button'):
            c = cv.dvz_gui_button(self._c_gui, c_name, 0)
        elif (ctype == 'label'):
            c_value = kwargs.get('value', "")
            c = cv.dvz_gui_label(self._c_gui, c_name, c_value)

        # Gui control object
        w = GuiControl()
        w.create(self._c_gui, c, name, ctype)
        return w
