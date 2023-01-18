# WARNING: parts of this file are auto-generated

from ._types cimport *


cdef extern from "<datoviz/request.h>":
    # Semi-opaque structs:

    ctypedef struct DvzRequest:
        DvzId id
        DvzRequestAction action
        DvzRequestObject type
        int flags

    ctypedef struct DvzRequester:
        uint32_t count
        uint32_t capacity
        DvzRequest * requests

    ctypedef struct DvzRecorder:
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
    DvzRecorder* dvz_recorder(int flags)

    DvzRequester* dvz_requester()

    void dvz_requester_destroy(DvzRequester* rqr)

    void dvz_requester_begin(DvzRequester* rqr)

    void dvz_requester_add(DvzRequester* rqr, DvzRequest req)

    DvzRequest* dvz_requester_end(DvzRequester* rqr, uint32_t* count)

    int dvz_requester_dump(DvzRequester* rqr, const char* filename)

    void dvz_requester_load(DvzRequester* rqr, const char* filename)

    DvzRequest* dvz_requester_flush(DvzRequester* rqr, uint32_t* count)

    void dvz_request_print(DvzRequest* req)

    DvzRequest dvz_create_board(DvzRequester* rqr, uint32_t width, uint32_t height, cvec4 background, int flags)

    DvzRequest dvz_set_background(DvzRequester* rqr, DvzId id, cvec4 background)

    DvzRequest dvz_update_board(DvzRequester* rqr, DvzId id)

    DvzRequest dvz_delete_board(DvzRequester* rqr, DvzId id)

    DvzRequest dvz_create_canvas(DvzRequester* rqr, uint32_t width, uint32_t height, cvec4 background, int flags)

    DvzRequest dvz_delete_canvas(DvzRequester* rqr, DvzId id)

    DvzRequest dvz_create_dat(DvzRequester* rqr, DvzBufferType type, DvzSize size, int flags)

    DvzRequest dvz_upload_dat(DvzRequester* rqr, DvzId dat, DvzSize offset, DvzSize size, void* data)

    DvzRequest dvz_create_tex(DvzRequester* rqr, DvzTexDims dims, DvzFormat format, uvec3 shape, int flags)

    DvzRequest dvz_upload_tex(DvzRequester* rqr, DvzId tex, uvec3 offset, uvec3 shape, DvzSize size, void* data)

    DvzRequest dvz_create_sampler(DvzRequester* rqr, DvzFilter filter, DvzSamplerAddressMode mode)

    DvzRequest dvz_create_graphics(DvzRequester* rqr, DvzGraphicsType type, int flags)

    DvzRequest dvz_set_vertex(DvzRequester* rqr, DvzId graphics, DvzId dat)

    DvzRequest dvz_bind_dat(DvzRequester* rqr, DvzId pipe, uint32_t slot_idx, DvzId dat)

    DvzRequest dvz_bind_tex(DvzRequester* rqr, DvzId pipe, uint32_t slot_idx, DvzId tex, DvzId sampler)

    DvzRequest dvz_record_begin(DvzRequester* rqr, DvzId board)

    DvzRequest dvz_record_viewport(DvzRequester* rqr, DvzId board, vec2 offset, vec2 shape)

    DvzRequest dvz_record_draw(DvzRequester* rqr, DvzId board, DvzId graphics, uint32_t first_vertex, uint32_t vertex_count)

    DvzRequest dvz_record_end(DvzRequester* rqr, DvzId board)


    # FUNCTION END


# cdef class Request:
#     cdef DvzRequest _c_req


# cdef class Requester:
#     cdef DvzRequest * _c_rqs
#     cdef uint32_t _c_count
#     cdef DvzRequester * _c_rqr

#     # HACK: keep a reference of the arrays to be uploaded, to prevent them from being
#     # collected by the garbage collector until they are effectively transferred to the GPU.
#     cdef object _np_cache
