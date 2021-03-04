# vklite API

## GPU

### `dvz_gpu()`
### `dvz_gpu_request_features()`
### `dvz_gpu_queue()`
### `dvz_gpu_create()`
### `dvz_gpu_destroy()`


## Coarse synchronization

### `dvz_queue_wait()`
### `dvz_app_wait()`
### `dvz_gpu_wait()`


## Window

### `dvz_window()`
### `dvz_window_get_size()`
### `dvz_window_poll_events()`
### `dvz_window_destroy()`


## Swapchain

### `dvz_swapchain()`
### `dvz_swapchain_format()`
### `dvz_swapchain_present_mode()`
### `dvz_swapchain_requested_size()`
### `dvz_swapchain_create()`
### `dvz_swapchain_recreate()`
### `dvz_swapchain_acquire()`
### `dvz_swapchain_present()`
### `dvz_swapchain_destroy()`


## Command buffers

### `dvz_commands()`
### `dvz_cmd_begin()`
### `dvz_cmd_end()`
### `dvz_cmd_reset()`
### `dvz_cmd_free()`
### `dvz_cmd_submit_sync()`
### `dvz_commands_destroy()`


## GPU buffer

### `dvz_buffer()`
### `dvz_buffer_size()`
### `dvz_buffer_type()`
### `dvz_buffer_usage()`
### `dvz_buffer_memory()`
### `dvz_buffer_queue_access()`
### `dvz_buffer_create()`
### `dvz_buffer_resize()`
### `dvz_buffer_map()`
### `dvz_buffer_unmap()`
### `dvz_buffer_download()`
### `dvz_buffer_upload()`
### `dvz_buffer_destroy()`
### `dvz_buffer_regions()`
### `dvz_buffer_regions_map()`
### `dvz_buffer_regions_unmap()`
### `dvz_buffer_regions_upload()`


## GPU images

### `dvz_images()`
### `dvz_images_format()`
### `dvz_images_layout()`
### `dvz_images_size()`
### `dvz_images_tiling()`
### `dvz_images_usage()`
### `dvz_images_memory()`
### `dvz_images_aspect()`
### `dvz_images_queue_access()`
### `dvz_images_create()`
### `dvz_images_resize()`
### `dvz_images_transition()`
### `dvz_images_download()`
### `dvz_images_destroy()`


## Sampler

### `dvz_sampler()`
### `dvz_sampler_min_filter()`
### `dvz_sampler_mag_filter()`
### `dvz_sampler_address_mode()`
### `dvz_sampler_create()`
### `dvz_sampler_destroy()`



## Pipeline slots

Vulkan terminology: descriptor set layout.

### `dvz_slots()`
### `dvz_slots_binding()`
### `dvz_slots_push()`
### `dvz_slots_create()`
### `dvz_slots_destroy()`


## Pipeline bindings

Vulkan terminology: descriptor sets

### `dvz_bindings()`
### `dvz_bindings_buffer()`
### `dvz_bindings_texture()`
### `dvz_bindings_update()`
### `dvz_bindings_destroy()`


## Graphics pipeline

### `dvz_graphics()`
### `dvz_graphics_renderpass()`
### `dvz_graphics_topology()`
### `dvz_graphics_shader_glsl()`
### `dvz_graphics_shader_spirv()`
### `dvz_graphics_shader()`
### `dvz_graphics_vertex_binding()`
### `dvz_graphics_vertex_attr()`
### `dvz_graphics_blend()`
### `dvz_graphics_depth_test()`
### `dvz_graphics_pick()`
### `dvz_graphics_polygon_mode()`
### `dvz_graphics_cull_mode()`
### `dvz_graphics_front_face()`
### `dvz_graphics_create()`
### `dvz_graphics_slot()`
### `dvz_graphics_push()`
### `dvz_graphics_destroy()`


## Compute pipeline

### `dvz_compute()`
### `dvz_compute_create()`
### `dvz_compute_code()`
### `dvz_compute_slot()`
### `dvz_compute_push()`
### `dvz_compute_bindings()`
### `dvz_compute_destroy()`


## Barrier

### `dvz_barrier()`
### `dvz_barrier_stages()`
### `dvz_barrier_buffer()`
### `dvz_barrier_buffer_queue()`
### `dvz_barrier_buffer_access()`
### `dvz_barrier_images()`
### `dvz_barrier_images_layout()`
### `dvz_barrier_images_queue()`
### `dvz_barrier_images_access()`


## Semaphores

### `dvz_semaphores()`
### `dvz_semaphores_destroy()`


## Fences

### `dvz_fences()`
### `dvz_fences_copy()`
### `dvz_fences_wait()`
### `dvz_fences_ready()`
### `dvz_fences_reset()`
### `dvz_fences_destroy()`


## Renderpass

### `dvz_renderpass()`
### `dvz_renderpass_clear()`
### `dvz_renderpass_attachment()`
### `dvz_renderpass_attachment_layout()`
### `dvz_renderpass_attachment_ops()`
### `dvz_renderpass_subpass_attachment()`
### `dvz_renderpass_subpass_dependency()`
### `dvz_renderpass_subpass_dependency_access()`
### `dvz_renderpass_subpass_dependency_stage()`
### `dvz_renderpass_create()`
### `dvz_renderpass_destroy()`


## Framebuffers

### `dvz_framebuffers()`
### `dvz_framebuffers_attachment()`
### `dvz_framebuffers_create()`
### `dvz_framebuffers_destroy()`


## Submit

### `dvz_submit()`
### `dvz_submit_commands()`
### `dvz_submit_wait_semaphores()`
### `dvz_submit_signal_semaphores()`
### `dvz_submit_send()`
### `dvz_submit_reset()`


## Command buffer recording

### `dvz_cmd_begin_renderpass()`
### `dvz_cmd_end_renderpass()`
### `dvz_cmd_compute()`
### `dvz_cmd_barrier()`
### `dvz_cmd_copy_buffer_to_image()`
### `dvz_cmd_copy_image_to_buffer()`
### `dvz_cmd_copy_image()`
### `dvz_cmd_copy_image_region()`
### `dvz_cmd_viewport()`
### `dvz_cmd_bind_graphics()`
### `dvz_cmd_bind_vertex_buffer()`
### `dvz_cmd_bind_index_buffer()`
### `dvz_cmd_draw()`
### `dvz_cmd_draw_indexed()`
### `dvz_cmd_draw_indirect()`
### `dvz_cmd_draw_indexed_indirect()`
### `dvz_cmd_copy_buffer()`
### `dvz_cmd_push()`
