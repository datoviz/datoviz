# WARNING: parts of this file are auto-generated

from ._types cimport *


cdef extern from "<datoviz/scene/app.h>":
    ctypedef struct DvzApp:
        pass

    ctypedef struct DvzDevice:
        pass

    ctypedef struct DvzRequester:
        pass

    ctypedef struct DvzRequest:
        pass

    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------
    # !!!!!!!!!!!!!!!!!!!! AUTOMATICALLY-GENERATED PART: DO NOT EDIT MANUALLY !!!!!!!!!!!!!!!!!!!!
    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------

    # Structures
    # ---------------------------------------------------------------------------------------------

    # STRUCT START

    # STRUCT END

    # Functions
    # ---------------------------------------------------------------------------------------------

    # FUNCTION START
    DvzApp* dvz_app(DvzBackend backend)

    DvzDevice* dvz_device(DvzApp* app)

    void dvz_device_frame(DvzDevice* device, DvzRequester* rqr)

    void dvz_device_run(DvzDevice* device, DvzRequester* rqr, uint64_t n_frames)

    void dvz_device_async(DvzDevice* device, DvzRequester* rqr, uint64_t n_frames)

    void dvz_device_wait(DvzDevice* device)

    void dvz_device_stop(DvzDevice* device)

    void dvz_device_update(DvzDevice* device, DvzRequester* rqr)

    void dvz_device_destroy(DvzDevice* device)

    void dvz_app_destroy(DvzApp* app)


    # FUNCTION END
