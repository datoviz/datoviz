const mountedBundles = new WeakMap();

function requireValue(condition, message) {
  if (!condition) throw new Error(message);
}

function safeRelativePath(value, label) {
  requireValue(typeof value === "string" && value.length > 0, `${label} must be a nonempty path`);
  requireValue(!value.startsWith("/"), `${label} must be relative`);
  const parts = value.split("/");
  requireValue(
    parts.every((part) => part.length > 0 && part !== "." && part !== ".."),
    `${label} contains an unsafe path segment`,
  );
  return parts.join("/");
}

function virtualRoot(value) {
  requireValue(typeof value === "string" && value.length > 0, "bundle virtualRoot is required");
  const normalized = value.startsWith("/") ? value.slice(1) : value;
  return `/${safeRelativePath(normalized, "bundle virtualRoot")}`;
}

function artifactBytes(artifact) {
  const value = artifact.byte_size ?? artifact.bytes;
  requireValue(Number.isSafeInteger(value) && value >= 0, `invalid byte size for ${artifact.path}`);
  return value;
}

async function sha256Hex(bytes) {
  requireValue(globalThis.crypto?.subtle !== undefined, "Web Crypto SHA-256 is unavailable");
  const digest = await globalThis.crypto.subtle.digest("SHA-256", bytes);
  return Array.from(new Uint8Array(digest), (value) => value.toString(16).padStart(2, "0")).join("");
}

function removeMountedFiles(FS, mounted) {
  if (mounted === undefined) return;
  for (const path of mounted.paths.slice().reverse()) {
    try {
      FS.unlink(path);
    } catch (_) {
      // A missing stale file is already in the desired state.
    }
  }
}

export async function mountDataBundles(Module, bundles, options = {}) {
  if (bundles === undefined || bundles === null || bundles.length === 0) return [];
  requireValue(Array.isArray(bundles), "dataBundles must be an array");
  const FS = Module?.FS;
  requireValue(FS !== undefined, "WASM module does not expose its filesystem");
  const fetchImpl = options.fetchImpl ?? globalThis.fetch;
  requireValue(typeof fetchImpl === "function", "fetch is unavailable for WebGPU data bundles");
  const verifyHashes = options.verifyHashes !== false;
  let moduleBundles = mountedBundles.get(Module);
  if (moduleBundles === undefined) {
    moduleBundles = new Map();
    mountedBundles.set(Module, moduleBundles);
  }

  const mounted = [];
  for (const descriptor of bundles) {
    requireValue(descriptor !== null && typeof descriptor === "object", "invalid data bundle descriptor");
    const id = String(descriptor.id ?? "");
    requireValue(id.length > 0, "data bundle id is required");
    requireValue(typeof descriptor.url === "string" && descriptor.url.length > 0, `${id}: bundle URL is required`);
    const response = await fetchImpl(descriptor.url);
    if (!response.ok) {
      if (descriptor.required === false) continue;
      throw new Error(`${id}: bundle manifest fetch failed (${response.status} ${response.statusText})`);
    }
    const manifest = await response.json();
    requireValue(manifest?.schema === "datoviz.example-data.v1", `${id}: unsupported bundle schema`);
    requireValue(manifest.status === "committed", `${id}: bundle status is not web-eligible`);
    requireValue(manifest.web !== null && typeof manifest.web === "object", `${id}: missing web metadata`);
    const version = String(manifest.web.version ?? "");
    requireValue(version.length > 0, `${id}: missing bundle version`);
    const root = virtualRoot(descriptor.virtualRoot ?? manifest.web.virtual_root);
    requireValue(
      virtualRoot(manifest.web.virtual_root) === root,
      `${id}: descriptor and manifest virtual roots differ`,
    );
    const maxBytes = Number(manifest.web.max_bytes);
    requireValue(Number.isSafeInteger(maxBytes) && maxBytes >= 0, `${id}: invalid browser byte budget`);
    const artifacts = manifest.artifacts;
    requireValue(Array.isArray(artifacts) && artifacts.length > 0, `${id}: bundle has no artifacts`);
    const paths = new Set();
    let totalBytes = 0;
    for (const artifact of artifacts) {
      artifact.path = safeRelativePath(artifact.path, `${id}: artifact path`);
      requireValue(!paths.has(artifact.path), `${id}: duplicate artifact path ${artifact.path}`);
      paths.add(artifact.path);
      totalBytes += artifactBytes(artifact);
    }
    requireValue(totalBytes <= maxBytes, `${id}: bundle exceeds its browser byte budget`);

    const previous = moduleBundles.get(id);
    if (previous?.version === version && previous.root === root) {
      mounted.push({ id, version, root, bytes: previous.bytes, cached: true });
      continue;
    }
    removeMountedFiles(FS, previous);
    FS.mkdirTree(root);
    const writtenPaths = [];
    const manifestUrl = new URL(descriptor.url, globalThis.location?.href ?? "http://localhost/");
    for (const artifact of artifacts) {
      const artifactUrl = new URL(artifact.path, manifestUrl);
      const artifactResponse = await fetchImpl(artifactUrl.href);
      requireValue(
        artifactResponse.ok,
        `${id}: artifact fetch failed for ${artifact.path} (${artifactResponse.status} ${artifactResponse.statusText})`,
      );
      const bytes = await artifactResponse.arrayBuffer();
      requireValue(bytes.byteLength === artifactBytes(artifact), `${id}: byte-size mismatch for ${artifact.path}`);
      if (verifyHashes && typeof artifact.sha256 === "string" && artifact.sha256.length > 0) {
        requireValue(await sha256Hex(bytes) === artifact.sha256.toLowerCase(), `${id}: SHA-256 mismatch for ${artifact.path}`);
      }
      const outputPath = `${root}/${artifact.path}`;
      FS.mkdirTree(outputPath.slice(0, outputPath.lastIndexOf("/")));
      FS.writeFile(outputPath, new Uint8Array(bytes));
      writtenPaths.push(outputPath);
    }
    const manifestPath = `${root}/manifest.json`;
    FS.writeFile(manifestPath, new TextEncoder().encode(`${JSON.stringify(manifest, null, 2)}\n`));
    writtenPaths.push(manifestPath);
    const record = { id, version, root, bytes: totalBytes, paths: writtenPaths };
    moduleBundles.set(id, record);
    mounted.push({ id, version, root, bytes: totalBytes, cached: false });
  }
  return mounted;
}
