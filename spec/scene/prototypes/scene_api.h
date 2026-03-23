#pragma once

/*
 * Prototype only.
 *
 * This file is an exploratory scene-API sketch kept under spec/.
 * The source of truth for the planning phase is the markdown material in spec/scene/.
 */

#include <stdbool.h>
#include <stdint.h>


/*************************************************************************************************/
/*  Forward declarations */
/*************************************************************************************************/

typedef struct DvzScene DvzScene;
typedef struct DvzPanel DvzPanel;
typedef struct DvzVisual DvzVisual;
typedef struct DvzCamera2D DvzCamera2D;
typedef struct DvzCamera3D DvzCamera3D;

typedef struct DvzResource DvzResource;
typedef struct DvzChannel DvzChannel;

typedef struct DvzEvent DvzEvent;
typedef struct DvzController DvzController;

typedef struct DvzFramegraph DvzFramegraph;
typedef struct DvzFramePass DvzFramePass;
typedef struct DvzFrameResource DvzFrameResource;

typedef struct DvzDrpBuilder DvzDrpBuilder;
typedef struct DvzPickInfo DvzPickInfo;
typedef struct DvzAnimation DvzAnimation;

/* From DRP frontend (opaque here, defined in drp.h). */
typedef struct DRPContext DRPContext;

/* Canonical scene id type (32-bit, matches DRPId). */
typedef uint32_t DvzId;

/*************************************************************************************************/
/*  Enumerations */
/*************************************************************************************************/

/** Visual channel mode: scalar/constant or attribute-bound. */
typedef enum
{
    DVZ_CHANNEL_CONSTANT = 0,
    DVZ_CHANNEL_ATTRIBUTE = 1,
} DvzChannelMode;

/** Visual stage (pipeline classification). */
typedef enum
{
    DVZ_VISUAL_STAGE_OPAQUE = 0,
    DVZ_VISUAL_STAGE_TRANSPARENT,
    DVZ_VISUAL_STAGE_OVERLAY,
    DVZ_VISUAL_STAGE_PICKING,
} DvzVisualStage;

/** Event type. */
typedef enum
{
    DVZ_EVENT_NONE = 0,
    DVZ_EVENT_MOUSE_MOVE,
    DVZ_EVENT_MOUSE_PRESS,
    DVZ_EVENT_MOUSE_RELEASE,
    DVZ_EVENT_MOUSE_WHEEL,
    DVZ_EVENT_KEY_PRESS,
    DVZ_EVENT_KEY_RELEASE,
    DVZ_EVENT_RESIZE,
    DVZ_EVENT_FRAME_TICK,
} DvzEventType;

/** Camera type. */
typedef enum
{
    DVZ_CAMERA_2D = 0,
    DVZ_CAMERA_3D,
} DvzCameraType;

typedef enum
{
    DVZ_ASPECT_FREE = 0,
    DVZ_ASPECT_FIXED,
    DVZ_ASPECT_EQUAL,
    DVZ_ASPECT_MATCH_X,
    DVZ_ASPECT_MATCH_Y,
} DvzAspectMode;

/** Pass type in the framegraph. */
typedef enum
{
    DVZ_PASS_RENDER = 0,
    DVZ_PASS_COMPUTE,
} DvzPassType;

/** Framegraph resource kind. */
typedef enum
{
    DVZ_FR_RES_NONE = 0,
    DVZ_FR_RES_CPU_BUFFER,   /* DvzResource* (CPU-side data → GPU buffer in DRP) */
    DVZ_FR_RES_COLOR_TARGET, /* per-panel color attachment for MAIN_COLOR pass   */
    DVZ_FR_RES_PICKING_TEX,  /* per-panel R32_UINT picking texture               */
} DvzFrameResType;

/** Scene resource kind (how it will map to GPU). */
typedef enum
{
    DVZ_RESOURCE_BUFFER = 0,
    DVZ_RESOURCE_TEXTURE,
} DvzResourceKind;

/** Channel element type (drives vertex format / packing). */
typedef enum
{
    DVZ_ELEM_F32 = 0,
    DVZ_ELEM_U32,
    DVZ_ELEM_I32,
    DVZ_ELEM_UN8_NORM,
} DvzElemType;

/*************************************************************************************************/
/*  Constants */
/*************************************************************************************************/

/* Indices for virtual per-panel targets used in framegraph/DRP wiring. */
enum
{
    DVZ_FRAME_TARGET_COLOR_MAIN = 0,
    DVZ_FRAME_TARGET_PICKING_ID = 1,
};

/*************************************************************************************************/
/*  Resources & Channels */
/*************************************************************************************************/

/* Dirty subrange descriptor for incremental uploads. */
typedef struct DvzDirtySpan
{
    uint32_t offset, size;
} DvzDirtySpan;

/* Logical buffer descriptor (scene-level, backend-agnostic). */
typedef struct DvzBufferDesc
{
    uint32_t usage_mask; /* vertex/index/uniform/storage/indirect (scene-side bitfield) */
} DvzBufferDesc;

/* Logical texture descriptor (scene-level, strings mirror DRP/WebGPU). */
typedef struct DvzTextureDesc
{
    const char* dimension; /* "1d","2d","3d" */
    uint32_t width, height, depth_or_layers;
    const char* format; /* e.g., "r32uint","rgba8unorm","rgba16float" */
    uint32_t mip_levels;
    uint32_t sample_count;
    uint32_t usage_mask; /* sampled/storage/render-target */
} DvzTextureDesc;

/** CPU-side resource of numerical data (vertex buffers, uniforms, textures, etc.).
 *
 * Data is always stored on the CPU; the DRP builder is responsible for creating/updating
 * the corresponding GPU object (buffer or texture) when `dirty` is true.
 */
struct DvzResource
{
    /* Optional scene-level id if the app wants to refer to this resource explicitly. */
    DvzId id;

    void* data;
    uint32_t size_bytes;
    uint32_t stride;      /* element stride for attributes, 0 for uniform blobs */
    DvzResourceKind kind; /* buffer or texture */

    /* Dirty tracking for incremental DRP updates. */
    bool dirty;       /* set by scene when content changes */
    uint32_t version; /* increments whenever modified */
    DvzDirtySpan dirty_spans[8];
    uint32_t dirty_span_count;

    /* GPU mapping (DRP-level identifiers). 0 means "not created yet". */
    uint32_t drp_id;

    /* Logical description for unambiguous DRP creation. */
    union
    {
        DvzBufferDesc buf;
        DvzTextureDesc tex;
    } desc;
};

/** Visual input channel: attribute or constant. */
struct DvzChannel
{
    DvzChannelMode mode;
    DvzResource* resource; /* only if ATTRIBUTE */

    union
    {
        float f;
        float v4[4];
        float m44[16];
    } constant;

    uint32_t location;     /* shader location index (semantic binding), not a DRP id */
    uint8_t components;    /* 1..4 (or 16 for mat4 when CONSTANT) */
    DvzElemType elem_type; /* element type for ATTRIBUTE */
};

/** Framegraph resource descriptor (lightweight descriptor used inside passes). */
struct DvzFrameResource
{
    DvzFrameResType type;

    union
    {
        DvzResource* cpu; /* when type == DVZ_FR_RES_CPU_BUFFER */
        uint32_t index;   /* for virtual textures: 0=main color, 1=picking */
    } u;
};

/*************************************************************************************************/
/*  Cameras */
/*************************************************************************************************/

struct DvzCamera2D
{
    float center[2];
    float scale_x;
    float scale_y;

    DvzAspectMode aspect_mode; /* FREE/FIXED/EQUAL/... */
    float aspect_ratio;

    float mat_view[16];
    float mat_proj[16];
};

struct DvzCamera3D
{
    float position[3];
    float target[3];
    float up[3];

    float fov;
    float near_clip;
    float far_clip;

    float mat_view[16];
    float mat_proj[16];
};

/*************************************************************************************************/
/*  Visuals */
/*************************************************************************************************/

/** Core visual structure (points, lines, mesh, image, text...). */
struct DvzVisual
{
    DvzId id;

    /* Channels */
    DvzChannel* channels;
    uint32_t channel_count;

    /* Transform chain placeholder (Phase 1: opaque to keep API stable). */
    void* transform_chain;

    /* Minimal render state knobs (replace former 'material' for v0.4). */
    bool depth_test;
    bool depth_write;
    const char* depth_compare; /* e.g., "less","always"; may be NULL for default */
    bool blend;                /* enable alpha blending */
    const char* blend_mode;    /* e.g., "alpha","premultiplied"; NULL for default */
    const char* cull_mode;     /* "none","back","front"; NULL for default */

    /* Render stage classification. */
    DvzVisualStage stage;

    /* Picking configuration. */
    bool enable_picking;
    DvzId picking_id;

    /* Assignment to passes (internal). */
    uint32_t pass_index;

    /* Optional CPU picking acceleration. */
    bool has_bounds;
    float bounds_min[3];
    float bounds_max[3];
    /* Optional per-visual ray test: returns true on hit and may fill info->distance. */
    bool (*ray_intersect)(
        const struct DvzVisual*, const float ray_origin[3], const float ray_dir[3],
        struct DvzPickInfo* out, void* user);
    void* pick_user;
};

/*************************************************************************************************/
/*  Panels */
/*************************************************************************************************/

/** Panel: rectangular region containing visuals + camera + local framegraph. */
struct DvzPanel
{
    DvzId id;

    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    float device_pixel_ratio; /* HiDPI scaling; 1.0 if unknown */

    /* Camera */
    DvzCameraType camera_type;
    union
    {
        DvzCamera2D cam2d;
        DvzCamera3D cam3d;
    } camera;

    /* Visuals */
    DvzVisual** visuals;
    uint32_t visual_count;

    /* Controllers */
    DvzController** controllers;
    uint32_t controller_count;

    /* Local framegraph (render + compute passes). */
    DvzFramegraph* framegraph;

    /* GPU picking configuration. */
    bool gpu_picking_enabled; /* if true, panel contributes to PICKING pass */
};

/*************************************************************************************************/
/*  Events & Controllers */
/*************************************************************************************************/

/** Event routed into the scene/panel/controller system. */
struct DvzEvent
{
    DvzEventType type;

    double timestamp;

    /* Coordinates normalized to panel-local space if applicable. */
    float x;
    float y;
    float dx;
    float dy;

    int button;
    int key;
    int modifiers;

    bool handled;    /* controllers may set this to stop propagation */
    DvzPanel* panel; /* panel receiving the event */
};

/** Interaction controller (pan/zoom/orbit/select/lasso...). */
struct DvzController
{
    /* Per-event update function. May modify panel camera, overlays, etc. */
    void (*update)(DvzController*, DvzPanel*, DvzEvent*);

    /* Optional destructor for user_data (may be NULL). */
    void (*destroy)(DvzController*);

    void* user_data;

    bool enabled;
};


struct DvzPickInfo
{
    DvzPanel* panel;
    DvzVisual* visual;
    DvzId id;
    float distance;
    float x;
    float y;
};

/*************************************************************************************************/
/*  Framegraph */
/*************************************************************************************************/

typedef struct DvzFrameEdge
{
    uint32_t src_pass, dst_pass;
} DvzFrameEdge;

struct DvzFramePass
{
    DvzPassType type;

    /* Optional debug name (not owned by the pass). */
    const char* debug_name;

    /* Visuals in this pass (render only). */
    DvzVisual** visuals;
    uint32_t visual_count;

    /* Resource dependencies */
    DvzFrameResource* read_resources;
    DvzFrameResource* write_resources;
    uint32_t read_count;
    uint32_t write_count;

    /* Whether this pass clears its color/depth targets. */
    bool clear;

    /* Viewport in panel coordinates (pixels). */
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;

    /* Compute dispatch size for compute passes (ignored for render). */
    uint32_t dispatch_x;
    uint32_t dispatch_y;
    uint32_t dispatch_z;
};

struct DvzFramegraph
{
    DvzFramePass* passes;
    uint32_t pass_count;
    /* Derived after build for debug/inspection. */
    DvzFrameEdge* edges;
    uint32_t edge_count;
};

/*************************************************************************************************/
/*  Scene */
/*************************************************************************************************/

struct DvzScene
{
    uint64_t frame_counter;
    double time_accum;

    /* Panels */
    DvzPanel** panels;
    uint32_t panel_count;

    /* Global resources */
    DvzResource** resources;
    uint32_t resource_count;

    /* Global animations */
    DvzAnimation** animations;
    uint32_t animation_count;

    /* Shader manager (placeholder). */
    void* shader_cache;

    /* Root framegraph builder (per frame). */
    DvzFramegraph* framegraph;

    /* DRP integration: scene emits commands into this context via dvz_scene_build().
     * The builder wraps a DRPContext* managed by the caller. */
    DvzDrpBuilder* drp_builder;
};


struct DvzAnimation
{
    void* target;
    const char* property;
    float duration;
    float elapsed;
    float (*easing)(float);
    void (*on_update)(DvzAnimation*, double);
    bool finished;
    bool loop;
};

/*************************************************************************************************/
/*  DRP Builder Interface (Phase 1) */
/*************************************************************************************************/

/** DRP builder: collects DRP commands for the core backend. */
struct DvzDrpBuilder
{
    /* Backing DRP context; ownership is external (core side). */
    DRPContext* ctx;
};


/**
 * Associate a DRP context with a builder.
 *
 * This must be called before dvz_scene_build() is used.
 */
void dvz_drp_builder_init(DvzDrpBuilder* builder, DRPContext* ctx);


/**
 * Retrieve the underlying DRP context.
 */
DRPContext* dvz_drp_builder_context(DvzDrpBuilder* builder);

/*************************************************************************************************/
/*  Scene lifecycle */
/*************************************************************************************************/

/**
 * Create a new scene.
 *
 * @return Pointer to a fresh DvzScene instance owned by the caller.
 */
DvzScene* dvz_scene_create(void);


/**
 * Destroy a scene and all its panels, visuals, resources and animations.
 *
 * @param scene Pointer to the scene to destroy.
 */
void dvz_scene_destroy(DvzScene* scene);


/**
 * Update the scene logic (animations, LOD, camera interactions).
 *
 * @param scene Pointer to the scene.
 * @param dt    Time delta since the last update, in seconds.
 */
void dvz_scene_update(DvzScene* scene, double dt);


/**
 * Build a DRP command stream for the current scene state.
 *
 * This function is responsible for:
 *   - creating/updating DRP buffers/textures from dirty DvzResource entries;
 *   - setting up per-panel framegraphs (MAIN_COLOR + PICKING when enabled);
 *   - emitting all passes (render + compute) into the DRPContext;
 *   - advancing frame_counter.
 *
 * @param scene Pointer to the scene.
 * @param drp   Builder that collects DRP commands.
 */
void dvz_scene_build(DvzScene* scene, DvzDrpBuilder* drp);


/*************************************************************************************************/
/*  Panel management */
/*************************************************************************************************/

/**
 * Create a panel within a scene.
 *
 * @param scene Owning scene.
 * @param x     Panel left coordinate.
 * @param y     Panel top coordinate.
 * @param w     Panel width.
 * @param h     Panel height.
 *
 * @return Pointer to the new panel.
 */
DvzPanel* dvz_panel(DvzScene* scene, int32_t x, int32_t y, uint32_t w, uint32_t h);


/**
 * Resize a panel (e.g. after window resize).
 *
 * @param panel Panel to resize.
 * @param w     New width.
 * @param h     New height.
 */
void dvz_panel_resize(DvzPanel* panel, uint32_t w, uint32_t h);


/**
 * Assign the camera type for a panel (2D or 3D).
 *
 * @param panel Panel to configure.
 * @param type  Camera type to assign.
 */
void dvz_panel_camera(DvzPanel* panel, DvzCameraType type);


/**
 * Attach a visual to the panel.
 *
 * @param panel  Panel receiving the visual.
 * @param visual Visual to add.
 */
void dvz_panel_visual(DvzPanel* panel, DvzVisual* visual);



void dvz_panel_remove_visual(DvzPanel* panel, DvzVisual* visual);


/**
 * Attach an interaction controller to the panel.
 *
 * @param panel      Target panel.
 * @param controller Controller instance.
 */
void dvz_panel_controller(DvzPanel* panel, DvzController* controller);


void dvz_panel_remove_controller(DvzPanel* panel, DvzController* controller);

/**
 * Enable or disable GPU picking on the panel.
 *
 * When enabled, the panel will be associated with a PICKING pass in the framegraph and
 * a per-panel picking texture will be wired through DRP.
 *
 * @param panel  Panel to configure.
 * @param enable True to enable GPU picking.
 */
void dvz_panel_gpu_picking(DvzPanel* panel, bool enable);


/*************************************************************************************************/
/*  Camera helpers */
/*************************************************************************************************/

/**
 * Update the center of a 2D camera.
 *
 * @param panel  Panel owning the camera.
 * @param center Two-component center coordinate.
 */
void dvz_camera2d_center(DvzPanel* panel, const float center[2]);


/**
 * Update the scale of a 2D camera.
 *
 * @param panel   Panel owning the camera.
 * @param scale_x Scale factor along X.
 * @param scale_y Scale factor along Y.
 */
void dvz_camera2d_scale(DvzPanel* panel, float scale_x, float scale_y);


/**
 * Set the aspect behavior for a 2D camera.
 *
 * @param panel        Panel owning the camera.
 * @param aspect_mode  How the aspect ratio is enforced.
 * @param aspect_ratio Target aspect ratio when applicable.
 */
void dvz_camera2d_aspect(DvzPanel* panel, DvzAspectMode aspect_mode, float aspect_ratio);


/**
 * Update the position of a 3D camera.
 *
 * @param panel    Panel owning the camera.
 * @param position Three-component world position.
 */
void dvz_camera3d_position(DvzPanel* panel, const float position[3]);


/**
 * Update the target of a 3D camera.
 *
 * @param panel  Panel owning the camera.
 * @param target Three-component look-at target.
 */
void dvz_camera3d_target(DvzPanel* panel, const float target[3]);


/**
 * Update the up vector of a 3D camera.
 *
 * @param panel Panel owning the camera.
 * @param up    Three-component up vector.
 */
void dvz_camera3d_up(DvzPanel* panel, const float up[3]);


/**
 * Configure the projection parameters of a 3D camera.
 *
 * @param panel     Panel owning the camera.
 * @param fov       Field of view in degrees.
 * @param near_clip Near clipping distance.
 * @param far_clip  Far clipping distance.
 */
void dvz_camera3d_perspective(DvzPanel* panel, float fov, float near_clip, float far_clip);


/**
 * Recompute the view and projection matrices for the active camera.
 *
 * @param panel Panel owning the camera.
 */
void dvz_camera_update(DvzPanel* panel);


/*************************************************************************************************/
/*  Visual configuration */
/*************************************************************************************************/

/**
 * Create a visual with the requested number of channels.
 *
 * @param scene         Owning scene.
 * @param channel_count Number of channels.
 *
 * @return Pointer to the new visual.
 */
DvzVisual* dvz_visual(DvzScene* scene, uint32_t channel_count);


/**
 * Set the render stage for a visual (opaque, transparent, overlay, picking).
 *
 * @param visual Visual to configure.
 * @param stage  Stage classification.
 */
void dvz_visual_stage(DvzVisual* visual, DvzVisualStage stage);


/**
 * Configure a visual channel with a constant value.
 *
 * @param v     Visual instance.
 * @param idx   Channel index.
 * @param value Pointer to the constant data.
 * @param dim   Dimension of the constant (1-4, or 16 for 4x4 matrix).
 */
void dvz_visual_channel_constant(DvzVisual* v, uint32_t idx, const float* value, uint32_t dim);


/**
 * Bind a CPU-side resource to a visual channel.
 *
 * @param v        Visual instance.
 * @param idx      Channel index.
 * @param resource Resource to bind.
 */
void dvz_visual_channel_attribute(DvzVisual* v, uint32_t idx, DvzResource* resource);


/**
 * Enable or disable picking for a visual.
 *
 * @param v      Visual instance.
 * @param enable True to enable picking.
 * @param id     Picking identifier when enabled.
 */
void dvz_visual_picking(DvzVisual* v, bool enable, uint32_t id);


void dvz_visual_picking(DvzVisual* v, bool enable, DvzId id);
void dvz_visual_visible(DvzVisual* v, bool visible);
void dvz_visual_sort_key(DvzVisual* v, uint32_t key);
void dvz_visual_bounds_aabb(DvzVisual* v, const float min3[3], const float max3[3]);
void dvz_visual_cpu_picker(
    DvzVisual* v,
    bool (*ray_intersect)(
        const DvzVisual*, const float[3], const float[3], struct DvzPickInfo*, void*),
    void* user);


/*************************************************************************************************/
/*  Resource management */
/*************************************************************************************************/

/**
 * Create a CPU-side resource (vertex buffer, uniform, texture, etc.).
 *
 * @param scene      Owning scene.
 * @param kind       Resource kind (buffer or texture).
 * @param size_bytes Size in bytes.
 * @param stride     Element stride for attributes (0 for uniform blobs).
 *
 * @return Pointer to the created resource.
 */
DvzResource*
dvz_resource(DvzScene* scene, DvzResourceKind kind, uint32_t size_bytes, uint32_t stride);


/**
 * Update the contents of a resource.
 *
 * This copies `size_bytes` into the resource, marks it dirty and increments its version.
 *
 * @param r           Resource to update.
 * @param data        Pointer to source data.
 * @param size_bytes  Size in bytes (must be <= r->size_bytes).
 */
void dvz_resource_update(DvzResource* r, const void* data, uint32_t size_bytes);


/**
 * Mark a resource dirty so its data is reuploaded.
 *
 * @param r Resource to mark dirty.
 */
void dvz_resource_dirty(DvzResource* r);

void dvz_resource_mark_span(DvzResource* r, uint32_t offset, uint32_t size);
void dvz_resource_config_buffer(DvzResource* r, uint32_t usage_mask);
void dvz_resource_config_texture(
    DvzResource* r, const char* dimension, uint32_t w, uint32_t h, uint32_t d_or_layers,
    const char* format, uint32_t mip_levels, uint32_t sample_count, uint32_t usage_mask);


/*************************************************************************************************/
/*  Event & controller helpers */
/*************************************************************************************************/

/**
 * Dispatch an event through the scene.
 *
 * @param scene Target scene.
 * @param ev    Event to dispatch. Controllers may modify `ev->handled`.
 */
void dvz_scene_event(DvzScene* scene, DvzEvent* ev);


/**
 * Create a controller with the provided update callback and user data.
 *
 * @param update    Callback executed when the controller processes an event.
 * @param destroy   Optional destructor for user_data (may be NULL).
 * @param user_data User data forwarded to the callback.
 *
 * @return Pointer to the allocated controller.
 */
DvzController* dvz_controller_create(
    void (*update)(DvzController*, DvzPanel*, DvzEvent*), void (*destroy)(DvzController*),
    void* user_data);


/**
 * Destroy a controller and release associated resources.
 *
 * @param controller Controller to destroy.
 */
void dvz_controller_destroy(DvzController* controller);


/**
 * Ask the panel to propagate an event to its controllers.
 *
 * @param panel Panel receiving the event.
 * @param ev    Event to process.
 */
void dvz_panel_controller_update(DvzPanel* panel, DvzEvent* ev);


/**
 * Perform CPU picking for coordinates within the scene.
 *
 * @param scene Scene to search in.
 * @param x     Normalized panel-local X coordinate.
 * @param y     Normalized panel-local Y coordinate.
 * @param info  Output pick result (must not be NULL).
 *
 * @return True when a visual was hit.
 */
bool dvz_scene_pick(const DvzScene* scene, float x, float y, DvzPickInfo* info);


/**
 * Perform picking within a single panel.
 *
 * @param panel Panel to test.
 * @param x     Normalized panel-local X coordinate.
 * @param y     Normalized panel-local Y coordinate.
 * @param info  Output pick result (must not be NULL).
 *
 * @return True when a visual was hit.
 */
bool dvz_panel_pick(const DvzPanel* panel, float x, float y, DvzPickInfo* info);


/**
 * Perform GPU-based picking for the scene.
 *
 * The implementation is expected to:
 *   - ensure the PICKING pass has rendered an ID buffer for the panel;
 *   - issue a DRP CopyTextureToBuffer for the requested pixel (implementation detail);
 *   - interpret the resulting ID and fill `info`.
 *
 * @param scene Scene to search in.
 * @param x     Normalized panel-local X coordinate.
 * @param y     Normalized panel-local Y coordinate.
 * @param info  Output pick result (must not be NULL).
 *
 * @return True when a visual was hit.
 */
bool dvz_scene_pick_gpu(const DvzScene* scene, float x, float y, DvzPickInfo* info);


/**
 * Perform GPU-based picking within a single panel.
 *
 * @param panel Panel to test.
 * @param x     Normalized panel-local X coordinate.
 * @param y     Normalized panel-local Y coordinate.
 * @param info  Output pick result (must not be NULL).
 *
 * @return True when a visual was hit.
 */
bool dvz_panel_pick_gpu(const DvzPanel* panel, float x, float y, DvzPickInfo* info);


/*************************************************************************************************/
/*  Framegraph helpers (internal but visible) */
/*************************************************************************************************/

/**
 * Build the panel framegraph prior to DRP generation.
 *
 * This configures at least:
 *   - MAIN_COLOR render pass targeting DVZ_FRAME_TARGET_COLOR_MAIN;
 *   - optional PICKING render pass targeting DVZ_FRAME_TARGET_PICKING_ID when enabled.
 *
 * @param panel Panel to build the framegraph for.
 */
void dvz_framegraph_build(DvzPanel* panel);


/**
 * Create a framegraph pass and append it to the framegraph.
 *
 * @param framegraph Target framegraph.
 * @param type       Pass type (render or compute).
 * @param debug_name Optional debug name (may be NULL).
 *
 * @return Pointer to the appended pass.
 */
DvzFramePass*
dvz_framegraph_pass(DvzFramegraph* framegraph, DvzPassType type, const char* debug_name);


/**
 * Append a visual to a frame pass.
 *
 * @param pass   Pass to update.
 * @param visual Visual to append.
 */
void dvz_framepass_add_visual(DvzFramePass* pass, DvzVisual* visual);


/**
 * Append a resource as a read dependency for a pass.
 *
 * @param pass     Pass to update.
 * @param resource Resource descriptor.
 */
void dvz_framepass_add_read(DvzFramePass* pass, const DvzFrameResource* resource);


/**
 * Append a resource as a write dependency for a pass.
 *
 * @param pass     Pass to update.
 * @param resource Resource descriptor.
 */
void dvz_framepass_add_write(DvzFramePass* pass, const DvzFrameResource* resource);


/**
 * Set whether a pass clears its attachments.
 *
 * @param pass  Pass to configure.
 * @param clear True to clear, false to leave contents intact.
 */
void dvz_framepass_set_clear(DvzFramePass* pass, bool clear);


/**
 * Set the viewport of a pass in panel pixel coordinates.
 *
 * @param pass   Pass to configure.
 * @param x      Left coordinate.
 * @param y      Top coordinate.
 * @param width  Width in pixels.
 * @param height Height in pixels.
 */
void dvz_framepass_set_viewport(
    DvzFramePass* pass, int32_t x, int32_t y, uint32_t width, uint32_t height);


/**
 * Configure compute dispatch size for a compute pass.
 *
 * @param pass Pass to configure.
 * @param nx   Workgroup count in X.
 * @param ny   Workgroup count in Y.
 * @param nz   Workgroup count in Z.
 */
void dvz_framepass_set_dispatch(DvzFramePass* pass, uint32_t nx, uint32_t ny, uint32_t nz);


/**
 * Create a CPU-side frame resource descriptor.
 *
 * @param resource Resource that backs the frame buffer.
 *
 * @return Frame resource descriptor.
 */
DvzFrameResource dvz_frame_resource_cpu(DvzResource* resource);


/**
 * Create a virtual texture descriptor (color/picking attachment).
 *
 * @param type  Frame resource type.
 * @param index Attachment index (per-panel color/picking target).
 *
 * @return Frame resource descriptor.
 */
DvzFrameResource dvz_frame_resource_target(DvzFrameResType type, uint32_t index);


/* Customization & inspection helpers */
const DvzFramegraph* dvz_panel_framegraph(const DvzPanel* panel);
void dvz_framegraph_depend(DvzFramegraph* fg, uint32_t src_pass_index, uint32_t dst_pass_index);



/*************************************************************************************************/
/*  Animation helpers */
/*************************************************************************************************/

/**
 * Create an animation descriptor.
 *
 * @param target    Animation target (camera, visual, etc.).
 * @param property  Property name (for logging/debug).
 * @param duration  Duration in seconds.
 * @param easing    Easing function (may be NULL for linear).
 * @param on_update Optional callback invoked each update tick.
 *
 * @return Pointer to the animation descriptor.
 */
DvzAnimation* dvz_animation_create(
    void* target, const char* property, float duration, float (*easing)(float),
    void (*on_update)(DvzAnimation*, double));


/**
 * Destroy an animation descriptor.
 *
 * @param animation Animation to destroy.
 */
void dvz_animation_destroy(DvzAnimation* animation);


/**
 * Register an animation inside the scene scheduler.
 *
 * @param scene     Scene owning the animation.
 * @param animation Animation to add.
 */
void dvz_scene_animation_add(DvzScene* scene, DvzAnimation* animation);


/**
 * Remove an animation from the scene scheduler.
 *
 * @param scene     Scene owning the animation.
 * @param animation Animation to remove.
 */
void dvz_scene_animation_remove(DvzScene* scene, DvzAnimation* animation);
