# C Coding Rules

These rules apply whenever agents edit or create Datoviz C or C++ implementation files.


## Naming

Use the established naming conventions:

| Type | Pattern | Example |
| --- | --- | --- |
| Public functions | `dvz_<module>_<action>()` | `dvz_math_vec3_add()` |
| Internal functions | `static`, `_dvz_`, or module-local names | `_dvz_log_error()` |
| Files | lowercase, underscores | `_alloc.c` |
| Public headers | `include/datoviz/*.h` | `datoviz/math.h` |
| Internal headers | `_name.h` | `_alloc.h` |
| Macros | uppercase with `DVZ_` prefix | `DVZ_PI` |

Reserve the `dvz_` prefix for symbols exposed through public headers. Internal helpers should avoid
the public naming space.

Filenames beginning with `_` are legal in this codebase. Keep the existing `_dvz_*` internal
convention, but avoid introducing reserved identifiers such as double-underscore names or
underscore-capital names.


## Function Documentation

Document every new module-level function with a short Doxygen-style docstring immediately above the
definition.

Use this shape:

```c
/**
 * One-sentence summary.
 *
 * @param foo Description.
 * @param bar Description.
 * @return Description.
 */
```

Add `@param` tags for every argument and an `@return` tag when the function returns a value. Use
`@note` or `@see` when helpful.


## File Organization

Structure `*.c` files like the Vulkan modules in `src/vk/` and `src/vklite/`.

Use delimiter blocks for sections:

```c
/*************************************************************************************************/
/*  Section Name                                                                                 */
/*************************************************************************************************/
```

Use sections in this order when applicable:

1. `Includes`
2. `Constants`
3. `Macros`
4. `Typedefs`
5. `Structs`
6. `Function prototypes` or `Helpers`
7. `Functions`

Omit empty sections. Keep lines within 100 characters whenever possible.

Maintain three blank lines between neighboring top-level definitions: functions, structs, enums, and
other top-level blocks.


## Allocation, Copying, And I/O

Never call `malloc`, `calloc`, or `free` directly. Use the helpers declared in `_alloc.h` and
`_compat.h`.

Prefer:

1. `dvz_calloc` over `dvz_malloc` when zeroed memory suffices.
2. `dvz_free` for deallocation.
3. `dvz_memcpy` and `dvz_memset` over `memcpy` and `memset`.
4. `dvz_fprintf` and `dvz_vfprintf` over `fprintf` and `vfprintf`.

All structs should be zero-initialized before use.


## C++ Translation Units

Prefer C-style function signatures throughout the codebase, including `*.cpp` files.

Avoid C++ standard library types such as `std::vector`, `std::string`, references, and templates in
function signatures. Prefer plain C arguments: pointers, counts, and POD structs.


## Robustness And Undefined Behavior

Treat compiler warnings as defects. Prefer fixing warnings over suppressing them.

Avoid:

1. Signed integer overflow.
2. Out-of-bounds pointer arithmetic.
3. Uninitialized storage.
4. Use-after-free and use-after-destroy.
5. Strict-aliasing-sensitive casts.
6. Dereferencing possibly NULL pointers.

Check size arithmetic before allocation, indexing, byte copies, and row/stride calculations.
Validate `count * sizeof(T)`, byte offsets, dimensions, pitches, and downcasts from `size_t` or
`uint64_t` to narrower integer types.

Do not pass function calls, increments, assignments, or other side-effectful expressions directly to
`ANN()`, `ASSERT()`, or `DVZ_ASSUME()`. Evaluate the expression once into a local variable first.

In `examples/c`, keep `EXAMPLE_CHECK()` conditions simple and side-effect free. Evaluate mutating or
status-returning calls into locals first, then check those locals with specific failure messages.


## Ownership And Lifetime

Make ownership explicit. Every pointer or handle should be clearly owned or borrowed by the current
object.

Destroy/free paths must be idempotent:

1. Set pointers to `NULL`.
2. Set Vulkan handles to `VK_NULL_HANDLE`.
3. Never destroy borrowed handles.

Avoid retaining pointers into growable arrays, registries, or object tables across calls that may
append, destroy, compact, grow, or reallocate them. Reacquire by stable ID or index after mutation.

Keep runtime and test-control state instance-scoped. Avoid file-scope mutable state and reset test
hooks in fixture/object lifecycle helpers.

Use assertions for violated internal invariants, not for expected runtime failures. Public or
recoverable failure paths should return an error/status and clean up partially initialized objects.

Do not leave half-created objects in lookup tables unless they are marked destroyed or unusable and
future lookups cannot accidentally use them.

Add focused regression coverage for lifetime, bounds, ownership, and multi-frame bugs. Prefer tests
that fail before the fix.
