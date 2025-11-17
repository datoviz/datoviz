/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  NVENC backend                                                                                */
/*************************************************************************************************/

#include "_alloc.h"
#include "_log.h"
#include "datoviz/common/macros.h"
#include "encoder_backend.h"
#include <cuda.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if OS_UNIX
#include <unistd.h>
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include "nvEncodeAPI.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#undef NV_ENC_INITIALIZE_PARAMS_VER
#define NV_ENC_INITIALIZE_PARAMS_VER (NVENCAPI_STRUCT_VERSION(7) | (1u << 31))

#undef NV_ENC_PRESET_CONFIG_VER
#define NV_ENC_PRESET_CONFIG_VER (NVENCAPI_STRUCT_VERSION(5) | (1u << 31))

#undef NV_ENC_CONFIG_VER
#define NV_ENC_CONFIG_VER (NVENCAPI_STRUCT_VERSION(9) | (1u << 31))

#undef NV_ENC_PIC_PARAMS_VER
#define NV_ENC_PIC_PARAMS_VER (NVENCAPI_STRUCT_VERSION(7) | (1u << 31))

#undef NV_ENC_LOCK_BITSTREAM_VER
#define NV_ENC_LOCK_BITSTREAM_VER (NVENCAPI_STRUCT_VERSION(2) | (1u << 31))

#define PITCH_ALIGN          256
#define NVENC_INVALID_OFFSET UINT64_MAX

static NV_ENCODE_API_FUNCTION_LIST g_nvenc = {0};



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define CU_CHECK(x)                                                                               \
    do                                                                                            \
    {                                                                                             \
        CUresult _e = (x);                                                                        \
        if (_e != CUDA_SUCCESS)                                                                   \
        {                                                                                         \
            const char* _s = NULL;                                                                \
            cuGetErrorName(_e, &_s);                                                              \
            dvz_fprintf(                                                                          \
                stderr, "CUDA error %s at %s:%d\n", _s ? _s : "?", __FILE__, __LINE__);           \
            exit(1);                                                                              \
        }                                                                                         \
    } while (0)

#define NVENC_API_CALL(x)                                                                         \
    do                                                                                            \
    {                                                                                             \
        NVENCSTATUS _s = (x);                                                                     \
        if (_s != NV_ENC_SUCCESS)                                                                 \
        {                                                                                         \
            dvz_fprintf(stderr, "NVENC error %d at %s:%d\n", (int)_s, __FILE__, __LINE__);         \
            exit(1);                                                                              \
        }                                                                                         \
    } while (0)



/*************************************************************************************************/
/*  CUDA kernel (PTX)                                                                            */
/*************************************************************************************************/

static const char* PTX = ".version 7.8\n"
                         ".target sm_52\n"
                         ".address_size 64\n"
                         "\n"
                         ".visible .entry rgba2nv12(\n"
                         "    .param .u64 param_src,\n"
                         "    .param .u32 param_src_pitch,\n"
                         "    .param .u64 param_dst,\n"
                         "    .param .u32 param_dst_pitch,\n"
                         "    .param .u32 param_width,\n"
                         "    .param .u32 param_height)\n"
                         "{\n"
                         "    .reg .pred %p<4>;\n"
                         "    .reg .b32  %r<60>;\n"
                         "    .reg .b64  %rd<12>;\n"
                         "\n"
                         "    ld.param.u64 %rd0, [param_src];\n"
                         "    ld.param.u32 %r0,  [param_src_pitch];\n"
                         "    ld.param.u64 %rd1, [param_dst];\n"
                         "    ld.param.u32 %r1,  [param_dst_pitch];\n"
                         "    ld.param.u32 %r2,  [param_width];\n"
                         "    ld.param.u32 %r3,  [param_height];\n"
                         "\n"
                         "    mov.u32 %r10, %ctaid.x;\n"
                         "    mov.u32 %r11, %ctaid.y;\n"
                         "    mov.u32 %r12, %ntid.x;\n"
                         "    mov.u32 %r13, %ntid.y;\n"
                         "    mov.u32 %r14, %tid.x;\n"
                         "    mov.u32 %r15, %tid.y;\n"
                         "\n"
                         "    mad.lo.u32 %r16, %r10, %r12, %r14;\n"
                         "    mad.lo.u32 %r17, %r11, %r13, %r15;\n"
                         "\n"
                         "    setp.ge.u32 %p0, %r16, %r2;\n"
                         "    @%p0 bra DONE;\n"
                         "    setp.ge.u32 %p1, %r17, %r3;\n"
                         "    @%p1 bra DONE;\n"
                         "\n"
                         "    mul.lo.u32 %r18, %r17, %r0;\n"
                         "    shl.b32 %r19, %r16, 2;\n"
                         "    add.u32 %r18, %r18, %r19;\n"
                         "    cvt.u64.u32 %rd2, %r18;\n"
                         "    add.u64 %rd2, %rd0, %rd2;\n"
                         "\n"
                         "    ld.global.u8 %r20, [%rd2];\n"
                         "    ld.global.u8 %r21, [%rd2+1];\n"
                         "    ld.global.u8 %r22, [%rd2+2];\n"
                         "\n"
                         "    cvt.u32.u8 %r23, %r20;\n"
                         "    cvt.u32.u8 %r24, %r21;\n"
                         "    cvt.u32.u8 %r25, %r22;\n"
                         "\n"
                         "    mul.lo.s32 %r50, %r23, 66;\n"
                         "    mad.lo.s32 %r50, %r24, 129, %r50;\n"
                         "    mad.lo.s32 %r50, %r25, 25, %r50;\n"
                         "    add.s32 %r50, %r50, 128;\n"
                         "    shr.s32 %r26, %r50, 8;\n"
                         "    add.s32 %r26, %r26, 16;\n"
                         "    max.s32 %r26, %r26, 0;\n"
                         "    min.s32 %r26, %r26, 255;\n"
                         "\n"
                         "    mul.lo.u32 %r27, %r17, %r1;\n"
                         "    add.u32 %r27, %r27, %r16;\n"
                         "    cvt.u64.u32 %rd3, %r27;\n"
                         "    add.u64 %rd3, %rd1, %rd3;\n"
                         "    st.global.u8 [%rd3], %r26;\n"
                         "\n"
                         "    and.b32 %r30, %r16, 1;\n"
                         "    and.b32 %r31, %r17, 1;\n"
                         "    or.b32 %r32, %r30, %r31;\n"
                         "    setp.ne.u32 %p2, %r32, 0;\n"
                         "    @%p2 bra DONE;\n"
                         "\n"
                         "    mul.lo.s32 %r40, %r23, -38;\n"
                         "    mad.lo.s32 %r40, %r24, -74, %r40;\n"
                         "    mad.lo.s32 %r40, %r25, 112, %r40;\n"
                         "    add.s32 %r40, %r40, 128;\n"
                         "    shr.s32 %r40, %r40, 8;\n"
                         "    add.s32 %r40, %r40, 128;\n"
                         "    max.s32 %r40, %r40, 0;\n"
                         "    min.s32 %r40, %r40, 255;\n"
                         "\n"
                         "    mul.lo.s32 %r41, %r23, 112;\n"
                         "    mad.lo.s32 %r41, %r24, -94, %r41;\n"
                         "    mad.lo.s32 %r41, %r25, -18, %r41;\n"
                         "    add.s32 %r41, %r41, 128;\n"
                         "    shr.s32 %r41, %r41, 8;\n"
                         "    add.s32 %r41, %r41, 128;\n"
                         "    max.s32 %r41, %r41, 0;\n"
                         "    min.s32 %r41, %r41, 255;\n"
                         "\n"
                         "    shr.u32 %r33, %r16, 1;\n"
                         "    shr.u32 %r34, %r17, 1;\n"
                         "\n"
                         "    mul.lo.u32 %r35, %r3, %r1;\n"
                         "    cvt.u64.u32 %rd4, %r35;\n"
                         "    add.u64 %rd4, %rd1, %rd4;\n"
                         "\n"
                         "    mul.lo.u32 %r36, %r34, %r1;\n"
                         "    shl.b32 %r37, %r33, 1;\n"
                         "    add.u32 %r36, %r36, %r37;\n"
                         "    cvt.u64.u32 %rd5, %r36;\n"
                         "    add.u64 %rd4, %rd4, %rd5;\n"
                         "\n"
                         "    st.global.u8 [%rd4], %r40;\n"
                         "    st.global.u8 [%rd4+1], %r41;\n"
                         "\n"
                         "DONE:\n"
                         "    ret;\n"
                         "}\n";



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// CUDA context.
typedef struct
{
    CUcontext cuCtx;
    CUexternalMemory extMem;
    CUmipmappedArray mmArr;
    CUarray arr0;
    CUmodule cuMod;
    CUfunction cuFun;
    CUstream stream;
} CudaCtx;



// NVENC context.
typedef struct
{
    NV_ENCODE_API_FUNCTION_LIST api;
    void* hEncoder;
    NV_ENC_INITIALIZE_PARAMS init;
    NV_ENC_CONFIG encCfg;
} NvEncCtx;



// NVENC IO.
typedef struct
{
    NV_ENC_REGISTERED_PTR regPtr;
    NV_ENC_INPUT_PTR mappedInput;
    NV_ENC_OUTPUT_PTR bitstreams[4];
} NvEncIO;



// RGBA buffer.
typedef struct
{
    CUdeviceptr dptr;
    uint32_t pitch;
    size_t size;
} RgbaBuf;



// NV12 buffer.
typedef struct
{
    CUdeviceptr dptr;
    uint32_t pitch;
    size_t size;
} Nv12Buf;



typedef struct
{
    const GUID* codec_guid;
    const GUID* preset_guid;
    NV_ENC_TUNING_INFO tuning;
    uint32_t qp_intra;
    uint32_t qp_inter_p;
    uint32_t qp_inter_b;
} DvzNvencProfile;



// NVENC video backend.
typedef struct
{
    CudaCtx cuda;
    NvEncCtx nvenc;
    NvEncIO io;
    RgbaBuf rgba;
    Nv12Buf nv12;
    NV_ENC_INPUT_PTR mapped_in;
    NV_ENC_OUTPUT_PTR bitstream;
    CUexternalSemaphore wait_semaphore;
    bool cuda_ready;
    bool nvenc_ready;
    bool wait_semaphore_ready;
} DvzVideoBackendNvenc;



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

static inline uint32_t align_up(uint32_t v, uint32_t a) { return (v + a - 1) & ~(a - 1); }



static DvzVideoBackendNvenc* nvenc_state(DvzVideoEncoder* enc)
{
    ANN(enc);
    return (DvzVideoBackendNvenc*)enc->backend_data;
}



static void nvenc_state_free(DvzVideoBackendNvenc* state)
{
    if (!state)
    {
        return;
    }
    if (state->nvenc_ready)
    {
        if (state->io.mappedInput)
        {
            NVENC_API_CALL(state->nvenc.api.nvEncUnmapInputResource(
                state->nvenc.hEncoder, state->io.mappedInput));
            state->io.mappedInput = NULL;
            state->mapped_in = NULL;
        }
        for (size_t i = 0; i < DVZ_ARRAY_COUNT(state->io.bitstreams); ++i)
        {
            if (state->io.bitstreams[i])
            {
                NVENC_API_CALL(state->nvenc.api.nvEncDestroyBitstreamBuffer(
                    state->nvenc.hEncoder, state->io.bitstreams[i]));
                state->io.bitstreams[i] = NULL;
            }
        }
        if (state->io.regPtr)
        {
            NVENC_API_CALL(
                state->nvenc.api.nvEncUnregisterResource(state->nvenc.hEncoder, state->io.regPtr));
            state->io.regPtr = NULL;
        }
        NVENC_API_CALL(state->nvenc.api.nvEncDestroyEncoder(state->nvenc.hEncoder));
        state->nvenc.hEncoder = NULL;
        state->bitstream = NULL;
        state->nvenc_ready = false;
    }

    if (state->rgba.dptr)
    {
        cuMemFree(state->rgba.dptr);
        state->rgba.dptr = 0;
    }
    if (state->nv12.dptr)
    {
        cuMemFree(state->nv12.dptr);
        state->nv12.dptr = 0;
    }

    if (state->wait_semaphore_ready)
    {
        cuDestroyExternalSemaphore(state->wait_semaphore);
        state->wait_semaphore = NULL;
        state->wait_semaphore_ready = false;
    }

    if (state->cuda.cuMod)
    {
        cuModuleUnload(state->cuda.cuMod);
        state->cuda.cuMod = NULL;
    }
    if (state->cuda.mmArr)
    {
        cuMipmappedArrayDestroy(state->cuda.mmArr);
        state->cuda.mmArr = NULL;
    }
    if (state->cuda.extMem)
    {
        cuDestroyExternalMemory(state->cuda.extMem);
        state->cuda.extMem = NULL;
    }
    if (state->cuda.stream)
    {
        CU_CHECK(cuStreamDestroy(state->cuda.stream));
        state->cuda.stream = NULL;
    }
    if (state->cuda.cuCtx)
    {
        cuCtxDestroy(state->cuda.cuCtx);
        state->cuda.cuCtx = NULL;
    }
    dvz_memset(state, sizeof(*state), 0, sizeof(*state));
}



static void
cuda_import_vk_memory(CudaCtx* cu, uint32_t width, uint32_t height, int mem_fd, size_t alloc_size)
{
    ANN(cu);
    dvz_memset(cu, sizeof(*cu), 0, sizeof(*cu));
    CUdevice dev;
    CU_CHECK(cuInit(0));
    CU_CHECK(cuDeviceGet(&dev, 0));
    CU_CHECK(cuCtxCreate(&cu->cuCtx, 0, dev));
    CU_CHECK(cuStreamCreate(&cu->stream, CU_STREAM_DEFAULT));

    CUDA_EXTERNAL_MEMORY_HANDLE_DESC hdesc = {
        .type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD,
        .handle = {
            .fd = mem_fd,
        },
        .size = alloc_size,
        .flags = 0,
    };
    CU_CHECK(cuImportExternalMemory(&cu->extMem, &hdesc));

    CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC mdesc = {0};
    mdesc.offset = 0;
    mdesc.numLevels = 1;
    mdesc.arrayDesc.Width = width;
    mdesc.arrayDesc.Height = height;
    mdesc.arrayDesc.Depth = 0;
    mdesc.arrayDesc.NumChannels = 4;
    mdesc.arrayDesc.Format = CU_AD_FORMAT_UNSIGNED_INT8;
    mdesc.arrayDesc.Flags = 0;

    CU_CHECK(cuExternalMemoryGetMappedMipmappedArray(&cu->mmArr, cu->extMem, &mdesc));
    CU_CHECK(cuMipmappedArrayGetLevel(&cu->arr0, cu->mmArr, 0));

    CU_CHECK(cuModuleLoadDataEx(&cu->cuMod, PTX, 0, NULL, NULL));
    CU_CHECK(cuModuleGetFunction(&cu->cuFun, cu->cuMod, "rgba2nv12"));
}



static void alloc_nv12(Nv12Buf* nb, uint32_t w, uint32_t h, uint32_t pitch_align)
{
    uint32_t pitch = align_up(w, pitch_align);
    size_t y_bytes = (size_t)pitch * h;
    size_t uv_bytes = (size_t)pitch * (h / 2);
    size_t total = y_bytes + uv_bytes;
    CU_CHECK(cuMemAlloc(&nb->dptr, total));
    nb->pitch = pitch;
    nb->size = total;
}



static void alloc_rgba(RgbaBuf* rb, uint32_t w, uint32_t h, uint32_t pitch_align)
{
    uint32_t pitch = align_up(w * 4, pitch_align);
    size_t total = (size_t)pitch * h;
    CU_CHECK(cuMemAlloc(&rb->dptr, total));
    rb->pitch = pitch;
    rb->size = total;
}



static void copy_array_to_linear_rgba(CudaCtx* cu, RgbaBuf* rb, uint32_t w, uint32_t h)
{
    ANN(cu);
    CUDA_MEMCPY2D c2d = {0};
    c2d.srcMemoryType = CU_MEMORYTYPE_ARRAY;
    c2d.srcArray = cu->arr0;
    c2d.dstMemoryType = CU_MEMORYTYPE_DEVICE;
    c2d.dstDevice = rb->dptr;
    c2d.dstPitch = rb->pitch;
    c2d.WidthInBytes = w * 4;
    c2d.Height = h;
    CU_CHECK(cuMemcpy2DAsync(&c2d, cu->stream));
}



static void launch_rgba_to_nv12(CudaCtx* cu, RgbaBuf* rb, Nv12Buf* nb, uint32_t w, uint32_t h)
{
    const int Bx = 32, By = 16;
    int gx = ((int)w + Bx - 1) / Bx;
    int gy = ((int)h + By - 1) / By;

    void* args[] = {(void*)&rb->dptr,  (void*)&rb->pitch, (void*)&nb->dptr,
                    (void*)&nb->pitch, (void*)&w,         (void*)&h};

    unsigned int grid_x = (unsigned int)gx;
    unsigned int grid_y = (unsigned int)gy;
    unsigned int block_x = (unsigned int)Bx;
    unsigned int block_y = (unsigned int)By;

    CU_CHECK(cuLaunchKernel(
        cu->cuFun, grid_x, grid_y, 1, block_x, block_y, 1, 0, cu->stream, args, NULL));
    CU_CHECK(cuStreamSynchronize(cu->stream));
}



static void nvenc_load_api(void)
{
    dvz_memset(&g_nvenc, sizeof(g_nvenc), 0, sizeof(g_nvenc));
    g_nvenc.version = (uint32_t)NV_ENCODE_API_FUNCTION_LIST_VER;
    NVENC_API_CALL(NvEncodeAPICreateInstance(&g_nvenc));
}



static bool nvenc_guid_equal(const GUID* a, const GUID* b)
{
    ANN(a);
    ANN(b);
    return (
        a->Data1 == b->Data1 && a->Data2 == b->Data2 && a->Data3 == b->Data3 &&
        memcmp(a->Data4, b->Data4, sizeof(a->Data4)) == 0);
}



static bool nvenc_supports_codec(void* hEncoder, const GUID* codec)
{
    ANN(codec);
    if (hEncoder == NULL || g_nvenc.nvEncGetEncodeGUIDCount == NULL ||
        g_nvenc.nvEncGetEncodeGUIDs == NULL)
    {
        return false;
    }
    uint32_t count = 0;
    if (g_nvenc.nvEncGetEncodeGUIDCount(hEncoder, &count) != NV_ENC_SUCCESS || count == 0)
    {
        return false;
    }
    GUID* guids = (GUID*)dvz_calloc(count, sizeof(GUID));
    if (!guids)
    {
        return false;
    }
    uint32_t written = 0;
    bool ok = false;
    if (g_nvenc.nvEncGetEncodeGUIDs(hEncoder, guids, count, &written) == NV_ENC_SUCCESS)
    {
        for (uint32_t i = 0; i < written; ++i)
        {
            if (nvenc_guid_equal(&guids[i], codec))
            {
                ok = true;
                break;
            }
        }
    }
    dvz_free(guids);
    return ok;
}



static DvzNvencProfile nvenc_profile(DvzVideoCodec codec)
{
    DvzNvencProfile prof = {
        .codec_guid = &NV_ENC_CODEC_H264_GUID,
        .preset_guid = &NV_ENC_PRESET_P4_GUID,
        .tuning = NV_ENC_TUNING_INFO_HIGH_QUALITY,
        .qp_intra = 18,
        .qp_inter_p = 18,
        .qp_inter_b = 20,
    };

    if (codec == DVZ_VIDEO_CODEC_HEVC)
    {
        prof.codec_guid = &NV_ENC_CODEC_HEVC_GUID;
        prof.preset_guid = &NV_ENC_PRESET_P5_GUID;
        prof.qp_intra = 20;
        prof.qp_inter_p = 20;
        prof.qp_inter_b = 22;
    }

    return prof;
}



static void nvenc_open_session_cuda(NvEncCtx* nctx, CUcontext cuCtx)
{
    ANN(nctx);
    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS open = {0};
    open.version = (uint32_t)NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
    open.device = cuCtx;
    open.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
    open.apiVersion = NVENCAPI_VERSION;
    NVENC_API_CALL(g_nvenc.nvEncOpenEncodeSessionEx(&open, &nctx->hEncoder));
    nctx->api = g_nvenc;
}



static void nvenc_init_codec(
    NvEncCtx* nctx, DvzVideoCodec codec_sel, uint32_t width, uint32_t height, uint32_t fps)
{
    ANN(nctx);
    DvzNvencProfile prof = nvenc_profile(codec_sel);

    dvz_memset(&nctx->init, sizeof(nctx->init), 0, sizeof(nctx->init));
    nctx->init.version = (uint32_t)NV_ENC_INITIALIZE_PARAMS_VER;
    nctx->init.encodeGUID = *prof.codec_guid;
    nctx->init.presetGUID = *prof.preset_guid;
    nctx->init.encodeWidth = width;
    nctx->init.encodeHeight = height;
    nctx->init.maxEncodeWidth = width;
    nctx->init.maxEncodeHeight = height;
    nctx->init.darWidth = width;
    nctx->init.darHeight = height;
    nctx->init.frameRateNum = fps;
    nctx->init.frameRateDen = 1;
    nctx->init.enablePTD = 1;
    nctx->init.reportSliceOffsets = 0;
    nctx->init.enableEncodeAsync = 0;
    nctx->init.enableSubFrameWrite = 0;
    nctx->init.enableOutputInVidmem = 0;

    NV_ENC_PRESET_CONFIG pcfg = {0};
    pcfg.version = (uint32_t)NV_ENC_PRESET_CONFIG_VER;
    pcfg.presetCfg.version = (uint32_t)NV_ENC_CONFIG_VER;
    nctx->init.tuningInfo = prof.tuning;
    if (g_nvenc.nvEncGetEncodePresetConfigEx != NULL)
    {
        NVENC_API_CALL(g_nvenc.nvEncGetEncodePresetConfigEx(
            nctx->hEncoder, *prof.codec_guid, *prof.preset_guid, nctx->init.tuningInfo, &pcfg));
    }
    else
    {
        NVENC_API_CALL(g_nvenc.nvEncGetEncodePresetConfig(
            nctx->hEncoder, *prof.codec_guid, *prof.preset_guid, &pcfg));
    }

    nctx->encCfg = pcfg.presetCfg;
    nctx->encCfg.version = (uint32_t)NV_ENC_CONFIG_VER;
    nctx->encCfg.rcParams.enableLookahead = 0;
    nctx->encCfg.rcParams.lookaheadDepth = 0;
    nctx->encCfg.rcParams.multiPass = NV_ENC_MULTI_PASS_DISABLED;
    nctx->encCfg.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
    nctx->encCfg.rcParams.constQP.qpIntra = prof.qp_intra;
    nctx->encCfg.rcParams.constQP.qpInterP = prof.qp_inter_p;
    nctx->encCfg.rcParams.constQP.qpInterB = prof.qp_inter_b;
    nctx->init.encodeConfig = &nctx->encCfg;
    nctx->init.encodeConfig->profileGUID = (codec_sel == DVZ_VIDEO_CODEC_HEVC)
                                               ? NV_ENC_HEVC_PROFILE_MAIN_GUID
                                               : NV_ENC_H264_PROFILE_HIGH_GUID;
    nctx->init.encodeConfig->gopLength = fps * 2;
    nctx->init.encodeConfig->frameIntervalP = 1;

    NVENC_API_CALL(g_nvenc.nvEncInitializeEncoder(nctx->hEncoder, &nctx->init));
}



static void nvenc_create_bitstreams(NvEncCtx* nctx, NvEncIO* io, int count)
{
    for (int i = 0; i < count; i++)
    {
        NV_ENC_CREATE_BITSTREAM_BUFFER c = {0};
        c.version = (uint32_t)NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
        NVENC_API_CALL(g_nvenc.nvEncCreateBitstreamBuffer(nctx->hEncoder, &c));
        io->bitstreams[i] = c.bitstreamBuffer;
    }
}



static void nvenc_register_input(
    NvEncCtx* nctx, NvEncIO* io, CUdeviceptr nv12_base, uint32_t pitch, uint32_t w, uint32_t h)
{
    NV_ENC_REGISTER_RESOURCE rr = {0};
    rr.version = (uint32_t)NV_ENC_REGISTER_RESOURCE_VER;
    rr.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
    rr.resourceToRegister = (void*)nv12_base;
    rr.width = w;
    rr.height = h;
    rr.pitch = pitch;
    rr.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;
    rr.bufferUsage = 0;
    NVENC_API_CALL(g_nvenc.nvEncRegisterResource(nctx->hEncoder, &rr));
    io->regPtr = rr.registeredResource;
}



static NV_ENC_INPUT_PTR nvenc_map_input(NvEncCtx* nctx, NvEncIO* io)
{
    NV_ENC_MAP_INPUT_RESOURCE m = {0};
    m.version = (uint32_t)NV_ENC_MAP_INPUT_RESOURCE_VER;
    m.registeredResource = io->regPtr;
    NVENC_API_CALL(g_nvenc.nvEncMapInputResource(nctx->hEncoder, &m));
    io->mappedInput = m.mappedResource;
    return io->mappedInput;
}



static void nvenc_write_spspps(NvEncCtx* nctx, DvzVideoEncoder* enc, DvzVideoBackendNvenc* state)
{
    if (!g_nvenc.nvEncGetSequenceParams)
    {
        return;
    }
    uint8_t header[1024];
    uint32_t header_size = 0;
    NV_ENC_SEQUENCE_PARAM_PAYLOAD sps = {0};
    sps.version = (uint32_t)NV_ENC_SEQUENCE_PARAM_PAYLOAD_VER;
    sps.inBufferSize = sizeof(header);
    sps.spsppsBuffer = header;
    sps.outSPSPPSPayloadSize = &header_size;
    NVENCSTATUS st = g_nvenc.nvEncGetSequenceParams(nctx->hEncoder, &sps);
    if (st == NV_ENC_SUCCESS && header_size > 0 && enc)
    {
        if (enc->fp)
        {
            fwrite(header, 1, header_size, enc->fp);
        }
        dvz_video_encoder_on_sample(enc, header, header_size, NVENC_INVALID_OFFSET, 0, true);
    }
}



static void nvenc_encode_frame(
    DvzVideoEncoder* enc, DvzVideoBackendNvenc* state, uint32_t frame_idx, uint32_t duration)
{
    NvEncCtx* nctx = &state->nvenc;
    NV_ENC_PIC_PARAMS pp = {0};
    pp.version = (uint32_t)NV_ENC_PIC_PARAMS_VER;
    pp.inputBuffer = state->mapped_in;
    pp.bufferFmt = NV_ENC_BUFFER_FORMAT_NV12;
    pp.inputWidth = enc->cfg.width;
    pp.inputHeight = enc->cfg.height;
    pp.outputBitstream = state->bitstream;
    pp.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    pp.inputTimeStamp = frame_idx;
    if (frame_idx == 0)
    {
        pp.encodePicFlags |= NV_ENC_PIC_FLAG_FORCEIDR;
    }
    NVENC_API_CALL(g_nvenc.nvEncEncodePicture(nctx->hEncoder, &pp));

    NV_ENC_LOCK_BITSTREAM lb = {0};
    lb.version = (uint32_t)NV_ENC_LOCK_BITSTREAM_VER;
    lb.outputBitstream = state->bitstream;
    lb.doNotWait = 0;
    NVENC_API_CALL(g_nvenc.nvEncLockBitstream(nctx->hEncoder, &lb));

    uint64_t sample_offset = NVENC_INVALID_OFFSET;
    if (enc->fp && enc->mux == DVZ_VIDEO_MUX_MP4_POST)
    {
        long pos = ftello(enc->fp);
        if (pos >= 0)
        {
            sample_offset = (uint64_t)pos;
        }
    }
    if (enc->fp)
    {
        fwrite(lb.bitstreamBufferPtr, 1, lb.bitstreamSizeInBytes, enc->fp);
    }
    dvz_video_encoder_on_sample(
        enc, (const uint8_t*)lb.bitstreamBufferPtr, (uint32_t)lb.bitstreamSizeInBytes,
        sample_offset, duration, frame_idx == 0);

    NVENC_API_CALL(g_nvenc.nvEncUnlockBitstream(nctx->hEncoder, state->bitstream));
}



static void nvenc_flush(NvEncCtx* nctx)
{
    ANN(nctx);
    NV_ENC_PIC_PARAMS eos = {0};
    eos.version = (uint32_t)NV_ENC_PIC_PARAMS_VER;
    eos.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
    NVENC_API_CALL(g_nvenc.nvEncEncodePicture(nctx->hEncoder, &eos));
}



/*************************************************************************************************/
/*  Backend interface                                                                            */
/*************************************************************************************************/

static bool nvenc_probe(const DvzVideoEncoderConfig* cfg)
{
    (void)cfg;
    return true;
}



static int nvenc_init(DvzVideoEncoder* enc)
{
    ANN(enc);
    if (enc->backend_data)
    {
        return 0;
    }
    DvzVideoBackendNvenc* state =
        (DvzVideoBackendNvenc*)dvz_calloc(1, sizeof(DvzVideoBackendNvenc));
    ANN(state);
    enc->backend_data = state;
    nvenc_load_api();
    return 0;
}



static int nvenc_start(DvzVideoEncoder* enc)
{
    ANN(enc);
    DvzVideoBackendNvenc* state = nvenc_state(enc);
    ANN(state);

    cuda_import_vk_memory(
        &state->cuda, enc->cfg.width, enc->cfg.height, enc->memory_fd, enc->memory_size);
    state->cuda_ready = true;

    if (enc->wait_semaphore_fd >= 0)
    {
        CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC shdesc = {
            .type = CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD,
            .handle = {
                .fd = enc->wait_semaphore_fd,
            },
            .flags = 0,
        };
        CU_CHECK(cuImportExternalSemaphore(&state->wait_semaphore, &shdesc));
        state->wait_semaphore_ready = true;
#if OS_UNIX
        close(enc->wait_semaphore_fd);
#endif
        enc->wait_semaphore_fd = -1;
    }

    nvenc_open_session_cuda(&state->nvenc, state->cuda.cuCtx);
    if (!nvenc_supports_codec(state->nvenc.hEncoder, nvenc_profile(enc->cfg.codec).codec_guid))
    {
        log_warn("requested video codec not supported");
        return -1;
    }
    nvenc_init_codec(&state->nvenc, enc->cfg.codec, enc->cfg.width, enc->cfg.height, enc->cfg.fps);
    state->nvenc_ready = true;

    alloc_rgba(&state->rgba, enc->cfg.width, enc->cfg.height, PITCH_ALIGN);
    alloc_nv12(&state->nv12, enc->cfg.width, enc->cfg.height, PITCH_ALIGN);

    nvenc_register_input(
        &state->nvenc, &state->io, state->nv12.dptr, state->nv12.pitch, enc->cfg.width,
        enc->cfg.height);
    nvenc_create_bitstreams(&state->nvenc, &state->io, 1);
    state->bitstream = state->io.bitstreams[0];
    state->mapped_in = nvenc_map_input(&state->nvenc, &state->io);
    nvenc_write_spspps(&state->nvenc, enc, state);
    return 0;
}



static int nvenc_submit(DvzVideoEncoder* enc, uint64_t wait_value)
{
    ANN(enc);
    DvzVideoBackendNvenc* state = nvenc_state(enc);
    ANN(state);

    if (state->wait_semaphore_ready && wait_value > 0)
    {
        CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS wait_params = {0};
        wait_params.params.fence.value = wait_value;
        CU_CHECK(cuWaitExternalSemaphoresAsync(
            &state->wait_semaphore, &wait_params, 1, state->cuda.stream));
    }

    copy_array_to_linear_rgba(&state->cuda, &state->rgba, enc->cfg.width, enc->cfg.height);
    launch_rgba_to_nv12(&state->cuda, &state->rgba, &state->nv12, enc->cfg.width, enc->cfg.height);
    uint32_t duration = dvz_video_encoder_next_duration(enc);
    nvenc_encode_frame(enc, state, enc->frame_idx, duration);
    return 0;
}



static int nvenc_stop(DvzVideoEncoder* enc)
{
    ANN(enc);
    DvzVideoBackendNvenc* state = nvenc_state(enc);
    ANN(state);
    if (state->nvenc_ready)
    {
        nvenc_flush(&state->nvenc);
    }
    return 0;
}



static void nvenc_destroy(DvzVideoEncoder* enc)
{
    if (!enc || !enc->backend_data)
    {
        return;
    }
    DvzVideoBackendNvenc* state = nvenc_state(enc);
    nvenc_state_free(state);
    dvz_free(state);
    enc->backend_data = NULL;
}



const DvzVideoBackend DVZ_VIDEO_BACKEND_NVENC = {
    .name = "nvenc",
    .probe = nvenc_probe,
    .init = nvenc_init,
    .start = nvenc_start,
    .submit = nvenc_submit,
    .stop = nvenc_stop,
    .destroy = nvenc_destroy,
};
