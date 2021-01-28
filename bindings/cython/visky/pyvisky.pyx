from functools import wraps, partial
import logging

cimport numpy as np
import numpy as np
from cpython.ref cimport Py_INCREF

cimport visky.cyvisky as cv


logger = logging.getLogger(__name__)


DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 800


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
    'arcball': cv.VKL_CONTROLLER_ARCBALL,
    'fps': cv.VKL_CONTROLLER_CAMERA,
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

def colormap(np.ndarray[DOUBLE, ndim=1] values, vmin=None, vmax=None, cmap=cv.VKL_CMAP_VIRIDIS):
    N = values.size
    # TODO: ndarrays
    cdef np.ndarray out = np.zeros((N, 4), dtype=np.uint8)
    if vmin is None:
        vmin = values.min()
    if vmax is None:
        vmax = values.max()
    cv.vkl_colormap_array(cmap, N, <double*>&values.data[0], vmin, vmax, <cv.cvec4*>&out.data[0])
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

    def canvas(self, int width=DEFAULT_WIDTH, int height=DEFAULT_HEIGHT, int rows=1, int cols=1):
        c_canvas = cv.vkl_canvas(self._c_gpu, width, height, cv.VKL_CANVAS_FLAGS_FPS)
        if c_canvas is NULL:
            raise MemoryError()
        c = Canvas()
        c.create(self, c_canvas, rows, cols)
        self._canvases.append(c)
        return c

    def run(self, int n_frames=0):
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

    def panel(self, int row=0, int col=0, controller='panzoom', transpose=None):
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
