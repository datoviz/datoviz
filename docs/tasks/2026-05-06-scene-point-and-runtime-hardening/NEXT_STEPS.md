# Next Steps

## Remaining follow-up

- Keep the borrowed-pointer lifetime contract around emitted streams explicit in docs/tests.
- Consider adding a dedicated mutation-after-emit rejection/guard path if the runtime contract should
  become stricter than documentation-only guidance.
- Consider extending runtime readback/capture guards beyond buffer-download bounds into broader
  layout/size assertions where practical.

## Resume commands

- `cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DDVZ_BUILD_SCENE=ON -DDVZ_BUILD_DRP2=ON -DDVZ_BUILD_APP=ON`
- `cmake --build build`
- `just test scene`
- `just test drp2`
- `git diff --check`
