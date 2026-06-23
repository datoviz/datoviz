import {
  Drp2WebGpuRuntime,
  WebGpuDemoSession,
  executeDrp2StreamChecked,
  initWebGPU,
} from "../../web/drp2/webgpu.js";
import { DatovizWasmScene } from "../../web/wasm/scene.js";

const rowsEl = document.querySelector("#fixture-rows");
const stressRowsEl = document.querySelector("#stress-rows");
const wasmRowsEl = document.querySelector("#wasm-rows");
const runAllEl = document.querySelector("#run-all");
const summaryEl = document.querySelector("#summary");
const stressSummaryEl = document.querySelector("#stress-summary");
const wasmSummaryEl = document.querySelector("#wasm-summary");
const viewportEl = document.querySelector("#viewport");

let runtime = null;
let fixtures = [];
let stressRows = [];
let wasmRows = [];
let running = false;

const STRESS_FRAME_COUNT = 10;
const STRESS_STREAMS = [
  { name: "runtime repeat: scene_point_wgsl", path: "./streams/scene_point_wgsl.json" },
  { name: "runtime repeat: scene_primitive_wgsl", path: "./streams/scene_primitive_wgsl.json" },
  { name: "runtime repeat: texture_sampling_wgsl", path: "./streams/texture_sampling_wgsl.json" },
  { name: "runtime repeat: attachment_depth_wgsl", path: "./streams/attachment_depth_wgsl.json" },
];
const DEMO_STRESS_CHECKS = [
  { name: "demo path: resize reload", fn: runDemoResizeStress },
  { name: "demo path: stream reload", fn: runDemoStreamReloadStress },
];
const WASM_SCENE_CHECKS = [
  { name: "wasm scene: 2D update/reload/lifecycle", fn: runWasm2DSceneSmoke },
  { name: "wasm scene: 3D update/lifecycle", fn: runWasm3DSceneSmoke },
];



function basename(path) {
  return path.split("/").pop();
}



function setSummary() {
  const counts = { pass: 0, fail: 0, unsupported: 0, pending: 0, running: 0 };
  for (const fixture of fixtures) {
    counts[fixture.status] = (counts[fixture.status] ?? 0) + 1;
  }
  summaryEl.textContent =
    `${fixtures.length} WebGPU checks: ` +
    `${counts.pass} pass, ${counts.unsupported} unsupported, ${counts.fail} fail, ` +
    `${counts.running} running, ${counts.pending} pending`;
}



function setStressSummary() {
  const counts = { pass: 0, fail: 0, pending: 0, running: 0 };
  for (const row of stressRows) {
    counts[row.status] = (counts[row.status] ?? 0) + 1;
  }
  stressSummaryEl.textContent =
    `${stressRows.length} runtime stress checks: ` +
    `${counts.pass} pass, ${counts.fail} fail, ` +
    `${counts.running} running, ${counts.pending} pending`;
}



function setWasmSummary() {
  const counts = { pass: 0, fail: 0, pending: 0, running: 0 };
  for (const row of wasmRows) {
    counts[row.status] = (counts[row.status] ?? 0) + 1;
  }
  wasmSummaryEl.textContent =
    `${wasmRows.length} WASM scene checks: ` +
    `${counts.pass} pass, ${counts.fail} fail, ` +
    `${counts.running} running, ${counts.pending} pending`;
}



function setFixtureStatus(fixture, status, detail = "") {
  fixture.status = status;
  fixture.statusEl.textContent = status;
  fixture.statusEl.className = `status-${status}`;
  fixture.detailEl.textContent = detail;
  setSummary();
}



function setStressStatus(row, status, detail = "") {
  row.status = status;
  row.statusEl.textContent = status;
  row.statusEl.className = `status-${status}`;
  row.detailEl.textContent = detail;
  setStressSummary();
}



function setWasmStatus(row, status, detail = "") {
  row.status = status;
  row.statusEl.textContent = status;
  row.statusEl.className = `status-${status}`;
  row.detailEl.textContent = detail;
  setWasmSummary();
}



function unsupportedMessage(error) {
  const message = error?.detail ?? error?.message ?? String(error);
  if (
    message.startsWith("unsupported ") ||
    message.includes("unsupported DRP2 command")
  ) {
    return message;
  }
  return null;
}



function errorDetail(error) {
  if (error?.commandIndex !== undefined || error?.code !== undefined || error?.cmd !== undefined) {
    const parts = [];
    if (error.commandIndex !== undefined) {
      parts.push(`command_index=${error.commandIndex}`);
    }
    if (error.cmd !== undefined && error.cmd !== null) {
      parts.push(`cmd=${error.cmd}`);
    }
    if (error.code !== undefined) {
      parts.push(`code=${error.code}`);
    }
    const detail = error.detail ?? error.message ?? String(error);
    return `${parts.join(" ")}: ${detail}`;
  }
  return error?.message ?? String(error);
}



function expectedFailureDetail(stream) {
  const expected = stream.expected ?? {};
  const expectedCmd = stream.commands?.[expected.command_index]?.cmd;
  return {
    commandIndex: expected.command_index,
    cmd: expectedCmd,
    code: expected.code,
  };
}



function errorMatchesExpected(error, expected) {
  return (
    error?.commandIndex === expected.commandIndex &&
    error?.cmd === expected.cmd &&
    error?.code === expected.code
  );
}



function expectedDetail(expected) {
  return `expected command_index=${expected.commandIndex} cmd=${expected.cmd} code=${expected.code}`;
}



function comparableResourceStats(stats) {
  const { refs: _refs, ...stable } = stats;
  return stable;
}



function assertResourceStatsStable(actual, expected, label) {
  const actualText = JSON.stringify(actual);
  const expectedText = JSON.stringify(expected);
  if (actualText !== expectedText) {
    throw new Error(`${label}: expected ${expectedText}, got ${actualText}`);
  }
}



function assertNoActiveRefs(stats, label) {
  if (stats.refs.open !== 0 || stats.refs.recorded !== 0 || stats.refs.submitted !== 0) {
    throw new Error(
      `${label}: resource refs leaked open=${stats.refs.open} recorded=${stats.refs.recorded} ` +
        `submitted=${stats.refs.submitted}`,
    );
  }
}



function assertWasmSceneDestroyed(scene, label) {
  if (scene.scene !== 0 || scene.figure !== 0 || scene.runtime !== null) {
    throw new Error(`${label}: WASM scene did not release JS-side handles`);
  }
}



function assertPositiveCommandCount(stream, label) {
  if (!stream || !Array.isArray(stream.commands) || stream.commands.length === 0) {
    throw new Error(`${label}: emitted no DRP2 commands`);
  }
}



function makeWasmTexture(width, height, phase) {
  const pixels = new Uint8Array(width * height * 4);
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = (y * width + x) * 4;
      const checker = ((x >> 1) + (y >> 1) + phase) & 1;
      pixels[i + 0] = checker ? 55 : 225;
      pixels[i + 1] = checker ? 165 : 245;
      pixels[i + 2] = checker ? 235 : 105;
      pixels[i + 3] = 255;
    }
  }
  return pixels;
}



function addWasm2DPoints(scene, panel) {
  const points = scene.visual("point");
  points.setF32("position", new Float32Array([
    -0.65, -0.45, 0,
    -0.18, 0.5, 0,
    0.38, -0.12, 0,
  ]), 3);
  points.setRGBA8("color", new Uint8Array([
    80, 200, 245, 255,
    245, 130, 95, 255,
    150, 130, 245, 255,
  ]), 3);
  points.setF32("diameter_px", new Float32Array([16, 20, 18]), 3);
  scene.addVisual(panel, points);
  return points;
}



function addWasm2DPixels(scene, panel) {
  const pixels = scene.visual("pixel");
  pixels.setF32("position", new Float32Array([
    -0.72, 0.18, 0.02,
    -0.52, 0.42, 0.02,
    -0.32, 0.18, 0.02,
    -0.12, 0.42, 0.02,
  ]), 4);
  pixels.setRGBA8("color", new Uint8Array([
    60, 190, 245, 255,
    120, 225, 170, 255,
    245, 175, 85, 255,
    210, 105, 220, 255,
  ]), 4);
  pixels.setF32("pixel_size_px", new Float32Array([8, 10, 9, 11]), 4);
  scene.addVisual(panel, pixels);
  return pixels;
}



function addWasm2DMarkers(scene, panel) {
  const markers = scene.visual("marker");
  markers.setF32("position", new Float32Array([
    0.08, 0.18, 0.04,
    0.28, 0.42, 0.04,
    0.48, 0.18, 0.04,
    0.68, 0.42, 0.04,
  ]), 4);
  markers.setRGBA8("color", new Uint8Array([
    245, 125, 90, 235,
    80, 210, 195, 235,
    170, 130, 245, 235,
    245, 215, 90, 235,
  ]), 4);
  markers.setF32("diameter_px", new Float32Array([12, 14, 13, 15]), 4);
  markers.setF32("angle", new Float32Array([0, 0.35, 0.7, 1.05]), 4);
  markers.setU32("symbol", new Uint32Array([0, 1, 2, 3]), 4);
  scene.addVisual(panel, markers);
  return markers;
}


function addWasm2DSegments(scene, panel) {
  const segments = scene.visual("segment");
  segments.setF32("position_start", new Float32Array([
    -0.76, -0.68, 0.08,
    -0.46, -0.68, 0.08,
    -0.16, -0.68, 0.08,
  ]), 3);
  segments.setF32("position_end", new Float32Array([
    -0.58, -0.34, 0.08,
    -0.28, -0.46, 0.08,
    -0.02, -0.28, 0.08,
  ]), 3);
  segments.setRGBA8("color", new Uint8Array([
    80, 205, 245, 230,
    245, 165, 75, 230,
    135, 225, 150, 230,
  ]), 3);
  segments.setF32("stroke_width_px", new Float32Array([4, 7, 5]), 3);
  scene.addVisual(panel, segments);
  return segments;
}



function addWasmImage(scene, panel) {
  const image = scene.visual("image");
  image.setF32("position", new Float32Array([
    0.05, -0.72, 0.05,
    0.05, -0.12, 0.05,
    0.72, -0.72, 0.05,
    0.72, -0.12, 0.05,
  ]), 4);
  image.setF32("texcoords", new Float32Array([0, 0, 0, 1, 1, 0, 1, 1]), 4);
  image.setTextureRGBA8(makeWasmTexture(4, 4, 0), 4, 4);
  scene.addVisual(panel, image);
  return image;
}



function makeWasmCubeMesh(size) {
  const s = size / 2;
  const faces = [
    { n: [0, 0, 1], c: [90, 170, 255, 255], v: [[-s, -s, s], [s, -s, s], [-s, s, s], [s, -s, s], [s, s, s], [-s, s, s]] },
    { n: [0, 0, -1], c: [245, 135, 190, 255], v: [[s, -s, -s], [-s, -s, -s], [s, s, -s], [-s, -s, -s], [-s, s, -s], [s, s, -s]] },
    { n: [1, 0, 0], c: [85, 220, 170, 255], v: [[s, -s, s], [s, -s, -s], [s, s, s], [s, -s, -s], [s, s, -s], [s, s, s]] },
    { n: [-1, 0, 0], c: [250, 180, 80, 255], v: [[-s, -s, -s], [-s, -s, s], [-s, s, -s], [-s, -s, s], [-s, s, s], [-s, s, -s]] },
    { n: [0, 1, 0], c: [160, 120, 245, 255], v: [[-s, s, s], [s, s, s], [-s, s, -s], [s, s, s], [s, s, -s], [-s, s, -s]] },
    { n: [0, -1, 0], c: [245, 115, 95, 255], v: [[-s, -s, -s], [s, -s, -s], [-s, -s, s], [s, -s, -s], [s, -s, s], [-s, -s, s]] },
  ];
  const positions = [];
  const colors = [];
  const normals = [];
  for (const face of faces) {
    for (const vertex of face.v) {
      positions.push(...vertex);
      colors.push(...face.c);
      normals.push(...face.n);
    }
  }
  return {
    positions: new Float32Array(positions),
    colors: new Uint8Array(colors),
    normals: new Float32Array(normals),
    count: positions.length / 3,
  };
}



function addWasm3DContent(scene, panel) {
  const cube = makeWasmCubeMesh(1.2);
  const mesh = scene.visual("mesh");
  mesh.setF32("position", cube.positions, cube.count);
  mesh.setRGBA8("color", cube.colors, cube.count);
  mesh.setF32("normal", cube.normals, cube.count);
  scene.addVisual(panel, mesh);
  return { mesh, cube };
}



async function runWasm2DSceneSmoke(row) {
  const scene = await DatovizWasmScene.create(viewportEl, { gpu: runtime });
  try {
    const panel = scene.panelFull();
    const points = addWasm2DPoints(scene, panel);
    const pixels = addWasm2DPixels(scene, panel);
    const markers = addWasm2DMarkers(scene, panel);
    const segments = addWasm2DSegments(scene, panel);
    const image = addWasmImage(scene, panel);

    const initial = await scene.renderInitial();
    assertPositiveCommandCount(initial, `${row.name} initial`);

    points.setRGBA8("color", new Uint8Array([
      245, 220, 90, 255,
      80, 210, 195, 255,
      225, 100, 170, 255,
    ]), 3);
    pixels.setF32("pixel_size_px", new Float32Array([10, 8, 11, 9]), 4);
    markers.setU32("symbol", new Uint32Array([3, 2, 1, 0]), 4);
    segments.setF32("stroke_width_px", new Float32Array([6, 4, 8]), 3);
    const update = await scene.renderIncremental();
    assertPositiveCommandCount(update, `${row.name} update`);

    image.setTextureRGBA8(makeWasmTexture(8, 8, 1), 8, 8);
    const reload = await scene.renderIncremental();
    assertPositiveCommandCount(reload, `${row.name} reload`);

    const stats = scene.runtime.resourceStats();
    const detail =
      `initial=${initial.commands.length}, update=${update.commands.length}, ` +
      `reload=${reload.commands.length}, objects=${stats.objects}`;
    scene.destroy();
    assertWasmSceneDestroyed(scene, row.name);
    setWasmStatus(row, "pass", detail);
  } catch (error) {
    scene.destroy();
    throw error;
  }
}



async function runWasm3DSceneSmoke(row) {
  const scene = await DatovizWasmScene.create(viewportEl, { gpu: runtime });
  try {
    const panel = scene.panelFull();
    scene.setCamera(panel);
    const { mesh, cube } = addWasm3DContent(scene, panel);
    scene.attachArcball(panel);
    scene.attachControllerInput(() => {});

    const initial = await scene.renderInitial();
    assertPositiveCommandCount(initial, `${row.name} initial`);

    const colors = new Uint8Array(cube.colors);
    for (let i = 0; i < colors.length; i += 4) {
      colors[i + 0] = 255 - colors[i + 0];
      colors[i + 1] = Math.max(40, colors[i + 1] - 35);
      colors[i + 2] = Math.min(255, colors[i + 2] + 20);
    }
    mesh.setRGBA8("color", colors, cube.count);
    const update = await scene.renderIncremental();
    assertPositiveCommandCount(update, `${row.name} update`);

    const stats = scene.runtime.resourceStats();
    const detail =
      `initial=${initial.commands.length}, update=${update.commands.length}, ` +
      `objects=${stats.objects}`;
    scene.destroy();
    assertWasmSceneDestroyed(scene, row.name);
    setWasmStatus(row, "pass", detail);
  } catch (error) {
    scene.destroy();
    throw error;
  }
}



async function createDemoSession(streamName) {
  const session = new WebGpuDemoSession(
    runtime.device,
    runtime.context,
    runtime.format,
    runtime.capabilities,
    { canvas: viewportEl },
  );
  await session.loadStream(streamName);
  return session;
}



function stressDetail(stats, frames) {
  return `frames=${frames}, objects=${stats.objects}, submitted_refs=${stats.refs.submitted}`;
}



async function fetchStream(path) {
  const response = await fetch(path, { cache: "no-cache" });
  if (!response.ok) {
    throw new Error(`${response.status} ${response.statusText}`);
  }
  return await response.json();
}



async function runStressRow(row) {
  setStressStatus(row, "running");
  try {
    if (typeof row.fn === "function") {
      await row.fn(row);
      return;
    }
    const stream = await fetchStream(row.path);
    const retainedRuntime = new Drp2WebGpuRuntime(
      runtime.device,
      runtime.context,
      runtime.format,
      {
        canvas: viewportEl,
        capabilities: runtime.capabilities,
        requireExplicitBindGroupLayouts: true,
        requireExplicitPipelineMetadata: true,
      },
    );
    await retainedRuntime.load(stream);
    await retainedRuntime.render();
    assertNoActiveRefs(retainedRuntime.resourceStats(), `${row.name} frame 1`);

    const stableStats = comparableResourceStats(retainedRuntime.resourceStats());
    for (let i = 1; i < STRESS_FRAME_COUNT; i++) {
      await retainedRuntime.render();
      const stats = retainedRuntime.resourceStats();
      assertResourceStatsStable(
        comparableResourceStats(stats),
        stableStats,
        `${row.name} frame ${i + 1}`,
      );
      assertNoActiveRefs(stats, `${row.name} frame ${i + 1}`);
    }

    const stats = retainedRuntime.resourceStats();
    setStressStatus(row, "pass", stressDetail(stats, STRESS_FRAME_COUNT));
  } catch (error) {
    setStressStatus(row, "fail", errorDetail(error));
  }
}



async function runWasmRow(row) {
  setWasmStatus(row, "running");
  try {
    await row.fn(row);
  } catch (error) {
    setWasmStatus(row, "fail", errorDetail(error));
  }
}



async function runDemoResizeStress(row) {
  const session = await createDemoSession("scene_point_wgsl");
  await session.render();
  const stableStats = comparableResourceStats(session.resourceStats());
  const oldWidth = viewportEl.width;
  const oldHeight = viewportEl.height;

  viewportEl.width = 1;
  viewportEl.height = 1;
  await session.render();
  const stats = session.resourceStats();
  assertResourceStatsStable(comparableResourceStats(stats), stableStats, row.name);
  assertNoActiveRefs(stats, row.name);

  viewportEl.width = oldWidth;
  viewportEl.height = oldHeight;
  setStressStatus(row, "pass", stressDetail(stats, 2));
}



async function runDemoStreamReloadStress(row) {
  const session = await createDemoSession("scene_point_wgsl");
  await session.render();
  const pointStats = comparableResourceStats(session.resourceStats());

  await session.loadStream("texture_sampling_wgsl");
  await session.render();
  assertNoActiveRefs(session.resourceStats(), `${row.name} texture reload`);

  await session.loadStream("scene_point_wgsl");
  await session.render();
  const stats = session.resourceStats();
  assertResourceStatsStable(comparableResourceStats(stats), pointStats, `${row.name} point reload`);
  assertNoActiveRefs(stats, `${row.name} point reload`);
  setStressStatus(row, "pass", stressDetail(stats, 3));
}



async function runPositiveFixture(fixture, stream) {
  const result = await executeDrp2StreamChecked(
    runtime.device,
    runtime.context,
    runtime.format,
    stream,
    {
      canvas: viewportEl,
      capabilities: runtime.capabilities,
      requireExplicitBindGroupLayouts: true,
      requireExplicitPipelineMetadata: true,
    },
  );
  const detail = result.readbacks.length > 0
    ? `readbacks=${result.readbacks.length}, nonzero=${result.readbacks[0].summary.nonzero}`
    : "no WebGPU errors";
  setFixtureStatus(fixture, "pass", detail);
}



async function runNegativeFixture(fixture, stream) {
  const expected = expectedFailureDetail(stream);
  try {
    await executeDrp2StreamChecked(runtime.device, runtime.context, runtime.format, stream, {
      canvas: viewportEl,
      capabilities: runtime.capabilities,
    });
  } catch (error) {
    if (errorMatchesExpected(error, expected)) {
      setFixtureStatus(fixture, "pass", expectedDetail(expected));
    } else {
      setFixtureStatus(fixture, "fail", `${expectedDetail(expected)}; got ${errorDetail(error)}`);
    }
    return;
  }

  setFixtureStatus(fixture, "fail", `${expectedDetail(expected)}; got success`);
}



async function runFixture(fixture) {
  setFixtureStatus(fixture, "running");
  try {
    const stream = await fetchStream(fixture.path);
    if (fixture.kind === "negative") {
      await runNegativeFixture(fixture, stream);
    } else {
      await runPositiveFixture(fixture, stream);
    }
  } catch (error) {
    const unsupported = unsupportedMessage(error);
    if (unsupported !== null) {
      setFixtureStatus(fixture, "unsupported", errorDetail(error));
    } else {
      setFixtureStatus(fixture, "fail", errorDetail(error));
    }
  }
}



async function runAll() {
  if (running) {
    return;
  }
  running = true;
  runAllEl.disabled = true;
  try {
    for (const fixture of fixtures) {
      await runFixture(fixture);
    }
    for (const row of stressRows) {
      await runStressRow(row);
    }
    for (const row of wasmRows) {
      await runWasmRow(row);
    }
  } finally {
    running = false;
    runAllEl.disabled = false;
  }
}



function addRow(container, name, kind = "") {
  const tr = document.createElement("tr");
  const nameTd = document.createElement("td");
  const statusTd = document.createElement("td");
  const detailTd = document.createElement("td");
  const kindEl = document.createElement("span");
  const code = document.createElement("code");

  kindEl.textContent = kind;
  code.textContent = name;
  nameTd.appendChild(kindEl);
  nameTd.appendChild(code);
  statusTd.textContent = "pending";
  statusTd.className = "status-pending";
  detailTd.textContent = "";

  tr.appendChild(nameTd);
  tr.appendChild(statusTd);
  tr.appendChild(detailTd);
  container.appendChild(tr);

  return { status: "pending", statusEl: statusTd, detailEl: detailTd };
}



function addFixture(path, kind = "fixture") {
  const row = addRow(rowsEl, basename(path), `${kind} `);

  fixtures.push({
    path,
    kind,
    ...row,
  });
}



function addStressRow(config) {
  const row = addRow(stressRowsEl, config.name);
  stressRows.push({
    ...config,
    ...row,
  });
}



function addWasmRow(config) {
  const row = addRow(wasmRowsEl, config.name);
  wasmRows.push({
    ...config,
    ...row,
  });
}



async function runWasmRows() {
  for (const row of wasmRows) {
    await runWasmRow(row);
  }
  return wasmSummaryEl.textContent;
}



async function main() {
  try {
    runtime = await initWebGPU(viewportEl);
    summaryEl.title = `WebGPU capabilities: ${JSON.stringify(runtime.capabilities)}`;
    const response = await fetch("./fixture_manifest.json", { cache: "no-cache" });
    if (!response.ok) {
      throw new Error(`failed to load fixture manifest: ${response.status} ${response.statusText}`);
    }
    const manifest = await response.json();
    for (const path of manifest.positive) {
      addFixture(path, "fixture");
    }
    for (const path of manifest.webgpu_streams ?? []) {
      addFixture(path, "stream");
    }
    for (const path of manifest.negative_parity ?? []) {
      addFixture(path, "negative");
    }
    for (const config of STRESS_STREAMS) {
      addStressRow(config);
    }
    for (const config of DEMO_STRESS_CHECKS) {
      addStressRow(config);
    }
    for (const config of WASM_SCENE_CHECKS) {
      addWasmRow(config);
    }
    window.__datovizRunWasmRows = runWasmRows;
    runAllEl.disabled = false;
    runAllEl.addEventListener("click", () => {
      runAll().catch((error) => {
        summaryEl.textContent = error?.message ?? String(error);
      });
    });
    setSummary();
    setStressSummary();
    setWasmSummary();
  } catch (error) {
    summaryEl.textContent = error?.message ?? String(error);
    stressSummaryEl.textContent = "Runtime stress unavailable";
    wasmSummaryEl.textContent = "WASM scene checks unavailable";
  }
}



main();
