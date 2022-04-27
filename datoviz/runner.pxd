# WARNING: parts of this file are auto-generated

from ._types cimport *


cdef class Runner:
    cdef DvzRunner* _c_runner
    # cdef DvzGpu* _c_gpu


cdef extern from "<datoviz/runner.h>":
    # Semi-opaque structs:

    ctypedef struct DvzRenderer:
        pass

    ctypedef struct DvzRequester:
        pass

    ctypedef struct DvzRunner:
        pass

    ctypedef struct DvzGpu:
        pass

    ctypedef struct DvzRequest:
        DvzId id
        DvzRequestAction action
        DvzRequestObject type
        int flags



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
    DvzRunner* dvz_runner(DvzRenderer* renderer)

    int dvz_runner_frame(DvzRunner* runner)

    int dvz_runner_loop(DvzRunner* runner, uint64_t frame_count)

    void dvz_runner_request(DvzRunner* runner, DvzRequest request)

    void dvz_runner_requests(DvzRunner* runner, uint32_t count, DvzRequest* requests)

    void dvz_runner_requester(DvzRunner* runner, DvzRequester* requester)

    void dvz_runner_destroy(DvzRunner* runner)


    # FUNCTION END
