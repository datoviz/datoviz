cimport visky.cyvisky as cv


cdef class App:
    cdef cv.VkyApp* _c_app

    def __cinit__(self):
        self._c_app = cv.vky_create_app(cv.VkyBackendType.VKY_BACKEND_NONE, NULL)
        if self._c_app is NULL:
            raise MemoryError()

    def __dealloc__(self):
        if self._c_app is not NULL:
            cv.vky_destroy_app(self._c_app)
