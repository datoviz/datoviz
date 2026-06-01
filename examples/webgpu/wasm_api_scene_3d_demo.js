import { DatovizWasmScene } from "./datoviz_wasm_scene.js";

const statusEl = document.querySelector("#status");
const statsEl = document.querySelector("#stats");
const canvas = document.querySelector("#viewport");

function setStatus(message, isError = false) {
  statusEl.textContent = message;
  statusEl.classList.toggle("error", isError);
}

function makeCubeMesh(size) {
  const s = size / 2;
  const faces = [
    { n: [0, 0, 1], c: [90, 170, 255, 255], v: [[-s, -s, s], [s, -s, s], [-s, s, s], [s, -s, s], [s, s, s], [-s, s, s]] },
    { n: [0, 0, -1], c: [255, 135, 210, 255], v: [[s, -s, -s], [-s, -s, -s], [s, s, -s], [-s, -s, -s], [-s, s, -s], [s, s, -s]] },
    { n: [1, 0, 0], c: [85, 230, 190, 255], v: [[s, -s, s], [s, -s, -s], [s, s, s], [s, -s, -s], [s, s, -s], [s, s, s]] },
    { n: [-1, 0, 0], c: [255, 190, 90, 255], v: [[-s, -s, -s], [-s, -s, s], [-s, s, -s], [-s, -s, s], [-s, s, s], [-s, s, -s]] },
    { n: [0, 1, 0], c: [170, 125, 255, 255], v: [[-s, s, s], [s, s, s], [-s, s, -s], [s, s, s], [s, s, -s], [-s, s, -s]] },
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

function addCube(scene, panel) {
  const cube = makeCubeMesh(1.25);
  const mesh = scene.visual("mesh");
  mesh.setF32("position", cube.positions, cube.count);
  mesh.setRGBA8("color", cube.colors, cube.count);
  mesh.setF32("normal", cube.normals, cube.count);
  scene.addVisual(panel, mesh);
}

async function main() {
  setStatus("Loading generic WASM scene API");
  const scene = await DatovizWasmScene.create(canvas);
  window.__datovizWasmScene = scene;
  const panel = scene.panelFull();
  scene.setCamera(panel);
  addCube(scene, panel);
  scene.attachArcball(panel);

  let rendering = false;
  let pending = false;
  const renderChange = async () => {
    if (rendering) {
      pending = true;
      return;
    }
    rendering = true;
    try {
      do {
        pending = false;
        const updateStream = await scene.renderIncremental();
        statsEl.textContent = `${updateStream.commands.length} commands`;
        setStatus("Rendered generic 3D scene");
      } while (pending);
    } finally {
      rendering = false;
    }
  };

  const stream = await scene.renderInitial();
  statsEl.textContent = `${stream.commands.length} commands`;
  setStatus("Rendered generic 3D cube + arcball");
  const requestRender = () => {
    renderChange().catch((error) => setStatus(error.message, true));
  };
  scene.attachControllerInput(requestRender);
  scene.attachResizeObserver(requestRender);
  window.addEventListener("pagehide", () => {
    scene.destroy();
    window.__datovizWasmScene = null;
  }, { once: true });
}

main().catch((error) => {
  setStatus(error.message, true);
  console.error(error);
});
