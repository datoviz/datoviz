#!/usr/bin/env node

// Compare browser/device availability with progressively larger Datoviz workloads.
// --headed uses the current DISPLAY (including Xvfb); it is not physical-display proof.
import { mkdir, writeFile } from 'node:fs/promises';
import { resolve } from 'node:path';
import {
  createPage, removeWithRetry, startChrome, startStaticServer, waitForProcessExit,
} from './webgpu_browser_smoke.mjs';

const headed = process.argv.includes('--headed');
const outputArg = process.argv.find((arg) => arg.startsWith('--output='));
const output = resolve(outputArg?.slice('--output='.length) ??
  `build/rc3-browser-diagnosis/${headed ? 'headed' : 'headless'}`);
await mkdir(output, { recursive: true });
const { server, baseUrl } = await startStaticServer();
const chrome = await startChrome();
const report = { headed, display: process.env.DISPLAY ?? null, chromeArgs: chrome.args,
  version: chrome.version, systemInfo: chrome.systemInfo, probes: [] };
let page;
try {
  page = await createPage(chrome.debugPort);
  await page.send('Page.addScriptToEvaluateOnNewDocument', { source: `
    window.__gpuDiagnostics = { adapters: [], devices: [], errors: [] };
    if (navigator.gpu) {
      const requestAdapter = navigator.gpu.requestAdapter.bind(navigator.gpu);
      navigator.gpu.requestAdapter = async (...args) => {
        const adapter = await requestAdapter(...args);
        if (!adapter) return adapter;
        const info = adapter.info;
        window.__gpuDiagnostics.adapters.push({
          vendor: info?.vendor, architecture: info?.architecture,
          device: info?.device, description: info?.description,
          isFallbackAdapter: info?.isFallbackAdapter ?? adapter.isFallbackAdapter,
          features: [...adapter.features],
        });
        const requestDevice = adapter.requestDevice.bind(adapter);
        adapter.requestDevice = async (...deviceArgs) => {
          const device = await requestDevice(...deviceArgs);
          const record = { lost: null, uncapturedErrors: [] };
          window.__gpuDiagnostics.devices.push(record);
          device.lost.then((info) => { record.lost = { reason: info.reason, message: info.message }; });
          device.addEventListener('uncapturederror', (event) => {
            record.uncapturedErrors.push(event.error.message);
          });
          return device;
        };
        return adapter;
      };
    }
    window.addEventListener('error', (event) => window.__gpuDiagnostics.errors.push(event.message));
  ` });
  const capture = async (name, callback) => {
    const probe = { name };
    try { probe.result = await callback(); }
    catch (error) { probe.error = error.stack ?? String(error); }
    probe.diagnostics = await page.evaluate('window.__gpuDiagnostics');
    probe.pageErrors = [...page.errors];
    probe.consoleErrors = [...page.consoleErrors];
    report.probes.push(probe);
    console.log(`${name}: ${probe.error ?? JSON.stringify(probe.result)}`);
  };
  await page.navigate(`${baseUrl}/examples/webgpu/streams/triangle_wgsl.json`);
  await capture('plain-clear-readback', () => page.evaluate(`(async () => {
    const adapter = await navigator.gpu.requestAdapter();
    if (!adapter) throw new Error('no WebGPU adapter');
    const device = await adapter.requestDevice();
    window.__plainDevice = device;
    device.pushErrorScope('validation');
    const texture = device.createTexture({ size: [4, 4], format: 'rgba8unorm',
      usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.COPY_SRC });
    const buffer = device.createBuffer({ size: 1024, usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ });
    const encoder = device.createCommandEncoder();
    const pass = encoder.beginRenderPass({ colorAttachments: [{ view: texture.createView(),
      clearValue: [1, 0.25, 0, 1], loadOp: 'clear', storeOp: 'store' }] });
    pass.end();
    encoder.copyTextureToBuffer({ texture }, { buffer, bytesPerRow: 256 }, [4, 4]);
    device.queue.submit([encoder.finish()]);
    await buffer.mapAsync(GPUMapMode.READ);
    const pixel = Array.from(new Uint8Array(buffer.getMappedRange()).slice(0, 4));
    buffer.unmap();
    const validation = await device.popErrorScope();
    buffer.destroy(); texture.destroy();
    if (validation) throw new Error(validation.message);
    if (pixel.join(',') !== '255,64,0,255') throw new Error('unexpected pixel ' + pixel);
    return { pixel };
  })()`));
  await capture('plain-submitted-work-done', () => page.evaluate(`(async () => {
    await window.__plainDevice.queue.onSubmittedWorkDone();
    return { completed: true };
  })()`));
  await capture('drp2-offscreen-triangle-readback', () => page.evaluate(`(async () => {
    const { executeDrp2StreamChecked } = await import('/web/drp2/webgpu.js');
    const stream = await (await fetch('/examples/webgpu/streams/triangle_offscreen_readback_wgsl.json')).json();
    // Retain the offscreen render/copy and submission, removing only the canvas render pass.
    stream.commands.splice(17, 5);
    const result = await executeDrp2StreamChecked(window.__plainDevice, null, 'bgra8unorm', stream);
    const summary = result.readbacks.map(({ size, summary }) => ({ size, summary }));
    if (!summary.length) throw new Error('missing DRP2 readback');
    return { readbacks: summary };
  })()`));
  await capture('plain-canvas-clear', () => page.evaluate(`(async () => {
    const adapter = await navigator.gpu.requestAdapter();
    const device = await adapter.requestDevice();
    const canvas = document.createElement('canvas');
    canvas.width = canvas.height = 64; document.body.append(canvas);
    const context = canvas.getContext('webgpu');
    context.configure({ device, format: navigator.gpu.getPreferredCanvasFormat() });
    const encoder = device.createCommandEncoder();
    const pass = encoder.beginRenderPass({ colorAttachments: [{ view: context.getCurrentTexture().createView(),
      clearValue: [1, 0.25, 0, 1], loadOp: 'clear', storeOp: 'store' }] });
    pass.end(); device.queue.submit([encoder.finish()]);
    await device.queue.onSubmittedWorkDone();
    return { completed: true };
  })()`));
  await capture('drp2-triangle-readback', () => page.evaluate(`(async () => {
    const { initWebGPU, executeDrp2StreamChecked } = await import('/web/drp2/webgpu.js');
    const canvas = document.createElement('canvas');
    canvas.width = canvas.height = 512; document.body.append(canvas);
    const { device, context, format } = await initWebGPU(canvas);
    const stream = await (await fetch('/examples/webgpu/streams/triangle_offscreen_readback_wgsl.json')).json();
    const result = await executeDrp2StreamChecked(device, context, format, stream, { canvas });
    return { readbacks: result.readbacks.map(({ size, summary }) => ({ size, summary })) };
  })()`));
  for (const route of ['composites_polygon', 'showcases_choropleth']) {
    await page.navigate(`${baseUrl}/examples/webgpu/live.html?id=${route}`);
    await capture(route, async () => {
      const status = await page.waitFor(`(() => {
        const node = document.querySelector('#status');
        const text = node?.textContent ?? '';
        return (node?.classList.contains('error') || text.startsWith('Rendered ')) && text;
      })()`, 45000);
      const errored = await page.evaluate("document.querySelector('#status')?.classList.contains('error')");
      if (errored) throw new Error(status);
      await page.screenshotCanvas(resolve(output, `${route}.png`));
      let palettePixels = null;
      if (route === 'composites_polygon') {
        palettePixels = await page.evaluate(`(async () => {
          const source = document.querySelector('#viewport');
          const image = new Image(); image.src = source.toDataURL(); await image.decode();
          const canvas = document.createElement('canvas');
          canvas.width = source.width; canvas.height = source.height;
          const context = canvas.getContext('2d'); context.drawImage(image, 0, 0);
          // Interior fill colors include the example's source-over alpha; the last point is the hole.
          const expected = [
            [250, 240, [34, 139, 164]], [600, 350, [216, 92, 77]],
            [1100, 250, [86, 177, 124]], [880, 460, [225, 177, 68]],
            [250, 360, [22, 27, 34]],
          ];
          return expected.map(([x, y, rgb]) => {
            const actual = Array.from(context.getImageData(x, y, 1, 1).data);
            if (rgb.some((value, i) => Math.abs(actual[i] - value) > 2)) {
              throw new Error('polygon palette mismatch at ' + [x, y] + ': ' + actual);
            }
            return { x, y, actual, expected: rgb };
          });
        })()`);
      }
      return { status, nonblankScreenshot: true, palettePixels };
    });
  }
} finally {
  page?.close();
  server.closeAllConnections?.();
  await new Promise((accept) => server.close(accept));
  chrome.chrome.kill('SIGTERM');
  await waitForProcessExit(chrome.chrome);
  await writeFile(resolve(output, 'report.json'), JSON.stringify(report, null, 2) + '\n');
  await writeFile(resolve(output, 'chrome.stderr.log'), chrome.getStderr());
  await removeWithRetry(chrome.userDataDir);
}
console.log(`Wrote ${output}`);
if (report.probes.some((probe) => probe.error)) process.exitCode = 1;
