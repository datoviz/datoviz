# Invalidation and Caching

Invalidation is the way retained scene changes become bounded runtime work. Caching is the matching
runtime behavior: reuse backend objects when the scene contract has not changed.

## Invalidate by Meaning

Datoviz should invalidate by meaning, not by convenience. A visual data update invalidates the
attribute and any derived upload stream. A visibility change invalidates draw planning. A resize
invalidates framebuffer and viewport-dependent state. A material or technique change may invalidate
pipeline-compatible state. A controller change invalidates transforms and frame work, but not the
underlying data arrays.

## Work Classes

The frame planner turns invalidation into three broad classes of work:

- setup work, for resources or pipelines whose shape or contract changed;
- update work, for retained resources whose content changed without changing shape;
- frame work, for commands that need to run each frame, such as render passes, draws, copies, and
  presentation or capture requests.

## Runtime Caching

Caching belongs below the scene boundary. The scene decides that a buffer, texture, pipeline, or
draw contract is needed. The runtime may keep backend objects alive and refresh only the changed
parts, but it should not infer new scene semantics from cached state.

## Interaction Cost

Good invalidation keeps interaction responsive. Panning a dense point cloud should update transform
state and redraw; it should not re-upload every point. Animating colors should upload changed color
data; it should not rebuild pipelines. Resizing a window should recreate size-dependent attachments
without losing retained visual data.

## Debugging

When debugging stale or missing output, ask which state changed and which work should have been
invalidated: data upload, resource shape, transform, panel layout, draw visibility, render target,
or capture/readback request.

See also:

- [Retained resources](retained-resources.md)
- [Frame lifecycle](frame-lifecycle.md)
- [Performance model](performance-model.md)
