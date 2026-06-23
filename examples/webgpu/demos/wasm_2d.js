export const demo = {
  id: "wasm-2d",
  label: "WASM 2D scene",
  build(scene) {
    const panel = scene.panelFull();
    addAxes(scene, panel);
    addPoints(scene, panel);
    addPixels(scene, panel);
    addMarkers(scene, panel);
    addSegments(scene, panel);
    addPath(scene, panel);
    addPrimitive(scene, panel);
    addImage(scene, panel);
    addLabels(scene, panel);
    addGlyphs(scene, panel);
    addText(scene, panel);
    addMesh(scene, panel);
    scene.attachPanzoom(panel);
  },
};

function addAxes(scene, panel) {
  scene.setDomain(panel, "x", -1.0, 1.0);
  scene.setDomain(panel, "y", -1.0, 1.0);

  const xAxis = scene.axis(panel, "x");
  xAxis.setGrid(true);
  xAxis.setLabel("x");

  const yAxis = scene.axis(panel, "y");
  yAxis.setGrid(true);
  yAxis.setLabel("y");
}

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
  const positionBuffer = scene.buffer({ usage: "vertex", stride: 12, byteSize: positions.byteLength });
  positionBuffer.setData(positions);
  points.setAttrBuffer("position", positionBuffer, count);
  points.setRGBA8("color", colors, count);
  points.setF32("diameter_px", diameters, count);
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
  const positionBuffer = scene.buffer({ usage: "vertex", stride: 12, byteSize: positions.byteLength });
  positionBuffer.setData(positions);
  pixels.setAttrBuffer("position", positionBuffer, count);
  pixels.setRGBA8("color", colors, count);
  pixels.setF32("pixel_size_px", sizes, count);
  scene.addVisual(panel, pixels);
}

function addMarkers(scene, panel) {
  const count = 7;
  const positions = new Float32Array(count * 3);
  const colors = new Uint8Array(count * 4);
  const diameters = new Float32Array(count);
  const angles = new Float32Array(count);
  const symbols = new Uint32Array([0, 1, 2, 3, 4, 5, 6]);
  for (let i = 0; i < count; i++) {
    const t = i / Math.max(1, count - 1);
    positions[3 * i + 0] = -0.82 + 0.58 * t;
    positions[3 * i + 1] = -0.1 + 0.12 * Math.sin(i * 1.7);
    positions[3 * i + 2] = 0.04;
    colors[4 * i + 0] = Math.round(245 - 120 * t);
    colors[4 * i + 1] = Math.round(110 + 130 * t);
    colors[4 * i + 2] = Math.round(95 + 125 * (1.0 - t));
    colors[4 * i + 3] = 230;
    diameters[i] = 10 + 4 * (i % 3);
    angles[i] = 0.18 * i;
  }

  const markers = scene.visual("marker");
  markers.setF32("position", positions, count);
  markers.setRGBA8("color", colors, count);
  markers.setF32("diameter_px", diameters, count);
  markers.setF32("angle", angles, count);
  markers.setU32("symbol", symbols, count);
  scene.addVisual(panel, markers);
}

function addSegments(scene, panel) {
  const count = 5;
  const starts = new Float32Array([
    -0.88, -0.34, 0.08,
    -0.66, -0.58, 0.08,
    -0.42, -0.42, 0.08,
    -0.18, -0.58, 0.08,
    -0.02, -0.34, 0.08,
  ]);
  const ends = new Float32Array([
    -0.68, -0.58, 0.08,
    -0.44, -0.32, 0.08,
    -0.18, -0.56, 0.08,
    -0.02, -0.28, 0.08,
    -0.34, -0.18, 0.08,
  ]);
  const colors = new Uint8Array([
    80, 205, 245, 225,
    245, 165, 75, 225,
    125, 220, 155, 225,
    230, 100, 185, 225,
    235, 220, 95, 225,
  ]);
  const widths = new Float32Array([3, 5, 7, 4, 6]);

  const segments = scene.visual("segment");
  segments.setF32("position_start", starts, count);
  segments.setF32("position_end", ends, count);
  segments.setRGBA8("color", colors, count);
  segments.setF32("stroke_width_px", widths, count);
  segments.setSegmentCaps("square", "triangle-out");
  scene.addVisual(panel, segments);
}

function addPath(scene, panel) {
  const count = 5;
  const positions = new Float32Array([
    -0.84, 0.12, 0.12,
    -0.66, 0.32, 0.12,
    -0.42, 0.18, 0.12,
    -0.22, 0.46, 0.12,
    -0.02, 0.22, 0.12,
  ]);
  const colors = new Uint8Array([
    70, 235, 180, 230,
    100, 210, 245, 230,
    170, 150, 245, 230,
    245, 125, 175, 230,
    245, 180, 90, 230,
  ]);
  const widths = new Float32Array([5, 7, 6, 8, 5]);

  const path = scene.visual("path");
  path.setF32("position", positions, count);
  path.setRGBA8("color", colors, count);
  path.setF32("stroke_width_px", widths, count);
  path.setPathCaps("round", "square");
  path.setPathJoin("miter", 2.5);
  scene.addVisual(panel, path);
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

function addLabels(scene, panel) {
  const labels = scene.visual("labels");
  const width = 12;
  const height = 10;
  const values = new Int32Array(width * height);
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const dx = x - width * 0.5;
      const dy = y - height * 0.5;
      const i = y * width + x;
      if (dx * dx + dy * dy < 8) {
        values[i] = 1;
      } else if (x > 6 && y > 2 && y < 8) {
        values[i] = 2;
      } else if (x > 2 && x < 7 && y > 5) {
        values[i] = 3;
      } else {
        values[i] = 4;
      }
    }
  }

  labels.setF32("position", new Float32Array([0.52, -0.45, 0.09]), 1);
  labels.setF32("extent", new Float32Array([0.58, 0.42]), 1);
  labels.setLabelsS32(values, width, height, [
    { id: 1, color: [245, 94, 92, 210] },
    { id: 2, color: [83, 203, 168, 210] },
    { id: 3, color: [86, 156, 244, 210] },
    { id: 4, color: [246, 207, 95, 170] },
  ]);
  scene.addVisual(panel, labels);
}

function makeGlyphAtlas(width, height) {
  const pixels = new Uint8Array(width * height * 4);
  const cell = Math.floor(width / 3);
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const letter = Math.min(2, Math.floor(x / cell));
      const lx = x - letter * cell;
      const top = y <= 3;
      const bottom = y >= height - 4;
      const left = lx <= 3;
      const right = lx >= cell - 5;
      const fill =
        (letter === 0 && (left || top || bottom || (right && y > 3 && y < height - 4))) ||
        (letter === 1 && y >= height / 2 && Math.abs(lx - cell / 2) <= Math.max(2, y - height / 2)) ||
        (letter === 2 && (top || bottom || Math.abs(lx - (cell - 1 - y)) <= 2));
      if (!fill) continue;
      const i = (y * width + x) * 4;
      pixels[i + 0] = 255;
      pixels[i + 1] = 255;
      pixels[i + 2] = 255;
      pixels[i + 3] = 255;
    }
  }
  return pixels;
}

function addGlyphs(scene, panel) {
  const glyph = scene.visual("glyph");
  const width = 96;
  const height = 32;
  const anchors = [
    [-0.72, 0.76, 0.18],
    [-0.50, 0.76, 0.18],
    [-0.28, 0.76, 0.18],
  ];
  const uvBounds = [
    [0, 0, 1 / 3, 1],
    [1 / 3, 0, 2 / 3, 1],
    [2 / 3, 0, 1, 1],
  ];
  const positions = [];
  const bounds = [];
  const texcoords = [];
  const colors = [];
  const angles = [];
  for (let i = 0; i < anchors.length; i++) {
    for (let j = 0; j < 6; j++) {
      positions.push(...anchors[i]);
      bounds.push(-22, -17, 22, 17);
      texcoords.push(...uvBounds[i]);
      colors.push(250, 250, 255, 245);
      angles.push(0);
    }
  }

  glyph.setF32("position", new Float32Array(positions), positions.length / 3);
  glyph.setF32("bounds", new Float32Array(bounds), bounds.length / 4);
  glyph.setF32("texcoords", new Float32Array(texcoords), texcoords.length / 4);
  glyph.setRGBA8("color", new Uint8Array(colors), colors.length / 4);
  glyph.setF32("angle", new Float32Array(angles), angles.length);
  glyph.setTextureRGBA8(makeGlyphAtlas(width, height), width, height);
  scene.addVisual(panel, glyph);
}

function addText(scene, panel) {
  const text = scene.visual("text");
  const strings = ["WASM"];
  const positions = new Float32Array([0.02, 0.78, 0.24]);
  const anchors = new Float32Array([0, 0.5]);
  const sizes = new Float32Array([15]);
  const colors = new Uint8Array([255, 255, 255, 245]);
  const angles = new Float32Array([0]);

  text.setStrings("text", strings);
  text.setF32("position", positions, strings.length);
  text.setF32("anchor", anchors, strings.length);
  text.setF32("size", sizes, strings.length);
  text.setRGBA8("color", colors, strings.length);
  text.setF32("angle", angles, strings.length);
  scene.addVisual(panel, text);
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
  mesh.setMaterial({
    model: "standard",
    baseColorFactor: [1.08, 0.96, 1.0, 1],
    lightDirection: [-0.35, -0.55, 0.76],
    standard: { roughness: 0.38, specular: 0.55, metallic: 0.08, rimStrength: 0.18 },
  });
  scene.addVisual(panel, mesh);
}
