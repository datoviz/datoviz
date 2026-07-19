import { WasmSceneSession } from "../../web/wasm/session.js";
import { NetworkLoadingOverlay } from "./loading.js";

const DEMOS = [
  {
    id: "wasm-2d",
    label: "WASM 2D scene",
    load: () => import("./demos/wasm_2d.js"),
  },
  {
    id: "wasm-3d",
    label: "WASM 3D arcball",
    load: () => import("./demos/wasm_3d.js"),
  },
  {
    id: "wasm-timer-animation",
    label: "WASM timer animation",
    load: () => import("./demos/wasm_timer_animation.js"),
  },
  {
    id: "wasm-picking",
    label: "WASM picking",
    load: () => import("./demos/wasm_picking.js"),
  },
  {
    id: "wasm-image-probe",
    label: "WASM image probe",
    load: () => import("./demos/wasm_image_probe.js"),
  },
];

const canvas = document.querySelector("#viewport");
const select = document.querySelector("#demo-select");
const statusEl = document.querySelector("#status");
const statsEl = document.querySelector("#stats");
const loading = new NetworkLoadingOverlay(document.querySelector("#network-loading"));
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

async function loadDemo(id) {
  const entry = DEMOS.find((item) => item.id === id) ?? DEMOS[0];
  select.value = entry.id;
  destroySession();
  setStats("");
  loading.start();
  const { demo } = await entry.load();
  session = new WasmSceneSession({
    canvas,
    status: setStatus,
    stats: setStats,
    networkProgress: (progress) => loading.update(progress),
    onScene(scene) {
      window.__datovizWasmScene = scene;
    },
  });
  window.__datovizWasmSession = session;
  await session.load(demo);
  loading.finish();
  const url = new URL(window.location.href);
  url.searchParams.set("demo", demo.id);
  window.history.replaceState(null, "", url);
}

for (const demo of DEMOS) {
  const option = document.createElement("option");
  option.value = demo.id;
  option.textContent = demo.label;
  select.appendChild(option);
}

select.addEventListener("change", () => {
  loadDemo(select.value).catch((error) => {
    loading.finish();
    setStatus(error instanceof Error ? error.message : String(error), true);
  });
});

window.addEventListener("pagehide", () => {
  destroySession();
}, { once: true });

const params = new URLSearchParams(window.location.search);
loadDemo(params.get("demo") ?? DEMOS[0].id).catch((error) => {
  loading.finish();
  setStatus(error instanceof Error ? error.message : String(error), true);
  console.error(error);
});
