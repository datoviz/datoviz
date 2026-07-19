export class NetworkLoadingOverlay {
  constructor(root, options = {}) {
    this.root = root;
    this.label = root.querySelector("[data-loading-label]");
    this.progress = root.querySelector("progress");
    this.delayMs = options.delayMs ?? 1000;
    this.timer = 0;
    this.active = false;
  }

  start() {
    this.stopTimer();
    this.active = true;
    this.root.hidden = true;
    this.progress.hidden = false;
    this.progress.max = 100;
    this.progress.value = 0;
    this.label.textContent = "Preparing download…";
    this.timer = window.setTimeout(() => {
      this.timer = 0;
      if (this.active) this.root.hidden = false;
    }, this.delayMs);
  }

  update(snapshot) {
    if (!this.active) return;
    if (snapshot.phase === "complete") {
      this.progress.hidden = true;
      this.label.textContent = "Initializing WebGPU…";
      return;
    }
    const percentage = Math.max(0, Math.min(100, snapshot.percentage));
    this.progress.hidden = false;
    this.progress.value = percentage;
    this.label.textContent = `Downloading… ${percentage}%`;
  }

  finish() {
    this.active = false;
    this.stopTimer();
    this.root.hidden = true;
  }

  stopTimer() {
    if (this.timer !== 0) {
      window.clearTimeout(this.timer);
      this.timer = 0;
    }
  }
}
