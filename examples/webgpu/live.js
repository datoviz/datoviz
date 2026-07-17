import { WasmSceneSession } from "../../web/wasm/session.js";
import { liveExampleById } from "./live_examples.js";

const canvas = document.querySelector("#viewport");
const statusEl = document.querySelector("#status");
const statsEl = document.querySelector("#stats");
const limitationsEl = document.querySelector("#limitations");
const limitationListEl = document.querySelector("#limitation-list");
const isEmbedded = new URLSearchParams(window.location.search).get("embedded") === "1";
let session = null;

const DESIGN_WIDTH = 1280;
const DESIGN_HEIGHT = 720;

function fitCanvasToStage() {
  const stage = canvas.parentElement;
  if (stage === null) return;
  const rect = stage.getBoundingClientRect();
  const scale = Math.min(rect.width / DESIGN_WIDTH, rect.height / DESIGN_HEIGHT);
  canvas.style.width = `${Math.max(1, Math.floor(DESIGN_WIDTH * scale))}px`;
  canvas.style.height = `${Math.max(1, Math.floor(DESIGN_HEIGHT * scale))}px`;
}

fitCanvasToStage();
const stageResizeObserver = new ResizeObserver(fitCanvasToStage);
stageResizeObserver.observe(canvas.parentElement);

function setStatus(message, isError = false) {
  statusEl.textContent = message;
  statusEl.classList.toggle("error", isError);
}

function setStats(message) {
  statsEl.textContent = message;
}

function showEffectLimitations(limitations) {
  limitationListEl.replaceChildren();
  for (const limitation of limitations) {
    const item = document.createElement("li");
    const effect = String(limitation.effect ?? "effect").replaceAll("-", " ").toUpperCase();
    item.textContent = `${effect}: ${limitation.warning}`;
    limitationListEl.append(item);
  }
  limitationsEl.hidden = isEmbedded || limitations.length === 0;
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
    dataBundles: example.dataBundles ?? [],
  };
  document.title = `${example.label} - Datoviz WebGPU`;
  showEffectLimitations(example.effectLimitations ?? []);
  destroySession();
  setStats("");
  session = new WasmSceneSession({
    canvas,
    logicalWidth: DESIGN_WIDTH,
    logicalHeight: DESIGN_HEIGHT,
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
  stageResizeObserver.disconnect();
  destroySession();
}, { once: true });

const params = new URLSearchParams(window.location.search);
const id = params.get("id") ?? "features_timer_animation";
loadLiveExample(id).catch((error) => {
  setStatus(error instanceof Error ? error.message : String(error), true);
  console.error(error);
});
