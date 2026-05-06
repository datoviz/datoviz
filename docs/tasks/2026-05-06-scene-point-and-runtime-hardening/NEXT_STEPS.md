# Next Steps

## Remaining follow-up

- Decide whether the live-stream guard should remain B1+B5 (release on destroy, scene-wide scope) or
  later relax toward execute-or-destroy release and/or finer-than-scene tracking.
- Consider whether `dvz_visual_destroy()` also deserves direct focused regression coverage, even though
  it now shares the same live-stream guard path as other scene-owned visual destruction.
- Consider extending live-stream protection to any future scene APIs that mutate figure/panel topology if
  those operations should also be forbidden while emitted streams are live.

## Resume commands

- `cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DDVZ_BUILD_SCENE=ON -DDVZ_BUILD_DRP2=ON -DDVZ_BUILD_APP=ON`
- `cmake --build build`
- `just test scene`
- `just test drp2`
- `git diff --check`
