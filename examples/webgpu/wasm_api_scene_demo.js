import { DatovizWasmScene } from "./datoviz_wasm_scene.js";

const statusEl = document.querySelector("#status");
const statsEl = document.querySelector("#stats");
const canvas = document.querySelector("#viewport");

function setStatus(message, isError = false) {
  statusEl.textContent = message;
  statusEl.classList.toggle("error", isError);
}

function makePoints() {
  const count = 96;
  const positions = new Float32Array(count * 3);
  const colors = new Uint8Array(count * 4);
  const diameters = new Float32Array(count);
  for (let i = 0; i < count; i++) {
    const t = i / Math.max(1, count - 1);
    const angle = t * Math.PI * 10.0;
    const radius = 0.08 + 0.82 * t;
    positions[3 * i + 0] = Math.cos(angle) * radius;
    positions[3 * i + 1] = Math.sin(angle) * radius;
    positions[3 * i + 2] = 0;
    colors[4 * i + 0] = Math.round(55 + 180 * t);
    colors[4 * i + 1] = Math.round(210 - 120 * t);
    colors[4 * i + 2] = Math.round(245 - 150 * Math.abs(0.5 - t));
    colors[4 * i + 3] = 255;
    diameters[i] = 8 + 9 * (0.5 + 0.5 * Math.sin(i * 0.53));
  }
  return { positions, colors, diameters, count };
}

async function main() {
  setStatus("Loading generic WASM scene API");
  const scene = await DatovizWasmScene.create(canvas);
  const panel = scene.panelFull();
  const points = scene.visual("point");
  const { positions, colors, diameters, count } = makePoints();
  points.setF32("position", positions, count);
  points.setRGBA8("color", colors, count);
  points.setF32("diameter", diameters, count);
  scene.addVisual(panel, points);

  const stream = await scene.renderInitial();
  statsEl.textContent = `${stream.commands.length} commands`;
  setStatus("Rendered generic point scene");
}

main().catch((error) => {
  setStatus(error.message, true);
  console.error(error);
});
