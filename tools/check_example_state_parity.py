#!/usr/bin/env python3
"""Compare canonical native/WASM scenario state, independently of GPU rendering."""

import json
import math
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "build/example-state-parity"


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    sources = ["tools/tests/fixtures/scenario_state.c", "examples/c/runner/scenario_helpers.c",
               "examples/c/example_common.c", "examples/c/example_style.c",
               "examples/c/example_random.c", "examples/c/example_controller_preview.c",
               "examples/c/showcases/cortical_activity.c", "examples/c/features/controller_arcball.c"]
    common = [*sources, "-Iinclude", "-Iexamples/c", "-Isrc/common", "-Isrc/scene",
              "-Iexternal/cglm/include",
              "-DDVZ_EXAMPLE_NO_APP=1", "-DDVZ_EXAMPLE_NO_MAIN=1", "-ffunction-sections",
              "-fdata-sections", "-O2", "-lm"]
    native = OUTPUT / "native-state"
    wasm = OUTPUT / "wasm-state.mjs"
    subprocess.run(["cc", *common, "-Wl,--gc-sections", "-Lbuild/src", "-ldatoviz",
                    "-Wl,-rpath," + str(ROOT / "build/src"), "-o", str(native)], cwd=ROOT, check=True)
    subprocess.run(["emcc", *common, "build-wasm-scene/src/libdatoviz.a", "-sENVIRONMENT=node",
                    "-sWASM_ASYNC_COMPILATION=0", "-sALLOW_MEMORY_GROWTH=1",
                    "-sEXPORTED_RUNTIME_METHODS=['FS']",
                    "-o", str(wasm)], cwd=ROOT, check=True)
    report = {"scope": "actual canonical scene/controller CPU state; no GPU rendering", "examples": {}}
    for name in ("cortical", "arcball"):
        direct = subprocess.check_output([str(native), name], cwd=ROOT, text=True)
        runner = (f"import fs from 'node:fs'; import create from {json.dumps(wasm.as_uri())};"
                  f"await create({{arguments:[{json.dumps(name)}],preRun:[m=>{{"
                  "m.FS.mkdirTree('/.cache/datoviz/examples/cortical_activity/prepared');"
                  "m.FS.writeFile('/.cache/datoviz/examples/cortical_activity/prepared/cortical_activity.bin',"
                  "fs.readFileSync('.cache/datoviz/examples/cortical_activity/prepared/cortical_activity.bin'));"
                  "}]});")
        compiled = subprocess.check_output(["node", "--input-type=module", "-e", runner], cwd=ROOT, text=True)
        a, b = json.loads(direct), json.loads(compiled)
        assert len(a) == len(b)
        assert all(math.isclose(x, y, rel_tol=2e-6, abs_tol=2e-6) for x, y in zip(a, b)), (name, a, b)
        assert math.isclose(a[41] / a[0], math.exp(0.05), rel_tol=2e-6)
        if name == "cortical":
            assert a[9:25] == a[25:41], "presentation must pause during controller interaction"
            assert all(
                math.isclose(x, y, rel_tol=2e-6) for x, y in zip(a[58:62], [0.225, 0.864, 0.129, 56.687]))
        report["examples"][name] = {"native": a, "wasm": b, "max_absolute_difference": max(abs(x-y) for x,y in zip(a,b))}
    (OUTPUT / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("Canonical native/WASM scene state parity passed: cortical, arcball; camera/time/pause/drag/zoom")


if __name__ == "__main__":
    main()
