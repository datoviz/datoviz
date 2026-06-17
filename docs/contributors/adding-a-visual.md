# Adding A Visual

Adding a visual is a cross-cutting scene change. Treat it as a public feature, not as an isolated
shader or example.


## Start From The Contract

Before editing code, identify the visual's public contract:

1. durable semantics in `spec/scene/visuals/`;
2. API and status classification in `spec/scene/api/API_SURFACE.md` and
   `docs/reference/feature-status.md`;
3. rendering and lowering rules in `spec/scene/implementation/`;
4. required example coverage in `spec/docs/EXAMPLE_COVERAGE.md` and
   `spec/scene/examples/`.

If the visual needs a new DRP2 command, read [Adding A DRP2 Command](adding-a-drp2-command.md)
first. Most visuals should use existing upload, bind, pipeline, and draw commands.


## Implementation Route

Use the existing scene path:

```text
public scene API -> retained visual state -> frame plan ->
DRP2 command stream -> vklite/WebGPU backend execution
```

Typical edit points are:

| Layer | Expected change |
| --- | --- |
| public API | declarations in `include/datoviz/`, implementation in `src/scene/` |
| visual state | family-specific retained state, attributes, defaults, validation |
| lowering | visual descriptor, draw contract, uploads, metadata |
| shaders | GLSL and WGSL only when the visual needs new shader behavior |
| runtime | backend support through existing DRP2 commands and descriptor contracts |
| docs/examples | minimal example, feature/status row, reference notes |

Keep family-specific behavior behind the family descriptor or lowering helper. Do not add concrete
visual checks to generic render emission unless the normalized interface is missing a reusable fact.


## Shader And ABI Checks

When changing shader inputs, layouts, generated shader registries, or visual pipeline bindings, run:

```sh
just shader-abi-check
```

Update both GLSL and WGSL paths when the visual is meant to participate in the WebGPU subset. If
WebGPU support is deferred, classify that gap explicitly instead of adding a divergent scene
contract.


## Tests And Examples

Every public visual should have:

1. focused construction/lifetime tests for invalid sizes, missing attributes, and repeated updates;
2. frame-plan or DRP2 stream-shape tests for emitted resources, pipelines, binds, and draws;
3. runtime smoke or readback coverage when the environment supports it;
4. one minimal example under `examples/c/visuals/`;
5. manifest metadata and generated docs updated with the example.

Use [Adding Examples](adding-examples.md) for manifest and copy-safety rules.


## Validation

Start narrow, then broaden according to the touched layers:

```sh
just build
just test scene
just test drp2
just spec-check
git diff --check
```

For Vulkan, canvas, app, or live presentation changes on macOS, prefer:

```sh
direnv exec . just test scene
```

Record any unsupported platform, WebGPU, or optional-provider limitation in the feature/status docs
instead of leaving it implicit.
