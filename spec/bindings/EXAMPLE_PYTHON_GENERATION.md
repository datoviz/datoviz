# C Example To Python Facade Generation

Status: proposed v0.4 documentation and example-generation direction.

This note records the intended path for generating Python examples from the canonical C examples.
It sits beside the raw `ctypes` and array-aware facade specs because the generated Python examples
depend on both layers: exact C symbol metadata and policy-declared NumPy argument adaptation.


## Goal

Datoviz v0.4 examples should remain C-first while allowing documentation to present Python examples
that are mechanically derived, reviewable, and easy to refresh when the C examples change.

The preferred documentation shape is:

```text
C | Python
```

where `Python` means the top-level array-aware facade:

```python
import datoviz as dvz
```

Raw `ctypes` examples remain useful for ABI validation, debugging, and advanced FFI documentation,
but they should not be the default user-facing Python tab once the facade exists.


## Source Of Truth

The source of truth should be:

```text
examples/c/<lane>/<name>.c          canonical runnable C example
examples/c/<lane>/<name>.dvzpy.yml  optional conversion hints
generated/examples/python/<name>.py generated Python facade example
```

The generated Python file should be treated as build or documentation output. It should not carry
additional design truth that is missing from the C example or sidecar policy.

Full Python examples should not be embedded in C comments. Co-locating full examples in comments
creates a second source of truth, makes the C harder to read, and still does not guarantee that the
Python version is regenerated when the C changes.


## Generation Pipeline

The example generator should consume three inputs:

1. the C example source;
2. `build/bindings/datoviz_api.json` for function, enum, record, and typedef facts;
3. binding and example policy from `spec/bindings/ctypes.yml` and optional `.dvzpy.yml` sidecars.

The generated Python should preserve `dvz_*` names and rely on the array-aware facade for declared
string, pointer/count, and pointer/byte-size adaptation.

The generator should fail with a specific diagnostic when it cannot translate a construct safely.
It should not silently invent pointer ownership, array shape, callback lifetime, or vectorization
semantics.


## Sidecar Hints

Sidecar files should contain only facts that cannot be inferred safely from C syntax or API
metadata. They should be small, structured, and machine-checkable.

Example shape:

```yaml
python:
  status: generated

arrays:
  positions:
    dtype: float32
    shape: [POINT_COUNT, 3]
  colors:
    dtype: uint8
    shape: [POINT_COUNT, 4]
  diameters:
    dtype: float32
    shape: [POINT_COUNT]

loops:
  _fill_points:
    mode: preserve
  _fill_field:
    mode: vectorize_grid_2d

uploads:
  dvz_visual_set_data_many:
    expand: true
```

Sidecars may also mark an example as intentionally manual:

```yaml
python:
  status: manual
  reason: "interactive GUI callback and mutable runtime state"
```


## Loop Handling

Dataset-generation loops should be handled in two tiers.

The first tier is faithful conversion: allocate NumPy arrays and preserve the loop in Python. This
is expressive enough for correctness and for validating that the generated example follows the C
example.

The second tier is optional vectorization. It should be applied only by named recognizers or
explicit sidecar policy, such as:

1. one-dimensional samples over `i`;
2. two-dimensional scalar fields over `y, x`;
3. three-dimensional volume fields over `z, y, x`;
4. regular grid positions;
5. repeated constant colors, widths, diameters, or shapes;
6. simple `linspace`, `meshgrid`, `column_stack`, `tile`, and `ravel` rewrites.

Unsupported loops should remain Python loops. Vectorization is a readability and performance pass,
not a requirement for generated examples to be correct.


## Expressiveness Tiers

The public C examples should be classified by generation difficulty.

`generated` examples are expected to translate mechanically with no or small sidecars. This should
cover most retained visual and feature examples that allocate arrays, fill data, set style records,
upload visual data, create a view, and run for a fixed frame count.

`generated-with-hints` examples require sidecar policy for array shapes, sampled-field views,
visual data update expansion, data-to-visual transforms, or vectorization preferences.

`manual` examples remain C-first but may have separately authored Python versions. This tier is for
callbacks, GUI state, file I/O, complex mutable runtime state, external data preparation, or examples
whose Python version is expected to be intentionally different from the C implementation.

The expected initial classification is:

1. most `examples/c/visuals/*.c`: `generated` or `generated-with-hints`;
2. many `examples/c/features/*.c`: `generated-with-hints`;
3. composed workflows and showcases: `generated-with-hints` or `manual`;
4. scientific/data-loading examples such as protein viewers: usually `manual` until data and helper
   policy is explicit.


## Agent Workflow

Agents may write or update C examples, but the durable Python path should remain deterministic.

Recommended workflow:

1. write or update the canonical C example;
2. run the example Python generator;
3. if generation fails, add the smallest useful `.dvzpy.yml` hint;
4. regenerate the Python facade example;
5. validate the C example and generated Python example through their narrow smoke commands.

Agents should not paste full Python examples into C comments. If co-located hints are ever needed,
they should be short structured annotations that the generator can parse and validate. Sidecar YAML
is preferred because it is easier to diff, lint, and route through documentation generation.


## Documentation Tabs

User-facing docs should prefer:

```text
C | Python
```

Advanced binding docs may use:

```text
C | Python facade | Python raw ctypes
```

The raw `ctypes` tab should be reserved for low-level binding documentation, ABI examples, and
debugging. It should not become the primary Python story for scientific users.


## Validation

The first implementation slice should validate:

1. sidecar schema parsing;
2. generated Python imports through `import datoviz as dvz`;
3. array dtype, shape, contiguity, and lifetime behavior through the array facade;
4. expansion of `DvzVisualDataUpdate` arrays into facade calls or a policy-backed wrapper;
5. faithful Python-loop fallback for unsupported vectorization;
6. docs-tab generation from the C and generated Python outputs;
7. per-example status reporting: `generated`, `generated-with-hints`, `manual`, or `unsupported`.

Where practical, dataset helpers should compare generated NumPy arrays against C-produced reference
data or fixture snapshots. For graphics examples, the normal C and Python smoke commands should
remain the final proof.


## Non-Goals

This system must not provide:

1. a general-purpose C-to-Python transpiler;
2. a Python plotting API or prefixless alias layer;
3. guessed pointer/count relationships outside binding policy;
4. automatic vectorization of arbitrary C loops;
5. callback or ownership semantics that are not declared by the binding policy;
6. a replacement for hand-written Python examples where the Python workflow is intentionally
   different from the C workflow.
