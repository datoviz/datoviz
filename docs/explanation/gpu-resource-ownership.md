# GPU Resource Ownership

Datoviz separates scene ownership from GPU ownership. Scene objects describe what should exist;
runtime objects own or borrow the concrete GPU handles needed to execute a frame.

Owned handles may be created, refreshed, and destroyed by Datoviz. Borrowed handles come from a
host, platform, swapchain, window system, embedding provider, or interop API. Datoviz may use a
borrowed handle only within the contract that introduced it.

The practical rule is simple: do not destroy, reset, submit, transition, or otherwise take lifetime
authority over a borrowed object unless the API explicitly grants that authority. This applies to
swapchain images, host-owned windows, Qt surfaces, command buffers, textures, external memory, and
interop resources.

The scene layer should not manipulate GPU handles directly. It should emit frame artifacts and DRP2
streams. The runtime maps those streams to backend resources, command encoders, queues, barriers,
and presentation operations.

Synchronization is also ownership-sensitive. A compute pass that writes a storage buffer and a
render pass that reads it need an explicit ordering boundary. In v0.4, that boundary is represented
through DRP2 synchronization commands such as `ResourceBarrier`, then mapped by native and WebGPU
runtimes where supported. Hidden backend shortcuts should not replace the stream contract.

Capture and readback have borrowed-lifetime pressure. A mapped readback pointer, packet span, or
diagnostic string is usually valid only until a documented release point, the next emit, or object
destruction. Long-lived user data should be copied out of borrowed views.

See also:

- [Frame lifecycle](frame-lifecycle.md)
- [Retained resources](retained-resources.md)
- [Compute and graphics](../reference/compute-graphics.md)
