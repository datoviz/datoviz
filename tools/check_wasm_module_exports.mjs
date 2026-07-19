#!/usr/bin/env node

import { dirname, join, resolve } from "node:path";
import { pathToFileURL } from "node:url";

const modulePath = resolve(
  process.argv[2] ?? "build-wasm-scene/wasm/datoviz_wasm_scene.mjs",
);
const { default: createModule } = await import(pathToFileURL(modulePath).href);
const Module = await createModule({
  locateFile: (path) => join(dirname(modulePath), path),
});

function requireExport(condition, name) {
  if (!condition) throw new Error(`WASM module does not expose ${name}`);
}

requireExport(Module.FS !== undefined, "its filesystem");
for (const method of ["mkdirTree", "writeFile", "unlink"]) {
  requireExport(typeof Module.FS[method] === "function", `FS.${method}`);
}

console.log(`WASM module exports OK: ${modulePath}`);
