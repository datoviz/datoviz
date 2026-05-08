# Next steps

- No immediate follow-up is required for this warning fix.
- If a future cleanup touches the SPIR-V resource or shader path, keep the raw-byte contract and aligned runtime copy in place.

## Risks

- The DRP2 shader-module command still borrows SPIR-V bytes until execution, so future refactors must preserve that lifetime contract.
- Vulkan shader creation still requires 4-byte aligned code at the point of `vkCreateShaderModule`, which is now handled by the runtime copy.
