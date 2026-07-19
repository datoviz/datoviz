import {
  NetworkProgress,
  trackedFetch,
  validateWasmAssetManifest,
  withTrackedFetch,
} from "../web/wasm/network.js";

function requireOk(condition, message) {
  if (!condition) throw new Error(message);
}

const events = [];
const progress = new NetworkProgress((event) => events.push(event));
progress.expect("runtime:wasm", 6);
progress.expect("runtime:data", 4);
progress.emit();

const fetchImpl = async () => new Response(
  new Uint8Array([1, 2, 3, 4, 5, 6]),
  { headers: { "Content-Type": "application/wasm" } },
);
const response = await trackedFetch(
  fetchImpl,
  "https://example.test/runtime.wasm",
  "runtime:wasm",
  progress,
);
requireOk((await response.arrayBuffer()).byteLength === 6, "tracked response changed its body");
progress.complete("runtime:data");
progress.finish();
requireOk(events.at(-1).phase === "complete", "network progress did not complete");
requireOk(
  events.at(-1).loaded === 10 && events.at(-1).percentage === 100,
  "aggregate byte count is wrong",
);
requireOk(
  events.every((event, index) => index === 0 || event.percentage >= events[index - 1].percentage),
  "network percentage moved backward",
);

const manifest = validateWasmAssetManifest({
  schema: "datoviz.wasm-assets.v1",
  version: "sha256-test",
  artifacts: Object.fromEntries(
    ["datoviz_wasm_scene.mjs", "datoviz_wasm_scene.wasm", "datoviz_wasm_scene.data"].map(
      (name) => [name, { bytes: 1, sha256: "0".repeat(64) }],
    ),
  ),
});
requireOk(manifest.version === "sha256-test", "asset manifest validation changed the version");

const wrappedEvents = [];
const wrappedProgress = new NetworkProgress((event) => wrappedEvents.push(event));
const target = "https://example.test/runtime.data?v=sha256-test";
wrappedProgress.expect("runtime:data", 3);
await withTrackedFetch(
  async () => new Response(new Uint8Array([7, 8, 9])),
  new Map([[target, "runtime:data"]]),
  wrappedProgress,
  async () => {
    const tracked = await globalThis.fetch(target);
    requireOk((await tracked.arrayBuffer()).byteLength === 3, "wrapped fetch changed its body");
  },
);
requireOk(wrappedProgress.snapshot().loaded === 3, "wrapped fetch did not report bytes");

console.log("PASS WebGPU network progress smoke");
