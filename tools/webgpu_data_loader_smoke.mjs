import { createHash } from "node:crypto";
import { mountDataBundles } from "../web/wasm/data_loader.js";

function requireOk(condition, message) {
  if (!condition) throw new Error(message);
}

class FakeFs {
  constructor() {
    this.files = new Map();
  }

  mkdirTree() {}

  writeFile(path, bytes) {
    this.files.set(path, Uint8Array.from(bytes));
  }

  unlink(path) {
    if (!this.files.delete(path)) throw new Error(`missing ${path}`);
  }
}

function response(body, type = "json") {
  return {
    ok: true,
    status: 200,
    statusText: "OK",
    async json() {
      requireOk(type === "json", "response is not JSON");
      return structuredClone(body);
    },
    async arrayBuffer() {
      requireOk(type === "bytes", "response is not binary");
      return Uint8Array.from(body).buffer;
    },
  };
}

const artifact = new TextEncoder().encode("datoviz-webgpu-data");
const sha256 = createHash("sha256").update(artifact).digest("hex");
const manifestUrl = "https://example.test/webgpu-data/examples/demo/sha256-test/manifest.json";
const artifactUrl = "https://example.test/webgpu-data/examples/demo/sha256-test/prepared/demo.bin";
const manifest = {
  schema: "datoviz.example-data.v1",
  id: "demo",
  status: "committed",
  web: {
    version: "sha256-test",
    virtual_root: "data/examples/demo",
    max_bytes: artifact.byteLength,
    required: true,
  },
  artifacts: [
    {
      path: "prepared/demo.bin",
      bytes: artifact.byteLength,
      sha256,
    },
  ],
};
let fetchCount = 0;
const fetchImpl = async (url) => {
  fetchCount++;
  if (String(url) === manifestUrl) return response(manifest);
  if (String(url) === artifactUrl) return response(artifact, "bytes");
  return { ok: false, status: 404, statusText: "Not Found" };
};
const Module = { FS: new FakeFs() };
const descriptor = {
  id: "demo",
  url: manifestUrl,
  virtualRoot: "data/examples/demo",
  required: true,
};

const first = await mountDataBundles(Module, [descriptor], { fetchImpl });
requireOk(first.length === 1 && first[0].cached === false, "first mount was not fresh");
requireOk(Module.FS.files.has("/data/examples/demo/prepared/demo.bin"), "artifact was not mounted");
requireOk(Module.FS.files.has("/data/examples/demo/manifest.json"), "manifest was not mounted");
const second = await mountDataBundles(Module, [descriptor], { fetchImpl });
requireOk(second.length === 1 && second[0].cached === true, "second mount was not cached");
requireOk(fetchCount === 3, "cached mount should fetch only the version manifest");

const unsafe = structuredClone(manifest);
unsafe.artifacts[0].path = "../escape.bin";
let rejected = false;
try {
  await mountDataBundles(
    { FS: new FakeFs() },
    [descriptor],
    { fetchImpl: async () => response(unsafe) },
  );
} catch (error) {
  rejected = String(error).includes("unsafe path segment");
}
requireOk(rejected, "unsafe artifact path was not rejected");
console.log("PASS WebGPU data loader smoke");
