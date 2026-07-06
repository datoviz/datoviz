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
    this.animationFrame = 0;
    this.animationStartTime = null;
    this.lastAnimationTime = null;
  }

  async load(demo) {
    await this.destroy();
    if (demo === null) {
      return null;
    }
    if (typeof demo.build !== "function" && typeof demo.scenarioId !== "string") {
      throw new Error("WASM demo needs a build(scene) function or scenarioId");
    }

    this.demo = demo;
    this.status(`Loading ${demo.label ?? demo.id ?? "WASM scene"}`);
    const createOptions = this.gpu !== null ? { gpu: this.gpu } : {};
    if (typeof demo.scenarioId === "string") {
      this.scene = await DatovizWasmScene.createScenario(this.canvas, demo.scenarioId, createOptions);
    } else {
      this.scene = await DatovizWasmScene.create(this.canvas, createOptions);
      await demo.build(this.scene);
    }
    await this.render();

    const requestRender = () => {
      this.requestRender();
    };
    this.scene.attachControllerInput(requestRender);
    this.scene.attachResizeObserver(requestRender);
    if (demo.animate === true) {
      this.startAnimationLoop();
    }
    return this.scene;
  }

  async render() {
    if (this.scene === null) {
      return null;
    }
    this.scene.resize();
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

  startAnimationLoop() {
    if (this.scene === null || this.demo?.animate !== true) {
      return;
    }
    this.stopAnimationLoop();
    const fps = Number.isFinite(this.scene.scenario?.fps) && this.scene.scenario.fps > 0
      ? this.scene.scenario.fps
      : 60;
    const fallbackDt = 1 / fps;
    const tick = (nowMs) => {
      if (this.scene === null || this.demo?.animate !== true) {
        this.animationFrame = 0;
        return;
      }
      const nowAbsolute = nowMs / 1000;
      if (this.animationStartTime === null) {
        this.animationStartTime = nowAbsolute;
      }
      const now = nowAbsolute - this.animationStartTime;
      const dt = this.lastAnimationTime === null ? fallbackDt : Math.max(0, now - this.lastAnimationTime);
      this.lastAnimationTime = now;
      void (async () => {
        try {
          this.scene.scenarioFrame(now, dt);
          await this.render();
          this.animationFrame = requestAnimationFrame(tick);
        } catch (error) {
          this.animationFrame = 0;
          this.status(error instanceof Error ? error.message : String(error), true);
        }
      })();
    };
    this.animationFrame = requestAnimationFrame(tick);
  }

  stopAnimationLoop() {
    if (this.animationFrame !== 0) {
      cancelAnimationFrame(this.animationFrame);
      this.animationFrame = 0;
    }
    this.lastAnimationTime = null;
    this.animationStartTime = null;
  }

  destroy() {
    this.stopAnimationLoop();
    this.rendering = false;
    this.pending = false;
    if (this.scene !== null) {
      this.scene.destroy();
      this.scene = null;
    }
    this.demo = null;
  }
}
