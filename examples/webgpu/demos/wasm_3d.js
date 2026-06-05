export const demo = {
  id: "wasm-3d",
  label: "WASM 3D arcball",
  build(scene) {
    const panel = scene.panelFull();
    scene.setCamera(panel);
    addSpheres(scene, panel);
    addCube(scene, panel);
    scene.attachArcball(panel);
  },
};

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
  const texcoords = [];
  const faceUv = [[0, 0], [1, 0], [0, 1], [1, 0], [1, 1], [0, 1]];
  for (const face of faces) {
    for (let i = 0; i < face.v.length; i++) {
      const vertex = face.v[i];
      positions.push(...vertex);
      colors.push(255, 255, 255, 255);
      normals.push(...face.n);
      texcoords.push(...faceUv[i]);
    }
  }
  return {
    positions: new Float32Array(positions),
    colors: new Uint8Array(colors),
    normals: new Float32Array(normals),
    texcoords: new Float32Array(texcoords),
    count: positions.length / 3,
  };
}

function makeCheckerTexture(width, height) {
  const pixels = new Uint8Array(width * height * 4);
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = (y * width + x) * 4;
      const checker = ((x >> 2) ^ (y >> 2)) & 1;
      pixels[i + 0] = checker ? 65 : 245;
      pixels[i + 1] = checker ? 180 : 115;
      pixels[i + 2] = checker ? 230 : 80;
      pixels[i + 3] = 255;
    }
  }
  return pixels;
}

function addCube(scene, panel) {
  const cube = makeCubeMesh(1.25);
  const mesh = scene.visual("mesh");
  mesh.setF32("position", cube.positions, cube.count);
  mesh.setRGBA8("color", cube.colors, cube.count);
  mesh.setF32("normal", cube.normals, cube.count);
  mesh.setF32("texcoords", cube.texcoords, cube.count);
  mesh.setTextureRGBA8(makeCheckerTexture(16, 16), 16, 16);
  mesh.setMaterial({
    model: "standard",
    lightDirection: [-0.45, -0.38, 0.8],
    standard: { roughness: 0.42, specular: 0.52, metallic: 0.04, rimStrength: 0.16 },
  });
  scene.addVisual(panel, mesh);
}

function addSpheres(scene, panel) {
  const count = 3;
  const spheres = scene.visual("sphere");
  spheres.setF32("position", new Float32Array([
    -0.72, -0.38, 0.32,
    0.74, -0.34, -0.18,
    0.0, 0.72, 0.18,
  ]), count);
  spheres.setRGBA8("color", new Uint8Array([
    245, 120, 90, 255,
    80, 210, 195, 255,
    245, 215, 90, 255,
  ]), count);
  spheres.setF32("radius", new Float32Array([0.18, 0.16, 0.14]), count);
  scene.addVisual(panel, spheres);
}
