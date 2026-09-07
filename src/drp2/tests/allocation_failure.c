/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/*************************************************************************************************/
/*  DRP2 allocation-failure tests                                                                */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdint.h>

#include "_alloc.h"
#include "_assertions.h"
#include "../_runtime.h"
#include "../_stream.h"
#include "datoviz/drp2.h"
#include "datoviz_testing.h"
#include "test_drp2.h"
#include "test_drp2_helpers.h"
#include "testing.h"

#if DVZ_DRP2_HAS_VKLITE
#include "datoviz/vk/gpu_ctx.h"
#include "datoviz/vk/memory.h"
#include "datoviz/vklite/buffers.h"
#include "datoviz/vklite/descriptors.h"
#endif



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

#if DVZ_DRP2_HAS_VKLITE
/**
 * Reject a calloc request made through the process allocator.
 *
 * @param count allocation element count
 * @param size allocation element size
 * @return always NULL
 */
static void* _reject_calloc(DvzSize count, DvzSize size)
{
    (void)count;
    (void)size;
    return NULL;
}



/**
 * Reject a realloc request made through the process allocator.
 *
 * @param pointer previous allocation
 * @param size requested byte size
 * @return always NULL
 */
static void* _reject_realloc(void* pointer, DvzSize size)
{
    (void)pointer;
    (void)size;
    return NULL;
}
#endif



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#if DVZ_DRP2_HAS_VKLITE
/**
 * Verify lightweight wrapper constructors report heap allocation failure.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_wrapper_allocation_failure(TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    const DvzAllocator* previous_allocator = dvz_get_allocator();
    DvzAllocator failing_allocator = *previous_allocator;
    failing_allocator.calloc_fn = _reject_calloc;
    dvz_set_allocator(&failing_allocator);
    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    DvzDescriptors* descriptors = dvz_descriptors_create_wrapper();
    DvzAllocation* allocation = dvz_allocation_create();
    dvz_set_allocator(previous_allocator);

    AT(buffer == NULL);
    AT(descriptors == NULL);
    AT(allocation == NULL);
    return 0;
}



/**
 * Preserve deferred-storage invariants when initial allocation fails, then allow retry.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_deferred_initial_allocation_failure(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    Drp2VkliteState state = {0};
    Drp2VkliteObject object = {
        .id = 1, .kind = DRP2_OBJECT_TEXTURE, .borrowed_frame_target = true};
    VkCommandBuffer token = (VkCommandBuffer)(uintptr_t)0x123;
    const DvzAllocator* previous_allocator = dvz_get_allocator();
    DvzAllocator failing_allocator = *previous_allocator;
    failing_allocator.calloc_fn = _reject_calloc;
    failing_allocator.realloc_fn = _reject_realloc;
    dvz_set_allocator(&failing_allocator);
    bool deferred = _vklite_defer_destroy_object(&state, &object, token);
    bool reserved = _vklite_deferred_reserve(&state, 1);
    dvz_set_allocator(previous_allocator);

    AT(!deferred);
    AT(!reserved);
    AT(object.id == 1);
    AT(!object.destroyed);
    AT(state.deferred == NULL);
    AT(state.deferred_count == 0);
    AT(state.deferred_capacity == 0);

    AT(_vklite_deferred_reserve(&state, 1));
    AT(state.deferred != NULL);
    AT(_vklite_defer_destroy_object(&state, &object, token));
    AT(state.deferred_count == 1);
    _vklite_state_cleanup(&state);
    return 0;
}
#endif



/**
 * Preserve all committed semantic arrays when cloning one of them fails.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_semantic_clone_allocation_failure(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;

    DvzDrp2RuntimeConfig cfg = dvz_drp2_runtime_vklite_config(NULL, NULL);
    cfg.semantic_only = true;
    DvzDrp2Runtime* runtime = dvz_drp2_runtime_vklite(&cfg);
    ANN(runtime);
    DvzDrp2CommandStream* setup = dvz_drp2_stream();
    ANN(setup);
    AT(dvz_drp2_stream_hello_renderer(setup, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        setup, 1, 64, DVZ_DRP2_BUFFER_USAGE_COPY_SRC | DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_create_buffer(setup, 2, 64, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(setup, 3));
    AT(dvz_drp2_stream_finish_command_encoder(setup, 3, 4));
    AT(dvz_drp2_stream_queue_submit_readback(setup, 4, 5, 1, 0, 16));
    AT(dvz_drp2_stream_begin_command_encoder(setup, 6));
    AT(dvz_drp2_stream_copy_buffer_to_buffer(setup, 6, 1, 0, 2, 0, 16));
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
    AT(result.ok);

    const Drp2TestAllocationSite sites[] = {
        DRP2_TEST_ALLOC_SEMANTIC_OBJECTS,
        DRP2_TEST_ALLOC_SEMANTIC_REFERENCES,
        DRP2_TEST_ALLOC_SEMANTIC_READBACKS,
    };
    for (uint32_t i = 0; i < sizeof(sites) / sizeof(sites[0]); i++)
    {
        Drp2RuntimeState* state = runtime->semantic_state;
        Drp2Object* objects = state->objects;
        Drp2WorkReference* references = state->references;
        Drp2PendingReadback* readbacks = state->pending_readbacks;
        runtime->test_alloc_site = sites[i];
        runtime->test_alloc_fail_at = 1;
        DvzDrp2CommandStream* empty = dvz_drp2_stream();
        ANN(empty);
        if (i == 0)
        {
            DvzDrp2Runtime* other = dvz_drp2_runtime_vklite(&cfg);
            ANN(other);
            result = dvz_drp2_runtime_execute(other, empty);
            AT(result.ok);
            AT(runtime->test_alloc_site == sites[i]);
            AT(runtime->test_alloc_fail_at == 1);
            dvz_drp2_runtime_destroy(other);
        }
        result = dvz_drp2_runtime_execute(runtime, empty);
        AT(!result.ok);
        AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
        AT(runtime->semantic_state == state);
        AT(state->objects == objects);
        AT(state->references == references);
        AT(state->pending_readbacks == readbacks);
        AT(state->reference_count == 2);
        AT(state->pending_readback_count == 1);
        AT(_drp2_find_any_object(state, 1)->size == 64);
        AT(references[0].owner_id == 6 && references[0].resource_id == 1);
        AT(references[1].owner_id == 6 && references[1].resource_id == 2);
        AT(readbacks[0].submission_id == 5 && readbacks[0].buffer_id == 1);
        AT(readbacks[0].offset == 0 && readbacks[0].size == 16);
        result = dvz_drp2_runtime_execute(runtime, empty);
        AT(result.ok);
        AT(runtime->semantic_state->reference_count == 2);
        AT(runtime->semantic_state->pending_readback_count == 1);
        dvz_drp2_stream_destroy(empty);
    }

    DvzDrp2CommandStream* submit = dvz_drp2_stream();
    ANN(submit);
    AT(dvz_drp2_stream_finish_command_encoder(submit, 6, 7));
    AT(dvz_drp2_stream_queue_submit(submit, 7, 8));
    result = dvz_drp2_runtime_execute(runtime, submit);
    AT(result.ok);
    AT(runtime->semantic_state->reference_count == 0);
    AT(_drp2_pending_readback_release(runtime->semantic_state, 1, 0, 16));
    DvzDrp2CommandStream* destroy = dvz_drp2_stream();
    ANN(destroy);
    AT(dvz_drp2_stream_destroy_buffer(destroy, 1));
    AT(dvz_drp2_stream_destroy_buffer(destroy, 2));
    result = dvz_drp2_runtime_execute(runtime, destroy);
    AT(result.ok);

    runtime->test_alloc_site = DRP2_TEST_ALLOC_SEMANTIC_OBJECTS;
    runtime->test_alloc_fail_at = 1;
    dvz_drp2_runtime_reset(runtime);
    AT(runtime->test_alloc_site == DRP2_TEST_ALLOC_NONE);
    AT(runtime->test_alloc_fail_at == 0);
    dvz_drp2_stream_destroy(destroy);
    dvz_drp2_stream_destroy(submit);
    dvz_drp2_stream_destroy(setup);
    dvz_drp2_runtime_destroy(runtime);
    return 0;
}



#if DVZ_DRP2_HAS_VKLITE
/**
 * Preserve an existing bind group when replacement allocation or retirement reserve fails.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_bind_group_replacement_allocation_failure(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;

    DvzDrp2CommandStream* setup = dvz_drp2_stream();
    ANN(setup);
    AT(dvz_drp2_stream_hello_renderer(setup, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        setup, 1, 64, DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_create_buffer(
        setup, 4, 64, DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_create_uniform_bind_group_layout(setup, 2));
    AT(dvz_drp2_stream_create_uniform_bind_group(setup, 3, 2, 1, 0, 64));
    DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
    AT(result.ok);
    Drp2VkliteState* state = runtime->vklite_state;
    Drp2VkliteObject* initial = _vklite_find(state, 3);
    ANN(initial);
    DvzDescriptors* initial_descriptors = initial->descriptors;
    uint32_t initial_deferred_count = state->deferred_count;

    DvzDrp2CommandStream* replacement = dvz_drp2_stream();
    ANN(replacement);
    AT(dvz_drp2_stream_create_uniform_bind_group(replacement, 3, 2, 4, 0, 64));
    runtime->test_alloc_site = DRP2_TEST_ALLOC_DESCRIPTOR_WRAPPER;
    runtime->test_alloc_fail_at = 1;
    result = _vklite_create_bind_group(state, &replacement->commands[0], 0);
    AT(!result.ok);
    Drp2VkliteObject* retained = _vklite_find(state, 3);
    ANN(retained);
    AT(retained->descriptors == initial_descriptors);
    AT(retained->bind_group_entries[0].resource_id == 1);
    AT(state->deferred_count == initial_deferred_count);
    result = _vklite_create_bind_group(state, &replacement->commands[0], 0);
    AT(result.ok);
    Drp2VkliteObject* replaced = _vklite_find(state, 3);
    ANN(replaced);
    AT(replaced->bind_group_entries[0].resource_id == 4);

    DvzDescriptors* replacement_descriptors = replaced->descriptors;
    DvzDrp2CommandStream* replacement_back = dvz_drp2_stream();
    ANN(replacement_back);
    AT(dvz_drp2_stream_create_uniform_bind_group(replacement_back, 3, 2, 1, 0, 64));
    VkCommandBuffer token = (VkCommandBuffer)(uintptr_t)0x123;
    state->retirement_borrowed_command_buffer = token;
    runtime->test_alloc_site = DRP2_TEST_ALLOC_DEFERRED_RESERVE;
    runtime->test_alloc_fail_at = 1;
    result = _vklite_create_bind_group(state, &replacement_back->commands[0], 0);
    AT(!result.ok);
    retained = _vklite_find(state, 3);
    ANN(retained);
    AT(retained->descriptors == replacement_descriptors);
    AT(retained->bind_group_entries[0].resource_id == 4);
    AT(state->deferred_count == initial_deferred_count);
    result = _vklite_create_bind_group(state, &replacement_back->commands[0], 0);
    AT(result.ok);
    AT(_vklite_find(state, 3)->bind_group_entries[0].resource_id == 1);
    AT(state->deferred_count == initial_deferred_count + 1);
    _vklite_flush_deferred_for_command_buffer(state, token);
    state->retirement_borrowed_command_buffer = VK_NULL_HANDLE;
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    dvz_drp2_stream_destroy(replacement_back);
    dvz_drp2_stream_destroy(replacement);
    dvz_drp2_stream_destroy(setup);
    return 0;
}



/**
 * Preserve a buffer and all dependent descriptors across injected replacement failures.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_buffer_replacement_allocation_failure(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    DvzGpuCtx* ctx = NULL;
    DvzDrp2Runtime* runtime = drp2_test_vklite_fixture_runtime(suite, &ctx);
    if (runtime == NULL)
        return 0;

    DvzDrp2CommandStream* setup = dvz_drp2_stream();
    ANN(setup);
    AT(dvz_drp2_stream_hello_renderer(setup, "test-client"));
    AT(dvz_drp2_stream_renderer_hello_reply(setup, "test-renderer"));
    AT(dvz_drp2_stream_create_buffer(
        setup, 1, 16,
        DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_COPY_SRC |
            DVZ_DRP2_BUFFER_USAGE_COPY_DST |
            DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    AT(dvz_drp2_stream_write_buffer_base64(setup, 1, 0, 16, "AQIDBAUGBwgJCgsMDQ4PEA=="));
    AT(dvz_drp2_stream_create_uniform_bind_group_layout(setup, 2));
    AT(dvz_drp2_stream_create_uniform_bind_group_layout(setup, 3));
    AT(dvz_drp2_stream_create_uniform_bind_group(setup, 4, 2, 1, 0, 16));
    AT(dvz_drp2_stream_create_uniform_bind_group(setup, 5, 3, 1, 0, 16));
    AT(dvz_drp2_stream_create_buffer(setup, 6, 16, DVZ_DRP2_BUFFER_USAGE_COPY_DST));
    AT(dvz_drp2_stream_begin_command_encoder(setup, 7));
    AT(dvz_drp2_stream_copy_buffer_to_buffer(setup, 7, 1, 0, 6, 0, 16));
    AT(dvz_drp2_stream_finish_command_encoder(setup, 7, 8));
    AT(dvz_drp2_stream_queue_submit(setup, 8, 9));

    DvzDrp2CommandStream* replacement = dvz_drp2_stream();
    ANN(replacement);
    AT(dvz_drp2_stream_create_buffer(
        replacement, 1, 32,
        DVZ_DRP2_BUFFER_USAGE_UNIFORM | DVZ_DRP2_BUFFER_USAGE_COPY_DST |
            DVZ_DRP2_BUFFER_USAGE_MAP_READ));
    const Drp2TestAllocationSite sites[] = {
        DRP2_TEST_ALLOC_BUFFER_WRAPPER,
        DRP2_TEST_ALLOC_DESCRIPTOR_REPLACEMENTS,
        DRP2_TEST_ALLOC_DESCRIPTOR_WRAPPER,
        DRP2_TEST_ALLOC_DEFERRED_RESERVE,
    };
    const uint32_t ordinals[] = {1, 1, 2, 1};
    for (uint32_t i = 0; i < sizeof(sites) / sizeof(sites[0]); i++)
    {
        dvz_drp2_runtime_reset(runtime);
        DvzDrp2ValidationResult result = dvz_drp2_runtime_execute(runtime, setup);
        AT(result.ok);
        Drp2VkliteState* state = runtime->vklite_state;
        Drp2VkliteObject* buffer = _vklite_find(state, 1);
        Drp2VkliteObject* first_group = _vklite_find(state, 4);
        Drp2VkliteObject* second_group = _vklite_find(state, 5);
        ANN(buffer);
        ANN(first_group);
        ANN(second_group);
        DvzBuffer* old_buffer = buffer->buffer;
        DvzDescriptors* old_first_descriptors = first_group->descriptors;
        DvzDescriptors* old_second_descriptors = second_group->descriptors;
        uint32_t old_deferred_count = state->deferred_count;
        if (sites[i] == DRP2_TEST_ALLOC_DEFERRED_RESERVE)
        {
            state->retirement_borrowed_command_buffer =
                (VkCommandBuffer)(uintptr_t)0x123;
        }
        runtime->test_alloc_site = sites[i];
        runtime->test_alloc_fail_at = ordinals[i];
        result = dvz_drp2_runtime_execute(runtime, replacement);
        AT(!result.ok);
        AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
        AT(result.command_index == 0);
        AT(runtime->backend_failed);
        AT(runtime->test_alloc_site == DRP2_TEST_ALLOC_NONE);
        AT(runtime->test_alloc_fail_at == 0);
        AT(_drp2_find_any_object(runtime->semantic_state, 1)->size == 16);
        AT(_vklite_find(state, 1)->buffer == old_buffer);
        AT(_vklite_find(state, 4)->descriptors == old_first_descriptors);
        AT(_vklite_find(state, 5)->descriptors == old_second_descriptors);
        AT(state->deferred_count == old_deferred_count);
        uint8_t downloaded[16] = {0};
        dvz_buffer_download(old_buffer, 0, sizeof(downloaded), downloaded);
        for (uint32_t j = 0; j < sizeof(downloaded); j++)
            AT(downloaded[j] == j + 1);
        state->retirement_borrowed_command_buffer = VK_NULL_HANDLE;

        DvzDrp2CommandStream* rejected = dvz_drp2_stream();
        ANN(rejected);
        result = dvz_drp2_runtime_execute(runtime, rejected);
        AT(!result.ok);
        AT(result.code == DVZ_DRP2_VALIDATION_INVALID_STATE);
        dvz_drp2_stream_destroy(rejected);
        dvz_drp2_runtime_reset(runtime);
        AT(!runtime->backend_failed);
        result = dvz_drp2_runtime_execute(runtime, setup);
        AT(result.ok);
    }
    AT(drp2_test_vklite_validation_clean(suite, ctx));

    dvz_drp2_stream_destroy(replacement);
    dvz_drp2_stream_destroy(setup);
    return 0;
}



/**
 * Keep a configured buffer wrapper reusable when allocation-wrapper creation fails.
 *
 * @param suite test suite
 * @param item test item
 * @return 0 on success
 */
int test_drp2_runtime_vklite_buffer_allocation_wrapper_failure(
    TstContext* suite, const TstCase* item)
{
    ANN(suite);
    (void)item;
    if (!drp2_test_vklite_runtime_available())
    {
        tst_skip(suite, "Vulkan instance creation failed");
        return 0;
    }
    DvzGpuCtxConfig gpu_cfg = dvz_testing_gpu_ctx_config(suite);
    DvzGpuCtx* ctx = dvz_gpu_ctx(&gpu_cfg);
    if (ctx == NULL)
    {
        tst_skip(suite, "no GPU");
        return 0;
    }
    DvzBuffer* buffer = dvz_buffer_create_wrapper();
    ANN(buffer);
    dvz_buffer(dvz_gpu_ctx_device(ctx), dvz_gpu_ctx_alloc(ctx), buffer);
    dvz_buffer_size(buffer, 64);
    dvz_buffer_usage(buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    const DvzAllocator* previous_allocator = dvz_get_allocator();
    DvzAllocator failing_allocator = *previous_allocator;
    failing_allocator.calloc_fn = _reject_calloc;
    dvz_set_allocator(&failing_allocator);
    int create_result = dvz_buffer_create(buffer);
    dvz_set_allocator(previous_allocator);
    AT(create_result != 0);
    AT(dvz_buffer_handle(buffer) == VK_NULL_HANDLE);
    AT(dvz_buffer_create(buffer) == 0);
    AT(dvz_buffer_handle(buffer) != VK_NULL_HANDLE);
    dvz_buffer_destroy(buffer);
    dvz_buffer_free(buffer);
    AT(drp2_test_vklite_validation_clean(suite, ctx));
    dvz_gpu_ctx_destroy(ctx);
    return 0;
}
#endif
