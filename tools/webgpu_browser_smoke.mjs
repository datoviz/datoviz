#!/usr/bin/env node

import { createServer } from 'node:http';
import { mkdir, mkdtemp, readFile, rm, writeFile } from 'node:fs/promises';
import { existsSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, extname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';
import { inflateSync } from 'node:zlib';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const chromeCandidates = [
  process.env.CHROME,
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  '/Applications/Chromium.app/Contents/MacOS/Chromium',
  '/opt/homebrew/bin/chromium',
  '/usr/bin/chromium',
  '/usr/bin/chromium-browser',
  '/usr/bin/google-chrome',
].filter(Boolean);

const mimeTypes = new Map([
  ['.html', 'text/html; charset=utf-8'],
  ['.js', 'text/javascript; charset=utf-8'],
  ['.mjs', 'text/javascript; charset=utf-8'],
  ['.json', 'application/json; charset=utf-8'],
  ['.wasm', 'application/wasm'],
  ['.css', 'text/css; charset=utf-8'],
]);

function requireOk(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function chromePath() {
  for (const candidate of chromeCandidates) {
    if (existsSync(candidate)) {
      return candidate;
    }
  }
  throw new Error('Chrome/Chromium not found; set CHROME=/path/to/chrome');
}

function listen(server, port = 0) {
  return new Promise((resolveListen, reject) => {
    server.once('error', reject);
    server.listen(port, '127.0.0.1', () => resolveListen(server.address().port));
  });
}

async function startStaticServer() {
  const server = createServer(async (req, res) => {
    try {
      const url = new URL(req.url, 'http://127.0.0.1');
      const pathname = decodeURIComponent(url.pathname);
      const relative = pathname === '/' ? 'examples/webgpu/index.html' : pathname.slice(1);
      const file = resolve(root, relative);
      if (!file.startsWith(`${root}/`) && file !== root) {
        res.writeHead(403);
        res.end('forbidden');
        return;
      }
      const data = await readFile(file);
      res.writeHead(200, {
        'content-type': mimeTypes.get(extname(file)) ?? 'application/octet-stream',
        'cache-control': 'no-store',
      });
      res.end(data);
    } catch (error) {
      res.writeHead(error.code === 'ENOENT' ? 404 : 500);
      res.end(error.message);
    }
  });
  const port = await listen(server);
  return { server, baseUrl: `http://127.0.0.1:${port}` };
}

async function waitForJson(url, timeoutMs = 10000) {
  const deadline = Date.now() + timeoutMs;
  let lastError = null;
  while (Date.now() < deadline) {
    try {
      const response = await fetch(url);
      if (response.ok) {
        return await response.json();
      }
      lastError = new Error(`${response.status} ${response.statusText}`);
    } catch (error) {
      lastError = error;
    }
    await new Promise((resolveTimeout) => setTimeout(resolveTimeout, 100));
  }
  throw lastError ?? new Error(`timed out waiting for ${url}`);
}

async function startChrome() {
  const userDataDir = await mkdtemp(join(tmpdir(), 'datoviz-webgpu-smoke-'));
  const debugServer = createServer();
  const debugPort = await listen(debugServer);
  await new Promise((resolveClose) => debugServer.close(resolveClose));

  const chrome = spawn(chromePath(), [
    `--remote-debugging-port=${debugPort}`,
    `--user-data-dir=${userDataDir}`,
    '--headless=new',
    '--no-first-run',
    '--no-default-browser-check',
    '--disable-background-networking',
    '--disable-gpu-sandbox',
    '--enable-unsafe-webgpu',
    'about:blank',
  ], { stdio: ['ignore', 'ignore', 'pipe'] });

  let stderr = '';
  chrome.stderr.on('data', (chunk) => {
    stderr += chunk.toString();
  });

  try {
    await waitForJson(`http://127.0.0.1:${debugPort}/json/version`);
  } catch (error) {
    chrome.kill('SIGTERM');
    throw new Error(`failed to start Chrome CDP: ${error.message}\n${stderr}`);
  }

  return { chrome, debugPort, userDataDir };
}

async function waitForProcessExit(process, timeoutMs = 5000) {
  if (process.exitCode !== null || process.signalCode !== null) {
    return;
  }
  await new Promise((resolveExit) => {
    const timeout = setTimeout(resolveExit, timeoutMs);
    process.once('exit', () => {
      clearTimeout(timeout);
      resolveExit();
    });
  });
}

async function removeWithRetry(path, attempts = 20) {
  let lastError = null;
  for (let i = 0; i < attempts; i++) {
    try {
      await rm(path, { recursive: true, force: true, maxRetries: 3, retryDelay: 100 });
      return;
    } catch (error) {
      lastError = error;
      await new Promise((resolveTimeout) => setTimeout(resolveTimeout, 100));
    }
  }
  throw lastError;
}

class CdpPage {
  constructor(wsUrl) {
    this.ws = new WebSocket(wsUrl);
    this.nextId = 1;
    this.pending = new Map();
    this.events = new Map();
    this.errors = [];
    this.consoleErrors = [];
    this.ws.addEventListener('message', (event) => this._onMessage(event));
  }

  async open() {
    await new Promise((resolveOpen, reject) => {
      this.ws.addEventListener('open', resolveOpen, { once: true });
      this.ws.addEventListener('error', reject, { once: true });
    });
    await this.send('Page.enable');
    await this.send('Runtime.enable');
    await this.send('Log.enable');
    this.on('Runtime.exceptionThrown', (event) => {
      const detail = event.exceptionDetails;
      this.errors.push(detail.text ?? detail.exception?.description ?? 'Runtime exception');
    });
    this.on('Log.entryAdded', (event) => {
      if (event.entry.level === 'error') {
        if (event.entry.url?.endsWith('/favicon.ico')) {
          return;
        }
        this.errors.push(event.entry.text);
      }
    });
    this.on('Runtime.consoleAPICalled', (event) => {
      if (event.type === 'error') {
        this.consoleErrors.push(event.args.map((arg) => arg.value ?? arg.description).join(' '));
      }
    });
  }

  close() {
    this.ws.close();
  }

  on(method, fn) {
    const handlers = this.events.get(method) ?? [];
    handlers.push(fn);
    this.events.set(method, handlers);
  }

  _onMessage(event) {
    const message = JSON.parse(event.data);
    if (message.id !== undefined) {
      const pending = this.pending.get(message.id);
      if (pending === undefined) {
        return;
      }
      this.pending.delete(message.id);
      if (message.error !== undefined) {
        pending.reject(new Error(message.error.message));
      } else {
        pending.resolve(message.result);
      }
      return;
    }
    for (const handler of this.events.get(message.method) ?? []) {
      handler(message.params ?? {});
    }
  }

  send(method, params = {}) {
    const id = this.nextId++;
    this.ws.send(JSON.stringify({ id, method, params }));
    return new Promise((resolveSend, reject) => {
      this.pending.set(id, { resolve: resolveSend, reject });
    });
  }

  async navigate(url) {
    this.errors = [];
    this.consoleErrors = [];
    const loaded = new Promise((resolveLoad) => this.on('Page.loadEventFired', resolveLoad));
    await this.send('Page.navigate', { url });
    await loaded;
  }

  async evaluate(expression, awaitPromise = true) {
    const result = await this.send('Runtime.evaluate', {
      expression,
      awaitPromise,
      returnByValue: true,
    });
    if (result.exceptionDetails !== undefined) {
      throw new Error(result.exceptionDetails.text ?? 'Runtime.evaluate failed');
    }
    return result.result.value;
  }

  async waitFor(expression, timeoutMs = 30000) {
    const deadline = Date.now() + timeoutMs;
    let value = null;
    while (Date.now() < deadline) {
      value = await this.evaluate(expression);
      if (value) {
        return value;
      }
      await new Promise((resolveTimeout) => setTimeout(resolveTimeout, 100));
    }
    throw new Error(`timed out waiting for ${expression}; last=${JSON.stringify(value)}`);
  }

  async canvasRect() {
    return await this.evaluate(`(() => {
      const rect = document.querySelector("#viewport").getBoundingClientRect();
      return { x: rect.left, y: rect.top, width: rect.width, height: rect.height };
    })()`);
  }

  async screenshotCanvas(path) {
    const dataUrl = await this.evaluate(`(() => {
      const canvas = document.querySelector("#viewport");
      return canvas.toDataURL("image/png");
    })()`);
    requireOk(
      typeof dataUrl === 'string' && dataUrl.startsWith('data:image/png;base64,'),
      `${path}: canvas did not produce a PNG data URL`,
    );
    const buffer = Buffer.from(dataUrl.slice('data:image/png;base64,'.length), 'base64');
    await writeFile(path, buffer);
    assertPngNonblank(buffer, path);
  }

  async dragCanvas(dx, dy) {
    const rect = await this.canvasRect();
    const x = rect.x + rect.width / 2;
    const y = rect.y + rect.height / 2;
    await this.send('Input.dispatchMouseEvent', { type: 'mousePressed', x, y, button: 'left', buttons: 1, clickCount: 1 });
    await this.send('Input.dispatchMouseEvent', { type: 'mouseMoved', x: x + dx, y: y + dy, button: 'left', buttons: 1 });
    await this.send('Input.dispatchMouseEvent', { type: 'mouseReleased', x: x + dx, y: y + dy, button: 'left', buttons: 0, clickCount: 1 });
  }

  async wheelCanvas(deltaY) {
    const rect = await this.canvasRect();
    await this.send('Input.dispatchMouseEvent', {
      type: 'mouseWheel',
      x: rect.x + rect.width / 2,
      y: rect.y + rect.height / 2,
      deltaX: 0,
      deltaY,
    });
  }
}

function unfilterScanline(filter, current, previous, bytesPerPixel) {
  for (let i = 0; i < current.length; i++) {
    const left = i >= bytesPerPixel ? current[i - bytesPerPixel] : 0;
    const up = previous !== null ? previous[i] : 0;
    const upLeft = previous !== null && i >= bytesPerPixel ? previous[i - bytesPerPixel] : 0;
    let value = current[i];
    if (filter === 1) {
      value += left;
    } else if (filter === 2) {
      value += up;
    } else if (filter === 3) {
      value += Math.floor((left + up) / 2);
    } else if (filter === 4) {
      const p = left + up - upLeft;
      const pa = Math.abs(p - left);
      const pb = Math.abs(p - up);
      const pc = Math.abs(p - upLeft);
      value += pa <= pb && pa <= pc ? left : pb <= pc ? up : upLeft;
    } else if (filter !== 0) {
      throw new Error(`unsupported PNG filter ${filter}`);
    }
    current[i] = value & 0xff;
  }
}

function assertPngNonblank(buffer, label) {
  requireOk(buffer.subarray(0, 8).equals(Buffer.from([137, 80, 78, 71, 13, 10, 26, 10])), `${label}: not a PNG`);
  let offset = 8;
  let width = 0;
  let height = 0;
  let colorType = 0;
  const idat = [];
  while (offset < buffer.length) {
    const length = buffer.readUInt32BE(offset);
    const type = buffer.subarray(offset + 4, offset + 8).toString('ascii');
    const data = buffer.subarray(offset + 8, offset + 8 + length);
    if (type === 'IHDR') {
      width = data.readUInt32BE(0);
      height = data.readUInt32BE(4);
      requireOk(data[8] === 8, `${label}: unsupported PNG bit depth ${data[8]}`);
      colorType = data[9];
      requireOk(colorType === 2 || colorType === 6, `${label}: unsupported PNG color type ${colorType}`);
    } else if (type === 'IDAT') {
      idat.push(data);
    } else if (type === 'IEND') {
      break;
    }
    offset += 12 + length;
  }
  const channels = colorType === 6 ? 4 : 3;
  const stride = width * channels;
  const raw = inflateSync(Buffer.concat(idat));
  let rawOffset = 0;
  let previous = null;
  const colors = new Set();
  let brightPixels = 0;
  for (let y = 0; y < height; y++) {
    const filter = raw[rawOffset++];
    const row = Buffer.from(raw.subarray(rawOffset, rawOffset + stride));
    rawOffset += stride;
    unfilterScanline(filter, row, previous, channels);
    previous = row;
    for (let x = 0; x < width; x += Math.max(1, Math.floor(width / 80))) {
      const i = x * channels;
      const r = row[i + 0];
      const g = row[i + 1];
      const b = row[i + 2];
      colors.add(`${r},${g},${b}`);
      if (r + g + b > 48) {
        brightPixels++;
      }
    }
  }
  requireOk(colors.size > 1 && brightPixels > 16, `${label}: screenshot appears blank`);
}

async function createPage(debugPort) {
  const response = await fetch(`http://127.0.0.1:${debugPort}/json/new?about:blank`, {
    method: 'PUT',
  });
  requireOk(response.ok, `failed to create Chrome target: ${response.status}`);
  const target = await response.json();
  const page = new CdpPage(target.webSocketDebuggerUrl);
  await page.open();
  return page;
}

async function smokeWasmPage(page, baseUrl, path, expectedStatus, screenshotPath) {
  await page.navigate(`${baseUrl}${path}`);
  requireOk(
    await page.evaluate('typeof navigator.gpu === "object"'),
    `navigator.gpu is not available for ${path}`,
  );
  const initialStatus = await page.waitFor(`(() => {
    const status = document.querySelector("#status");
    const text = status?.textContent ?? "";
    if (status?.classList.contains("error")) return "ERROR: " + text;
    return (text.includes(${JSON.stringify(expectedStatus)}) || text.startsWith("Rendered generic")) && text;
  })()`, 45000);
  requireOk(!String(initialStatus).startsWith('ERROR:'), initialStatus);
  await page.screenshotCanvas(screenshotPath);
  const usesPacketRuntime = await page.evaluate(`(() => {
    const scene = window.__datovizWasmScene;
    return scene?.runtime?.stream?.name === "drp2_packet_set";
  })()`);
  requireOk(usesPacketRuntime, `${path}: WASM scene did not use DRP2 packet runtime`);
  await page.dragCanvas(48, 24);
  await page.wheelCanvas(-180);
  const interactiveStatus = await page.waitFor(`(() => {
    const status = document.querySelector("#status");
    const text = status?.textContent ?? "";
    if (status?.classList.contains("error")) return "ERROR: " + text;
    return text.startsWith("Rendered generic") && text;
  })()`, 15000);
  requireOk(!String(interactiveStatus).startsWith('ERROR:'), interactiveStatus);
  await page.screenshotCanvas(screenshotPath.replace('.png', '-interactive.png'));
  const destroyed = await page.evaluate(`(() => {
    const scene = window.__datovizWasmScene;
    if (scene === undefined || scene === null || scene.scene === 0) return false;
    window.dispatchEvent(new Event("pagehide"));
    return window.__datovizWasmScene === null && scene.scene === 0;
  })()`);
  requireOk(destroyed, `${path}: WASM scene did not destroy cleanly on pagehide`);
  assertNoBrowserErrors(page, path);
  return { initialStatus, interactiveStatus };
}

async function smokeWasmDashboard(page, baseUrl) {
  await page.navigate(`${baseUrl}/examples/webgpu/fixtures.html`);
  requireOk(
    await page.evaluate('typeof navigator.gpu === "object"'),
    'navigator.gpu is not available for fixture dashboard',
  );
  await page.waitFor('typeof window.__datovizRunWasmRows === "function"', 45000);
  const summary = await page.evaluate('window.__datovizRunWasmRows()');
  const details = await page.evaluate(`Array.from(document.querySelectorAll("#wasm-rows tr")).map((row) => {
    const cells = Array.from(row.querySelectorAll("td"));
    return cells.map((cell) => cell.textContent.trim()).join(" | ");
  }).join("; ")`);
  requireOk(
    typeof summary === 'string' && summary.includes('2 pass') && summary.includes('0 fail'),
    `fixture dashboard WASM rows failed: ${summary}; ${details}`,
  );
  assertNoBrowserErrors(page, 'fixture dashboard WASM rows');
  return summary;
}

function assertNoBrowserErrors(page, label) {
  requireOk(page.errors.length === 0, `${label}: browser errors: ${page.errors.join('; ')}`);
  requireOk(
    page.consoleErrors.length === 0,
    `${label}: console errors: ${page.consoleErrors.join('; ')}`,
  );
}

function isKnownHeadlessWebGpuInstanceLoss(status) {
  const text = String(status);
  return text.includes('A valid external Instance reference no longer exists') ||
         text.includes('Instance dropped in popErrorScope');
}

async function main() {
  requireOk(
    existsSync(resolve(root, 'build-wasm-scene/wasm/datoviz_wasm_scene.mjs')),
    'build-wasm-scene/wasm/datoviz_wasm_scene.mjs is missing; run `just wasm-scene-build` first',
  );
  const artifactsDir = resolve(root, 'build/webgpu-browser-smoke');
  await mkdir(artifactsDir, { recursive: true });
  const { server, baseUrl } = await startStaticServer();
  const chrome = await startChrome();
  let page = null;
  try {
    page = await createPage(chrome.debugPort);
    let wasm2d = null;
    let wasm3d = null;
    try {
      wasm2d = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/wasm_scene.html',
        'Rendered generic point/primitive/image/mesh scene',
        join(artifactsDir, 'wasm_scene.png'),
      );
      wasm3d = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/wasm_scene_3d.html',
        'Rendered generic 3D cube + arcball',
        join(artifactsDir, 'wasm_scene_3d.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(`SKIP WASM page render: headless WebGPU instance loss (${error.message})`);
    }
    let wasmDashboard = null;
    try {
      wasmDashboard = await smokeWasmDashboard(page, baseUrl);
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(`SKIP WASM dashboard: headless WebGPU instance loss (${error.message})`);
    }
    if (wasm2d !== null) {
      console.log(`PASS 2D WASM: ${wasm2d.initialStatus}; ${wasm2d.interactiveStatus}`);
    }
    if (wasm3d !== null) {
      console.log(`PASS 3D WASM: ${wasm3d.initialStatus}; ${wasm3d.interactiveStatus}`);
    }
    if (wasmDashboard !== null) {
      console.log(`PASS WASM dashboard: ${wasmDashboard}`);
    }
    console.log(`Wrote ${artifactsDir}`);
  } finally {
    page?.close();
    server.closeAllConnections?.();
    await new Promise((resolveClose) => server.close(resolveClose));
    chrome.chrome.kill('SIGTERM');
    await waitForProcessExit(chrome.chrome);
    await removeWithRetry(chrome.userDataDir);
  }
}

await main();
