import assert from "node:assert/strict";
import { DatovizWasmScene } from "../web/wasm/scene.js";

const calls = [];
const listeners = new Map();
const scene = Object.create(DatovizWasmScene.prototype);
Object.assign(scene, {
  scene: 7,
  scenario: {},
  canvas: {
    getBoundingClientRect: () => ({ height: 600 }),
    addEventListener: (name, callback) => listeners.set(name, callback),
    removeEventListener: (name) => listeners.delete(name),
  },
  Module: {
    _dvz_wasm_api_wheel: (...args) => { calls.push(["controller", args]); return 0; },
    _dvz_wasm_api_scenario_wheel: (...args) => { calls.push(["scenario", args]); return 0; },
  },
  _requireAlive() {},
  _requireStatus(status) { assert.equal(status, 0); },
  _canvasPoint: () => ({ x: 20, y: 30, scale: 2 }),
  _modifierMask: () => 4,
  _cleanup: [],
});

let renders = 0;
const detach = scene.attachControllerInput(() => renders++);
function wheel(deltaY, deltaMode, expected) {
  calls.length = 0;
  let prevented = false;
  listeners.get("wheel")({
    deltaY, deltaMode,
    preventDefault() { prevented = true; },
  });
  assert.ok(prevented);
  assert.deepEqual(calls.map(([name]) => name), ["controller", "scenario"]);
  for (const [, args] of calls) {
    assert.deepEqual(args.slice(0, 7), [7, 20, 30, 0, expected, 4, 2]);
    assert.ok(Number.isFinite(args[7]));
  }
  return calls[0][1][4];
}

// Equivalent calibrated gestures must reach both C input entry points identically.
wheel(-100, 0, 1);
wheel(-3, 1, 1);
wheel(-1 / 6, 2, 1);
wheel(100, 0, -1);
wheel(3, 1, -1);
wheel(1, 2, -6);
wheel(-0.5, 0, 0.005);
wheel(-0.75, 1, 0.25);
wheel(0, 0, -0);

// Splitting a smooth gesture across events preserves accumulated logarithmic zoom.
const whole = wheel(-100, 0, 1);
let split = 0;
for (let i = 0; i < 8; i++) split += wheel(-12.5, 0, 0.125);
assert.equal(split, whole);
assert.equal(Math.exp(0.05 * split), Math.exp(0.05 * whole));
assert.equal(renders, 18);
detach();
assert.equal(listeners.size, 0);
console.log("WebGPU wheel input smoke passed (pixel, line, page, fractional, routing)");
