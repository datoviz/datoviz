export const demo = {
  id: "wasm-2d",
  label: "WASM 2D scene",
  build(scene) {
    const panel = scene.panelFull();
    addPoints(scene, panel);
    addPixels(scene, panel);
    addPrimitive(scene, panel);
    addImage(scene, panel);
    addMesh(scene, panel);
    scene.attachPanzoom(panel);
  },
};

function addPoints(scene, panel) {
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

  const points = scene.visual("point");
  points.setF32("position", positions, count);
  points.setRGBA8("color", colors, count);
  points.setF32("diameter", diameters, count);
  scene.addVisual(panel, points);
}

function addPixels(scene, panel) {
  const columns = 10;
  const rows = 8;
  const count = columns * rows;
  const positions = new Float32Array(count * 3);
  const colors = new Uint8Array(count * 4);
  const sizes = new Float32Array(count);
  for (let y = 0; y < rows; y++) {
    for (let x = 0; x < columns; x++) {
      const i = y * columns + x;
      const u = x / Math.max(1, columns - 1);
      const v = y / Math.max(1, rows - 1);
      positions[3 * i + 0] = -0.86 + 0.54 * u;
      positions[3 * i + 1] = 0.34 + 0.48 * v;
      positions[3 * i + 2] = 0.02;
      colors[4 * i + 0] = Math.round(45 + 180 * u);
      colors[4 * i + 1] = Math.round(110 + 120 * v);
      colors[4 * i + 2] = Math.round(245 - 85 * u);
      colors[4 * i + 3] = 235;
      sizes[i] = 5 + 4 * (0.5 + 0.5 * Math.sin((x + y) * 0.9));
    }
  }

  const pixels = scene.visual("pixel");
  pixels.setF32("position", positions, count);
  pixels.setRGBA8("color", colors, count);
  pixels.setF32("pixel_size", sizes, count);
  scene.addVisual(panel, pixels);
}

function addPrimitive(scene, panel) {
  const primitive = scene.visual("primitive");
  const positions = new Float32Array([
    -0.85, -0.72, 0.15,
    -0.12, -0.72, 0.15,
    -0.48, 0.18, 0.15,
  ]);
  const colors = new Uint8Array([
    255, 125, 85, 220,
    255, 185, 85, 220,
    255, 85, 155, 220,
  ]);
  primitive.setF32("position", positions, positions.length / 3);
  primitive.setRGBA8("color", colors, colors.length / 4);
  scene.addVisual(panel, primitive);
}

function addImage(scene, panel) {
  const image = scene.visual("image");
  const positions = new Float32Array([
    0.18, -0.78, 0.05,
    0.18, -0.12, 0.05,
    0.86, -0.78, 0.05,
    0.86, -0.12, 0.05,
  ]);
  const texcoords = new Float32Array([
    0, 0,
    0, 1,
    1, 0,
    1, 1,
  ]);
  const width = 32;
  const height = 32;
  const pixels = new Uint8Array(width * height * 4);
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = (y * width + x) * 4;
      const checker = ((x >> 3) + (y >> 3)) & 1;
      pixels[i + 0] = checker ? 45 : 235;
      pixels[i + 1] = checker ? 120 : 245;
      pixels[i + 2] = checker ? 215 : 120;
      pixels[i + 3] = 255;
    }
  }

  image.setF32("position", positions, positions.length / 3);
  image.setF32("texcoords", texcoords, texcoords.length / 2);
  image.setTextureRGBA8(pixels, width, height);
  scene.addVisual(panel, image);
}

function addMesh(scene, panel) {
  const mesh = scene.visual("mesh");
  const positions = new Float32Array([
    0.18, 0.18, 0.22,
    0.86, 0.18, 0.22,
    0.18, 0.78, 0.22,
    0.86, 0.18, 0.22,
    0.86, 0.78, 0.22,
    0.18, 0.78, 0.22,
  ]);
  const colors = new Uint8Array([
    90, 170, 255, 240,
    85, 230, 190, 240,
    160, 120, 255, 240,
    85, 230, 190, 240,
    255, 135, 210, 240,
    160, 120, 255, 240,
  ]);
  const normals = new Float32Array([
    0, 0, 1,
    0, 0, 1,
    0, 0, 1,
    0, 0, 1,
    0, 0, 1,
    0, 0, 1,
  ]);
  mesh.setF32("position", positions, positions.length / 3);
  mesh.setRGBA8("color", colors, colors.length / 4);
  mesh.setF32("normal", normals, normals.length / 3);
  scene.addVisual(panel, mesh);
}
