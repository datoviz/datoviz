cdef extern from "../../include/visky/app.h":
    ctypedef struct VkyApp:
        pass

    ctypedef enum VkyBackendType:
        VKY_BACKEND_NONE = 0
        VKY_BACKEND_GLFW = 1
        VKY_BACKEND_OFFSCREEN = 10
        VKY_BACKEND_SCREENSHOT = 11
        VKY_BACKEND_VIDEO = 12

    VkyApp* vky_create_app(VkyBackendType backend, void* params)
    void vky_destroy_app(VkyApp* app)
