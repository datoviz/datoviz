import { DatovizWasmScene } from "./scene.js";

function noop() {}

function messageCallback(callback) {
  return typeof callback === "function" ? callback : noop;
}

export class WasmSceneSession {
  static async create(options = {}) {
    const session = new WasmSceneSession(options);
    await session.load(options.demo ?? null);
    return session;
  }

  constructor(options = {}) {
    this.canvas = options.canvas;
    if (!(this.canvas instanceof HTMLCanvasElement)) {
      throw new Error("WasmSceneSession needs a canvas");
    }
    this.gpu = options.gpu ?? null;
    this.status = messageCallback(options.status);
    this.stats = messageCallback(options.stats);
    this.scene = null;
    this.demo = null;
    this.rendering = false;
    this.pending = false;
  }

  async load(demo) {
    await this.destroy();
    if (demo === null) {
      return null;
    }
    if (typeof demo.build !== "function") {
      throw new Error("WASM demo needs a build(scene) function");
    }

    this.demo = demo;
    this.status(`Loading ${demo.label ?? demo.id ?? "WASM scene"}`);
    this.scene = await DatovizWasmScene.create(
      this.canvas,
      this.gpu !== null ? { gpu: this.gpu } : {},
    );
    await demo.build(this.scene);
    await this.render();

    const requestRender = () => {
      this.requestRender();
    };
    this.scene.attachControllerInput(requestRender);
    this.scene.attachResizeObserver(requestRender);
    return this.scene;
  }

  async render() {
    if (this.scene === null) {
      return null;
    }
    const stream = this.scene.runtime === null
      ? await this.scene.renderInitial()
      : await this.scene.renderIncremental();
    this.stats(`${stream.commands.length} commands`);
    this.status(`Rendered ${this.demo?.label ?? "WASM scene"}`);
    return stream;
  }

  requestRender() {
    if (this.rendering) {
      this.pending = true;
      return;
    }
    this.rendering = true;
    void (async () => {
      try {
        do {
          this.pending = false;
          await this.render();
        } while (this.pending);
      } catch (error) {
        this.status(error instanceof Error ? error.message : String(error), true);
      } finally {
        this.rendering = false;
      }
    })();
  }

  destroy() {
    this.rendering = false;
    this.pending = false;
    if (this.scene !== null) {
      this.scene.destroy();
      this.scene = null;
    }
    this.demo = null;
  }
}
