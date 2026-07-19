import { NetworkLoadingOverlay } from "../examples/webgpu/loading.js";

function requireOk(condition, message) {
  if (!condition) throw new Error(message);
}

globalThis.window = {
  setTimeout: globalThis.setTimeout,
  clearTimeout: globalThis.clearTimeout,
};
const label = { textContent: "" };
const progress = { hidden: false, max: 0, value: 0 };
const root = {
  hidden: true,
  querySelector(selector) {
    return selector === "progress" ? progress : label;
  },
};
const loading = new NetworkLoadingOverlay(root, { delayMs: 5 });
loading.start();
requireOk(root.hidden, "loading overlay appeared before its delay");
await new Promise((resolve) => setTimeout(resolve, 10));
requireOk(!root.hidden, "loading overlay did not appear after its delay");
loading.update({ phase: "download", percentage: 42 });
requireOk(
  progress.value === 42 && label.textContent.includes("42%"),
  "download percentage was not shown",
);
loading.update({ phase: "complete", percentage: 100 });
requireOk(progress.hidden && !label.textContent.includes("%"), "initialization displayed a percentage");
loading.finish();
requireOk(root.hidden, "loading overlay remained visible after completion");

console.log("PASS WebGPU loading overlay smoke");
