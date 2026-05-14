import { executeDrp2StreamChecked, initWebGPU } from "./drp2_webgpu.js";

const rowsEl = document.querySelector("#fixture-rows");
const runAllEl = document.querySelector("#run-all");
const summaryEl = document.querySelector("#summary");

let runtime = null;
let fixtures = [];
let running = false;



function basename(path) {
  return path.split("/").pop();
}



function setSummary() {
  const counts = { pass: 0, fail: 0, unsupported: 0, pending: 0, running: 0 };
  for (const fixture of fixtures) {
    counts[fixture.status] = (counts[fixture.status] ?? 0) + 1;
  }
  summaryEl.textContent =
    `${fixtures.length} positive fixtures: ` +
    `${counts.pass} pass, ${counts.unsupported} unsupported, ${counts.fail} fail`;
}



function setFixtureStatus(fixture, status, detail = "") {
  fixture.status = status;
  fixture.statusEl.textContent = status;
  fixture.statusEl.className = `status-${status}`;
  fixture.detailEl.textContent = detail;
  setSummary();
}



function unsupportedMessage(error) {
  const message = error?.message ?? String(error);
  if (
    message.startsWith("unsupported ") ||
    message.includes("unsupported DRP2 command") ||
    message.includes("only one color attachment is supported")
  ) {
    return message;
  }
  return null;
}



async function runFixture(fixture) {
  setFixtureStatus(fixture, "running");
  try {
    const response = await fetch(fixture.path, { cache: "no-cache" });
    if (!response.ok) {
      throw new Error(`${response.status} ${response.statusText}`);
    }
    const stream = await response.json();
    const result = await executeDrp2StreamChecked(
      runtime.device,
      runtime.context,
      runtime.format,
      stream,
    );
    const detail = result.readbacks.length > 0
      ? `readbacks=${result.readbacks.length}, nonzero=${result.readbacks[0].summary.nonzero}`
      : "no WebGPU errors";
    setFixtureStatus(fixture, "pass", detail);
  } catch (error) {
    const unsupported = unsupportedMessage(error);
    if (unsupported !== null) {
      setFixtureStatus(fixture, "unsupported", unsupported);
    } else {
      setFixtureStatus(fixture, "fail", error?.message ?? String(error));
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
  } finally {
    running = false;
    runAllEl.disabled = false;
  }
}



function addFixture(path) {
  const tr = document.createElement("tr");
  const nameTd = document.createElement("td");
  const statusTd = document.createElement("td");
  const detailTd = document.createElement("td");
  const code = document.createElement("code");

  code.textContent = basename(path);
  nameTd.appendChild(code);
  statusTd.textContent = "pending";
  statusTd.className = "status-pending";
  detailTd.textContent = "";

  tr.appendChild(nameTd);
  tr.appendChild(statusTd);
  tr.appendChild(detailTd);
  rowsEl.appendChild(tr);

  fixtures.push({
    path,
    status: "pending",
    statusEl: statusTd,
    detailEl: detailTd,
  });
}



async function main() {
  try {
    runtime = await initWebGPU();
    const response = await fetch("./fixture_manifest.json", { cache: "no-cache" });
    if (!response.ok) {
      throw new Error(`failed to load fixture manifest: ${response.status} ${response.statusText}`);
    }
    const manifest = await response.json();
    for (const path of manifest.positive) {
      addFixture(path);
    }
    runAllEl.disabled = false;
    runAllEl.addEventListener("click", () => {
      runAll().catch((error) => {
        summaryEl.textContent = error?.message ?? String(error);
      });
    });
    setSummary();
  } catch (error) {
    summaryEl.textContent = error?.message ?? String(error);
  }
}



main();
