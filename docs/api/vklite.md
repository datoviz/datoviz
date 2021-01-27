# vklite API

## GPU

### `vkl_gpu()`
### `vkl_gpu_request_features()`
### `vkl_gpu_queue()`
### `vkl_gpu_create()`
### `vkl_gpu_destroy()`


## Coarse synchronization

### `vkl_queue_wait()`
### `vkl_app_wait()`
### `vkl_gpu_wait()`


## Window

### `vkl_window()`
### `vkl_window_get_size()`
### `vkl_window_poll_events()`
### `vkl_window_destroy()`


## Swapchain

### `vkl_swapchain()`
### `vkl_swapchain_format()`
### `vkl_swapchain_present_mode()`
### `vkl_swapchain_requested_size()`
### `vkl_swapchain_create()`
### `vkl_swapchain_recreate()`
### `vkl_swapchain_acquire()`
### `vkl_swapchain_present()`
### `vkl_swapchain_destroy()`


## Command buffers

### `vkl_commands()`
### `vkl_cmd_begin()`
### `vkl_cmd_end()`
### `vkl_cmd_reset()`
### `vkl_cmd_free()`
### `vkl_cmd_submit_sync()`
### `vkl_commands_destroy()`


## GPU buffer

### `vkl_buffer()`
### `vkl_buffer_size()`
### `vkl_buffer_type()`
### `vkl_buffer_usage()`
### `vkl_buffer_memory()`
### `vkl_buffer_queue_access()`
### `vkl_buffer_create()`
### `vkl_buffer_resize()`
### `vkl_buffer_map()`
### `vkl_buffer_unmap()`
### `vkl_buffer_download()`
### `vkl_buffer_upload()`
### `vkl_buffer_destroy()`
### `vkl_buffer_regions()`
### `vkl_buffer_regions_map()`
### `vkl_buffer_regions_unmap()`
### `vkl_buffer_regions_upload()`


## GPU images

### `vkl_images()`
### `vkl_images_format()`
### `vkl_images_layout()`
### `vkl_images_size()`
### `vkl_images_tiling()`
### `vkl_images_usage()`
### `vkl_images_memory()`
### `vkl_images_aspect()`
### `vkl_images_queue_access()`
### `vkl_images_create()`
### `vkl_images_resize()`
### `vkl_images_transition()`
### `vkl_images_download()`
### `vkl_images_destroy()`


## Sampler

### `vkl_sampler()`
### `vkl_sampler_min_filter()`
### `vkl_sampler_mag_filter()`
### `vkl_sampler_address_mode()`
### `vkl_sampler_create()`
### `vkl_sampler_destroy()`



## Pipeline slots

Vulkan terminology: descriptor set layout.

### `vkl_slots()`
### `vkl_slots_binding()`
### `vkl_slots_push()`
### `vkl_slots_create()`
### `vkl_slots_destroy()`


## Pipeline bindings

Vulkan terminology: descriptor sets

### `vkl_bindings()`
### `vkl_bindings_buffer()`
### `vkl_bindings_texture()`
### `vkl_bindings_update()`
### `vkl_bindings_destroy()`


## Graphics pipeline

### `vkl_graphics()`
### `vkl_graphics_renderpass()`
### `vkl_graphics_topology()`
### `vkl_graphics_shader_glsl()`
### `vkl_graphics_shader_spirv()`
### `vkl_graphics_shader()`
### `vkl_graphics_vertex_binding()`
### `vkl_graphics_vertex_attr()`
### `vkl_graphics_blend()`
### `vkl_graphics_depth_test()`
### `vkl_graphics_polygon_mode()`
### `vkl_graphics_cull_mode()`
### `vkl_graphics_front_face()`
### `vkl_graphics_create()`
### `vkl_graphics_slot()`
### `vkl_graphics_push()`
### `vkl_graphics_destroy()`


## Compute pipeline

### `vkl_compute()`
### `vkl_compute_create()`
### `vkl_compute_code()`
### `vkl_compute_slot()`
### `vkl_compute_push()`
### `vkl_compute_bindings()`
### `vkl_compute_destroy()`


## Barrier

### `vkl_barrier()`
### `vkl_barrier_stages()`
### `vkl_barrier_buffer()`
### `vkl_barrier_buffer_queue()`
### `vkl_barrier_buffer_access()`
### `vkl_barrier_images()`
### `vkl_barrier_images_layout()`
### `vkl_barrier_images_queue()`
### `vkl_barrier_images_access()`


## Semaphores

### `vkl_semaphores()`
### `vkl_semaphores_destroy()`


## Fences

### `vkl_fences()`
### `vkl_fences_copy()`
### `vkl_fences_wait()`
### `vkl_fences_ready()`
### `vkl_fences_reset()`
### `vkl_fences_destroy()`


## Renderpass

### `vkl_renderpass()`
### `vkl_renderpass_clear()`
### `vkl_renderpass_attachment()`
### `vkl_renderpass_attachment_layout()`
### `vkl_renderpass_attachment_ops()`
### `vkl_renderpass_subpass_attachment()`
### `vkl_renderpass_subpass_dependency()`
### `vkl_renderpass_subpass_dependency_access()`
### `vkl_renderpass_subpass_dependency_stage()`
### `vkl_renderpass_create()`
### `vkl_renderpass_destroy()`


## Framebuffers

### `vkl_framebuffers()`
### `vkl_framebuffers_attachment()`
### `vkl_framebuffers_create()`
### `vkl_framebuffers_destroy()`


## Submit

### `vkl_submit()`
### `vkl_submit_commands()`
### `vkl_submit_wait_semaphores()`
### `vkl_submit_signal_semaphores()`
### `vkl_submit_send()`
### `vkl_submit_reset()`


## Command buffer recording

### `vkl_cmd_begin_renderpass()`
### `vkl_cmd_end_renderpass()`
### `vkl_cmd_compute()`
### `vkl_cmd_barrier()`
### `vkl_cmd_copy_buffer_to_image()`
### `vkl_cmd_copy_image_to_buffer()`
### `vkl_cmd_copy_image()`
### `vkl_cmd_viewport()`
### `vkl_cmd_bind_graphics()`
### `vkl_cmd_bind_vertex_buffer()`
### `vkl_cmd_bind_index_buffer()`
### `vkl_cmd_draw()`
### `vkl_cmd_draw_indexed()`
### `vkl_cmd_draw_indirect()`
### `vkl_cmd_draw_indexed_indirect()`
### `vkl_cmd_copy_buffer()`
### `vkl_cmd_push()`
