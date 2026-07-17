# Windows RC1 Wheel Build Handoff

Status: active RC1 blocker. Updated: 2026-07-17.

This handoff is for a Codex session running locally on Windows. Read
[`../../AGENTS.md`](../../AGENTS.md) first, then debug the Windows build locally until the complete
native build succeeds.


## Repository Position

- Branch: `v0.4-dev`
- Expected starting commit: `7d69a864b` (`build: restore Windows object lifecycle exports`)
- Linux x86_64/aarch64 and macOS Intel/arm64 wheels pass at this commit.
- Do not modify or commit the `data` or `external/msdf-atlas-gen` submodules.
- Do not stage generated DLLs, libraries, wheels, or other build output.


## CI Failure Progression

The first RC1 wheel run was
[29593610274](https://github.com/datoviz/datoviz/actions/runs/29593610274). Both Windows jobs failed
while linking `datoviz_vk.dll` because these internal object-lifecycle symbols were no longer
exported from `datoviz_core.dll`:

```text
dvz_obj_init
dvz_obj_created
dvz_obj_destroyed
dvz_obj_is_created
```

Commit `7d69a864b` restored `DVZ_EXPORT` on those four declarations in `src/common/obj.h`.

The replacement run was
[29595414678](https://github.com/datoviz/datoviz/actions/runs/29595414678). The original linker
failure is resolved: both Windows jobs successfully link `datoviz_vk.dll` and proceed almost to the
end of the native build. They now fail compiling `src/scene/text/text_atlas.cpp`:

- [AMD64 job 87934855164](https://github.com/datoviz/datoviz/actions/runs/29595414678/job/87934855164)
- [ARM64 job 87934855173](https://github.com/datoviz/datoviz/actions/runs/29595414678/job/87934855173)

Representative errors:

```text
external/msdf-atlas-gen/msdfgen/core/arithmetics.hpp(11): error C2988:
unrecognizable template declaration/definition
external/msdf-atlas-gen/msdfgen/core/arithmetics.hpp(11): error C2146:
syntax error: missing ')' before identifier 'b'
external/msdf-atlas-gen/msdfgen/core/arithmetics.hpp(17): error C2059:
syntax error: '<parameter-list>'
```

Lines 11 and 17 define `msdfgen::min(T, T)` and `msdfgen::max(T, T)`. The likely cause is the
legacy Win32 `min` and `max` macros entering the translation unit through Windows/Vulkan headers.


## Preferred First Fix

Add `NOMINMAX` to the Windows compile definitions in `src/CMakeLists.txt`, beside
`VK_USE_PLATFORM_WIN32_KHR`:

```cmake
if(OS_WINDOWS)
    # Enable Win32 platform surface/interop declarations in Vulkan/Volk headers.
    list(APPEND DVZ_COMPILE_DEFINITIONS VK_USE_PLATFORM_WIN32_KHR NOMINMAX)
endif()
```

This is preferred over patching the vendored msdf-atlas sources. Verify the diagnosis with a local
MSVC build; do not assume this is the final Windows issue merely because it is the next error CI
revealed.


## Local Windows Loop

Start from an MSVC Developer PowerShell with the repository and submodules current:

```powershell
git switch v0.4-dev
git pull --ff-only origin v0.4-dev
git submodule update --init --recursive
```

After applying the first fix, reconfigure the existing build and compile the failing component:

```powershell
just build
cmake --build build --target datoviz_scene
```

If `just build` completes the full build, the second command is redundant. If the existing build
directory is unsuitable, reproduce the CI configuration using the local `VCPKG_ROOT`, the
appropriate `x64-windows` or `arm64-windows` triplet, Ninja, Debug mode, shaderc enabled, and tests
and examples disabled. The authoritative commands are in `.github/workflows/wheels.yml` under the
Windows `Build native artifacts` step.

Continue iterating on subsequent MSVC compilation or linkage failures until the complete native
build succeeds. Keep every fix narrow and avoid weakening the active v0.4 architecture or disabling
required release features to make the build pass.


## Completion Checklist

Before handing back:

1. The complete Windows native build succeeds, not only `datoviz_scene`.
2. Run `git diff --check`.
3. Run `git status --short` and inspect every changed or untracked path.
4. Stage only intentional source/build-system changes; exclude submodules and generated binaries.
5. Run `git diff --cached --stat` before committing.
6. Commit the focused fix set, but do not push unless the user explicitly approves that push in
   the Windows session.
7. Report the commit hash, changed files, exact build command, architecture, and validation result.
