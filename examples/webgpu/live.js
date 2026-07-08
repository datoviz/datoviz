import { WasmSceneSession } from "../../web/wasm/session.js";
import { liveExampleById } from "./live_examples.js";

const canvas = document.querySelector("#viewport");
const titleEl = document.querySelector("#title");
const statusEl = document.querySelector("#status");
const statsEl = document.querySelector("#stats");
let session = null;

function setStatus(message, isError = false) {
  statusEl.textContent = message;
  statusEl.classList.toggle("error", isError);
}

function setStats(message) {
  statsEl.textContent = message;
}

function destroySession() {
  if (session !== null) {
    session.destroy();
    session = null;
  }
  window.__datovizWasmSession = null;
  window.__datovizWasmScene = null;
}

async function loadLiveExample(id) {
  const example = liveExampleById(id);
  if (example === null) {
    throw new Error(`unknown WebGPU live example ${id}`);
  }
  const demo = {
    id: example.id,
    label: example.label,
    scenarioId: example.scenarioId,
    animate: example.animate === true,
  };
  titleEl.textContent = example.label;
  destroySession();
  setStats("");
  session = new WasmSceneSession({
    canvas,
    status: setStatus,
    stats: setStats,
    onScene(scene) {
      window.__datovizWasmScene = scene;
    },
  });
  window.__datovizWasmSession = session;
  await session.load(demo);
}

window.addEventListener("pagehide", () => {
  destroySession();
}, { once: true });

const params = new URLSearchParams(window.location.search);
const id = params.get("id") ?? "features_timer_animation";
loadLiveExample(id).catch((error) => {
  setStatus(error instanceof Error ? error.message : String(error), true);
  console.error(error);
});
