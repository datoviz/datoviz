// drp.c — Datoviz Rendering Protocol (DRP) – Builder-oriented C API implementation

/*
 * Prototype only.
 *
 * This file is an exploratory implementation sketch for the future DRP2 API shape.
 * It is not part of the build graph and it is not the normative source of truth.
 */

#include "drp_api.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ==============================================================================================
 */
/*  Helpers */
/* ==============================================================================================
 */

static void* xmalloc(size_t n)
{
    void* p = malloc(n);
    assert(p || n == 0);
    return p;
}
static void* xcalloc(size_t n, size_t s)
{
    void* p = calloc(n, s);
    assert(p || n * s == 0);
    return p;
}
static void* xrealloc(void* p, size_t n)
{
    void* q = realloc(p, n);
    assert(q || n == 0);
    return q;
}

static char* xstrdup(const char* s)
{
    if (!s)
        return NULL;
    size_t n = strlen(s) + 1;
    char* d = (char*)xmalloc(n);
    memcpy(d, s, n);
    return d;
}

static char** dup_cstr_array(const char** src, uint32_t count)
{
    if (!src || count == 0)
        return NULL;
    char** out = (char**)xcalloc(count, sizeof(char*));
    for (uint32_t i = 0; i < count; ++i)
        out[i] = xstrdup(src[i]);
    return out;
}

static void free_cstr_array(char** arr, uint32_t count)
{
    if (!arr)
        return;
    for (uint32_t i = 0; i < count; ++i)
        free(arr[i]);
    free(arr);
}

/* ==============================================================================================
 */
/*  Internal command payloads */
/* ==============================================================================================
 */

typedef enum
{
    DRP_CMD_CREATE_BUFFER = 0,
    DRP_CMD_WRITE_BUFFER,
    DRP_CMD_CREATE_TEXTURE,
    DRP_CMD_CREATE_TEXTURE_VIEW,
    DRP_CMD_WRITE_TEXTURE,
    DRP_CMD_CREATE_SAMPLER,

    DRP_CMD_CREATE_BIND_GROUP_LAYOUT,
    DRP_CMD_CREATE_PIPELINE_LAYOUT,
    DRP_CMD_CREATE_BIND_GROUP,

    DRP_CMD_CREATE_SHADER_MODULE,

    DRP_CMD_CREATE_RENDER_PIPELINE,
    DRP_CMD_CREATE_COMPUTE_PIPELINE,

    DRP_CMD_BEGIN_COMMAND_ENCODER,
    DRP_CMD_FINISH_COMMAND_ENCODER,

    DRP_CMD_BEGIN_RENDER_PASS,
    DRP_CMD_END_RENDER_PASS,
    DRP_CMD_BEGIN_COMPUTE_PASS,
    DRP_CMD_END_COMPUTE_PASS,

    DRP_CMD_SET_PIPELINE,
    DRP_CMD_SET_BIND_GROUP,
    DRP_CMD_SET_VIEWPORT,
    DRP_CMD_SET_SCISSOR,
    DRP_CMD_SET_BLEND_CONSTANT,
    DRP_CMD_SET_STENCIL_REFERENCE,

    DRP_CMD_DRAW,
    DRP_CMD_DRAW_INDEXED,
    DRP_CMD_DRAW_INDIRECT,
    DRP_CMD_DRAW_INDEXED_INDIRECT,

    DRP_CMD_DISPATCH_WORKGROUPS,
    DRP_CMD_DISPATCH_WORKGROUPS_INDIRECT,

    DRP_CMD_COPY_BUFFER_TO_BUFFER,
    DRP_CMD_COPY_BUFFER_TO_TEXTURE,
    DRP_CMD_COPY_TEXTURE_TO_BUFFER,

    DRP_CMD_QUEUE_SUBMIT,

    DRP_CMD_RESOURCE_BARRIER,

} DRPCommandType;

/* --- Create buffer --------------------------------------------------------------------------- */
typedef struct
{
    DRPId id;
    uint64_t size;
    char** usage; /* deep-copied strings */
    uint32_t usage_count;
    bool mapped_at_creation;
} CmdCreateBuffer;

typedef struct
{
    DRPId buffer;
    uint64_t offset;
    void* data; /* deep copy */
    uint64_t size;
} CmdWriteBuffer;

/* --- Texture / view / write ------------------------------------------------------------------ */
typedef struct
{
    DRPId id;
    char* dimension;
    uint32_t width, height, depth_or_layers;
    char* format;
    char** usage; /* deep copy */
    uint32_t usage_count;
    uint32_t mip_level_count;
    uint32_t sample_count;
} CmdCreateTexture;

typedef struct
{
    DRPId id;
    DRPId texture;
    char* format;    /* may be NULL */
    char* dimension; /* may be NULL */
    char* aspect;    /* may be NULL */
    uint32_t mip_base, mip_count;
    uint32_t layer_base, layer_count;
} CmdCreateTextureView;

typedef struct
{
    DRPId texture;
    uint32_t x, y, z, width, height, depth;
    void* data; /* deep copy */
    uint64_t size;
    uint32_t bytes_per_row;
    uint32_t rows_per_image;
} CmdWriteTexture;

/* --- Sampler --------------------------------------------------------------------------------- */
typedef struct
{
    DRPId id;
    char* min_filter;
    char* mag_filter;
    char* address_u;
    char* address_v;
    char* address_w;
    char* mip_filter;
    float lod_min, lod_max, max_aniso;
} CmdCreateSampler;

/* --- Bind-group layout (builder result) ------------------------------------------------------ */
typedef struct
{
    uint32_t binding;
    char* visibility; /* union string */
    char* type;       /* "uniform"|"storage"|"read-only-storage" */
    bool has_dynamic_offs;
    uint64_t min_binding_size;
} BGLBuf;

typedef struct
{
    uint32_t binding;
    char* visibility;
    char* sample_type; /* "float","unfilterable-float","depth","sint","uint" */
    char* view_dim;    /* "2d","2d-array","cube","cube-array","3d" */
    bool multisampled;
} BGLTex;

typedef struct
{
    uint32_t binding;
    char* visibility;
    char* type; /* "filtering","non-filtering","comparison" */
} BGLSamp;

typedef struct
{
    DRPId id;
    BGLBuf* bufs;
    uint32_t nbufs;
    BGLTex* texs;
    uint32_t ntexs;
    BGLSamp* samps;
    uint32_t nsamps;
} CmdCreateBGL;

/* --- Pipeline layout (builder result) -------------------------------------------------------- */
typedef struct
{
    char* stages;
    uint32_t offset;
    uint32_t size;
} PCRange;

typedef struct
{
    DRPId id;
    DRPId* bgls;
    uint32_t nbgls;
    PCRange* ranges;
    uint32_t nranges;
} CmdCreatePipelineLayout;

/* --- Bind group (builder result) ------------------------------------------------------------- */
typedef enum
{
    BG_ITEM_BUFFER = 0,
    BG_ITEM_TEXVIEW,
    BG_ITEM_SAMPLER
} BGItemType;

typedef struct
{
    BGItemType type;
    uint32_t binding;
    union
    {
        struct
        {
            DRPId buffer;
            uint64_t offset;
            uint64_t size;
        } buf;
        DRPId view;
        DRPId sampler;
    } u;
} BGItem;

typedef struct
{
    DRPId id;
    DRPId layout;
    BGItem* items;
    uint32_t count;
} CmdCreateBindGroup;

/* --- Shader module --------------------------------------------------------------------------- */
typedef struct
{
    DRPId id;
    char* format; /* "wgsl"|"spirv"|"glsl" */
    void* code;   /* deep copy */
    uint64_t size;
} CmdCreateShaderModule;

/* --- Render pipeline (builder result) -------------------------------------------------------- */
typedef struct
{
    uint32_t location;
    char* format; /* "float32x3", ... */
    uint64_t offset;
} VPAttr;

typedef struct
{
    uint64_t stride;
    char* step_mode; /* "vertex"|"instance" */
    VPAttr* attrs;
    uint32_t nattrs;
} VPBuf;

typedef struct
{
    char* format;     /* attachment format */
    char* write_mask; /* "all"|"none"|... */
    bool has_blend;
    char* src_color;
    char* dst_color;
    char* op_color;
    char* src_alpha;
    char* dst_alpha;
    char* op_alpha;
} ColorTarget;

typedef struct
{
    DRPId id;
    DRPId layout;

    /* vertex */
    bool has_vertex;
    DRPId vs_module;
    char* vs_entry;
    VPBuf* vbufs;
    uint32_t nvbufs;

    /* fragment */
    bool has_fragment;
    DRPId fs_module;
    char* fs_entry;
    ColorTarget* cts;
    uint32_t ncts;

    /* primitive */
    char* topology;
    char* strip_index_fmt; /* or NULL */
    char* front_face;
    char* cull_mode;

    /* depth/stencil */
    bool depth_set;
    char* depth_format;
    bool depth_write;
    char* depth_compare;
    float depth_bias;
    float depth_bias_slope;
    float depth_bias_clamp;
    bool has_stencil_front;
    char* f_compare;
    char* f_fail;
    char* f_depth_fail;
    char* f_pass;
    bool has_stencil_back;
    char* b_compare;
    char* b_fail;
    char* b_depth_fail;
    char* b_pass;

    /* multisample */
    bool ms_set;
    uint32_t ms_count;
    uint32_t ms_mask;
    bool ms_alpha_cov;
} CmdCreateRenderPipeline;

/* --- Compute pipeline ------------------------------------------------------------------------ */
typedef struct
{
    DRPId id;
    DRPId layout;
    DRPId module;
    char* entry_point;
} CmdCreateComputePipeline;

/* --- Encoders & passes ----------------------------------------------------------------------- */
typedef struct
{
    DRPId encoder_id;
} CmdBeginEncoder;
typedef struct
{
    DRPId encoder_id;
    DRPId command_buffer_id;
} CmdFinishEncoder;

typedef struct
{
    DRPId view;
    char* load_op;
    char* store_op;
    bool has_clear;
    float r, g, b, a;
    bool has_resolve;
    DRPId resolve_view;
} RPColor;

typedef struct
{
    bool present;
    DRPId view;
    bool has_depth;
    char* depth_load_op;
    char* depth_store_op;
    float depth_clear;
    bool has_stencil;
    char* stencil_load_op;
    char* stencil_store_op;
    uint32_t stencil_clear;
} RPDepthStencil;

typedef struct
{
    DRPId pass_id;
    DRPId encoder_id;
    RPColor* colors;
    uint32_t ncolors;
    RPDepthStencil depth_stencil; /* present flag indicates if configured */
} CmdBeginRenderPass;

typedef struct
{
    DRPId pass_id;
} CmdSimplePass;

typedef struct
{
    DRPId pass_id;
    DRPId encoder_id;
} CmdBeginComputePass;

/* --- Per-pass commands ----------------------------------------------------------------------- */
typedef struct
{
    DRPId pass_id;
    DRPId pipeline;
} CmdSetPipeline;
typedef struct
{
    DRPId pass_id;
    uint32_t index;
    DRPId bind_group;
    uint32_t* dyn;
    uint32_t ndyn;
} CmdSetBindGroup;
typedef struct
{
    DRPId pass_id;
    float x, y, w, h, minD, maxD;
} CmdSetViewport;
typedef struct
{
    DRPId pass_id;
    uint32_t x, y, w, h;
} CmdSetScissor;
typedef struct
{
    DRPId pass_id;
    float r, g, b, a;
} CmdSetBlendConst;
typedef struct
{
    DRPId pass_id;
    uint32_t refv;
} CmdSetStencilRef;

typedef struct
{
    DRPId pass_id;
    uint32_t vtx, inst, first_vtx, first_inst;
} CmdDraw;
typedef struct
{
    DRPId pass_id;
    uint32_t idx, inst, first_idx;
    int32_t base_vtx;
    uint32_t first_inst;
} CmdDrawIndexed;
typedef struct
{
    DRPId pass_id;
    DRPId buf;
    uint64_t off;
    uint32_t cnt;
} CmdDrawIndirect;
typedef struct
{
    DRPId pass_id;
    DRPId buf;
    uint64_t off;
    uint32_t cnt;
} CmdDrawIndexedIndirect;

typedef struct
{
    DRPId pass_id;
    uint32_t x, y, z;
} CmdDispatch;
typedef struct
{
    DRPId pass_id;
    DRPId buf;
    uint64_t off;
} CmdDispatchIndirect;

/* --- Copies ---------------------------------------------------------------------------------- */
typedef struct
{
    DRPId src;
    uint64_t src_off;
    DRPId dst;
    uint64_t dst_off;
    uint64_t size;
} CmdCopyB2B;
typedef struct
{
    DRPId src;
    uint64_t src_off;
    DRPId dst_tex;
    uint32_t x, y, z, w, h, d, bpr, rpi;
} CmdCopyB2T;
typedef struct
{
    DRPId src_tex;
    uint32_t x, y, z, w, h, d;
    DRPId dst;
    uint64_t dst_off;
    uint32_t bpr, rpi;
} CmdCopyT2B;

/* --- Submit & barrier ------------------------------------------------------------------------ */
typedef struct
{
    DRPId queue;
    DRPId* cbs;
    uint32_t n;
} CmdQueueSubmit;
typedef struct
{
    DRPId resource;
    char* old_st;
    char* new_st;
    char* src_stg;
    char* dst_stg;
    char* src_acc;
    char* dst_acc;
} CmdResourceBarrier;

/* --- Command union ----------------------------------------------------------------------------
 */
struct DRPCommand
{
    DRPCommandType type;
    union
    {
        CmdCreateBuffer create_buffer;
        CmdWriteBuffer write_buffer;

        CmdCreateTexture create_texture;
        CmdCreateTextureView create_texture_view;
        CmdWriteTexture write_texture;

        CmdCreateSampler create_sampler;

        CmdCreateBGL create_bgl;
        CmdCreatePipelineLayout create_pl;
        CmdCreateBindGroup create_bg;

        CmdCreateShaderModule create_shader;

        CmdCreateRenderPipeline create_rp;
        CmdCreateComputePipeline create_cp;

        CmdBeginEncoder begin_enc;
        CmdFinishEncoder finish_enc;

        CmdBeginRenderPass begin_rp_pass;
        CmdSimplePass end_rp_pass;
        CmdBeginComputePass begin_cp_pass;
        CmdSimplePass end_cp_pass;

        CmdSetPipeline set_pipeline;
        CmdSetBindGroup set_bg;
        CmdSetViewport set_viewport;
        CmdSetScissor set_scissor;
        CmdSetBlendConst set_blend_const;
        CmdSetStencilRef set_stencil_ref;

        CmdDraw draw;
        CmdDrawIndexed draw_indexed;
        CmdDrawIndirect draw_indirect;
        CmdDrawIndexedIndirect draw_indexed_indirect;

        CmdDispatch dispatch;
        CmdDispatchIndirect dispatch_indirect;

        CmdCopyB2B copy_b2b;
        CmdCopyB2T copy_b2t;
        CmdCopyT2B copy_t2b;

        CmdQueueSubmit submit;

        CmdResourceBarrier barrier;
    } u;
};

/* ==============================================================================================
 */
/*  Builder storage */
/* ==============================================================================================
 */

typedef struct
{
    DRPId id;
    /* arrays of entries */
    BGLBuf* bufs;
    uint32_t nbufs, cbufs;
    BGLTex* texs;
    uint32_t ntexs, ctexs;
    BGLSamp* samps;
    uint32_t nsamps, csamps;
    bool in_use;
} PendingBGL;

typedef struct
{
    DRPId id;
    DRPId* bgls;
    uint32_t nbgls, cbgls;
    PCRange* ranges;
    uint32_t nranges, cranges;
    bool in_use;
} PendingPL;

typedef struct
{
    DRPId id;
    DRPId layout;
    BGItem* items;
    uint32_t count, cap;
    bool in_use;
} PendingBG;

typedef struct
{
    DRPId id;
    DRPId layout;

    /* vertex */
    bool has_vertex;
    DRPId vs_module;
    char* vs_entry;
    VPBuf* vbufs;
    uint32_t nvbufs, cvbufs;

    /* fragment */
    bool has_fragment;
    DRPId fs_module;
    char* fs_entry;
    ColorTarget* cts;
    uint32_t ncts, ccts;

    /* primitive */
    char* topology;
    char* strip_index_fmt;
    char* front_face;
    char* cull_mode;

    /* depth/stencil */
    bool depth_set;
    char* depth_format;
    bool depth_write;
    char* depth_compare;
    float depth_bias;
    float depth_bias_slope;
    float depth_bias_clamp;
    bool has_stencil_front;
    char* f_compare;
    char* f_fail;
    char* f_depth_fail;
    char* f_pass;
    bool has_stencil_back;
    char* b_compare;
    char* b_fail;
    char* b_depth_fail;
    char* b_pass;

    /* multisample */
    bool ms_set;
    uint32_t ms_count;
    uint32_t ms_mask;
    bool ms_alpha_cov;

    bool in_use;
} PendingRP;

typedef struct
{
    DRPId id;
    RPColor* colors;
    uint32_t ncolors, ccolors;
    RPDepthStencil ds;
    bool ds_set;
    bool in_use;
} PendingPass;

/* ==============================================================================================
 */
/*  Context */
/* ==============================================================================================
 */

struct DRPContext
{
    /* command stream */
    DRPCommand* cmds;
    uint32_t ncmds, ccmds;

    /* builders */
    PendingBGL* bgls;
    uint32_t nbgls, cbgls;
    PendingPL* pls;
    uint32_t npls, cpls;
    PendingBG* bgs;
    uint32_t nbgs, cbgs;
    PendingRP* rps;
    uint32_t nrps, crps;
    PendingPass* pbs;
    uint32_t npbs, cpbs;
};

/* ==============================================================================================
 */
/*  Dynamic array helpers for builders */
/* ==============================================================================================
 */

static PendingBGL* get_bgl(DRPContext* ctx, DRPId id, bool create)
{
    for (uint32_t i = 0; i < ctx->nbgls; i++)
        if (ctx->bgls[i].in_use && ctx->bgls[i].id == id)
            return &ctx->bgls[i];
    if (!create)
        return NULL;
    if (ctx->nbgls == ctx->cbgls)
    {
        ctx->cbgls = ctx->cbgls ? ctx->cbgls * 2 : 8;
        ctx->bgls = xrealloc(ctx->bgls, ctx->cbgls * sizeof(PendingBGL));
    }
    PendingBGL* b = &ctx->bgls[ctx->nbgls++];
    memset(b, 0, sizeof(*b));
    b->id = id;
    b->in_use = true;
    return b;
}
static PendingPL* get_pl(DRPContext* ctx, DRPId id, bool create)
{
    for (uint32_t i = 0; i < ctx->npls; i++)
        if (ctx->pls[i].in_use && ctx->pls[i].id == id)
            return &ctx->pls[i];
    if (!create)
        return NULL;
    if (ctx->npls == ctx->cpls)
    {
        ctx->cpls = ctx->cpls ? ctx->cpls * 2 : 8;
        ctx->pls = xrealloc(ctx->pls, ctx->cpls * sizeof(PendingPL));
    }
    PendingPL* p = &ctx->pls[ctx->npls++];
    memset(p, 0, sizeof(*p));
    p->id = id;
    p->in_use = true;
    return p;
}
static PendingBG* get_bg(DRPContext* ctx, DRPId id, bool create)
{
    for (uint32_t i = 0; i < ctx->nbgs; i++)
        if (ctx->bgs[i].in_use && ctx->bgs[i].id == id)
            return &ctx->bgs[i];
    if (!create)
        return NULL;
    if (ctx->nbgs == ctx->cbgs)
    {
        ctx->cbgs = ctx->cbgs ? ctx->cbgs * 2 : 8;
        ctx->bgs = xrealloc(ctx->bgs, ctx->cbgs * sizeof(PendingBG));
    }
    PendingBG* b = &ctx->bgs[ctx->nbgs++];
    memset(b, 0, sizeof(*b));
    b->id = id;
    b->in_use = true;
    return b;
}
static PendingRP* get_rp(DRPContext* ctx, DRPId id, bool create)
{
    for (uint32_t i = 0; i < ctx->nrps; i++)
        if (ctx->rps[i].in_use && ctx->rps[i].id == id)
            return &ctx->rps[i];
    if (!create)
        return NULL;
    if (ctx->nrps == ctx->crps)
    {
        ctx->crps = ctx->crps ? ctx->crps * 2 : 8;
        ctx->rps = xrealloc(ctx->rps, ctx->crps * sizeof(PendingRP));
    }
    PendingRP* p = &ctx->rps[ctx->nrps++];
    memset(p, 0, sizeof(*p));
    p->id = id;
    p->in_use = true;
    return p;
}
static PendingPass* get_pb(DRPContext* ctx, DRPId id, bool create)
{
    for (uint32_t i = 0; i < ctx->npbs; i++)
        if (ctx->pbs[i].in_use && ctx->pbs[i].id == id)
            return &ctx->pbs[i];
    if (!create)
        return NULL;
    if (ctx->npbs == ctx->cpbs)
    {
        ctx->cpbs = ctx->cpbs ? ctx->cpbs * 2 : 8;
        ctx->pbs = xrealloc(ctx->pbs, ctx->cpbs * sizeof(PendingPass));
    }
    PendingPass* p = &ctx->pbs[ctx->npbs++];
    memset(p, 0, sizeof(*p));
    p->id = id;
    p->in_use = true;
    return p;
}

/* ==============================================================================================
 */
/*  Command stream helpers */
/* ==============================================================================================
 */

static DRPCommand* push_cmd(DRPContext* ctx, DRPCommandType type)
{
    if (ctx->ncmds == ctx->ccmds)
    {
        ctx->ccmds = ctx->ccmds ? ctx->ccmds * 2 : 64;
        ctx->cmds = xrealloc(ctx->cmds, ctx->ccmds * sizeof(DRPCommand));
    }
    DRPCommand* c = &ctx->cmds[ctx->ncmds++];
    memset(c, 0, sizeof(*c));
    c->type = type;
    return c;
}

/* deep free a command payload */
static void free_cmd(DRPCommand* c)
{
    if (!c)
        return;
    switch (c->type)
    {
    case DRP_CMD_CREATE_BUFFER:
        free_cstr_array(c->u.create_buffer.usage, c->u.create_buffer.usage_count);
        break;
    case DRP_CMD_WRITE_BUFFER:
        free(c->u.write_buffer.data);
        break;
    case DRP_CMD_CREATE_TEXTURE:
    {
        CmdCreateTexture* t = &c->u.create_texture;
        free(t->dimension);
        free(t->format);
        free_cstr_array(t->usage, t->usage_count);
    }
    break;
    case DRP_CMD_CREATE_TEXTURE_VIEW:
    {
        CmdCreateTextureView* v = &c->u.create_texture_view;
        free(v->format);
        free(v->dimension);
        free(v->aspect);
    }
    break;
    case DRP_CMD_WRITE_TEXTURE:
        free(c->u.write_texture.data);
        break;

    case DRP_CMD_CREATE_SAMPLER:
    {
        CmdCreateSampler* s = &c->u.create_sampler;
        free(s->min_filter);
        free(s->mag_filter);
        free(s->address_u);
        free(s->address_v);
        free(s->address_w);
        free(s->mip_filter);
    }
    break;

    case DRP_CMD_CREATE_BIND_GROUP_LAYOUT:
    {
        CmdCreateBGL* b = &c->u.create_bgl;
        for (uint32_t i = 0; i < b->nbufs; i++)
        {
            free(b->bufs[i].visibility);
            free(b->bufs[i].type);
        }
        for (uint32_t i = 0; i < b->ntexs; i++)
        {
            free(b->texs[i].visibility);
            free(b->texs[i].sample_type);
            free(b->texs[i].view_dim);
        }
        for (uint32_t i = 0; i < b->nsamps; i++)
        {
            free(b->samps[i].visibility);
            free(b->samps[i].type);
        }
        free(b->bufs);
        free(b->texs);
        free(b->samps);
    }
    break;

    case DRP_CMD_CREATE_PIPELINE_LAYOUT:
    {
        CmdCreatePipelineLayout* p = &c->u.create_pl;
        free(p->bgls);
        for (uint32_t i = 0; i < p->nranges; i++)
            free(p->ranges[i].stages);
        free(p->ranges);
    }
    break;

    case DRP_CMD_CREATE_BIND_GROUP:
    {
        CmdCreateBindGroup* bg = &c->u.create_bg;
        free(bg->items);
    }
    break;

    case DRP_CMD_CREATE_SHADER_MODULE:
    {
        CmdCreateShaderModule* s = &c->u.create_shader;
        free(s->format);
        free(s->code);
    }
    break;

    case DRP_CMD_CREATE_RENDER_PIPELINE:
    {
        CmdCreateRenderPipeline* p = &c->u.create_rp;
        free(p->vs_entry);
        for (uint32_t i = 0; i < p->nvbufs; i++)
        {
            for (uint32_t j = 0; j < p->vbufs[i].nattrs; j++)
                free(p->vbufs[i].attrs[j].format);
            free(p->vbufs[i].attrs);
            free(p->vbufs[i].step_mode);
        }
        free(p->vbufs);
        free(p->fs_entry);
        for (uint32_t i = 0; i < p->ncts; i++)
        {
            free(p->cts[i].format);
            free(p->cts[i].write_mask);
            if (p->cts[i].has_blend)
            {
                free(p->cts[i].src_color);
                free(p->cts[i].dst_color);
                free(p->cts[i].op_color);
                free(p->cts[i].src_alpha);
                free(p->cts[i].dst_alpha);
                free(p->cts[i].op_alpha);
            }
        }
        free(p->cts);
        free(p->topology);
        free(p->strip_index_fmt);
        free(p->front_face);
        free(p->cull_mode);
        if (p->depth_set)
        {
            free(p->depth_format);
            free(p->depth_compare);
            if (p->has_stencil_front)
            {
                free(p->f_compare);
                free(p->f_fail);
                free(p->f_depth_fail);
                free(p->f_pass);
            }
            if (p->has_stencil_back)
            {
                free(p->b_compare);
                free(p->b_fail);
                free(p->b_depth_fail);
                free(p->b_pass);
            }
        }
    }
    break;

    case DRP_CMD_CREATE_COMPUTE_PIPELINE:
        free(c->u.create_cp.entry_point);
        break;

    case DRP_CMD_BEGIN_RENDER_PASS:
    {
        CmdBeginRenderPass* rp = &c->u.begin_rp_pass;
        for (uint32_t i = 0; i < rp->ncolors; i++)
        {
            free(rp->colors[i].load_op);
            free(rp->colors[i].store_op);
        }
        free(rp->colors);
        if (rp->depth_stencil.present)
        {
            if (rp->depth_stencil.has_depth)
            {
                free(rp->depth_stencil.depth_load_op);
                free(rp->depth_stencil.depth_store_op);
            }
            if (rp->depth_stencil.has_stencil)
            {
                free(rp->depth_stencil.stencil_load_op);
                free(rp->depth_stencil.stencil_store_op);
            }
        }
    }
    break;

    case DRP_CMD_SET_BIND_GROUP:
        free(c->u.set_bg.dyn);
        break;

    case DRP_CMD_QUEUE_SUBMIT:
        free(c->u.submit.cbs);
        break;

    case DRP_CMD_RESOURCE_BARRIER:
    {
        CmdResourceBarrier* b = &c->u.barrier;
        free(b->old_st);
        free(b->new_st);
        free(b->src_stg);
        free(b->dst_stg);
        free(b->src_acc);
        free(b->dst_acc);
    }
    break;

    default: /* POD only */
        break;
    }
}

/* ==============================================================================================
 */
/*  Public API */
/* ==============================================================================================
 */

DRPContext* drp_create_context(void) { return (DRPContext*)xcalloc(1, sizeof(DRPContext)); }

void drp_destroy_context(DRPContext* ctx)
{
    if (!ctx)
        return;
    /* free command stream */
    for (uint32_t i = 0; i < ctx->ncmds; i++)
        free_cmd(&ctx->cmds[i]);
    free(ctx->cmds);

    /* free builders */
    for (uint32_t i = 0; i < ctx->nbgls; i++)
    {
        PendingBGL* b = &ctx->bgls[i];
        if (!b->in_use)
            continue;
        for (uint32_t j = 0; j < b->nbufs; j++)
        {
            free(b->bufs[j].visibility);
            free(b->bufs[j].type);
        }
        for (uint32_t j = 0; j < b->ntexs; j++)
        {
            free(b->texs[j].visibility);
            free(b->texs[j].sample_type);
            free(b->texs[j].view_dim);
        }
        for (uint32_t j = 0; j < b->nsamps; j++)
        {
            free(b->samps[j].visibility);
            free(b->samps[j].type);
        }
        free(b->bufs);
        free(b->texs);
        free(b->samps);
    }
    for (uint32_t i = 0; i < ctx->npls; i++)
    {
        PendingPL* p = &ctx->pls[i];
        if (!p->in_use)
            continue;
        free(p->bgls);
        for (uint32_t j = 0; j < p->nranges; j++)
            free(p->ranges[j].stages);
        free(p->ranges);
    }
    for (uint32_t i = 0; i < ctx->nbgs; i++)
    { /* items are POD */
        free(ctx->bgs[i].items);
    }
    for (uint32_t i = 0; i < ctx->nrps; i++)
    {
        PendingRP* p = &ctx->rps[i];
        if (!p->in_use)
            continue;
        free(p->vs_entry);
        free(p->fs_entry);
        for (uint32_t vb = 0; vb < p->nvbufs; vb++)
        {
            for (uint32_t a = 0; a < p->vbufs[vb].nattrs; a++)
                free(p->vbufs[vb].attrs[a].format);
            free(p->vbufs[vb].attrs);
            free(p->vbufs[vb].step_mode);
        }
        free(p->vbufs);
        for (uint32_t ct = 0; ct < p->ncts; ct++)
        {
            free(p->cts[ct].format);
            free(p->cts[ct].write_mask);
            if (p->cts[ct].has_blend)
            {
                free(p->cts[ct].src_color);
                free(p->cts[ct].dst_color);
                free(p->cts[ct].op_color);
                free(p->cts[ct].src_alpha);
                free(p->cts[ct].dst_alpha);
                free(p->cts[ct].op_alpha);
            }
        }
        free(p->cts);
        free(p->topology);
        free(p->strip_index_fmt);
        free(p->front_face);
        free(p->cull_mode);
        if (p->depth_set)
        {
            free(p->depth_format);
            free(p->depth_compare);
            if (p->has_stencil_front)
            {
                free(p->f_compare);
                free(p->f_fail);
                free(p->f_depth_fail);
                free(p->f_pass);
            }
            if (p->has_stencil_back)
            {
                free(p->b_compare);
                free(p->b_fail);
                free(p->b_depth_fail);
                free(p->b_pass);
            }
        }
    }
    for (uint32_t i = 0; i < ctx->npbs; i++)
    {
        PendingPass* pb = &ctx->pbs[i];
        if (!pb->in_use)
            continue;
        for (uint32_t c = 0; c < pb->ncolors; c++)
        {
            free(pb->colors[c].load_op);
            free(pb->colors[c].store_op);
        }
        free(pb->colors);
        if (pb->ds_set)
        {
            if (pb->ds.has_depth)
            {
                free(pb->ds.depth_load_op);
                free(pb->ds.depth_store_op);
            }
            if (pb->ds.has_stencil)
            {
                free(pb->ds.stencil_load_op);
                free(pb->ds.stencil_store_op);
            }
        }
    }
    free(ctx->bgls);
    free(ctx->pls);
    free(ctx->bgs);
    free(ctx->rps);
    free(ctx->pbs);

    free(ctx);
}

const DRPCommand* drp_get_commands(const DRPContext* ctx, uint32_t* out_count)
{
    if (out_count)
        *out_count = ctx ? ctx->ncmds : 0;
    return ctx ? ctx->cmds : NULL;
}

void drp_clear(DRPContext* ctx)
{
    if (!ctx)
        return;
    for (uint32_t i = 0; i < ctx->ncmds; i++)
        free_cmd(&ctx->cmds[i]);
    ctx->ncmds = 0;
}

/* ==============================================================================================
 */
/*  Resource creation */
/* ==============================================================================================
 */

void drp_create_buffer(
    DRPContext* ctx, DRPId id, uint64_t size, const char** usage, uint32_t usage_count,
    bool mappedAtCreation)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_BUFFER);
    c->u.create_buffer.id = id;
    c->u.create_buffer.size = size;
    c->u.create_buffer.usage_count = usage_count;
    c->u.create_buffer.mapped_at_creation = mappedAtCreation;
    c->u.create_buffer.usage = dup_cstr_array(usage, usage_count);
}

void drp_write_buffer(
    DRPContext* ctx, DRPId buffer, uint64_t offset, const void* data, uint64_t size)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_WRITE_BUFFER);
    c->u.write_buffer.buffer = buffer;
    c->u.write_buffer.offset = offset;
    c->u.write_buffer.size = size;
    c->u.write_buffer.data = xmalloc((size_t)size);
    if (size)
        memcpy(c->u.write_buffer.data, data, (size_t)size);
}

void drp_create_texture(
    DRPContext* ctx, DRPId id, const char* dimension, uint32_t width, uint32_t height,
    uint32_t depth_or_layers, const char* format, const char** usage, uint32_t usage_count,
    uint32_t mip_level_count, uint32_t sample_count)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_TEXTURE);
    CmdCreateTexture* t = &c->u.create_texture;
    t->id = id;
    t->dimension = xstrdup(dimension);
    t->width = width;
    t->height = height;
    t->depth_or_layers = depth_or_layers;
    t->format = xstrdup(format);
    t->usage = dup_cstr_array(usage, usage_count);
    t->usage_count = usage_count;
    t->mip_level_count = mip_level_count;
    t->sample_count = sample_count;
}

void drp_create_texture_view(
    DRPContext* ctx, DRPId id, DRPId texture, const char* format, const char* dimension,
    const char* aspect, uint32_t mip_base, uint32_t mip_count, uint32_t layer_base,
    uint32_t layer_count)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_TEXTURE_VIEW);
    CmdCreateTextureView* v = &c->u.create_texture_view;
    v->id = id;
    v->texture = texture;
    v->format = xstrdup(format);
    v->dimension = xstrdup(dimension);
    v->aspect = xstrdup(aspect);
    v->mip_base = mip_base;
    v->mip_count = mip_count;
    v->layer_base = layer_base;
    v->layer_count = layer_count;
}

void drp_write_texture(
    DRPContext* ctx, DRPId texture, uint32_t x, uint32_t y, uint32_t z, uint32_t width,
    uint32_t height, uint32_t depth, const void* data, uint64_t size, uint32_t bpr, uint32_t rpi)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_WRITE_TEXTURE);
    CmdWriteTexture* t = &c->u.write_texture;
    t->texture = texture;
    t->x = x;
    t->y = y;
    t->z = z;
    t->width = width;
    t->height = height;
    t->depth = depth;
    t->bytes_per_row = bpr;
    t->rows_per_image = rpi;
    t->size = size;
    t->data = xmalloc((size_t)size);
    if (size)
        memcpy(t->data, data, (size_t)size);
}

void drp_create_sampler(
    DRPContext* ctx, DRPId id, const char* min_filter, const char* mag_filter,
    const char* address_mode_u, const char* address_mode_v, const char* address_mode_w,
    const char* mip_filter, float lod_min_clamp, float lod_max_clamp, float max_anisotropy)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_SAMPLER);
    CmdCreateSampler* s = &c->u.create_sampler;
    s->id = id;
    s->min_filter = xstrdup(min_filter);
    s->mag_filter = xstrdup(mag_filter);
    s->address_u = xstrdup(address_mode_u);
    s->address_v = xstrdup(address_mode_v);
    s->address_w = xstrdup(address_mode_w);
    s->mip_filter = xstrdup(mip_filter);
    s->lod_min = lod_min_clamp;
    s->lod_max = lod_max_clamp;
    s->max_aniso = max_anisotropy;
}

/* ==============================================================================================
 */
/*  Bind-group layout (builder) */
/* ==============================================================================================
 */

void drp_bgl_begin(DRPContext* ctx, DRPId id)
{
    PendingBGL* b = get_bgl(ctx, id, true);
    /* reset */
    for (uint32_t i = 0; i < b->nbufs; i++)
    {
        free(b->bufs[i].visibility);
        free(b->bufs[i].type);
    }
    for (uint32_t i = 0; i < b->ntexs; i++)
    {
        free(b->texs[i].visibility);
        free(b->texs[i].sample_type);
        free(b->texs[i].view_dim);
    }
    for (uint32_t i = 0; i < b->nsamps; i++)
    {
        free(b->samps[i].visibility);
        free(b->samps[i].type);
    }
    free(b->bufs);
    free(b->texs);
    free(b->samps);
    memset(b, 0, sizeof(*b));
    b->id = id;
    b->in_use = true;
}

void drp_bgl_add_buffer(
    DRPContext* ctx, DRPId id, uint32_t binding, const char* visibility, const char* type,
    bool has_dynamic_offs, uint64_t min_binding_size)
{
    PendingBGL* b = get_bgl(ctx, id, false);
    assert(b);
    if (b->nbufs == b->cbufs)
    {
        b->cbufs = b->cbufs ? b->cbufs * 2 : 4;
        b->bufs = xrealloc(b->bufs, b->cbufs * sizeof(BGLBuf));
    }
    BGLBuf* e = &b->bufs[b->nbufs++];
    e->binding = binding;
    e->visibility = xstrdup(visibility);
    e->type = xstrdup(type);
    e->has_dynamic_offs = has_dynamic_offs;
    e->min_binding_size = min_binding_size;
}

void drp_bgl_add_texture(
    DRPContext* ctx, DRPId id, uint32_t binding, const char* visibility, const char* sample_type,
    const char* view_dim, bool multisampled)
{
    PendingBGL* b = get_bgl(ctx, id, false);
    assert(b);
    if (b->ntexs == b->ctexs)
    {
        b->ctexs = b->ctexs ? b->ctexs * 2 : 4;
        b->texs = xrealloc(b->texs, b->ctexs * sizeof(BGLTex));
    }
    BGLTex* e = &b->texs[b->ntexs++];
    e->binding = binding;
    e->visibility = xstrdup(visibility);
    e->sample_type = xstrdup(sample_type);
    e->view_dim = xstrdup(view_dim);
    e->multisampled = multisampled;
}

void drp_bgl_add_sampler(
    DRPContext* ctx, DRPId id, uint32_t binding, const char* visibility, const char* type)
{
    PendingBGL* b = get_bgl(ctx, id, false);
    assert(b);
    if (b->nsamps == b->csamps)
    {
        b->csamps = b->csamps ? b->csamps * 2 : 4;
        b->samps = xrealloc(b->samps, b->csamps * sizeof(BGLSamp));
    }
    BGLSamp* e = &b->samps[b->nsamps++];
    e->binding = binding;
    e->visibility = xstrdup(visibility);
    e->type = xstrdup(type);
}

void drp_bgl_end(DRPContext* ctx, DRPId id)
{
    PendingBGL* b = get_bgl(ctx, id, false);
    assert(b);
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_BIND_GROUP_LAYOUT);
    CmdCreateBGL* out = &c->u.create_bgl;
    out->id = id;

    /* deep-copy entries into command */
    out->nbufs = b->nbufs;
    out->bufs = (BGLBuf*)xcalloc(out->nbufs, sizeof(BGLBuf));
    for (uint32_t i = 0; i < out->nbufs; i++)
    {
        out->bufs[i] = b->bufs[i];
        out->bufs[i].visibility = xstrdup(b->bufs[i].visibility);
        out->bufs[i].type = xstrdup(b->bufs[i].type);
    }

    out->ntexs = b->ntexs;
    out->texs = (BGLTex*)xcalloc(out->ntexs, sizeof(BGLTex));
    for (uint32_t i = 0; i < out->ntexs; i++)
    {
        out->texs[i] = b->texs[i];
        out->texs[i].visibility = xstrdup(b->texs[i].visibility);
        out->texs[i].sample_type = xstrdup(b->texs[i].sample_type);
        out->texs[i].view_dim = xstrdup(b->texs[i].view_dim);
    }

    out->nsamps = b->nsamps;
    out->samps = (BGLSamp*)xcalloc(out->nsamps, sizeof(BGLSamp));
    for (uint32_t i = 0; i < out->nsamps; i++)
    {
        out->samps[i] = b->samps[i];
        out->samps[i].visibility = xstrdup(b->samps[i].visibility);
        out->samps[i].type = xstrdup(b->samps[i].type);
    }

    /* clear builder */
    drp_bgl_begin(ctx, id); /* resets entries while keeping slot */
}

/* ==============================================================================================
 */
/*  Pipeline layout (builder) */
/* ==============================================================================================
 */

void drp_pipeline_layout_begin(DRPContext* ctx, DRPId id)
{
    PendingPL* p = get_pl(ctx, id, true);
    free(p->bgls);
    p->bgls = NULL;
    p->nbgls = p->cbgls = 0;
    for (uint32_t i = 0; i < p->nranges; i++)
        free(p->ranges[i].stages);
    free(p->ranges);
    p->ranges = NULL;
    p->nranges = p->cranges = 0;
}

void drp_pipeline_layout_add_bgl(DRPContext* ctx, DRPId id, DRPId bgl)
{
    PendingPL* p = get_pl(ctx, id, false);
    assert(p);
    if (p->nbgls == p->cbgls)
    {
        p->cbgls = p->cbgls ? p->cbgls * 2 : 4;
        p->bgls = xrealloc(p->bgls, p->cbgls * sizeof(DRPId));
    }
    p->bgls[p->nbgls++] = bgl;
}

void drp_pipeline_layout_add_push_constant(
    DRPContext* ctx, DRPId id, const char* stages, uint32_t offset, uint32_t size)
{
    PendingPL* p = get_pl(ctx, id, false);
    assert(p);
    if (p->nranges == p->cranges)
    {
        p->cranges = p->cranges ? p->cranges * 2 : 2;
        p->ranges = xrealloc(p->ranges, p->cranges * sizeof(PCRange));
    }
    PCRange* r = &p->ranges[p->nranges++];
    r->stages = xstrdup(stages);
    r->offset = offset;
    r->size = size;
}

void drp_pipeline_layout_end(DRPContext* ctx, DRPId id)
{
    PendingPL* p = get_pl(ctx, id, false);
    assert(p);
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_PIPELINE_LAYOUT);
    CmdCreatePipelineLayout* out = &c->u.create_pl;
    out->id = id;

    out->nbgls = p->nbgls;
    out->bgls = (DRPId*)xcalloc(out->nbgls, sizeof(DRPId));
    memcpy(out->bgls, p->bgls, out->nbgls * sizeof(DRPId));

    out->nranges = p->nranges;
    out->ranges = (PCRange*)xcalloc(out->nranges, sizeof(PCRange));
    for (uint32_t i = 0; i < out->nranges; i++)
    {
        out->ranges[i] = p->ranges[i];
        out->ranges[i].stages = xstrdup(p->ranges[i].stages);
    }

    drp_pipeline_layout_begin(ctx, id);
}

/* ==============================================================================================
 */
/*  Bind group (builder) */
/* ==============================================================================================
 */

void drp_bind_group_begin(DRPContext* ctx, DRPId id, DRPId layout)
{
    PendingBG* b = get_bg(ctx, id, true);
    free(b->items);
    memset(b, 0, sizeof(*b));
    b->id = id;
    b->layout = layout;
    b->in_use = true;
}

void drp_bind_group_add_buffer(
    DRPContext* ctx, DRPId id, uint32_t binding, DRPId buffer, uint64_t offset, uint64_t size)
{
    PendingBG* b = get_bg(ctx, id, false);
    assert(b);
    if (b->count == b->cap)
    {
        b->cap = b->cap ? b->cap * 2 : 4;
        b->items = xrealloc(b->items, b->cap * sizeof(BGItem));
    }
    BGItem* it = &b->items[b->count++];
    memset(it, 0, sizeof(*it));
    it->type = BG_ITEM_BUFFER;
    it->binding = binding;
    it->u.buf.buffer = buffer;
    it->u.buf.offset = offset;
    it->u.buf.size = size;
}

void drp_bind_group_add_texture_view(DRPContext* ctx, DRPId id, uint32_t binding, DRPId view)
{
    PendingBG* b = get_bg(ctx, id, false);
    assert(b);
    if (b->count == b->cap)
    {
        b->cap = b->cap ? b->cap * 2 : 4;
        b->items = xrealloc(b->items, b->cap * sizeof(BGItem));
    }
    BGItem* it = &b->items[b->count++];
    memset(it, 0, sizeof(*it));
    it->type = BG_ITEM_TEXVIEW;
    it->binding = binding;
    it->u.view = view;
}

void drp_bind_group_add_sampler(DRPContext* ctx, DRPId id, uint32_t binding, DRPId sampler)
{
    PendingBG* b = get_bg(ctx, id, false);
    assert(b);
    if (b->count == b->cap)
    {
        b->cap = b->cap ? b->cap * 2 : 4;
        b->items = xrealloc(b->items, b->cap * sizeof(BGItem));
    }
    BGItem* it = &b->items[b->count++];
    memset(it, 0, sizeof(*it));
    it->type = BG_ITEM_SAMPLER;
    it->binding = binding;
    it->u.sampler = sampler;
}

void drp_bind_group_end(DRPContext* ctx, DRPId id)
{
    PendingBG* b = get_bg(ctx, id, false);
    assert(b);
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_BIND_GROUP);
    CmdCreateBindGroup* out = &c->u.create_bg;
    out->id = id;
    out->layout = b->layout;
    out->count = b->count;
    out->items = (BGItem*)xcalloc(out->count, sizeof(BGItem));
    memcpy(out->items, b->items, out->count * sizeof(BGItem));

    /* reset builder */
    drp_bind_group_begin(ctx, id, b->layout);
}

/* ==============================================================================================
 */
/*  Shader modules */
/* ==============================================================================================
 */

void drp_create_shader_module(
    DRPContext* ctx, DRPId id, const char* format, const void* code, uint64_t size)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_SHADER_MODULE);
    c->u.create_shader.id = id;
    c->u.create_shader.format = xstrdup(format);
    c->u.create_shader.size = size;
    c->u.create_shader.code = xmalloc((size_t)size);
    if (size)
        memcpy(c->u.create_shader.code, code, (size_t)size);
}

/* ==============================================================================================
 */
/*  Render pipeline (builder) */
/* ==============================================================================================
 */

void drp_pipeline_begin(DRPContext* ctx, DRPId id, DRPId layout)
{
    PendingRP* p = get_rp(ctx, id, true);
    /* free existing */
    free(p->vs_entry);
    free(p->fs_entry);
    for (uint32_t vb = 0; vb < p->nvbufs; vb++)
    {
        for (uint32_t a = 0; a < p->vbufs[vb].nattrs; a++)
            free(p->vbufs[vb].attrs[a].format);
        free(p->vbufs[vb].attrs);
        free(p->vbufs[vb].step_mode);
    }
    free(p->vbufs);
    for (uint32_t ct = 0; ct < p->ncts; ct++)
    {
        free(p->cts[ct].format);
        free(p->cts[ct].write_mask);
        if (p->cts[ct].has_blend)
        {
            free(p->cts[ct].src_color);
            free(p->cts[ct].dst_color);
            free(p->cts[ct].op_color);
            free(p->cts[ct].src_alpha);
            free(p->cts[ct].dst_alpha);
            free(p->cts[ct].op_alpha);
        }
    }
    free(p->cts);
    free(p->topology);
    free(p->strip_index_fmt);
    free(p->front_face);
    free(p->cull_mode);
    if (p->depth_set)
    {
        free(p->depth_format);
        free(p->depth_compare);
        if (p->has_stencil_front)
        {
            free(p->f_compare);
            free(p->f_fail);
            free(p->f_depth_fail);
            free(p->f_pass);
        }
        if (p->has_stencil_back)
        {
            free(p->b_compare);
            free(p->b_fail);
            free(p->b_depth_fail);
            free(p->b_pass);
        }
    }
    memset(p, 0, sizeof(*p));
    p->id = id;
    p->layout = layout;
    p->in_use = true;
}

void drp_pipeline_vertex_module(DRPContext* ctx, DRPId id, DRPId module, const char* entry_point)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    p->has_vertex = true;
    p->vs_module = module;
    free(p->vs_entry);
    p->vs_entry = xstrdup(entry_point);
}

uint32_t drp_pipeline_add_vertex_buffer(
    DRPContext* ctx, DRPId id, uint64_t array_stride, const char* step_mode)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    if (p->nvbufs == p->cvbufs)
    {
        p->cvbufs = p->cvbufs ? p->cvbufs * 2 : 2;
        p->vbufs = xrealloc(p->vbufs, p->cvbufs * sizeof(VPBuf));
    }
    VPBuf* vb = &p->vbufs[p->nvbufs++];
    memset(vb, 0, sizeof(*vb));
    vb->stride = array_stride;
    vb->step_mode = xstrdup(step_mode);
    return p->nvbufs - 1;
}

void drp_pipeline_add_vertex_attribute(
    DRPContext* ctx, DRPId id, uint32_t buffer_index, uint32_t shader_location, const char* format,
    uint64_t offset)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    assert(buffer_index < p->nvbufs);
    VPBuf* vb = &p->vbufs[buffer_index];
    if (vb->nattrs == 0)
    {
        vb->attrs = NULL;
    }
    vb->attrs = xrealloc(vb->attrs, (vb->nattrs + 1) * sizeof(VPAttr));
    VPAttr* a = &vb->attrs[vb->nattrs++];
    a->location = shader_location;
    a->format = xstrdup(format);
    a->offset = offset;
}

void drp_pipeline_fragment_module(DRPContext* ctx, DRPId id, DRPId module, const char* entry_point)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    p->has_fragment = true;
    p->fs_module = module;
    free(p->fs_entry);
    p->fs_entry = xstrdup(entry_point);
}

uint32_t drp_pipeline_add_color_target(DRPContext* ctx, DRPId id, const char* format)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    if (p->ncts == p->ccts)
    {
        p->ccts = p->ccts ? p->ccts * 2 : 2;
        p->cts = xrealloc(p->cts, p->ccts * sizeof(ColorTarget));
    }
    ColorTarget* ct = &p->cts[p->ncts++];
    memset(ct, 0, sizeof(*ct));
    ct->format = xstrdup(format);
    ct->write_mask = xstrdup("all");
    return p->ncts - 1;
}

void drp_pipeline_set_color_target_blend(
    DRPContext* ctx, DRPId id, uint32_t target_idx, const char* src_color, const char* dst_color,
    const char* op_color, const char* src_alpha, const char* dst_alpha, const char* op_alpha)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    assert(target_idx < p->ncts);
    ColorTarget* ct = &p->cts[target_idx];
    ct->has_blend = true;
    free(ct->src_color);
    free(ct->dst_color);
    free(ct->op_color);
    free(ct->src_alpha);
    free(ct->dst_alpha);
    free(ct->op_alpha);
    ct->src_color = xstrdup(src_color);
    ct->dst_color = xstrdup(dst_color);
    ct->op_color = xstrdup(op_color);
    ct->src_alpha = xstrdup(src_alpha);
    ct->dst_alpha = xstrdup(dst_alpha);
    ct->op_alpha = xstrdup(op_alpha);
}

void drp_pipeline_set_color_target_write_mask(
    DRPContext* ctx, DRPId id, uint32_t target_idx, const char* mask)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    assert(target_idx < p->ncts);
    ColorTarget* ct = &p->cts[target_idx];
    free(ct->write_mask);
    ct->write_mask = xstrdup(mask);
}

void drp_pipeline_set_primitive(
    DRPContext* ctx, DRPId id, const char* topology, const char* strip_index_fmt,
    const char* front_face, const char* cull_mode)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    free(p->topology);
    free(p->strip_index_fmt);
    free(p->front_face);
    free(p->cull_mode);
    p->topology = xstrdup(topology);
    p->strip_index_fmt = strip_index_fmt ? xstrdup(strip_index_fmt) : NULL;
    p->front_face = xstrdup(front_face);
    p->cull_mode = xstrdup(cull_mode);
}

void drp_pipeline_set_depth_stencil(
    DRPContext* ctx, DRPId id, const char* format, bool depth_write_enabled,
    const char* depth_compare, float depth_bias, float depth_bias_slope, float depth_bias_clamp,
    bool has_stencil_front, const char* front_compare, const char* front_fail,
    const char* front_depth_fail, const char* front_pass, bool has_stencil_back,
    const char* back_compare, const char* back_fail, const char* back_depth_fail,
    const char* back_pass)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    p->depth_set = (format != NULL) || p->depth_set;
    free(p->depth_format);
    p->depth_format = format ? xstrdup(format) : NULL;
    p->depth_write = depth_write_enabled;
    free(p->depth_compare);
    p->depth_compare = depth_compare ? xstrdup(depth_compare) : NULL;
    p->depth_bias = depth_bias;
    p->depth_bias_slope = depth_bias_slope;
    p->depth_bias_clamp = depth_bias_clamp;

    p->has_stencil_front = has_stencil_front;
    if (has_stencil_front)
    {
        free(p->f_compare);
        free(p->f_fail);
        free(p->f_depth_fail);
        free(p->f_pass);
        p->f_compare = xstrdup(front_compare);
        p->f_fail = xstrdup(front_fail);
        p->f_depth_fail = xstrdup(front_depth_fail);
        p->f_pass = xstrdup(front_pass);
    }
    p->has_stencil_back = has_stencil_back;
    if (has_stencil_back)
    {
        free(p->b_compare);
        free(p->b_fail);
        free(p->b_depth_fail);
        free(p->b_pass);
        p->b_compare = xstrdup(back_compare);
        p->b_fail = xstrdup(back_fail);
        p->b_depth_fail = xstrdup(back_depth_fail);
        p->b_pass = xstrdup(back_pass);
    }
}

void drp_pipeline_set_multisample(
    DRPContext* ctx, DRPId id, uint32_t count, uint32_t mask, bool alpha_cov)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    p->ms_set = true;
    p->ms_count = count;
    p->ms_mask = mask;
    p->ms_alpha_cov = alpha_cov;
}

void drp_pipeline_end(DRPContext* ctx, DRPId id)
{
    PendingRP* p = get_rp(ctx, id, false);
    assert(p);
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_RENDER_PIPELINE);
    CmdCreateRenderPipeline* out = &c->u.create_rp;

    /* copy */
    out->id = p->id;
    out->layout = p->layout;

    out->has_vertex = p->has_vertex;
    out->vs_module = p->vs_module;
    out->vs_entry = xstrdup(p->vs_entry);
    out->nvbufs = p->nvbufs;
    out->vbufs = (VPBuf*)xcalloc(out->nvbufs, sizeof(VPBuf));
    for (uint32_t i = 0; i < out->nvbufs; i++)
    {
        out->vbufs[i].stride = p->vbufs[i].stride;
        out->vbufs[i].step_mode = xstrdup(p->vbufs[i].step_mode);
        out->vbufs[i].nattrs = p->vbufs[i].nattrs;
        out->vbufs[i].attrs = (VPAttr*)xcalloc(out->vbufs[i].nattrs, sizeof(VPAttr));
        for (uint32_t j = 0; j < out->vbufs[i].nattrs; j++)
        {
            out->vbufs[i].attrs[j].location = p->vbufs[i].attrs[j].location;
            out->vbufs[i].attrs[j].format = xstrdup(p->vbufs[i].attrs[j].format);
            out->vbufs[i].attrs[j].offset = p->vbufs[i].attrs[j].offset;
        }
    }

    out->has_fragment = p->has_fragment;
    out->fs_module = p->fs_module;
    out->fs_entry = xstrdup(p->fs_entry);
    out->ncts = p->ncts;
    out->cts = (ColorTarget*)xcalloc(out->ncts, sizeof(ColorTarget));
    for (uint32_t i = 0; i < out->ncts; i++)
    {
        out->cts[i].format = xstrdup(p->cts[i].format);
        out->cts[i].write_mask = xstrdup(p->cts[i].write_mask);
        out->cts[i].has_blend = p->cts[i].has_blend;
        if (out->cts[i].has_blend)
        {
            out->cts[i].src_color = xstrdup(p->cts[i].src_color);
            out->cts[i].dst_color = xstrdup(p->cts[i].dst_color);
            out->cts[i].op_color = xstrdup(p->cts[i].op_color);
            out->cts[i].src_alpha = xstrdup(p->cts[i].src_alpha);
            out->cts[i].dst_alpha = xstrdup(p->cts[i].dst_alpha);
            out->cts[i].op_alpha = xstrdup(p->cts[i].op_alpha);
        }
    }

    out->topology = xstrdup(p->topology);
    out->strip_index_fmt = p->strip_index_fmt ? xstrdup(p->strip_index_fmt) : NULL;
    out->front_face = xstrdup(p->front_face);
    out->cull_mode = xstrdup(p->cull_mode);

    out->depth_set = p->depth_set;
    if (out->depth_set)
    {
        out->depth_format = p->depth_format ? xstrdup(p->depth_format) : NULL;
        out->depth_write = p->depth_write;
        out->depth_compare = p->depth_compare ? xstrdup(p->depth_compare) : NULL;
        out->depth_bias = p->depth_bias;
        out->depth_bias_slope = p->depth_bias_slope;
        out->depth_bias_clamp = p->depth_bias_clamp;
        out->has_stencil_front = p->has_stencil_front;
        if (out->has_stencil_front)
        {
            out->f_compare = xstrdup(p->f_compare);
            out->f_fail = xstrdup(p->f_fail);
            out->f_depth_fail = xstrdup(p->f_depth_fail);
            out->f_pass = xstrdup(p->f_pass);
        }
        out->has_stencil_back = p->has_stencil_back;
        if (out->has_stencil_back)
        {
            out->b_compare = xstrdup(p->b_compare);
            out->b_fail = xstrdup(p->b_fail);
            out->b_depth_fail = xstrdup(p->b_depth_fail);
            out->b_pass = xstrdup(p->b_pass);
        }
    }

    out->ms_set = p->ms_set;
    out->ms_count = p->ms_count;
    out->ms_mask = p->ms_mask;
    out->ms_alpha_cov = p->ms_alpha_cov;

    /* reset builder for reuse */
    drp_pipeline_begin(ctx, id, p->layout);
}

/* ==============================================================================================
 */
/*  Compute pipeline */
/* ==============================================================================================
 */

void drp_create_compute_pipeline(
    DRPContext* ctx, DRPId id, DRPId layout, DRPId module, const char* entryPoint)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_CREATE_COMPUTE_PIPELINE);
    c->u.create_cp.id = id;
    c->u.create_cp.layout = layout;
    c->u.create_cp.module = module;
    c->u.create_cp.entry_point = xstrdup(entryPoint);
}

/* ==============================================================================================
 */
/*  Encoders */
/* ==============================================================================================
 */

void drp_begin_command_encoder(DRPContext* ctx, DRPId encoder_id)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_BEGIN_COMMAND_ENCODER);
    c->u.begin_enc.encoder_id = encoder_id;
}

void drp_finish_command_encoder(DRPContext* ctx, DRPId encoder_id, DRPId command_buffer_id)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_FINISH_COMMAND_ENCODER);
    c->u.finish_enc.encoder_id = encoder_id;
    c->u.finish_enc.command_buffer_id = command_buffer_id;
}

/* ==============================================================================================
 */
/*  Render pass (builder + begin/end) */
/* ==============================================================================================
 */

void drp_pass_builder_begin(DRPContext* ctx, DRPId pass_id)
{
    PendingPass* pb = get_pb(ctx, pass_id, true);
    for (uint32_t i = 0; i < pb->ncolors; i++)
    {
        free(pb->colors[i].load_op);
        free(pb->colors[i].store_op);
    }
    free(pb->colors);
    if (pb->ds_set)
    {
        if (pb->ds.has_depth)
        {
            free(pb->ds.depth_load_op);
            free(pb->ds.depth_store_op);
        }
        if (pb->ds.has_stencil)
        {
            free(pb->ds.stencil_load_op);
            free(pb->ds.stencil_store_op);
        }
    }
    memset(pb, 0, sizeof(*pb));
    pb->id = pass_id;
    pb->in_use = true;
}

void drp_pass_add_color_attachment(
    DRPContext* ctx, DRPId pass_id, DRPId view, const char* load_op, const char* store_op,
    bool has_clear, float r, float g, float b, float a, bool has_resolve, DRPId resolve_view)
{
    PendingPass* pb = get_pb(ctx, pass_id, false);
    assert(pb);
    if (pb->ncolors == pb->ccolors)
    {
        pb->ccolors = pb->ccolors ? pb->ccolors * 2 : 2;
        pb->colors = xrealloc(pb->colors, pb->ccolors * sizeof(RPColor));
    }
    RPColor* ca = &pb->colors[pb->ncolors++];
    memset(ca, 0, sizeof(*ca));
    ca->view = view;
    ca->load_op = xstrdup(load_op);
    ca->store_op = xstrdup(store_op);
    ca->has_clear = has_clear;
    ca->r = r;
    ca->g = g;
    ca->b = b;
    ca->a = a;
    ca->has_resolve = has_resolve;
    ca->resolve_view = resolve_view;
}

void drp_pass_set_depth_stencil(
    DRPContext* ctx, DRPId pass_id, DRPId view, bool has_depth, const char* depth_load_op,
    const char* depth_store_op, float depth_clear_value, bool has_stencil,
    const char* stencil_load_op, const char* stencil_store_op, uint32_t stencil_clear)
{
    PendingPass* pb = get_pb(ctx, pass_id, false);
    assert(pb);
    pb->ds_set = true;
    pb->ds.present = true;
    pb->ds.view = view;
    pb->ds.has_depth = has_depth;
    if (has_depth)
    {
        pb->ds.depth_load_op = xstrdup(depth_load_op);
        pb->ds.depth_store_op = xstrdup(depth_store_op);
        pb->ds.depth_clear = depth_clear_value;
    }
    pb->ds.has_stencil = has_stencil;
    if (has_stencil)
    {
        pb->ds.stencil_load_op = xstrdup(stencil_load_op);
        pb->ds.stencil_store_op = xstrdup(stencil_store_op);
        pb->ds.stencil_clear = stencil_clear;
    }
}

void drp_pass_begin(DRPContext* ctx, DRPId pass_id, DRPId encoder_id)
{
    PendingPass* pb = get_pb(ctx, pass_id, false);
    assert(pb);
    DRPCommand* c = push_cmd(ctx, DRP_CMD_BEGIN_RENDER_PASS);
    CmdBeginRenderPass* rp = &c->u.begin_rp_pass;
    rp->pass_id = pass_id;
    rp->encoder_id = encoder_id;

    rp->ncolors = pb->ncolors;
    rp->colors = (RPColor*)xcalloc(rp->ncolors, sizeof(RPColor));
    for (uint32_t i = 0; i < rp->ncolors; i++)
    {
        rp->colors[i] = pb->colors[i];
        rp->colors[i].load_op = xstrdup(pb->colors[i].load_op);
        rp->colors[i].store_op = xstrdup(pb->colors[i].store_op);
    }
    rp->depth_stencil.present = pb->ds.present;
    if (pb->ds.present)
    {
        rp->depth_stencil.view = pb->ds.view;
        rp->depth_stencil.has_depth = pb->ds.has_depth;
        if (pb->ds.has_depth)
        {
            rp->depth_stencil.depth_load_op = xstrdup(pb->ds.depth_load_op);
            rp->depth_stencil.depth_store_op = xstrdup(pb->ds.depth_store_op);
            rp->depth_stencil.depth_clear = pb->ds.depth_clear;
        }
        rp->depth_stencil.has_stencil = pb->ds.has_stencil;
        if (pb->ds.has_stencil)
        {
            rp->depth_stencil.stencil_load_op = xstrdup(pb->ds.stencil_load_op);
            rp->depth_stencil.stencil_store_op = xstrdup(pb->ds.stencil_store_op);
            rp->depth_stencil.stencil_clear = pb->ds.stencil_clear;
        }
    }
}

void drp_pass_end(DRPContext* ctx, DRPId pass_id)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_END_RENDER_PASS);
    c->u.end_rp_pass.pass_id = pass_id;
}

/* ==============================================================================================
 */
/*  Compute pass */
/* ==============================================================================================
 */

void drp_begin_compute_pass(DRPContext* ctx, DRPId pass_id, DRPId encoder_id)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_BEGIN_COMPUTE_PASS);
    c->u.begin_cp_pass.pass_id = pass_id;
    c->u.begin_cp_pass.encoder_id = encoder_id;
}

void drp_end_compute_pass(DRPContext* ctx, DRPId pass_id)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_END_COMPUTE_PASS);
    c->u.end_cp_pass.pass_id = pass_id;
}

/* ==============================================================================================
 */
/*  Common pass commands */
/* ==============================================================================================
 */

void drp_set_pipeline(DRPContext* ctx, DRPId pass_id, DRPId pipeline)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_SET_PIPELINE);
    c->u.set_pipeline.pass_id = pass_id;
    c->u.set_pipeline.pipeline = pipeline;
}

void drp_set_bind_group(
    DRPContext* ctx, DRPId pass_id, uint32_t index, DRPId bind_group,
    const uint32_t* dynamic_offsets, uint32_t offset_count)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_SET_BIND_GROUP);
    c->u.set_bg.pass_id = pass_id;
    c->u.set_bg.index = index;
    c->u.set_bg.bind_group = bind_group;
    c->u.set_bg.ndyn = offset_count;
    if (offset_count)
    {
        c->u.set_bg.dyn = (uint32_t*)xmalloc(offset_count * sizeof(uint32_t));
        memcpy(c->u.set_bg.dyn, dynamic_offsets, offset_count * sizeof(uint32_t));
    }
}

void drp_set_viewport(
    DRPContext* ctx, DRPId pass_id, float x, float y, float w, float h, float minD, float maxD)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_SET_VIEWPORT);
    c->u.set_viewport.pass_id = pass_id;
    c->u.set_viewport.x = x;
    c->u.set_viewport.y = y;
    c->u.set_viewport.w = w;
    c->u.set_viewport.h = h;
    c->u.set_viewport.minD = minD;
    c->u.set_viewport.maxD = maxD;
}

void drp_set_scissor(
    DRPContext* ctx, DRPId pass_id, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_SET_SCISSOR);
    c->u.set_scissor.pass_id = pass_id;
    c->u.set_scissor.x = x;
    c->u.set_scissor.y = y;
    c->u.set_scissor.w = w;
    c->u.set_scissor.h = h;
}

void drp_set_blend_constant(DRPContext* ctx, DRPId pass_id, float r, float g, float b, float a)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_SET_BLEND_CONSTANT);
    c->u.set_blend_const.pass_id = pass_id;
    c->u.set_blend_const.r = r;
    c->u.set_blend_const.g = g;
    c->u.set_blend_const.b = b;
    c->u.set_blend_const.a = a;
}

void drp_set_stencil_reference(DRPContext* ctx, DRPId pass_id, uint32_t reference)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_SET_STENCIL_REFERENCE);
    c->u.set_stencil_ref.pass_id = pass_id;
    c->u.set_stencil_ref.refv = reference;
}

void drp_draw(
    DRPContext* ctx, DRPId pass_id, uint32_t vtx_count, uint32_t inst_count, uint32_t first_vtx,
    uint32_t first_inst)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_DRAW);
    c->u.draw.pass_id = pass_id;
    c->u.draw.vtx = vtx_count;
    c->u.draw.inst = inst_count;
    c->u.draw.first_vtx = first_vtx;
    c->u.draw.first_inst = first_inst;
}

void drp_draw_indexed(
    DRPContext* ctx, DRPId pass_id, uint32_t idx_count, uint32_t inst_count, uint32_t first_idx,
    int32_t base_vtx, uint32_t first_inst)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_DRAW_INDEXED);
    c->u.draw_indexed.pass_id = pass_id;
    c->u.draw_indexed.idx = idx_count;
    c->u.draw_indexed.inst = inst_count;
    c->u.draw_indexed.first_idx = first_idx;
    c->u.draw_indexed.base_vtx = base_vtx;
    c->u.draw_indexed.first_inst = first_inst;
}

void drp_draw_indirect(
    DRPContext* ctx, DRPId pass_id, DRPId buffer, uint64_t offset, uint32_t count)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_DRAW_INDIRECT);
    c->u.draw_indirect.pass_id = pass_id;
    c->u.draw_indirect.buf = buffer;
    c->u.draw_indirect.off = offset;
    c->u.draw_indirect.cnt = count;
}

void drp_draw_indexed_indirect(
    DRPContext* ctx, DRPId pass_id, DRPId buffer, uint64_t offset, uint32_t count)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_DRAW_INDEXED_INDIRECT);
    c->u.draw_indexed_indirect.pass_id = pass_id;
    c->u.draw_indexed_indirect.buf = buffer;
    c->u.draw_indexed_indirect.off = offset;
    c->u.draw_indexed_indirect.cnt = count;
}

void drp_dispatch_workgroups(DRPContext* ctx, DRPId pass_id, uint32_t x, uint32_t y, uint32_t z)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_DISPATCH_WORKGROUPS);
    c->u.dispatch.pass_id = pass_id;
    c->u.dispatch.x = x;
    c->u.dispatch.y = y;
    c->u.dispatch.z = z;
}

void drp_dispatch_workgroups_indirect(
    DRPContext* ctx, DRPId pass_id, DRPId buffer, uint64_t offset)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_DISPATCH_WORKGROUPS_INDIRECT);
    c->u.dispatch_indirect.pass_id = pass_id;
    c->u.dispatch_indirect.buf = buffer;
    c->u.dispatch_indirect.off = offset;
}

/* ==============================================================================================
 */
/*  Copy commands */
/* ==============================================================================================
 */

void drp_copy_buffer_to_buffer(
    DRPContext* ctx, DRPId src, uint64_t src_offset, DRPId dst, uint64_t dst_offset, uint64_t size)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_COPY_BUFFER_TO_BUFFER);
    c->u.copy_b2b.src = src;
    c->u.copy_b2b.src_off = src_offset;
    c->u.copy_b2b.dst = dst;
    c->u.copy_b2b.dst_off = dst_offset;
    c->u.copy_b2b.size = size;
}

void drp_copy_buffer_to_texture(
    DRPContext* ctx, DRPId src, uint64_t src_offset, DRPId dst_texture, uint32_t x, uint32_t y,
    uint32_t z, uint32_t width, uint32_t height, uint32_t depth, uint32_t bpr, uint32_t rpi)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_COPY_BUFFER_TO_TEXTURE);
    c->u.copy_b2t.src = src;
    c->u.copy_b2t.src_off = src_offset;
    c->u.copy_b2t.dst_tex = dst_texture;
    c->u.copy_b2t.x = x;
    c->u.copy_b2t.y = y;
    c->u.copy_b2t.z = z;
    c->u.copy_b2t.w = width;
    c->u.copy_b2t.h = height;
    c->u.copy_b2t.d = depth;
    c->u.copy_b2t.bpr = bpr;
    c->u.copy_b2t.rpi = rpi;
}

void drp_copy_texture_to_buffer(
    DRPContext* ctx, DRPId src_texture, uint32_t x, uint32_t y, uint32_t z, uint32_t width,
    uint32_t height, uint32_t depth, DRPId dst, uint64_t dst_offset, uint32_t bpr, uint32_t rpi)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_COPY_TEXTURE_TO_BUFFER);
    c->u.copy_t2b.src_tex = src_texture;
    c->u.copy_t2b.x = x;
    c->u.copy_t2b.y = y;
    c->u.copy_t2b.z = z;
    c->u.copy_t2b.w = width;
    c->u.copy_t2b.h = height;
    c->u.copy_t2b.d = depth;
    c->u.copy_t2b.dst = dst;
    c->u.copy_t2b.dst_off = dst_offset;
    c->u.copy_t2b.bpr = bpr;
    c->u.copy_t2b.rpi = rpi;
}

/* ==============================================================================================
 */
/*  Submission & barriers */
/* ==============================================================================================
 */

void drp_queue_submit(DRPContext* ctx, DRPId queue, const DRPId* cmd_bufs, uint32_t count)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_QUEUE_SUBMIT);
    c->u.submit.queue = queue;
    c->u.submit.n = count;
    if (count)
    {
        c->u.submit.cbs = (DRPId*)xmalloc(count * sizeof(DRPId));
        memcpy(c->u.submit.cbs, cmd_bufs, count * sizeof(DRPId));
    }
}

void drp_resource_barrier(
    DRPContext* ctx, DRPId resource, const char* old_state, const char* new_state,
    const char* src_stage, const char* dst_stage, const char* src_access, const char* dst_access)
{
    DRPCommand* c = push_cmd(ctx, DRP_CMD_RESOURCE_BARRIER);
    c->u.barrier.resource = resource;
    c->u.barrier.old_st = xstrdup(old_state);
    c->u.barrier.new_st = xstrdup(new_state);
    c->u.barrier.src_stg = xstrdup(src_stage);
    c->u.barrier.dst_stg = xstrdup(dst_stage);
    c->u.barrier.src_acc = xstrdup(src_access);
    c->u.barrier.dst_acc = xstrdup(dst_access);
}
