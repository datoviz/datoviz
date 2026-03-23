#pragma once
/*
 * Prototype only.
 *
 * Datoviz Rendering Protocol (DRP) — Builder-oriented C API
 *
 * This header exposes a WASM-friendly, struct-free public API:
 * - Only scalars, C strings, and raw byte/ID arrays cross the ABI boundary.
 * - Complex objects (pipelines, bind-group layouts, render passes) are built incrementally
 *   via begin/add/set/end calls. All intermediate state lives inside DRPContext.
 * - When a builder is finalized (e.g., *_end), DRP records exactly one creation command
 *   into the internal command stream.
 *
 * ID model:
 * - DRPId is a logical identifier chosen by the caller (stable across frames).
 * - All create/begin functions take an explicit id; there is no hidden allocation.
 *
 * String enums:
 * - For pipeline state, formats, topology, etc., use WebGPU-style strings
 *   (e.g., "triangle-list", "ccw", "back", "rgba8unorm", "float32x3", "vertex").
 *
 * Ownership:
 * - All strings and small arrays passed to functions are copied internally at call time.
 *   The caller may free or mutate its buffers immediately after the call returns.
 *
 * Command stream:
 * - DRP collects an append-only array of opaque DRPCommand entries inside DRPContext.
 * - You can fetch a pointer to that stream with drp_get_commands() (opaque to API users).
 * - Backends that interpret DRP link against drp.c and know the internal command layout.
 *
 * This file is an exploratory sketch kept under spec/, not a production public header.
 * If it diverges from the markdown contract in spec/drp2/, the markdown contract wins.
 */


#include <stdbool.h>
#include <stdint.h>

/* ----------------------------------------------------------------------------------------------
 */
/* Opaque types & basic ids */
/* ----------------------------------------------------------------------------------------------
 */

typedef struct DRPContext DRPContext; /* Holds builder state + command stream. */
typedef struct DRPCommand DRPCommand; /* Opaque command entries. */
typedef uint32_t DRPId;               /* Logical ids for buffers, textures, pipelines, etc. */

/* ----------------------------------------------------------------------------------------------
 */
/* Context / lifecycle */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Create a DRP context with an empty command stream.
 */
DRPContext* drp_create_context(void);

/**
 * Destroy a DRP context, freeing all internal allocations and command payloads.
 */
void drp_destroy_context(DRPContext* ctx);

/**
 * Get a pointer to the current command stream (opaque entries owned by ctx).
 *
 * @param ctx       Context to inspect.
 * @param out_count Receives the number of commands.
 * @return          Pointer to a contiguous array of DRPCommand entries (opaque).
 */
const DRPCommand* drp_get_commands(const DRPContext* ctx, uint32_t* out_count);

/**
 * Clear the command stream while keeping internal allocations for reuse.
 * Builder objects (pipelines, BGLs, passes) are unaffected.
 */
void drp_clear(DRPContext* ctx);

/* ----------------------------------------------------------------------------------------------
 */
/* Resource creation (no builders needed) */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Create a buffer (mirrors WebGPU GPUBuffer).
 *
 * @param ctx               Context.
 * @param id                Buffer id.
 * @param size              Byte size.
 * @param usage             Array of WebGPU usage strings (e.g., "vertex", "index", "copy-src").
 * @param usage_count       Number of strings in `usage`.
 * @param mappedAtCreation  Whether the buffer starts mapped.
 */
void drp_create_buffer(
    DRPContext* ctx, DRPId id, uint64_t size, const char** usage, uint32_t usage_count,
    bool mappedAtCreation);

/**
 * Write bytes into a buffer (queue.writeBuffer equivalent).
 *
 * @param ctx    Context.
 * @param buffer Buffer id.
 * @param offset Byte offset inside the buffer.
 * @param data   Pointer to source bytes.
 * @param size   Number of bytes to copy.
 */
void drp_write_buffer(
    DRPContext* ctx, DRPId buffer, uint64_t offset, const void* data, uint64_t size);

/**
 * Create a texture (mirrors WebGPU GPUTexture).
 *
 * @param ctx             Context.
 * @param id              Texture id.
 * @param dimension       "1d" | "2d" | "3d".
 * @param width           Width in pixels.
 * @param height          Height in pixels.
 * @param depth_or_layers Depth (3D) or layer count (array/cube).
 * @param format          GPUTextureFormat string.
 * @param usage           Array of usage strings, e.g. "texture-binding","render-attachment".
 * @param usage_count     Number of usage strings.
 * @param mip_level_count Number of mip levels.
 * @param sample_count    MSAA sample count.
 */
void drp_create_texture(
    DRPContext* ctx, DRPId id, const char* dimension, uint32_t width, uint32_t height,
    uint32_t depth_or_layers, const char* format, const char** usage, uint32_t usage_count,
    uint32_t mip_level_count, uint32_t sample_count);

/**
 * Create a texture view (mirrors GPUTextureView).
 *
 * @param ctx        Context.
 * @param id         Texture view id.
 * @param texture    Underlying texture id.
 * @param format     View format or NULL to inherit.
 * @param dimension  "2d","2d-array","cube","cube-array","3d", etc.
 * @param aspect     "all","stencil-only","depth-only".
 * @param mip_base   First mip level.
 * @param mip_count  Number of mips (0 to mean "all").
 * @param layer_base First array layer.
 * @param layer_count Number of layers (0 to mean "all").
 */
void drp_create_texture_view(
    DRPContext* ctx, DRPId id, DRPId texture, const char* format, const char* dimension,
    const char* aspect, uint32_t mip_base, uint32_t mip_count, uint32_t layer_base,
    uint32_t layer_count);

/**
 * Write pixels into a texture (queue.writeTexture).
 *
 * @param ctx            Context.
 * @param texture        Texture id.
 * @param x,y,z          Origin of updated region.
 * @param width,height,depth Dimensions of updated region.
 * @param data           Source bytes.
 * @param size           Number of bytes.
 * @param bytes_per_row  Row pitch (bytes).
 * @param rows_per_image Rows per image slice.
 */
void drp_write_texture(
    DRPContext* ctx, DRPId texture, uint32_t x, uint32_t y, uint32_t z, uint32_t width,
    uint32_t height, uint32_t depth, const void* data, uint64_t size, uint32_t bytes_per_row,
    uint32_t rows_per_image);

/**
 * Create a sampler (mirrors GPUSampler).
 *
 * @param ctx             Context.
 * @param id              Sampler id.
 * @param min_filter      "nearest"|"linear".
 * @param mag_filter      "nearest"|"linear".
 * @param address_mode_u  "clamp-to-edge"|"repeat"|"mirror-repeat".
 * @param address_mode_v  Same as U.
 * @param address_mode_w  Same as U.
 * @param mip_filter      "nearest"|"linear".
 * @param lod_min_clamp   Minimum LOD.
 * @param lod_max_clamp   Maximum LOD.
 * @param max_anisotropy  >=1.0 for anisotropy; 1.0 to disable.
 */
void drp_create_sampler(
    DRPContext* ctx, DRPId id, const char* min_filter, const char* mag_filter,
    const char* address_mode_u, const char* address_mode_v, const char* address_mode_w,
    const char* mip_filter, float lod_min_clamp, float lod_max_clamp, float max_anisotropy);

/* ----------------------------------------------------------------------------------------------
 */
/* Bind-group layout (builder) */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Begin building a bind-group layout.
 *
 * @param ctx Context.
 * @param id  Layout id.
 */
void drp_bgl_begin(DRPContext* ctx, DRPId id);

/**
 * Add a buffer binding entry to the current bind-group layout.
 *
 * @param ctx              Context.
 * @param id               Layout id (same as passed to drp_bgl_begin()).
 * @param binding          Binding index.
 * @param visibility       Shader stage union string, e.g., "VERTEX_FRAGMENT" or "COMPUTE".
 * @param type             "uniform"|"storage"|"read-only-storage".
 * @param has_dynamic_offs true if the binding uses a dynamic offset.
 * @param min_binding_size Minimum required size in bytes (0 if not constrained).
 */
void drp_bgl_add_buffer(
    DRPContext* ctx, DRPId id, uint32_t binding, const char* visibility, const char* type,
    bool has_dynamic_offs, uint64_t min_binding_size);

/**
 * Add a texture binding entry to the current bind-group layout.
 *
 * @param ctx          Context.
 * @param id           Layout id.
 * @param binding      Binding index.
 * @param visibility   Shader stage union string.
 * @param sample_type  "float"|"unfilterable-float"|"depth"|"sint"|"uint".
 * @param view_dim     "2d","2d-array","cube","cube-array","3d".
 * @param multisampled Whether the underlying texture is multisampled.
 */
void drp_bgl_add_texture(
    DRPContext* ctx, DRPId id, uint32_t binding, const char* visibility, const char* sample_type,
    const char* view_dim, bool multisampled);

/**
 * Add a sampler binding entry to the current bind-group layout.
 *
 * @param ctx        Context.
 * @param id         Layout id.
 * @param binding    Binding index.
 * @param visibility Shader stage union string.
 * @param type       "filtering"|"non-filtering"|"comparison".
 */
void drp_bgl_add_sampler(
    DRPContext* ctx, DRPId id, uint32_t binding, const char* visibility, const char* type);

/**
 * Finalize the bind-group layout and emit a creation command.
 *
 * After this call, the builder state for `id` is cleared.
 */
void drp_bgl_end(DRPContext* ctx, DRPId id);

/* ----------------------------------------------------------------------------------------------
 */
/* Pipeline layout (builder) */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Begin building a pipeline layout.
 *
 * @param ctx Context.
 * @param id  Pipeline layout id.
 */
void drp_pipeline_layout_begin(DRPContext* ctx, DRPId id);

/**
 * Append a bind-group layout reference.
 *
 * @param ctx Context.
 * @param id  Pipeline layout id.
 * @param bgl Bind-group-layout id to include.
 */
void drp_pipeline_layout_add_bgl(DRPContext* ctx, DRPId id, DRPId bgl);

/**
 * Append a push-constant range.
 *
 * @param ctx    Context.
 * @param id     Pipeline layout id.
 * @param stages Shader stage union string.
 * @param offset Byte offset.
 * @param size   Byte size.
 */
void drp_pipeline_layout_add_push_constant(
    DRPContext* ctx, DRPId id, const char* stages, uint32_t offset, uint32_t size);

/**
 * Finalize the pipeline layout and emit a creation command.
 */
void drp_pipeline_layout_end(DRPContext* ctx, DRPId id);

/* ----------------------------------------------------------------------------------------------
 */
/* Bind group (builder) */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Begin building a bind group.
 *
 * @param ctx     Context.
 * @param id      Bind-group id.
 * @param layout  Pipeline layout's bind-group-layout id to match.
 */
void drp_bind_group_begin(DRPContext* ctx, DRPId id, DRPId layout);

/**
 * Add a buffer binding to the bind group.
 *
 * @param ctx     Context.
 * @param id      Bind-group id.
 * @param binding Binding index (must match layout).
 * @param buffer  Buffer id.
 * @param offset  Byte offset into the buffer.
 * @param size    Byte size; 0 means "use layout's minBindingSize".
 */
void drp_bind_group_add_buffer(
    DRPContext* ctx, DRPId id, uint32_t binding, DRPId buffer, uint64_t offset, uint64_t size);

/**
 * Add a texture-view binding to the bind group.
 *
 * @param ctx     Context.
 * @param id      Bind-group id.
 * @param binding Binding index.
 * @param view    Texture view id.
 */
void drp_bind_group_add_texture_view(DRPContext* ctx, DRPId id, uint32_t binding, DRPId view);

/**
 * Add a sampler binding to the bind group.
 *
 * @param ctx     Context.
 * @param id      Bind-group id.
 * @param binding Binding index.
 * @param sampler Sampler id.
 */
void drp_bind_group_add_sampler(DRPContext* ctx, DRPId id, uint32_t binding, DRPId sampler);

/**
 * Finalize the bind group and emit a creation command.
 */
void drp_bind_group_end(DRPContext* ctx, DRPId id);

/* ----------------------------------------------------------------------------------------------
 */
/* Shader modules */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Create a shader module.
 *
 * @param ctx    Context.
 * @param id     Shader id.
 * @param format "wgsl"|"spirv"|"glsl".
 * @param code   Pointer to source/binary bytes.
 * @param size   Number of bytes (or characters for WGSL/GLSL).
 */
void drp_create_shader_module(
    DRPContext* ctx, DRPId id, const char* format, const void* code, uint64_t size);

/* ----------------------------------------------------------------------------------------------
 */
/* Render pipeline (builder) */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Begin building a render pipeline.
 *
 * @param ctx     Context.
 * @param id      Render-pipeline id.
 * @param layout  Pipeline layout id.
 */
void drp_pipeline_begin(DRPContext* ctx, DRPId id, DRPId layout);

/**
 * Set the vertex stage module + entry point.
 *
 * @param ctx         Context.
 * @param id          Pipeline id.
 * @param module      Shader module id.
 * @param entry_point Entry point string (e.g., "main").
 */
void drp_pipeline_vertex_module(DRPContext* ctx, DRPId id, DRPId module, const char* entry_point);

/**
 * Add a vertex buffer layout. Returns the index of the buffer slot.
 *
 * @param ctx        Context.
 * @param id         Pipeline id.
 * @param array_stride  Stride in bytes.
 * @param step_mode  "vertex"|"instance".
 * @return           Buffer index (0..N-1) to be used by subsequent attribute calls.
 */
uint32_t drp_pipeline_add_vertex_buffer(
    DRPContext* ctx, DRPId id, uint64_t array_stride, const char* step_mode);

/**
 * Add a vertex attribute to a previously added vertex buffer.
 *
 * @param ctx             Context.
 * @param id              Pipeline id.
 * @param buffer_index    Index returned by drp_pipeline_add_vertex_buffer().
 * @param shader_location Shader location.
 * @param format          Vertex format string (e.g., "float32x3", "uint32").
 * @param offset          Byte offset within the vertex element.
 */
void drp_pipeline_add_vertex_attribute(
    DRPContext* ctx, DRPId id, uint32_t buffer_index, uint32_t shader_location, const char* format,
    uint64_t offset);

/**
 * Set the fragment stage module + entry point.
 *
 * @param ctx         Context.
 * @param id          Pipeline id.
 * @param module      Shader module id.
 * @param entry_point Entry point string.
 */
void drp_pipeline_fragment_module(
    DRPContext* ctx, DRPId id, DRPId module, const char* entry_point);

/**
 * Add a color target. Returns the target index.
 *
 * @param ctx     Context.
 * @param id      Pipeline id.
 * @param format  GPUTextureFormat string for the color target.
 * @return        Target index (0..N-1).
 */
uint32_t drp_pipeline_add_color_target(DRPContext* ctx, DRPId id, const char* format);

/**
 * Set blending parameters for a color target.
 *
 * @param ctx        Context.
 * @param id         Pipeline id.
 * @param target_idx Color target index.
 * @param src_color  BlendFactor string (e.g., "src-alpha").
 * @param dst_color  BlendFactor string.
 * @param op_color   BlendOperation string (e.g., "add").
 * @param src_alpha  BlendFactor string.
 * @param dst_alpha  BlendFactor string.
 * @param op_alpha   BlendOperation string.
 */
void drp_pipeline_set_color_target_blend(
    DRPContext* ctx, DRPId id, uint32_t target_idx, const char* src_color, const char* dst_color,
    const char* op_color, const char* src_alpha, const char* dst_alpha, const char* op_alpha);

/**
 * Set the color write mask for a color target.
 *
 * @param ctx        Context.
 * @param id         Pipeline id.
 * @param target_idx Color target index.
 * @param mask       "none"|"red"|"green"|"blue"|"alpha"|"all".
 */
void drp_pipeline_set_color_target_write_mask(
    DRPContext* ctx, DRPId id, uint32_t target_idx, const char* mask);

/**
 * Set primitive state.
 *
 * @param ctx               Context.
 * @param id                Pipeline id.
 * @param topology          "point-list","line-list","line-strip","triangle-list","triangle-strip".
 * @param strip_index_fmt   "uint16","uint32" or NULL/"none" when not using strips.
 * @param front_face        "cw"|"ccw".
 * @param cull_mode         "none"|"front"|"back".
 */
void drp_pipeline_set_primitive(
    DRPContext* ctx, DRPId id, const char* topology, const char* strip_index_fmt,
    const char* front_face, const char* cull_mode);

/**
 * Set depth/stencil state (all fields optional via flags).
 *
 * @param ctx                 Context.
 * @param id                  Pipeline id.
 * @param format              Depth/stencil texture format (e.g., "depth24plus"), or NULL to skip.
 * @param depth_write_enabled true to enable depth write.
 * @param depth_compare       CompareFunction string (e.g., "less"), or NULL to skip.
 * @param depth_bias          Depth bias constant factor.
 * @param depth_bias_slope    Depth bias slope scale.
 * @param depth_bias_clamp    Depth bias clamp.
 * @param has_stencil_front   If true, the next 4 stencil-Front args are used.
 * @param front_compare       CompareFunction for stencil front face.
 * @param front_fail          StencilOperation for front fail.
 * @param front_depth_fail    StencilOperation for front depth fail.
 * @param front_pass          StencilOperation for front pass.
 * @param has_stencil_back    If true, the next 4 stencil-Back args are used.
 * @param back_compare        CompareFunction for stencil back face.
 * @param back_fail           StencilOperation for back fail.
 * @param back_depth_fail     StencilOperation for back depth fail.
 * @param back_pass           StencilOperation for back pass.
 */
void drp_pipeline_set_depth_stencil(
    DRPContext* ctx, DRPId id, const char* format, bool depth_write_enabled,
    const char* depth_compare, float depth_bias, float depth_bias_slope, float depth_bias_clamp,
    bool has_stencil_front, const char* front_compare, const char* front_fail,
    const char* front_depth_fail, const char* front_pass, bool has_stencil_back,
    const char* back_compare, const char* back_fail, const char* back_depth_fail,
    const char* back_pass);

/**
 * Set multisampling parameters.
 *
 * @param ctx        Context.
 * @param id         Pipeline id.
 * @param count      Sample count (1, 2, 4, 8, ...).
 * @param mask       Sample mask.
 * @param alpha_cov  Enable alpha-to-coverage.
 */
void drp_pipeline_set_multisample(
    DRPContext* ctx, DRPId id, uint32_t count, uint32_t mask, bool alpha_cov);

/**
 * Finalize the render pipeline and emit a creation command.
 */
void drp_pipeline_end(DRPContext* ctx, DRPId id);

/* ----------------------------------------------------------------------------------------------
 */
/* Compute pipeline (simple) */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Create a compute pipeline (no builder needed).
 *
 * @param ctx        Context.
 * @param id         Compute-pipeline id.
 * @param layout     Pipeline layout id.
 * @param module     Shader module id.
 * @param entryPoint Entry point string (e.g., "main").
 */
void drp_create_compute_pipeline(
    DRPContext* ctx, DRPId id, DRPId layout, DRPId module, const char* entryPoint);

/* ----------------------------------------------------------------------------------------------
 */
/* Command encoders */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Begin a command encoder (device.createCommandEncoder).
 *
 * @param ctx        Context.
 * @param encoder_id Encoder id.
 */
void drp_begin_command_encoder(DRPContext* ctx, DRPId encoder_id);

/**
 * Finish a command encoder and assign a command buffer id.
 *
 * @param ctx               Context.
 * @param encoder_id        Encoder id.
 * @param command_buffer_id Command buffer id to output.
 */
void drp_finish_command_encoder(DRPContext* ctx, DRPId encoder_id, DRPId command_buffer_id);

/* ----------------------------------------------------------------------------------------------
 */
/* Render pass (builder + begin/end) */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Begin configuring a render pass. This records attachment state in a builder.
 * Call drp_pass_begin() to actually emit a BeginRenderPass command.
 *
 * @param ctx     Context.
 * @param pass_id Render-pass id (builder slot).
 */
void drp_pass_builder_begin(DRPContext* ctx, DRPId pass_id);

/**
 * Add a color attachment to the current pass builder.
 *
 * @param ctx           Context.
 * @param pass_id       Render-pass id.
 * @param view          Texture view id.
 * @param load_op       "load"|"clear".
 * @param store_op      "store"|"discard".
 * @param has_clear     If true, clear values are used.
 * @param r,g,b,a       Clear color (ignored if has_clear==false).
 * @param has_resolve   If true, resolve_view is used.
 * @param resolve_view  Resolve target view id.
 */
void drp_pass_add_color_attachment(
    DRPContext* ctx, DRPId pass_id, DRPId view, const char* load_op, const char* store_op,
    bool has_clear, float r, float g, float b, float a, bool has_resolve, DRPId resolve_view);

/**
 * Set depth/stencil attachment on the current pass builder.
 *
 * @param ctx               Context.
 * @param pass_id           Render-pass id.
 * @param view              Depth/stencil texture view id.
 * @param has_depth         If true, the depth fields apply.
 * @param depth_load_op     "load"|"clear".
 * @param depth_store_op    "store"|"discard".
 * @param depth_clear_value Clear depth value.
 * @param has_stencil       If true, the stencil fields apply.
 * @param stencil_load_op   "load"|"clear".
 * @param stencil_store_op  "store"|"discard".
 * @param stencil_clear     Clear stencil value.
 */
void drp_pass_set_depth_stencil(
    DRPContext* ctx, DRPId pass_id, DRPId view, bool has_depth, const char* depth_load_op,
    const char* depth_store_op, float depth_clear_value, bool has_stencil,
    const char* stencil_load_op, const char* stencil_store_op, uint32_t stencil_clear);

/**
 * Emit BeginRenderPass using the attachments configured in the builder.
 *
 * @param ctx        Context.
 * @param pass_id    Render-pass id.
 * @param encoder_id Command-encoder id.
 */
void drp_pass_begin(DRPContext* ctx, DRPId pass_id, DRPId encoder_id);

/**
 * End the current render pass (EndRenderPass).
 *
 * @param ctx     Context.
 * @param pass_id Render-pass id.
 */
void drp_pass_end(DRPContext* ctx, DRPId pass_id);

/* ----------------------------------------------------------------------------------------------
 */
/* Compute pass */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Begin a compute pass (no attachments).
 *
 * @param ctx        Context.
 * @param pass_id    Compute-pass id.
 * @param encoder_id Command-encoder id.
 */
void drp_begin_compute_pass(DRPContext* ctx, DRPId pass_id, DRPId encoder_id);

/**
 * End a compute pass.
 *
 * @param ctx     Context.
 * @param pass_id Compute-pass id.
 */
void drp_end_compute_pass(DRPContext* ctx, DRPId pass_id);

/* ----------------------------------------------------------------------------------------------
 */
/* Common pass commands (bind pipeline/groups, viewport, draw, dispatch) */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Set the current pipeline inside a pass.
 *
 * @param ctx      Context.
 * @param pass_id  Pass id.
 * @param pipeline Pipeline id (render or compute).
 */
void drp_set_pipeline(DRPContext* ctx, DRPId pass_id, DRPId pipeline);

/**
 * Bind a bind group in a pass.
 *
 * @param ctx            Context.
 * @param pass_id        Pass id.
 * @param index          Bind-group index (set number).
 * @param bind_group     Bind-group id.
 * @param dynamic_offsets Optional array of dynamic offsets (may be NULL).
 * @param offset_count   Number of entries in dynamic_offsets.
 */
void drp_set_bind_group(
    DRPContext* ctx, DRPId pass_id, uint32_t index, DRPId bind_group,
    const uint32_t* dynamic_offsets, uint32_t offset_count);

/**
 * Set viewport.
 */
void drp_set_viewport(
    DRPContext* ctx, DRPId pass_id, float x, float y, float width, float height, float minDepth,
    float max_depth);

/**
 * Set scissor rectangle.
 */
void drp_set_scissor(
    DRPContext* ctx, DRPId pass_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

/**
 * Set blend constant.
 */
void drp_set_blend_constant(DRPContext* ctx, DRPId pass_id, float r, float g, float b, float a);

/**
 * Set stencil reference value.
 */
void drp_set_stencil_reference(DRPContext* ctx, DRPId pass_id, uint32_t reference);

/**
 * Draw (non-indexed).
 */
void drp_draw(
    DRPContext* ctx, DRPId pass_id, uint32_t vertex_count, uint32_t instance_count,
    uint32_t first_vertex, uint32_t first_instance);

/**
 * Draw indexed.
 */
void drp_draw_indexed(
    DRPContext* ctx, DRPId pass_id, uint32_t index_count, uint32_t instance_count,
    uint32_t first_index, int32_t base_vertex, uint32_t first_instance);

/**
 * Draw indirect.
 *
 * @param buffer Buffer containing draw arguments.
 * @param offset Byte offset to first draw struct.
 * @param count  Number of draws (stride is API-defined).
 */
void drp_draw_indirect(
    DRPContext* ctx, DRPId pass_id, DRPId buffer, uint64_t offset, uint32_t count);

/**
 * Draw indexed indirect.
 */
void drp_draw_indexed_indirect(
    DRPContext* ctx, DRPId pass_id, DRPId buffer, uint64_t offset, uint32_t count);

/**
 * Dispatch compute workgroups.
 */
void drp_dispatch_workgroups(DRPContext* ctx, DRPId pass_id, uint32_t x, uint32_t y, uint32_t z);

/**
 * Dispatch compute workgroups (indirect arguments in a buffer).
 */
void drp_dispatch_workgroups_indirect(
    DRPContext* ctx, DRPId pass_id, DRPId buffer, uint64_t offset);

/* ----------------------------------------------------------------------------------------------
 */
/* Copy commands */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Copy buffer → buffer.
 */
void drp_copy_buffer_to_buffer(
    DRPContext* ctx, DRPId src, uint64_t src_offset, DRPId dst, uint64_t dst_offset,
    uint64_t size);

/**
 * Copy buffer → texture.
 */
void drp_copy_buffer_to_texture(
    DRPContext* ctx, DRPId src, uint64_t src_offset, DRPId dst_texture, uint32_t x, uint32_t y,
    uint32_t z, uint32_t width, uint32_t height, uint32_t depth, uint32_t bytes_per_row,
    uint32_t rows_per_image);

/**
 * Copy texture → buffer.
 */
void drp_copy_texture_to_buffer(
    DRPContext* ctx, DRPId src_texture, uint32_t x, uint32_t y, uint32_t z, uint32_t width,
    uint32_t height, uint32_t depth, DRPId dst, uint64_t dst_offset, uint32_t bytes_per_row,
    uint32_t rows_per_image);

/* ----------------------------------------------------------------------------------------------
 */
/* Queue submission */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Submit command buffers to a queue.
 *
 * @param ctx        Context.
 * @param queue      Queue id (logical).
 * @param cmd_bufs   Array of command buffer ids.
 * @param count      Number of command buffers.
 */
void drp_queue_submit(DRPContext* ctx, DRPId queue, const DRPId* cmd_bufs, uint32_t count);

/* ----------------------------------------------------------------------------------------------
 */
/* Explicit barriers (optional; for Vulkan-like backends) */
/* ----------------------------------------------------------------------------------------------
 */

/**
 * Insert an explicit resource barrier (backend-defined states).
 *
 * @param ctx        Context.
 * @param resource   Resource id (buffer or texture).
 * @param old_state  Previous state string (backend-defined).
 * @param new_state  Next state string (backend-defined).
 * @param src_stage  Source stage string.
 * @param dst_stage  Destination stage string.
 * @param src_access Source access mask string.
 * @param dst_access Destination access mask string.
 */
void drp_resource_barrier(
    DRPContext* ctx, DRPId resource, const char* old_state, const char* new_state,
    const char* src_stage, const char* dst_stage, const char* src_access, const char* dst_access);
