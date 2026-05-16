# DRP2/vklite Descriptor Refresh Plan

> Status: implemented on 2026-05-16.
> Created: 2026-05-16.

## Implementation Notes

Implemented in the DRP2/vklite runtime:

1. `_vklite_create_bind_group()` now saves bind-group metadata and delegates descriptor allocation
   and population to `_vklite_build_bind_group_descriptors()`.
2. `_vklite_refresh_dependent_bind_groups()` rebuilds descriptor wrappers for live bind groups that
   reference a recreated resource id, then retires the old wrapper immediately or through the
   borrowed-command-buffer deferred-destroy queue.
3. Stable-id `CreateTexture`, `CreateBuffer`, and `CreateSampler` recreation all trigger the refresh
   path after the replacement backend object has been created.
4. Semantic validation now permits stable-id buffer and sampler recreation only after the object has
   been referenced by submitted work, matching the existing texture replacement rule. Recreated
   buffers are rejected when saved live bind-group ranges or usage flags would become invalid.
5. WBOIT and depth-peeling scene emitters no longer use target extent changes as sampled bind-group
   cache invalidators. They keep fingerprints only for the semantic dependency ids and sampler id.

Focused coverage added:

1. DRP2/vklite execution samples a texture through an existing bind group, recreates the same
   texture id at a new extent, and draws again without re-emitting `CreateBindGroup`.
2. DRP2/vklite refresh with a simulated active borrowed command buffer defers the retired descriptor
   wrapper.
3. Semantic validation covers buffer/sampler stable-id recreation through an existing bind group and
   rejects recreated buffers that would invalidate saved descriptor ranges.
4. Scene WBOIT/depth-peeling regressions now assert that resize emits recreated textures without
   re-emitting the sampled resolve/composite bind groups.

## Problem

The scene/runtime path keeps stable logical DRP2 resource ids across frames. When a texture id is
re-created at a new extent or format, the vklite runtime replaces the Vulkan image and image view
behind that id. Existing bind groups that sample that texture still own descriptor sets written
with the old image view. If those descriptor sets are bound after the backing image view is
destroyed, Vulkan validation reports invalid sampled-image descriptors and the device may be lost.

The recent depth-peeling and WBOIT fixes avoid this locally by rebuilding each technique's sampled
bind group when a target extent changes. Those local fingerprints are tactical guardrails, not the
desired long-term architecture. They should eventually be superseded by a generic DRP2/vklite
runtime invariant:

> A live bind group in the runtime must always describe the current backend handles of every
> resource id it references.

## Goal

Move stale-descriptor handling out of individual scene techniques and into the DRP2/vklite runtime.
Scene code should be able to reuse stable logical texture, buffer, and sampler ids without each
multi-pass technique hand-rolling descriptor cache fingerprints for resize/recreation safety.

## Proposed Runtime Pattern

1. Extract descriptor allocation/population from `_vklite_create_bind_group()` into a reusable
   helper, for example `_vklite_build_bind_group_descriptors()`.
2. Add a helper that detects whether a bind group references a recreated resource id through one of
   its saved `bind_group_entries`.
3. Add a refresh helper, for example `_vklite_refresh_dependent_bind_groups(state, resource_id,
   command_index)`, that iterates live `DRP2_OBJECT_BIND_GROUP` objects and rebuilds descriptor
   wrappers for any dependent bind group.
4. Call the refresh helper after `_vklite_create_texture()` replaces an existing live texture id.
5. Generalize the trigger to buffer and sampler recreation once the texture path is validated,
   because stable-id replacement can also stale buffer/sampler descriptors.
6. Do not update Vulkan descriptor sets in place. Allocate a fresh `DvzDescriptors` wrapper, write it
   from the saved bind-group entries, swap it onto the bind-group object, then destroy or defer the
   old descriptor wrapper using the existing borrowed-command-buffer deferred-destroy machinery.

## Lifetime Requirements

Descriptor refresh must preserve command-buffer safety:

1. If a borrowed frame command buffer is active, old descriptor wrappers may still be referenced by
   command buffers submitted for the previous frame. Defer their destruction against
   `state->active_borrowed_command_buffer`.
2. If no borrowed frame command buffer is active, destroy the retired descriptor wrapper
   immediately.
3. Rebuild failures must leave the runtime in a valid state or fail the current command without
   leaking the freshly allocated wrapper.
4. The refresh path should not destroy or mutate the dependent bind-group object until the new
   descriptor wrapper has been fully allocated and populated.

## Test Plan

Add DRP2/vklite-focused coverage before deleting scene-local guardrails:

1. A semantic/runtime test where a sampled texture id is bound, then `CreateTexture` reuses the same
   id with a different extent, and a subsequent draw samples it without re-emitting
   `CreateBindGroup`.
2. A borrowed-frame or simulated active-command-buffer test that verifies the old descriptor wrapper
   is deferred rather than destroyed immediately.
3. A scene regression that demonstrates WBOIT/depth-peeling resize remains valid after removing the
   technique-local extent fingerprints.

## Cleanup After Generic Fix

Once runtime-level refresh is implemented and validated, revisit the local scene patches:

1. Remove or simplify the WBOIT resolve bind-group extent fingerprint if runtime refresh fully
   covers stable texture id recreation.
2. Remove or simplify the depth-peeling sampled bind-group extent fingerprint for the same reason.
3. Keep technique-local bind-group cache keys only for real semantic dependencies that change the
   resource ids or binding shape, not for backend handle freshness after resize.
