# cython: c_string_type=unicode, c_string_encoding=ascii

from functools import wraps, partial
import logging

cimport numpy as np
import numpy as np
from cpython.ref cimport Py_INCREF

cimport datoviz.cydatoviz as cv


logger = logging.getLogger(__name__)

DEFAULT_WIDTH = 1024
DEFAULT_HEIGHT = 768


# TODO: add more keys
_KEYS = {
    # cv.DVZ_KEY_LEFT: 'left',
    # cv.DVZ_KEY_RIGHT: 'right',
    # cv.DVZ_KEY_UP: 'up',
    # cv.DVZ_KEY_DOWN: 'down',
    # cv.DVZ_KEY_HOME: 'home',
    # cv.DVZ_KEY_END: 'end',
    # cv.DVZ_KEY_KP_ADD: '+',
    # cv.DVZ_KEY_KP_SUBTRACT: '-',
    # cv.DVZ_KEY_G: 'g',
}

# HACK: these keys do not raise a Python key event
_EXCLUDED_KEYS = (
    # cv.DVZ_KEY_NONE,
    # cv.DVZ_KEY_LEFT_SHIFT,
    # cv.DVZ_KEY_LEFT_CONTROL,
    # cv.DVZ_KEY_LEFT_ALT,
    # cv.DVZ_KEY_LEFT_SUPER,
    # cv.DVZ_KEY_RIGHT_SHIFT,
    # cv.DVZ_KEY_RIGHT_CONTROL,
    # cv.DVZ_KEY_RIGHT_ALT,
    # cv.DVZ_KEY_RIGHT_SUPER,
)

_BUTTONS = {
    # cv.DVZ_MOUSE_BUTTON_LEFT: 'left',
    # cv.DVZ_MOUSE_BUTTON_MIDDLE: 'middle',
    # cv.DVZ_MOUSE_BUTTON_RIGHT: 'right',
}

_MOUSE_STATES = {
    # cv.DVZ_MOUSE_STATE_DRAG: 'drag',
    # cv.DVZ_MOUSE_STATE_WHEEL: 'wheel',
    # cv.DVZ_MOUSE_STATE_CLICK: 'click',
    # cv.DVZ_MOUSE_STATE_DOUBLE_CLICK: 'double_click',
}

_VISUALS = {
    'point': cv.DVZ_VISUAL_POINT,
    'marker': cv.DVZ_VISUAL_MARKER,
    'mesh': cv.DVZ_VISUAL_MESH,
    'volume_slice': cv.DVZ_VISUAL_VOLUME_SLICE,
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

_PROPS = {
    'pos': cv.DVZ_PROP_POS,
    'color': cv.DVZ_PROP_COLOR,
    'alpha': cv.DVZ_PROP_ALPHA,
    'ms': cv.DVZ_PROP_MARKER_SIZE,
    'normal': cv.DVZ_PROP_NORMAL,
    'texcoords': cv.DVZ_PROP_TEXCOORDS,
    'index': cv.DVZ_PROP_INDEX,
    'light_params': cv.DVZ_PROP_LIGHT_PARAMS,
    'light_pos': cv.DVZ_PROP_LIGHT_POS,
    'texcoefs': cv.DVZ_PROP_TEXCOEFS,
    'linewidth': cv.DVZ_PROP_LINE_WIDTH,
    'view_pos': cv.DVZ_PROP_VIEW_POS,
    'colormap': cv.DVZ_PROP_COLORMAP,
    'transferx': cv.DVZ_PROP_TRANSFER_X,
    'transfery': cv.DVZ_PROP_TRANSFER_Y,
    'clip': cv.DVZ_PROP_CLIP,
}

_CONTROLS = {
    'slider_float': cv.DVZ_GUI_CONTROL_SLIDER_FLOAT,
    'slider_int': cv.DVZ_GUI_CONTROL_SLIDER_INT,
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

_PROP_DTYPES = {
    'pos': np.double,
    'color': np.uint8,
    'alpha': np.uint8,
    'index': np.uint32,
    'colormap': np.uint8,
}

_EVENTS ={
    'mouse': cv.DVZ_EVENT_MOUSE_MOVE,
    'frame': cv.DVZ_EVENT_FRAME,
    'timer': cv.DVZ_EVENT_TIMER,
    'gui': cv.DVZ_EVENT_GUI,
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

def colormap(np.ndarray[DOUBLE, ndim=1] values, vmin=None, vmax=None, cmap=None, alpha=None):
    N = values.size
    cmap_ = _COLORMAPS.get(cmap, cv.DVZ_CMAP_VIRIDIS)
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


cdef class App:

    cdef cv.DvzApp* _c_app
    cdef cv.DvzGpu* _c_gpu

    _canvases = []

    def __cinit__(self):
        self._c_app = cv.dvz_app(cv.DVZ_BACKEND_GLFW)
        if self._c_app is NULL:
            raise MemoryError()
        self._c_gpu = cv.dvz_gpu(self._c_app, 0);
        if self._c_gpu is NULL:
            raise MemoryError()

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        if self._c_app is not NULL:
            for c in self._canvases:
                c.destroy()
            cv.dvz_app_destroy(self._c_app)
            self._c_app = NULL

    def canvas(
            self, int width=DEFAULT_WIDTH, int height=DEFAULT_HEIGHT, int rows=1, int cols=1,
            bint show_fps=False):
        cdef int fps = 0
        if show_fps:
            fps = cv.DVZ_CANVAS_FLAGS_FPS
        fps |= cv.DVZ_CANVAS_FLAGS_IMGUI
        c_canvas = cv.dvz_canvas(self._c_gpu, width, height, fps)
        if c_canvas is NULL:
            raise MemoryError()
        c = Canvas()
        c.create(self, c_canvas, rows, cols)
        self._canvases.append(c)
        return c

    def run(self, int n_frames=0, unicode screenshot=None, unicode video=None):
        # HACK: run a few frames to render the image, make a screenshot, and run the event loop.
        if screenshot and self._canvases:
            cv.dvz_app_run(self._c_app, 5)
            self._canvases[0].screenshot(screenshot)
        if video and self._canvases:
            self._canvases[0].video(video)
        cv.dvz_app_run(self._c_app, n_frames)

    def run_one_frame(self):
        cv.dvz_app_run(self._c_app, 1)



cdef _get_ev_args(cv.DvzEvent c_ev):
    cdef float* fvalue
    cdef int* ivalue
    dt = c_ev.type
    if dt == cv.DVZ_EVENT_GUI:
        if c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_SLIDER_FLOAT:
            fvalue = <float*>c_ev.u.g.control.value
            return (fvalue[0],)
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_SLIDER_INT:
            ivalue = <int*>c_ev.u.g.control.value
            return (ivalue[0],)
    return ()



cdef _wrapped_callback(cv.DvzCanvas* c_canvas, cv.DvzEvent c_ev):
    cdef object tup
    if c_ev.user_data != NULL:
        tup = <object>c_ev.user_data
        # For each type of event, get the arguments to the function
        ev_args = _get_ev_args(c_ev)
        f, args = tup # NOTE: args not used for now
        try:
            f(*ev_args)
        except Exception as e:
            print("Error: %s" % e)



cdef _add_event_callback(cv.DvzCanvas* c_canvas, cv.DvzEventType evtype, double param, f, args):
    cdef void* ptr_to_obj
    tup = (f, args)

    # IMPORTANT: need to either keep a reference of this tuple object somewhere in the class,
    # or increase the ref, otherwise this tuple will be deleted by the time we call it in the
    # C callback function.
    Py_INCREF(tup)

    ptr_to_obj = <void*>tup
    cv.dvz_event_callback(c_canvas, evtype, param, cv.DVZ_EVENT_MODE_ASYNC, <cv.DvzEventCallback>_wrapped_callback, ptr_to_obj)



cdef class Canvas:

    cdef cv.DvzCanvas* _c_canvas
    cdef cv.DvzScene* _c_scene
    cdef object _app

    _panels = []

    cdef create(self, app, cv.DvzCanvas* c_canvas, int rows, int cols):
        self._c_canvas = c_canvas
        self._c_scene = cv.dvz_scene(c_canvas, rows, cols)
        self._app = app
        # _add_close_callback(self._c_canvas, self._destroy_wrapper, ())

    def screenshot(self, unicode path):
        cdef char* _c_path = path
        cv.dvz_screenshot_file(self._c_canvas, _c_path);

    def video(self, unicode path):
        cdef char* _c_path = path
        cv.dvz_canvas_video(self._c_canvas, 30, 10000000, _c_path)

    def panel(self, int row=0, int col=0, controller='axes', transpose=None):
        ctl = _CONTROLLERS.get(controller, cv.DVZ_CONTROLLER_NONE)
        trans = _TRANSPOSES.get(transpose, cv.DVZ_CDS_TRANSPOSE_NONE)
        c_panel = cv.dvz_scene_panel(self._c_scene, row, col, ctl, 0)
        if c_panel is NULL:
            raise MemoryError()
        cv.dvz_panel_transpose(c_panel, trans)
        p = Panel()
        p.create(self._c_scene, c_panel)
        self._panels.append(p)
        return p

    def gui(self, unicode title):
        c_gui = cv.dvz_gui(self._c_canvas, title, 0)
        gui = Gui()
        gui.create(self._c_canvas, c_gui)
        return gui

    def __dealloc__(self):
        self.destroy()

    def _destroy_wrapper(self):
        # This is called when the user presses Esc, Datoviz organizes the canvas closing and
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
            cv.dvz_scene_destroy(self._c_scene)
        if self._c_canvas is not NULL:
            cv.dvz_canvas_to_close(self._c_canvas)
            self._c_canvas = NULL

    def connect(self, evtype_py, f, param=0):
        cdef cv.DvzEventType evtype
        evtype = _EVENTS.get(evtype_py, 0)
        _add_event_callback(self._c_canvas, evtype, param, f, ())

    # def _wrap_keyboard(self, f):
    #     @wraps(f)
    #     def wrapped(c):
    #         cdef cv.DvzKeyboard* keyboard
    #         cdef cv.DvzKey key
    #         keyboard = cv.dvz_event_keyboard(self._c_canvas)
    #         key = keyboard.key
    #         if keyboard.cur_state != cv.DVZ_KEYBOARD_STATE_CAPTURE and key not in _EXCLUDED_KEYS:
    #             # TODO: modifiers
    #             f(_key_name(key))
    #     return wrapped

    # def _wrap_mouse(self, f):
    #     @wraps(f)
    #     def wrapped(c):
    #         cdef cv.DvzMouse* mouse
    #         mouse = cv.dvz_event_mouse(self._c_canvas)
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

    cdef cv.DvzScene* _c_scene
    cdef cv.DvzPanel* _c_panel

    _visuals = []

    cdef create(self, cv.DvzScene* c_scene, cv.DvzPanel* c_panel):
        self._c_panel = c_panel
        self._c_scene = c_scene

    def visual(self, vtype, depth_test=None, transform='auto'):
        visual_type = _VISUALS.get(vtype, cv.DVZ_VISUAL_MARKER)
        flags = 0
        if depth_test:
            flags |= cv.DVZ_GRAPHICS_FLAGS_DEPTH_TEST_ENABLE
        if transform is None:
            flags |= cv.DVZ_VISUAL_FLAGS_TRANSFORM_NONE
        c_visual = cv.dvz_scene_visual(self._c_panel, visual_type, flags)
        if c_visual is NULL:
            raise MemoryError()
        v = Visual()
        v.create(self._c_panel, c_visual)
        self._visuals.append(v)
        return v



cdef class Visual:
    cdef cv.DvzPanel* _c_panel
    cdef cv.DvzVisual* _c_visual
    cdef cv.DvzContext* _c_context

    cdef create(self, cv.DvzPanel* c_panel, cv.DvzVisual* c_visual):
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
        cv.dvz_visual_data(self._c_visual, prop, idx, N, &value.data[0])

    def volume(self, np.ndarray value, idx=0):
        assert value.ndim == 3
        # TODO: choose format as a function of the array dtype
        cdef cv.uvec3 shape
        shape[0] = value.shape[0]
        shape[1] = value.shape[1]
        shape[2] = value.shape[2]
        cdef size = value.size
        cdef item_size = np.dtype(value.dtype).itemsize
        texture = cv.dvz_ctx_texture(self._c_context, 3, shape, cv.VK_FORMAT_R16_UNORM)
        cv.dvz_texture_filter(texture, cv.DVZ_FILTER_MAG, cv.VK_FILTER_LINEAR);
        cdef cv.uvec3 DVZ_ZERO_OFFSET = [0, 0, 0]
        cv.dvz_texture_upload(texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, size * item_size, &value.data[0])
        cv.dvz_visual_texture(self._c_visual, cv.DVZ_SOURCE_TYPE_VOLUME, idx, texture)

    def load_obj(self, unicode path, compute_normals=False):
        # TODO: check that it is a mesh visual

        cdef cv.DvzMesh mesh = cv.dvz_mesh_obj(path);

        if compute_normals:
            print("computing normals")
            cv.dvz_mesh_normals(&mesh)

        nv = mesh.vertices.item_count;
        ni = mesh.indices.item_count;

        cv.dvz_visual_data_source(self._c_visual, cv.DVZ_SOURCE_TYPE_VERTEX, 0, 0, nv, nv, mesh.vertices.data);
        cv.dvz_visual_data_source(self._c_visual, cv.DVZ_SOURCE_TYPE_INDEX, 0, 0, ni, ni, mesh.indices.data);



cdef class Gui:
    cdef cv.DvzCanvas* _c_canvas
    cdef cv.DvzGui* _c_gui

    cdef create(self, cv.DvzCanvas* c_canvas, cv.DvzGui* c_gui):
        self._c_canvas = c_canvas
        self._c_gui = c_gui

    def control(self, unicode ctype, unicode name, **kwargs):
        ctrl = _CONTROLS.get(ctype, 0)
        cdef char* c_name = name

        if (ctype == 'slider_float'):
            c_vmin = kwargs.get('vmin', 0)
            c_vmax = kwargs.get('vmax', 1)
            cv.dvz_gui_slider_float(self._c_gui, c_name, c_vmin, c_vmax)
        elif (ctype == 'slider_int'):
            c_vmin = kwargs.get('vmin', 0)
            c_vmax = kwargs.get('vmax', 1)
            cv.dvz_gui_slider_int(self._c_gui, c_name, c_vmin, c_vmax)

        def wrap(f):
            cdef cv.DvzEventType evtype
            evtype = cv.DVZ_EVENT_GUI
            _add_event_callback(self._c_canvas, evtype, 0, f, ())

        return wrap
