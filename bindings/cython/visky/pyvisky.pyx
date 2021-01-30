# cython: c_string_type=unicode, c_string_encoding=ascii

from functools import wraps, partial
import logging

cimport numpy as np
import numpy as np
from cpython.ref cimport Py_INCREF

cimport visky.cyvisky as cv


logger = logging.getLogger(__name__)


DEFAULT_WIDTH = 1024
DEFAULT_HEIGHT = 768


# TODO: add more keys
_KEYS = {
    # cv.VKL_KEY_LEFT: 'left',
    # cv.VKL_KEY_RIGHT: 'right',
    # cv.VKL_KEY_UP: 'up',
    # cv.VKL_KEY_DOWN: 'down',
    # cv.VKL_KEY_HOME: 'home',
    # cv.VKL_KEY_END: 'end',
    # cv.VKL_KEY_KP_ADD: '+',
    # cv.VKL_KEY_KP_SUBTRACT: '-',
    # cv.VKL_KEY_G: 'g',
}

# HACK: these keys do not raise a Python key event
_EXCLUDED_KEYS = (
    # cv.VKL_KEY_NONE,
    # cv.VKL_KEY_LEFT_SHIFT,
    # cv.VKL_KEY_LEFT_CONTROL,
    # cv.VKL_KEY_LEFT_ALT,
    # cv.VKL_KEY_LEFT_SUPER,
    # cv.VKL_KEY_RIGHT_SHIFT,
    # cv.VKL_KEY_RIGHT_CONTROL,
    # cv.VKL_KEY_RIGHT_ALT,
    # cv.VKL_KEY_RIGHT_SUPER,
)

_BUTTONS = {
    # cv.VKL_MOUSE_BUTTON_LEFT: 'left',
    # cv.VKL_MOUSE_BUTTON_MIDDLE: 'middle',
    # cv.VKL_MOUSE_BUTTON_RIGHT: 'right',
}

_MOUSE_STATES = {
    # cv.VKL_MOUSE_STATE_DRAG: 'drag',
    # cv.VKL_MOUSE_STATE_WHEEL: 'wheel',
    # cv.VKL_MOUSE_STATE_CLICK: 'click',
    # cv.VKL_MOUSE_STATE_DOUBLE_CLICK: 'double_click',
}

_VISUALS = {
    'point': cv.VKL_VISUAL_POINT,
    'marker': cv.VKL_VISUAL_MARKER,
    'mesh': cv.VKL_VISUAL_MESH,
    'volume_slice': cv.VKL_VISUAL_VOLUME_SLICE,
}

_CONTROLLERS = {
    'panzoom': cv.VKL_CONTROLLER_PANZOOM,
    'axes': cv.VKL_CONTROLLER_AXES_2D,
    'arcball': cv.VKL_CONTROLLER_ARCBALL,
    'camera': cv.VKL_CONTROLLER_CAMERA,
}

_TRANSPOSES = {
    None: cv.VKL_CDS_TRANSPOSE_NONE,
    'xfyrzu': cv.VKL_CDS_TRANSPOSE_XFYRZU,
    'xbydzl': cv.VKL_CDS_TRANSPOSE_XBYDZL,
    'xlybzd': cv.VKL_CDS_TRANSPOSE_XLYBZD,
}

_PROPS = {
    'pos': cv.VKL_PROP_POS,
    'color': cv.VKL_PROP_COLOR,
    'alpha': cv.VKL_PROP_ALPHA,
    'ms': cv.VKL_PROP_MARKER_SIZE,
    'normal': cv.VKL_PROP_NORMAL,
    'texcoords': cv.VKL_PROP_TEXCOORDS,
    'index': cv.VKL_PROP_INDEX,
    'light_params': cv.VKL_PROP_LIGHT_PARAMS,
    'light_pos': cv.VKL_PROP_LIGHT_POS,
    'texcoefs': cv.VKL_PROP_TEXCOEFS,
    'linewidth': cv.VKL_PROP_LINE_WIDTH,
    'view_pos': cv.VKL_PROP_VIEW_POS,
    'colormap': cv.VKL_PROP_COLORMAP,
    'transferx': cv.VKL_PROP_TRANSFER_X,
    'transfery': cv.VKL_PROP_TRANSFER_Y,
    'clip': cv.VKL_PROP_CLIP,
}

_COLORMAPS = {
    'binary': cv.VKL_CMAP_BINARY,
    'hsv': cv.VKL_CMAP_HSV,
    'cividis': cv.VKL_CMAP_CIVIDIS,
    'inferno': cv.VKL_CMAP_INFERNO,
    'magma': cv.VKL_CMAP_MAGMA,
    'plasma': cv.VKL_CMAP_PLASMA,
    'viridis': cv.VKL_CMAP_VIRIDIS,
    'blues': cv.VKL_CMAP_BLUES,
    'bugn': cv.VKL_CMAP_BUGN,
    'bupu': cv.VKL_CMAP_BUPU,
    'gnbu': cv.VKL_CMAP_GNBU,
    'greens': cv.VKL_CMAP_GREENS,
    'greys': cv.VKL_CMAP_GREYS,
    'oranges': cv.VKL_CMAP_ORANGES,
    'orrd': cv.VKL_CMAP_ORRD,
    'pubu': cv.VKL_CMAP_PUBU,
    'pubugn': cv.VKL_CMAP_PUBUGN,
    'purples': cv.VKL_CMAP_PURPLES,
    'rdpu': cv.VKL_CMAP_RDPU,
    'reds': cv.VKL_CMAP_REDS,
    'ylgn': cv.VKL_CMAP_YLGN,
    'ylgnbu': cv.VKL_CMAP_YLGNBU,
    'ylorbr': cv.VKL_CMAP_YLORBR,
    'ylorrd': cv.VKL_CMAP_YLORRD,
    'afmhot': cv.VKL_CMAP_AFMHOT,
    'autumn': cv.VKL_CMAP_AUTUMN,
    'bone': cv.VKL_CMAP_BONE,
    'cool': cv.VKL_CMAP_COOL,
    'copper': cv.VKL_CMAP_COPPER,
    'gist_heat': cv.VKL_CMAP_GIST_HEAT,
    'gray': cv.VKL_CMAP_GRAY,
    'hot': cv.VKL_CMAP_HOT,
    'pink': cv.VKL_CMAP_PINK,
    'spring': cv.VKL_CMAP_SPRING,
    'summer': cv.VKL_CMAP_SUMMER,
    'winter': cv.VKL_CMAP_WINTER,
    'wistia': cv.VKL_CMAP_WISTIA,
    'brbg': cv.VKL_CMAP_BRBG,
    'bwr': cv.VKL_CMAP_BWR,
    'coolwarm': cv.VKL_CMAP_COOLWARM,
    'piyg': cv.VKL_CMAP_PIYG,
    'prgn': cv.VKL_CMAP_PRGN,
    'puor': cv.VKL_CMAP_PUOR,
    'rdbu': cv.VKL_CMAP_RDBU,
    'rdgy': cv.VKL_CMAP_RDGY,
    'rdylbu': cv.VKL_CMAP_RDYLBU,
    'rdylgn': cv.VKL_CMAP_RDYLGN,
    'seismic': cv.VKL_CMAP_SEISMIC,
    'spectral': cv.VKL_CMAP_SPECTRAL,
    'twilight_shifted': cv.VKL_CMAP_TWILIGHT_SHIFTED,
    'twilight': cv.VKL_CMAP_TWILIGHT,
    'brg': cv.VKL_CMAP_BRG,
    'cmrmap': cv.VKL_CMAP_CMRMAP,
    'cubehelix': cv.VKL_CMAP_CUBEHELIX,
    'flag': cv.VKL_CMAP_FLAG,
    'gist_earth': cv.VKL_CMAP_GIST_EARTH,
    'gist_ncar': cv.VKL_CMAP_GIST_NCAR,
    'gist_rainbow': cv.VKL_CMAP_GIST_RAINBOW,
    'gist_stern': cv.VKL_CMAP_GIST_STERN,
    'gnuplot2': cv.VKL_CMAP_GNUPLOT2,
    'gnuplot': cv.VKL_CMAP_GNUPLOT,
    'jet': cv.VKL_CMAP_JET,
    'nipy_spectral': cv.VKL_CMAP_NIPY_SPECTRAL,
    'ocean': cv.VKL_CMAP_OCEAN,
    'prism': cv.VKL_CMAP_PRISM,
    'rainbow': cv.VKL_CMAP_RAINBOW,
    'terrain': cv.VKL_CMAP_TERRAIN,
    'bkr': cv.VKL_CMAP_BKR,
    'bky': cv.VKL_CMAP_BKY,
    'cet_d10': cv.VKL_CMAP_CET_D10,
    'cet_d11': cv.VKL_CMAP_CET_D11,
    'cet_d8': cv.VKL_CMAP_CET_D8,
    'cet_d13': cv.VKL_CMAP_CET_D13,
    'cet_d3': cv.VKL_CMAP_CET_D3,
    'cet_d1a': cv.VKL_CMAP_CET_D1A,
    'bjy': cv.VKL_CMAP_BJY,
    'gwv': cv.VKL_CMAP_GWV,
    'bwy': cv.VKL_CMAP_BWY,
    'cet_d12': cv.VKL_CMAP_CET_D12,
    'cet_r3': cv.VKL_CMAP_CET_R3,
    'cet_d9': cv.VKL_CMAP_CET_D9,
    'cwr': cv.VKL_CMAP_CWR,
    'cet_cbc1': cv.VKL_CMAP_CET_CBC1,
    'cet_cbc2': cv.VKL_CMAP_CET_CBC2,
    'cet_cbl1': cv.VKL_CMAP_CET_CBL1,
    'cet_cbl2': cv.VKL_CMAP_CET_CBL2,
    'cet_cbtc1': cv.VKL_CMAP_CET_CBTC1,
    'cet_cbtc2': cv.VKL_CMAP_CET_CBTC2,
    'cet_cbtl1': cv.VKL_CMAP_CET_CBTL1,
    'bgy': cv.VKL_CMAP_BGY,
    'bgyw': cv.VKL_CMAP_BGYW,
    'bmw': cv.VKL_CMAP_BMW,
    'cet_c1': cv.VKL_CMAP_CET_C1,
    'cet_c1s': cv.VKL_CMAP_CET_C1S,
    'cet_c2': cv.VKL_CMAP_CET_C2,
    'cet_c4': cv.VKL_CMAP_CET_C4,
    'cet_c4s': cv.VKL_CMAP_CET_C4S,
    'cet_c5': cv.VKL_CMAP_CET_C5,
    'cet_i1': cv.VKL_CMAP_CET_I1,
    'cet_i3': cv.VKL_CMAP_CET_I3,
    'cet_l10': cv.VKL_CMAP_CET_L10,
    'cet_l11': cv.VKL_CMAP_CET_L11,
    'cet_l12': cv.VKL_CMAP_CET_L12,
    'cet_l16': cv.VKL_CMAP_CET_L16,
    'cet_l17': cv.VKL_CMAP_CET_L17,
    'cet_l18': cv.VKL_CMAP_CET_L18,
    'cet_l19': cv.VKL_CMAP_CET_L19,
    'cet_l4': cv.VKL_CMAP_CET_L4,
    'cet_l7': cv.VKL_CMAP_CET_L7,
    'cet_l8': cv.VKL_CMAP_CET_L8,
    'cet_l9': cv.VKL_CMAP_CET_L9,
    'cet_r1': cv.VKL_CMAP_CET_R1,
    'cet_r2': cv.VKL_CMAP_CET_R2,
    'colorwheel': cv.VKL_CMAP_COLORWHEEL,
    'fire': cv.VKL_CMAP_FIRE,
    'isolum': cv.VKL_CMAP_ISOLUM,
    'kb': cv.VKL_CMAP_KB,
    'kbc': cv.VKL_CMAP_KBC,
    'kg': cv.VKL_CMAP_KG,
    'kgy': cv.VKL_CMAP_KGY,
    'kr': cv.VKL_CMAP_KR,
    'black_body': cv.VKL_CMAP_BLACK_BODY,
    'kindlmann': cv.VKL_CMAP_KINDLMANN,
    'extended_kindlmann': cv.VKL_CMAP_EXTENDED_KINDLMANN,
    'glasbey': cv.VKL_CPAL256_GLASBEY,
    'glasbey_cool': cv.VKL_CPAL256_GLASBEY_COOL,
    'glasbey_dark': cv.VKL_CPAL256_GLASBEY_DARK,
    'glasbey_hv': cv.VKL_CPAL256_GLASBEY_HV,
    'glasbey_light': cv.VKL_CPAL256_GLASBEY_LIGHT,
    'glasbey_warm': cv.VKL_CPAL256_GLASBEY_WARM,
    'accent': cv.VKL_CPAL032_ACCENT,
    'dark2': cv.VKL_CPAL032_DARK2,
    'paired': cv.VKL_CPAL032_PAIRED,
    'pastel1': cv.VKL_CPAL032_PASTEL1,
    'pastel2': cv.VKL_CPAL032_PASTEL2,
    'set1': cv.VKL_CPAL032_SET1,
    'set2': cv.VKL_CPAL032_SET2,
    'set3': cv.VKL_CPAL032_SET3,
    'tab10': cv.VKL_CPAL032_TAB10,
    'tab20': cv.VKL_CPAL032_TAB20,
    'tab20b': cv.VKL_CPAL032_TAB20B,
    'tab20c': cv.VKL_CPAL032_TAB20C,
    'category10_10': cv.VKL_CPAL032_CATEGORY10_10,
    'category20_20': cv.VKL_CPAL032_CATEGORY20_20,
    'category20b_20': cv.VKL_CPAL032_CATEGORY20B_20,
    'category20c_20': cv.VKL_CPAL032_CATEGORY20C_20,
    'colorblind8': cv.VKL_CPAL032_COLORBLIND8,
}

_PROP_DTYPES = {
    'pos': np.double,
    'color': np.uint8,
    'alpha': np.uint8,
    'index': np.uint32,
    'colormap': np.uint8,
}

_EVENTS ={
    'mouse': cv.VKL_EVENT_MOUSE_MOVE,
    'frame': cv.VKL_EVENT_FRAME,
    'timer': cv.VKL_EVENT_TIMER,
}



def _key_name(key):
    return _KEYS.get(key, key)

def _button_name(button):
    return _BUTTONS.get(button, None)

def _mouse_state(state):
    return _MOUSE_STATES.get(state, None)

def _get_prop(name):
    return _PROPS[name]


ctypedef np.double_t DOUBLE

def colormap(np.ndarray[DOUBLE, ndim=1] values, vmin=None, vmax=None, cmap=None):
    N = values.size
    cmap_ = _COLORMAPS.get(cmap, cv.VKL_CMAP_VIRIDIS)
    # TODO: ndarrays
    cdef np.ndarray out = np.zeros((N, 4), dtype=np.uint8)
    if vmin is None:
        vmin = values.min()
    if vmax is None:
        vmax = values.max()
    cv.vkl_colormap_array(cmap_, N, <double*>&values.data[0], vmin, vmax, <cv.cvec4*>&out.data[0])
    return out


cdef class App:

    cdef cv.VklApp* _c_app
    cdef cv.VklGpu* _c_gpu

    _canvases = []

    def __cinit__(self):
        self._c_app = cv.vkl_app(cv.VKL_BACKEND_GLFW)
        if self._c_app is NULL:
            raise MemoryError()
        self._c_gpu = cv.vkl_gpu(self._c_app, 0);
        if self._c_gpu is NULL:
            raise MemoryError()

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        if self._c_app is not NULL:
            for c in self._canvases:
                c.destroy()
            cv.vkl_app_destroy(self._c_app)
            self._c_app = NULL

    def canvas(
            self, int width=DEFAULT_WIDTH, int height=DEFAULT_HEIGHT, int rows=1, int cols=1,
            bint show_fps=False):
        cdef int fps = 0
        if show_fps:
            fps = cv.VKL_CANVAS_FLAGS_FPS
        c_canvas = cv.vkl_canvas(self._c_gpu, width, height, fps)
        if c_canvas is NULL:
            raise MemoryError()
        c = Canvas()
        c.create(self, c_canvas, rows, cols)
        self._canvases.append(c)
        return c

    def run(self, int n_frames=0, unicode screenshot=None):
        # HACK: run a few frames to render the image, make a screenshot, and run the event loop.
        if screenshot and self._canvases:
            cv.vkl_app_run(self._c_app, 5)
            self._canvases[0].screenshot(screenshot)
        cv.vkl_app_run(self._c_app, n_frames)

    def run_one_frame(self):
        cv.vkl_app_run(self._c_app, 1)


cdef _wrapped_callback(cv.VklCanvas* c_canvas, cv.VklEvent c_ev):
    cdef object tup
    if c_ev.user_data != NULL:
        tup = <object>c_ev.user_data
        # pos = (<int>c_ev.u.m.pos[0], <int>c_ev.u.m.pos[1])
        f, args = tup
        try:
            # f(pos)
            f()
        except Exception as e:
            print("Error: %s" % e)



cdef _add_event_callback(cv.VklCanvas* c_canvas, cv.VklEventType evtype, double param, f, args):
    cdef void* ptr_to_obj
    tup = (f, args)

    # IMPORTANT: need to either keep a reference of this tuple object somewhere in the class,
    # or increase the ref, otherwise this tuple will be deleted by the time we call it in the
    # C callback function.
    Py_INCREF(tup)

    ptr_to_obj = <void*>tup
    cv.vkl_event_callback(c_canvas, evtype, param, cv.VKL_EVENT_MODE_SYNC, <cv.VklEventCallback>_wrapped_callback, ptr_to_obj)



cdef class Canvas:

    cdef cv.VklCanvas* _c_canvas
    cdef cv.VklScene* _c_scene
    cdef object _app

    _panels = []

    cdef create(self, app, cv.VklCanvas* c_canvas, int rows, int cols):
        self._c_canvas = c_canvas
        self._c_scene = cv.vkl_scene(c_canvas, rows, cols)
        self._app = app
        # _add_close_callback(self._c_canvas, self._destroy_wrapper, ())

    def screenshot(self, unicode path):
        cdef char* _c_path = path
        cv.vkl_screenshot_file(self._c_canvas, _c_path);

    def panel(self, int row=0, int col=0, controller='axes', transpose=None):
        ctl = _CONTROLLERS.get(controller, cv.VKL_CONTROLLER_NONE)
        trans = _TRANSPOSES.get(transpose, cv.VKL_CDS_TRANSPOSE_NONE)
        c_panel = cv.vkl_scene_panel(self._c_scene, row, col, ctl, 0)
        if c_panel is NULL:
            raise MemoryError()
        cv.vkl_panel_transpose(c_panel, trans)
        p = Panel()
        p.create(self._c_scene, c_panel)
        self._panels.append(p)
        return p

    def __dealloc__(self):
        self.destroy()

    def _destroy_wrapper(self):
        # This is called when the user presses Esc, Visky organizes the canvas closing and
        # destruction, but we need the Python object to be destroyed as well and the
        # canvas to be removed from the canvas list in the App.
        self._c_canvas = NULL
        self._app._canvases.remove(self)

    def destroy(self):
        # This is called when the canvas is closed from Python.
        # The event loop will close the canvas and destroy it at the next frame.
        # However this doesn't work when the canvas is closed from C (for example by pressing Esc)
        # because then the C object will be destroyed, but not the Python one. We need to
        # destroy the Python via the close callback, which is called when the C library
        # is about to destroy the canvas, to give Python a chance to destroy the Python wrapper
        # as well.
        if self._c_scene is not NULL:
            cv.vkl_scene_destroy(self._c_scene)
        if self._c_canvas is not NULL:
            cv.vkl_canvas_to_close(self._c_canvas)
            self._c_canvas = NULL

    def connect(self, evtype_py, f, param=0):
        cdef cv.VklEventType evtype
        evtype = _EVENTS.get(evtype_py, 0)
        _add_event_callback(self._c_canvas, evtype, param, f, ())

    # def _wrap_keyboard(self, f):
    #     @wraps(f)
    #     def wrapped(c):
    #         cdef cv.VklKeyboard* keyboard
    #         cdef cv.VklKey key
    #         keyboard = cv.vkl_event_keyboard(self._c_canvas)
    #         key = keyboard.key
    #         if keyboard.cur_state != cv.VKL_KEYBOARD_STATE_CAPTURE and key not in _EXCLUDED_KEYS:
    #             # TODO: modifiers
    #             f(_key_name(key))
    #     return wrapped

    # def _wrap_mouse(self, f):
    #     @wraps(f)
    #     def wrapped(c):
    #         cdef cv.VklMouse* mouse
    #         mouse = cv.vkl_event_mouse(self._c_canvas)
    #         button = _button_name(mouse.button)
    #         pos = tuple(mouse.cur_pos)
    #         info = {'state': _mouse_state(mouse.cur_state)}
    #         f(button, pos, **info)
    #     return wrapped

    # def on_key(self, f):
    #     _add_frame_callback(self._c_canvas, self._wrap_keyboard(f), (self,))

    # def on_mouse(self, f):
    #     _add_frame_callback(self._c_canvas, self._wrap_mouse(f), (self,))


cdef class Panel:

    cdef cv.VklScene* _c_scene
    cdef cv.VklPanel* _c_panel

    _visuals = []

    cdef create(self, cv.VklScene* c_scene, cv.VklPanel* c_panel):
        self._c_panel = c_panel
        self._c_scene = c_scene

    def visual(self, vtype, depth_test=None, transform='auto'):
        visual_type = _VISUALS.get(vtype, cv.VKL_VISUAL_MARKER)
        flags = 0
        if depth_test:
            flags |= cv.VKL_GRAPHICS_FLAGS_DEPTH_TEST_ENABLE
        if transform is None:
            flags |= cv.VKL_VISUAL_FLAGS_TRANSFORM_NONE
        c_visual = cv.vkl_scene_visual(self._c_panel, visual_type, flags)
        if c_visual is NULL:
            raise MemoryError()
        v = Visual()
        v.create(self._c_panel, c_visual)
        self._visuals.append(v)
        return v



cdef class Visual:
    cdef cv.VklPanel* _c_panel
    cdef cv.VklVisual* _c_visual
    cdef cv.VklContext* _c_context

    cdef create(self, cv.VklPanel* c_panel, cv.VklVisual* c_visual):
        self._c_panel = c_panel
        self._c_visual = c_visual
        self._c_context = c_visual.canvas.gpu.context

    def data(self, name, np.ndarray value, idx=0):
        dtype = _PROP_DTYPES.get(name, np.float32)
        if value.dtype != dtype:
            value = value.astype(dtype)
        assert value.dtype == dtype

        if not value.flags['C_CONTIGUOUS']:
            value = np.ascontiguousarray(value)

        prop = _get_prop(name)
        N = value.shape[0]
        cv.vkl_visual_data(self._c_visual, prop, idx, N, &value.data[0])

    def volume(self, np.ndarray value, idx=0):
        assert value.ndim == 3
        # TODO: choose format as a function of the array dtype
        cdef cv.uvec3 shape
        shape[0] = value.shape[0]
        shape[1] = value.shape[1]
        shape[2] = value.shape[2]
        cdef size = value.size
        cdef item_size = np.dtype(value.dtype).itemsize
        texture = cv.vkl_ctx_texture(self._c_context, 3, shape, cv.VK_FORMAT_R16_UNORM)
        cv.vkl_texture_filter(texture, cv.VKL_FILTER_MAG, cv.VK_FILTER_LINEAR);
        cdef cv.uvec3 VKL_ZERO_OFFSET = [0, 0, 0]
        cv.vkl_texture_upload(texture, VKL_ZERO_OFFSET, VKL_ZERO_OFFSET, size * item_size, &value.data[0])
        cv.vkl_visual_texture(self._c_visual, cv.VKL_SOURCE_TYPE_VOLUME, idx, texture)
