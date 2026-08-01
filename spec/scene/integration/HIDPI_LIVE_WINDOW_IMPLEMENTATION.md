# Live HiDPI Window Implementation Record

Status: implemented v0.4 live-window sizing and coordinate contract. Updated: 2026-08-01.

Use [HIGH_DPI.md](HIGH_DPI.md) for durable coordinate semantics. This record preserves the implemented GLFW/App boundary.

## Implemented Contract

- Requested logical size, native window size, framebuffer size, device scale, user scale, and render scale remain distinct.
- GLFW live-window creation resolves native size from requested logical size and device scale instead of collapsing logical and physical dimensions.
- Backend resize and scale events update actual window/framebuffer metrics without silently redefining the requested logical contract.
- Figure, panel, input, text, viewport, scissor, capture, and presentation paths use their documented logical or framebuffer spaces.
- Focused GLFW/App tests cover high-DPI creation, metrics, resize, scale changes, and coordinate propagation where the host exposes those behaviors.

## Guardrails

Do not multiply sizes twice, infer framebuffer scale solely from requested dimensions, or expose backend-native pixels as logical scene coordinates. Preserve literal framebuffer dimensions for Vulkan targets and explicit logical-to-framebuffer conversion for input and layout.

Physical platform behavior remains evidence-sensitive: record hosts that do not deliver a requested resize or scale transition instead of weakening the coordinate contract.
