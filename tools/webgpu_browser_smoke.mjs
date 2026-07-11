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

const COLOR_GREEN = '\x1b[32m';
const COLOR_YELLOW = '\x1b[33m';
const COLOR_RESET = '\x1b[0m';

const CAPTURE_CANVAS_WIDTH = 1280;
const CAPTURE_CANVAS_HEIGHT = 720;
const CAPTURE_PAGE_CHROME_HEIGHT = 96;
const CAPTURE_DEVICE_SCALE_FACTOR = 1;

function useColor() {
  if (process.env.FORCE_COLOR !== undefined && process.env.FORCE_COLOR !== '0') return true;
  if (process.env.NO_COLOR !== undefined) return false;
  return Boolean(process.stdout.isTTY);
}

function color(text, colorCode) {
  return useColor() ? `${colorCode}${text}${COLOR_RESET}` : text;
}

function passLine(text) {
  return `${color('PASS', COLOR_GREEN)} ${text}`;
}

function skipLine(text) {
  return `${color('SKIP', COLOR_YELLOW)} ${text}`;
}

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
    await this.send('Emulation.setDeviceMetricsOverride', {
      width: CAPTURE_CANVAS_WIDTH,
      height: CAPTURE_CANVAS_HEIGHT + CAPTURE_PAGE_CHROME_HEIGHT,
      deviceScaleFactor: CAPTURE_DEVICE_SCALE_FACTOR,
      mobile: false,
    });
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

  async canvasMetrics() {
    return await this.evaluate(`(() => {
      const canvas = document.querySelector("#viewport");
      if (canvas === null) return null;
      const rect = canvas.getBoundingClientRect();
      return {
        cssWidth: rect.width,
        cssHeight: rect.height,
        framebufferWidth: canvas.width,
        framebufferHeight: canvas.height,
        devicePixelRatio: window.devicePixelRatio,
        viewportWidth: window.innerWidth,
        viewportHeight: window.innerHeight,
      };
    })()`);
  }

  async screenshotCanvas(path, expectedSize = null) {
    const metrics = await this.canvasMetrics();
    requireOk(metrics !== null, `${path}: canvas is missing`);
    if (expectedSize !== null) {
      const expectedWidth = expectedSize.width;
      const expectedHeight = expectedSize.height;
      const matches = (
        metrics.cssWidth === expectedWidth &&
        metrics.cssHeight === expectedHeight &&
        metrics.framebufferWidth === expectedWidth &&
        metrics.framebufferHeight === expectedHeight &&
        metrics.devicePixelRatio === CAPTURE_DEVICE_SCALE_FACTOR
      );
      requireOk(
        matches,
        `${path}: unexpected capture canvas metrics: ${JSON.stringify(metrics)}; ` +
          `expected CSS=${expectedWidth}x${expectedHeight}, ` +
          `framebuffer=${expectedWidth}x${expectedHeight}, ` +
          `DPR=${CAPTURE_DEVICE_SCALE_FACTOR}`,
      );
    }
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
    if (expectedSize !== null) {
      requireOk(
        buffer.readUInt32BE(16) === expectedSize.width &&
          buffer.readUInt32BE(20) === expectedSize.height,
        `${path}: PNG dimensions do not match ${expectedSize.width}x${expectedSize.height}`,
      );
    }
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
    return (text.includes(${JSON.stringify(expectedStatus)}) || text.startsWith("Rendered ")) && text;
  })()`, 45000);
  requireOk(!String(initialStatus).startsWith('ERROR:'), initialStatus);
  await page.screenshotCanvas(screenshotPath, {
    width: CAPTURE_CANVAS_WIDTH,
    height: CAPTURE_CANVAS_HEIGHT,
  });
  const usesPacketRuntime = await page.evaluate(`(() => {
    const scene = window.__datovizWasmScene;
    return scene?.runtime?.stream?.name === "drp2_packet_set";
  })()`);
  requireOk(usesPacketRuntime, `${path}: WASM scene did not use DRP2 packet runtime`);
  const packetLifecycle = await page.evaluate(`(async () => {
    const scene = window.__datovizWasmScene;
    if (scene === undefined || scene === null || scene.scene === 0 || scene.runtime === null) {
      return "missing live scene";
    }
    const cloneSpan = (span) => ({
      packet: new Uint8Array(span.packet),
      arena: new Uint8Array(span.arena),
    });
    const clonePacketSet = (packetSet) => ({
      setup: cloneSpan(packetSet.setup),
      update: cloneSpan(packetSet.update),
      frame: cloneSpan(packetSet.frame),
      resource_version: packetSet.resource_version,
      frame_index: packetSet.frame_index,
      source: packetSet.source,
      artifact_resource_version: packetSet.artifact_resource_version,
      artifact_frame_index: packetSet.artifact_frame_index,
      artifact_spans_copied: packetSet.artifact_spans_copied,
      artifact_released: packetSet.artifact_released,
    });
    const packetStats = (packetSet) => ({
      setup: packetSet.setup.packet.byteLength,
      update: packetSet.update.packet.byteLength,
      frame: packetSet.frame.packet.byteLength,
      updateArena: packetSet.update.arena.byteLength,
    });

    const first = clonePacketSet(scene.emitPackets());
    const firstStats = packetStats(first);
    if (firstStats.update === 0 || firstStats.frame === 0) {
      return "missing split packet bytes " + JSON.stringify(firstStats);
    }
    if (firstStats.updateArena === 0) {
      return "missing update payload arena";
    }
    await scene.runtime.executePacketSet(first);
    const second = clonePacketSet(scene.emitPackets());
    await scene.runtime.executePacketSet(second);

    let staleRejected = false;
    try {
      await scene.runtime.executePacketSet(first);
    } catch (error) {
      staleRejected = String(error.message).includes("stale DRP2 packet");
    }
    if (!staleRejected) {
      return "stale packet was accepted after newer emit";
    }

    const resetPacket = clonePacketSet(scene.emitPackets());
    const resetStats = packetStats(resetPacket);
    if (resetStats.setup > 0) {
      await scene.runtime.executePacketSet(resetPacket, { reset: true, replaceExistingResources: false });
      const afterReset = clonePacketSet(scene.emitPackets());
      await scene.runtime.executePacketSet(afterReset);

      let staleAfterResetRejected = false;
      try {
        await scene.runtime.executePacketSet(resetPacket);
      } catch (error) {
        staleAfterResetRejected = String(error.message).includes("stale DRP2 packet");
      }
      if (!staleAfterResetRejected) {
        return "stale packet was accepted after runtime reset";
      }
    }

    return {
      first: firstStats,
      reset: resetStats,
      resource_version: scene.runtime.packetResourceVersion,
      frame_index: scene.runtime.packetFrameIndex,
    };
  })()`);
  requireOk(typeof packetLifecycle === 'object', `${path}: packet lifecycle failed: ${packetLifecycle}`);
  const resizeLifecycle = await page.evaluate(`(async () => {
    const scene = window.__datovizWasmScene;
    const session = window.__datovizWasmSession;
    const canvas = document.querySelector("#viewport");
    if (scene === undefined || scene === null || scene.scene === 0 || scene.runtime === null) {
      return "missing live scene";
    }
    if (session === undefined || session === null) {
      return "missing live session";
    }
    const before = {
      width: canvas.width,
      height: canvas.height,
      resource_version: scene.runtime.packetResourceVersion,
      frame_index: scene.runtime.packetFrameIndex,
    };
    const rect = canvas.getBoundingClientRect();
    canvas.style.width = Math.max(96, Math.floor(rect.width * 0.75)) + "px";
    canvas.style.height = Math.max(96, Math.floor(rect.height * 0.75)) + "px";
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    scene.resize();
    if (canvas.width === before.width && canvas.height === before.height) {
      return "canvas framebuffer size did not change";
    }
    await session.render();
    const afterScene = window.__datovizWasmScene;
    if (afterScene?.runtime?.stream?.name !== "drp2_packet_set") {
      return "resize render left packet runtime";
    }
    if (afterScene.runtime.packetFrameIndex <= before.frame_index) {
      return "resize packet frame index did not advance";
    }
    return {
      before,
      after: {
        width: canvas.width,
        height: canvas.height,
        resource_version: afterScene.runtime.packetResourceVersion,
        frame_index: afterScene.runtime.packetFrameIndex,
      },
    };
  })()`);
  requireOk(typeof resizeLifecycle === 'object', `${path}: resize lifecycle failed: ${resizeLifecycle}`);
  await page.dragCanvas(48, 24);
  await page.wheelCanvas(-180);
  const interactiveStatus = await page.waitFor(`(() => {
    const status = document.querySelector("#status");
    const text = status?.textContent ?? "";
    if (status?.classList.contains("error")) return "ERROR: " + text;
    return text.startsWith("Rendered ") && text;
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

async function smokeAnimatedWasmPage(
  page,
  baseUrl,
  path,
  expectedStatus,
  screenshotPath,
  expectedScenarioId = 'features_timer_animation',
) {
  await page.navigate(`${baseUrl}${path}`);
  requireOk(
    await page.evaluate('typeof navigator.gpu === "object"'),
    `navigator.gpu is not available for ${path}`,
  );
  const initialStatus = await page.waitFor(`(() => {
    const status = document.querySelector("#status");
    const text = status?.textContent ?? "";
    if (status?.classList.contains("error")) return "ERROR: " + text;
    return text.includes(${JSON.stringify(expectedStatus)}) && text;
  })()`, 45000);
  requireOk(!String(initialStatus).startsWith('ERROR:'), initialStatus);
  const initialFrame = await page.evaluate(`(() => {
    const scene = window.__datovizWasmScene;
    return scene?.runtime?.packetFrameIndex ?? 0;
  })()`);
  const animatedFrame = await page.waitFor(`(() => {
    const scene = window.__datovizWasmScene;
    const session = window.__datovizWasmSession;
    if (scene?.runtime?.stream?.name !== "drp2_packet_set") return false;
    if (scene?.scenario?.id !== ${JSON.stringify(expectedScenarioId)}) return false;
    return (scene.runtime.packetFrameIndex > ${Number(initialFrame)} + 1) && session?.animationFrame !== 0;
  })()`, 15000);
  requireOk(animatedFrame, `${path}: scenario animation did not advance`);
  const pointerContinuity = await page.evaluate(`(async () => {
    const session = window.__datovizWasmSession;
    const scene = window.__datovizWasmScene;
    const canvas = document.querySelector("#viewport");
    if (session === null || scene === null || canvas === null) return "missing live session";
    const startTime = session.animationStartTime;
    const frameIndex = scene.runtime?.packetFrameIndex ?? 0;
    const rect = canvas.getBoundingClientRect();
    canvas.dispatchEvent(new PointerEvent("pointermove", {
      clientX: rect.left + 0.6 * rect.width,
      clientY: rect.top + 0.45 * rect.height,
      pointerId: 1,
      bubbles: true,
    }));
    for (let i = 0; i < 120; i++) {
      await new Promise((resolve) => requestAnimationFrame(resolve));
      if ((scene.runtime?.packetFrameIndex ?? 0) > frameIndex) {
        return session.animationStartTime === startTime || "pointer movement restarted animation time";
      }
    }
    return "animation did not advance after pointer movement";
  })()`);
  requireOk(pointerContinuity === true, `${path}: ${pointerContinuity}`);
  await page.screenshotCanvas(screenshotPath, {
    width: CAPTURE_CANVAS_WIDTH,
    height: CAPTURE_CANVAS_HEIGHT,
  });
  await page.evaluate(`(() => {
    window.__datovizWasmSession?.stopAnimationLoop();
  })()`);
  const destroyed = await page.evaluate(`(() => {
    const scene = window.__datovizWasmScene;
    if (scene === undefined || scene === null || scene.scene === 0) return false;
    window.dispatchEvent(new Event("pagehide"));
    return window.__datovizWasmScene === null && scene.scene === 0;
  })()`);
  requireOk(destroyed, `${path}: WASM scenario did not destroy cleanly on pagehide`);
  assertNoBrowserErrors(page, path);
  return { initialStatus, initialFrame };
}

async function smokeQueryWasmPage(page, baseUrl, scenario, screenshotPath) {
  const path = scenario.path ?? `/examples/webgpu/examples.html?demo=${scenario.demo}`;
  await page.navigate(`${baseUrl}${path}`);
  requireOk(
    await page.evaluate('typeof navigator.gpu === "object"'),
    `navigator.gpu is not available for ${path}`,
  );
  const initialStatus = await page.waitFor(`(() => {
    const status = document.querySelector("#status");
    const text = status?.textContent ?? "";
    if (status?.classList.contains("error")) return "ERROR: " + text;
    return text.includes(${JSON.stringify(`Rendered ${scenario.label}`)}) && text;
  })()`, 45000);
  requireOk(!String(initialStatus).startsWith('ERROR:'), initialStatus);
  await page.screenshotCanvas(screenshotPath);

  const queryDelivery = await page.waitFor(`(async () => {
    let stage = "start";
    try {
      const scene = window.__datovizWasmScene;
      const session = window.__datovizWasmSession;
      const status = document.querySelector("#status");
      const text = status?.textContent ?? "";
      if (status?.classList.contains("error")) return "ERROR: " + text;
      if (scene === undefined || scene === null || scene.scene === 0 || scene.runtime === null) {
        return false;
      }
      if (session === undefined || session === null) {
        return false;
      }
      if (scene.scenario?.id !== ${JSON.stringify(scenario.scenarioId)}) {
        return "wrong scenario id " + scene.scenario?.id;
      }
      const Module = scene.Module;
      if (
        typeof Module._dvz_wasm_api_query_pending_count !== "function" ||
        typeof scene.flushScenarioQueries !== "function"
      ) {
        return "missing query readback ABI";
      }
      for (let i = 0; i < 100 && session.rendering; i++) {
        await new Promise((resolve) => setTimeout(resolve, 50));
      }

      stage = "prime-pointer";
      const canvas = document.querySelector("#viewport");
      const rect = canvas.getBoundingClientRect();
      const pointerX = rect.left + rect.width * ${Number(scenario.pointerX ?? 0.5)};
      const pointerY = rect.top + rect.height * ${Number(scenario.pointerY ?? 0.5)};
      scene.scenarioPointer(2, {
        clientX: pointerX,
        clientY: pointerY,
        button: -1,
        buttons: 0,
        shiftKey: false,
        ctrlKey: false,
        altKey: false,
        metaKey: false,
      });

      stage = "queue-query";
      await session.render();
      const pendingBefore = Module._dvz_wasm_api_query_pending_count(scene.scene);
      if (pendingBefore === 0) {
        return false;
      }

      stage = "drain-query";
      const frameBefore = scene.runtime.packetFrameIndex;
      const processed = await scene.flushScenarioQueries();
      const pendingAfter = Module._dvz_wasm_api_query_pending_count(scene.scene);
      const activeAfter = Module._dvz_wasm_api_query_active(scene.scene);
      const frameAfter = scene.runtime.packetFrameIndex;
      if (processed <= 0) {
        return "query drain processed no requests";
      }
      if (pendingAfter !== 0 || activeAfter !== 0) {
        return "query drain left pending=" + pendingAfter + " active=" + activeAfter;
      }
      if (frameAfter <= frameBefore) {
        return "query packet execution did not advance the runtime frame";
      }

      stage = "render-resolved-result";
      await session.render();
      if (${scenario.pressAfterResolve === true ? 'true' : 'false'}) {
        stage = "press-resolved-result";
        scene.scenarioPointer(1, {
          clientX: pointerX,
          clientY: pointerY,
          button: 0,
          buttons: 1,
          shiftKey: false,
          ctrlKey: false,
          altKey: false,
          metaKey: false,
        });
        await session.render();
      }
      return { processed, pendingBefore, frameBefore, frameAfter };
    } catch (error) {
      return stage + ": " + (error instanceof Error ? error.message : String(error));
    }
  })()`, 30000);
  requireOk(typeof queryDelivery === 'object', `${path}: query/readback delivery failed: ${queryDelivery}`);
  await page.screenshotCanvas(screenshotPath.replace('.png', '-resolved.png'));
  const destroyed = await page.evaluate(`(() => {
    const scene = window.__datovizWasmScene;
    if (scene === undefined || scene === null || scene.scene === 0) return false;
    window.dispatchEvent(new Event("pagehide"));
    return window.__datovizWasmScene === null && scene.scene === 0;
  })()`);
  requireOk(destroyed, `${path}: WASM query scenario did not destroy cleanly on pagehide`);
  assertNoBrowserErrors(page, path);
  return { initialStatus, queryDelivery };
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
         text.includes('Instance dropped in popErrorScope') ||
         text.includes('Instance dropped error in getCompilationInfo');
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
    let wasmBasic = null;
    let wasmColorbar = null;
    let wasmScalebar = null;
    let wasmScalebarUnits = null;
    let wasmLegend = null;
    let wasmReadout = null;
    let wasmQuery = null;
    let wasmLinkedProbe = null;
    let wasmScientific = null;
    let wasmVector = null;
    let wasmSegment = null;
    let wasmLabels = null;
    let wasmWindField = null;
    let wasmIsolines = null;
    let wasmParticleSmoke = null;
    let wasmPanelGrid = null;
    let wasmPanzoom = null;
    let wasmAxisLabels = null;
    try {
      wasmBasic = await smokeAnimatedWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_timer_animation',
        'Rendered Timer Animation',
        join(artifactsDir, 'webgpu_live_timer_animation.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live basic smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmColorbar = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_colorbar',
        'Rendered Colorbar',
        join(artifactsDir, 'webgpu_live_colorbar.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live colorbar smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmScalebar = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_scalebar',
        'Rendered Scale Bar',
        join(artifactsDir, 'webgpu_live_scale_bar.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live scale-bar smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmScalebarUnits = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_scalebar_units',
        'Rendered Scale Bar Units',
        join(artifactsDir, 'webgpu_live_scalebar_units.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(
        skipLine(`WebGPU live scale-bar units smoke: headless WebGPU instance loss (${error.message})`),
      );
    }
    try {
      wasmLegend = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_legend_categorical',
        'Rendered Categorical Legend',
        join(artifactsDir, 'webgpu_live_legend_categorical.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live legend smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmReadout = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_annotation_readout',
        'Rendered Annotation Readout',
        join(artifactsDir, 'webgpu_live_annotation_readout.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live annotation readout smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmQuery = await smokeQueryWasmPage(
        page,
        baseUrl,
        {
          path: '/examples/webgpu/live.html?id=features_picking',
          label: 'Picking',
          scenarioId: 'features_picking',
          pressAfterResolve: true,
        },
        join(artifactsDir, 'webgpu_live_picking.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live query smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmLinkedProbe = await smokeQueryWasmPage(
        page,
        baseUrl,
        {
          path: '/examples/webgpu/live.html?id=showcases_linked_probe_colorbar',
          label: 'Linked Probe With Colorbar',
          scenarioId: 'showcases_linked_probe_colorbar',
          pointerX: 0.25,
          pointerY: 0.52,
        },
        join(artifactsDir, 'webgpu_live_linked_probe_colorbar.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live linked-probe smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmScientific = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=showcases_scientific_plotting',
        'Rendered Scientific Plotting Workflow',
        join(artifactsDir, 'webgpu_live_scientific_plotting.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live scientific plotting smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmVector = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=visuals_vector',
        'Rendered Vector',
        join(artifactsDir, 'webgpu_live_vector.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live vector smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmSegment = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=visuals_segment',
        'Rendered Segment',
        join(artifactsDir, 'webgpu_live_visual_segment.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live segment smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmLabels = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=visuals_labels',
        'Rendered Labels',
        join(artifactsDir, 'webgpu_live_visual_labels.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live labels smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmWindField = await smokeAnimatedWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=showcases_wind_field',
        'Rendered Wind Field',
        join(artifactsDir, 'webgpu_live_wind_field.png'),
        'showcases_wind_field',
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live wind-field smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmIsolines = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_isolines',
        'Rendered Isolines',
        join(artifactsDir, 'webgpu_live_isolines.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live isolines smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmParticleSmoke = await smokeAnimatedWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=showcases_gpu_particle_smoke',
        'Rendered GPU Particle Smoke',
        join(artifactsDir, 'webgpu_live_gpu_particle_smoke.png'),
        'showcases_gpu_particle_smoke',
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live particle-smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmPanelGrid = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_panel_grid',
        'Rendered Panel Grid',
        join(artifactsDir, 'webgpu_live_panel_grid.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live panel-grid smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmPanzoom = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_panzoom',
        'Rendered Panzoom',
        join(artifactsDir, 'webgpu_live_panzoom.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live panzoom smoke: headless WebGPU instance loss (${error.message})`));
    }
    try {
      wasmAxisLabels = await smokeWasmPage(
        page,
        baseUrl,
        '/examples/webgpu/live.html?id=features_axis_labels',
        'Rendered Axis Labels',
        join(artifactsDir, 'webgpu_live_axis_labels.png'),
      );
    } catch (error) {
      if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
        throw error;
      }
      console.log(skipLine(`WebGPU live axis-labels smoke: headless WebGPU instance loss (${error.message})`));
    }
    const promotedLiveRoutes = [
      ['features_panel_multi', 'Multiple Panels', 'webgpu_live_panel_multi.png', 'multi-panel'],
      ['features_panel_linked', 'Linked Panels', 'webgpu_live_panel_linked.png', 'linked-panel'],
      ['features_text_block', 'Text Block', 'webgpu_live_text_block.png', 'text-block'],
      ['features_overlay_card', 'Overlay Card', 'webgpu_live_overlay_card.png', 'overlay-card'],
      ['features_guide_lines', 'Guide Lines', 'webgpu_live_guide_lines.png', 'guide-lines'],
      ['features_guide_spans', 'Guide Spans', 'webgpu_live_guide_spans.png', 'guide-spans'],
      ['features_bars_bands', 'Bars And Bands', 'webgpu_live_bars_bands.png', 'bars-bands'],
      ['features_controller_fly', 'Fly Controller', 'webgpu_live_controller_fly.png', 'fly-controller'],
      [
        'features_controller_turntable',
        'Turntable Controller',
        'webgpu_live_controller_turntable.png',
        'turntable-controller',
      ],
      ['features_colormap_scale', 'Scalar Color Scale', 'webgpu_live_colormap_scale.png', 'colormap-scale'],
      ['features_panel_background', 'Panel Background', 'webgpu_live_panel_background.png', 'panel-background'],
      ['composites_polygon', 'Polygon Composite', 'webgpu_live_composite_polygon.png', 'composite-polygon'],
      [
        'showcases_panel_linked_axes',
        'Linked Panels With Axes',
        'webgpu_live_linked_panels_axes.png',
        'linked-panels-axes',
      ],
      [
        'showcases_scalebar_measurement',
        'Scale Bar Measurement Workflow',
        'webgpu_live_scalebar_measurement.png',
        'scalebar-measurement',
      ],
      ['showcases_surface_grid', 'Surface Grid', 'webgpu_live_surface_grid.png', 'surface-grid'],
      [
        'showcases_choropleth',
        'U.S. State Choropleth',
        'webgpu_live_us_state_choropleth.png',
        'us-state-choropleth',
      ],
      ['features_technique_depth_test', 'Depth Test Toggle', 'webgpu_live_depth_test.png', 'depth-test'],
      ['features_alpha_blending', 'Alpha Blending', 'webgpu_live_alpha_blending.png', 'alpha-blending'],
      [
        'features_material_mesh',
        'Mesh Materials',
        'webgpu_live_material_mesh.png',
        'material-mesh',
      ],
      ['features_lighting', 'Lighting', 'webgpu_live_lighting.png', 'lighting'],
      [
        'showcases_textured_planet',
        'Textured Planets',
        'webgpu_live_textured_planets.png',
        'textured-planets',
      ],
      [
        'showcases_protein',
        'Protein',
        'webgpu_live_protein.png',
        'protein',
      ],
    ];
    for (const [id, label, filename, shortLabel] of promotedLiveRoutes) {
      try {
        const result = await smokeWasmPage(
          page,
          baseUrl,
          `/examples/webgpu/live.html?id=${id}`,
          `Rendered ${label}`,
          join(artifactsDir, filename),
        );
        console.log(passLine(`live ${shortLabel}: ${result.initialStatus}`));
      } catch (error) {
        if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
          throw error;
        }
        console.log(skipLine(`WebGPU live ${shortLabel} smoke: headless WebGPU instance loss (${error.message})`));
      }
    }

    const promotedAnimatedRoutes = [
      [
        'features_sampled_field_update',
        'Sampled Field Update',
        'webgpu_live_sampled_field_update.png',
        'sampled-field-update',
        'features_sampled_field_update',
      ],
      [
        'features_update_partial',
        'Partial Data Update',
        'webgpu_live_update_partial.png',
        'partial-update',
        'features_update_partial',
      ],
      [
        'features_update_visual_data',
        'Visual Data Update',
        'webgpu_live_update_visual_data.png',
        'visual-data-update',
        'features_update_visual_data',
      ],
      [
        'features_visibility',
        'Visual Visibility',
        'webgpu_live_visibility.png',
        'visibility',
        'features_visibility',
      ],
    ];
    for (const [id, label, filename, shortLabel, scenarioId] of promotedAnimatedRoutes) {
      try {
        const result = await smokeAnimatedWasmPage(
          page,
          baseUrl,
          `/examples/webgpu/live.html?id=${id}`,
          `Rendered ${label}`,
          join(artifactsDir, filename),
          scenarioId,
        );
        console.log(passLine(`live ${shortLabel}: ${result.initialStatus}`));
      } catch (error) {
        if (!isKnownHeadlessWebGpuInstanceLoss(error.message)) {
          throw error;
        }
        console.log(skipLine(`WebGPU live ${shortLabel} smoke: headless WebGPU instance loss (${error.message})`));
      }
    }
    if (wasmBasic !== null) {
      console.log(passLine(`live basic: ${wasmBasic.initialStatus}; initial_frame=${wasmBasic.initialFrame}`));
    }
    if (wasmColorbar !== null) {
      console.log(passLine(`live colorbar: ${wasmColorbar.initialStatus}`));
    }
    if (wasmScalebar !== null) {
      console.log(passLine(`live scale-bar: ${wasmScalebar.initialStatus}`));
    }
    if (wasmScalebarUnits !== null) {
      console.log(passLine(`live scale-bar units: ${wasmScalebarUnits.initialStatus}`));
    }
    if (wasmLegend !== null) {
      console.log(passLine(`live legend: ${wasmLegend.initialStatus}`));
    }
    if (wasmReadout !== null) {
      console.log(passLine(`live annotation readout: ${wasmReadout.initialStatus}`));
    }
    if (wasmQuery !== null) {
      console.log(
        passLine(
          `live query: ${wasmQuery.initialStatus}; processed=${wasmQuery.queryDelivery.processed}`,
        ),
      );
    }
    if (wasmLinkedProbe !== null) {
      console.log(
        passLine(
          `live linked probe: ${wasmLinkedProbe.initialStatus}; processed=${wasmLinkedProbe.queryDelivery.processed}`,
        ),
      );
    }
    if (wasmScientific !== null) {
      console.log(passLine(`live scientific plotting: ${wasmScientific.initialStatus}`));
    }
    if (wasmVector !== null) {
      console.log(passLine(`live vector: ${wasmVector.initialStatus}`));
    }
    if (wasmSegment !== null) {
      console.log(passLine(`live segment: ${wasmSegment.initialStatus}`));
    }
    if (wasmLabels !== null) {
      console.log(passLine(`live labels: ${wasmLabels.initialStatus}`));
    }
    if (wasmWindField !== null) {
      console.log(passLine(`live wind-field: ${wasmWindField.initialStatus}`));
    }
    if (wasmIsolines !== null) {
      console.log(passLine(`live isolines: ${wasmIsolines.initialStatus}`));
    }
    if (wasmParticleSmoke !== null) {
      console.log(passLine(`live particle-smoke: ${wasmParticleSmoke.initialStatus}`));
    }
    if (wasmPanelGrid !== null) {
      console.log(passLine(`live panel-grid: ${wasmPanelGrid.initialStatus}`));
    }
    if (wasmPanzoom !== null) {
      console.log(passLine(`live panzoom: ${wasmPanzoom.initialStatus}`));
    }
    if (wasmAxisLabels !== null) {
      console.log(passLine(`live axis-labels: ${wasmAxisLabels.initialStatus}`));
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
