import { DatovizWasmScene } from "./scene.js";

function noop() {}

function messageCallback(callback) {
  return typeof callback === "function" ? callback : noop;
}

function sceneCallback(callback) {
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
    this.logicalWidth = options.logicalWidth;
    this.logicalHeight = options.logicalHeight;
    this.status = messageCallback(options.status);
    this.stats = messageCallback(options.stats);
    this.onScene = sceneCallback(options.onScene);
    this.networkProgress = messageCallback(options.networkProgress);
    this.scene = null;
    this.demo = null;
    this.rendering = false;
    this.pending = false;
    this.recovering = false;
    this.renderQueue = Promise.resolve();
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
    await this._createScene();
    await this.render();
    this._attachSceneEvents();
    if (demo.animate === true) {
      this.startAnimationLoop();
    }
    return this.scene;
  }

  async _createScene() {
    if (this.demo === null) {
      return null;
    }
    const createOptions = {
      ...(this.gpu !== null ? { gpu: this.gpu } : {}),
      logicalWidth: this.logicalWidth,
      logicalHeight: this.logicalHeight,
      dataBundles: this.demo.dataBundles ?? [],
      networkProgress: this.networkProgress,
    };
    if (typeof this.demo.scenarioId === "string") {
      this.scene = await DatovizWasmScene.createScenario(this.canvas, this.demo.scenarioId, createOptions);
    } else {
      this.scene = await DatovizWasmScene.create(this.canvas, createOptions);
      await this.demo.build(this.scene);
    }
    this.gpu = this.scene.gpu;
    this.onScene(this.scene);
    return this.scene;
  }

  _attachSceneEvents() {
    if (this.scene === null) {
      return;
    }
    const requestInputRender = () => {
      // Animated scenarios consume input on their next animation frame. A second render request
      // can race a stateful frame update and force recovery, which restarts animation time.
      if (this.demo?.animate === true && this.animationFrame !== 0) {
        return;
      }
      this.requestRender();
    };
    this.scene.attachControllerInput(requestInputRender);
    this.scene.attachResizeObserver(() => this.requestRender());
  }

  async render() {
    const execute = this.renderQueue.then(async () => await this._renderNow());
    this.renderQueue = execute.catch(() => {});
    return await execute;
  }

  async _renderNow() {
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

  async recoverAfterRenderError(error, options = {}) {
    if (this.recovering || this.demo === null || this.scene === null) {
      throw error;
    }
    this.recovering = true;
    const restartAnimation = options.restartAnimation ?? (
      this.demo.animate === true && this.animationFrame !== 0
    );
    try {
      this.status(`Recovering WebGPU runtime for ${this.demo.label ?? this.demo.id ?? "WASM scene"}`);
      this.stopAnimationLoop();
      const stream = await this.scene.recoverRuntime();
      this.stats(`${stream.commands.length} commands`);
      this.status(`Rendered ${this.demo?.label ?? "WASM scene"}`);
      if (restartAnimation) {
        this.startAnimationLoop();
      }
    } finally {
      this.recovering = false;
    }
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
        try {
          await this.recoverAfterRenderError(error);
        } catch (recoveryError) {
          const message = recoveryError instanceof Error ? recoveryError.message : String(recoveryError);
          this.status(message, true);
        }
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
          try {
            await this.recoverAfterRenderError(error, { restartAnimation: true });
          } catch (recoveryError) {
            const message = recoveryError instanceof Error ? recoveryError.message : String(recoveryError);
            this.status(message, true);
          }
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
    this.recovering = false;
    if (this.scene !== null) {
      this.scene.destroy();
      this.scene = null;
    }
    this.onScene(null);
    this.demo = null;
  }
}
