function noop() {}

function requireValue(condition, message) {
  if (!condition) throw new Error(message);
}

function byteCount(value, label) {
  const count = Number(value);
  requireValue(Number.isSafeInteger(count) && count >= 0, `${label} must be a byte count`);
  return count;
}

function responseWithBody(response, body) {
  return new Response(body, {
    status: response.status,
    statusText: response.statusText,
    headers: response.headers,
  });
}

export class NetworkProgress {
  constructor(callback = noop) {
    this.callback = typeof callback === "function" ? callback : noop;
    this.resources = new Map();
    this.finished = false;
  }

  expect(id, total) {
    requireValue(!this.finished, "network progress is already finished");
    const key = String(id);
    const expected = byteCount(total, `${key} total`);
    const previous = this.resources.get(key);
    if (previous !== undefined) {
      requireValue(previous.total === expected, `${key} has conflicting byte totals`);
      return;
    }
    this.resources.set(key, { loaded: 0, total: expected });
  }

  add(id, count) {
    const key = String(id);
    const resource = this.resources.get(key);
    requireValue(resource !== undefined, `unexpected network resource ${key}`);
    resource.loaded = Math.min(resource.total, resource.loaded + byteCount(count, `${key} chunk`));
    this.emit();
  }

  complete(id) {
    const key = String(id);
    const resource = this.resources.get(key);
    requireValue(resource !== undefined, `unexpected network resource ${key}`);
    resource.loaded = resource.total;
    this.emit();
  }

  snapshot() {
    let loaded = 0;
    let total = 0;
    for (const resource of this.resources.values()) {
      loaded += resource.loaded;
      total += resource.total;
    }
    return {
      phase: this.finished || loaded === total ? "complete" : "download",
      loaded,
      total,
      percentage: total === 0 ? 100 : Math.min(100, Math.floor((100 * loaded) / total)),
    };
  }

  emit() {
    this.callback(this.snapshot());
  }

  finish() {
    for (const id of this.resources.keys()) this.complete(id);
    this.finished = true;
    this.emit();
  }
}

export function trackResponse(response, id, tracker) {
  if (tracker === null || tracker === undefined) return response;
  const resource = tracker.resources.get(String(id));
  requireValue(resource !== undefined, `unexpected network resource ${id}`);
  if (response.body === null || response.body === undefined) {
    tracker.complete(id);
    return response;
  }
  const stream = response.body.pipeThrough(
    new TransformStream({
      transform(chunk, controller) {
        tracker.add(id, chunk.byteLength);
        controller.enqueue(chunk);
      },
      flush() {
        tracker.complete(id);
      },
    }),
  );
  return responseWithBody(response, stream);
}

export async function trackedFetch(fetchImpl, url, id, tracker, options = undefined) {
  const response = await fetchImpl(url, options);
  if (!response.ok) return response;
  return trackResponse(response, id, tracker);
}

export async function withTrackedFetch(fetchImpl, targets, tracker, callback) {
  if (tracker === null || tracker === undefined || targets.size === 0) return await callback();
  const originalFetch = globalThis.fetch;
  requireValue(typeof originalFetch === "function", "fetch is unavailable for the WASM runtime");
  const wrappedFetch = async (input, options) => {
    const response = await fetchImpl(input, options);
    const href = new URL(
      typeof input === "string" || input instanceof URL ? input : input.url,
      globalThis.location?.href ?? "http://localhost/",
    ).href;
    const id = targets.get(href);
    return id === undefined || !response.ok ? response : trackResponse(response, id, tracker);
  };
  globalThis.fetch = wrappedFetch;
  try {
    return await callback();
  } finally {
    if (globalThis.fetch === wrappedFetch) globalThis.fetch = originalFetch;
  }
}

export function validateWasmAssetManifest(manifest) {
  requireValue(manifest?.schema === "datoviz.wasm-assets.v1", "unsupported WASM asset manifest");
  requireValue(
    typeof manifest.version === "string" && manifest.version.length > 0,
    "missing WASM asset version",
  );
  const required = ["datoviz_wasm_scene.mjs", "datoviz_wasm_scene.wasm", "datoviz_wasm_scene.data"];
  for (const name of required) {
    const artifact = manifest.artifacts?.[name];
    byteCount(artifact?.bytes, `${name} size`);
    requireValue(
      typeof artifact?.sha256 === "string" && artifact.sha256.length === 64,
      `${name} hash is invalid`,
    );
  }
  return manifest;
}
