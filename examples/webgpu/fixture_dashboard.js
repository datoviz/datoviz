import {
  Drp2WebGpuRuntime,
  WebGpuDemoSession,
  executeDrp2StreamChecked,
  initWebGPU,
} from "./drp2_webgpu.js";

const rowsEl = document.querySelector("#fixture-rows");
const stressRowsEl = document.querySelector("#stress-rows");
const runAllEl = document.querySelector("#run-all");
const summaryEl = document.querySelector("#summary");
const stressSummaryEl = document.querySelector("#stress-summary");
const viewportEl = document.querySelector("#viewport");

let runtime = null;
let fixtures = [];
let stressRows = [];
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



async function createDemoSession(streamName) {
  const session = new WebGpuDemoSession(
    runtime.device,
    runtime.context,
    runtime.format,
    runtime.capabilities,
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



async function main() {
  try {
    runtime = await initWebGPU();
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
    runAllEl.disabled = false;
    runAllEl.addEventListener("click", () => {
      runAll().catch((error) => {
        summaryEl.textContent = error?.message ?? String(error);
      });
    });
    setSummary();
    setStressSummary();
  } catch (error) {
    summaryEl.textContent = error?.message ?? String(error);
    stressSummaryEl.textContent = "Runtime stress unavailable";
  }
}



main();
