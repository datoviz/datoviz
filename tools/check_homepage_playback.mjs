#!/usr/bin/env node

// Deterministic policy checks for the gallery/homepage video playback controller.
// The default fixture mocks media decoding; pass --video=PATH to also verify real decoding.

import { createReadStream } from 'node:fs';
import { createServer } from 'node:http';
import { existsSync } from 'node:fs';
import { readFile, stat } from 'node:fs/promises';
import { spawn } from 'node:child_process';
import { join, resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const script = await readFile(join(root, 'docs/javascripts/gallery-media.js'), 'utf8');
const videoArg = process.argv.find((arg) => arg.startsWith('--video='));
const videoPath = videoArg ? resolve(videoArg.slice('--video='.length)) : null;
const browserCandidates = [process.env.CHROME, '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome', '/Applications/Chromium.app/Contents/MacOS/Chromium', '/opt/homebrew/bin/chromium', '/usr/bin/chromium', '/usr/bin/chromium-browser', '/usr/bin/google-chrome'].filter(Boolean);

function check(value, message) {
  if (!value) throw new Error(message);
}

function browserPath() {
  const found = browserCandidates.find((path) => existsSync(path));
  check(found, 'Chrome/Chromium not found; set CHROME=/path/to/chrome');
  return found;
}

class Cdp {
  constructor(url) {
    this.ws = new WebSocket(url);
    this.id = 0;
    this.pending = new Map();
    this.events = new Map();
    this.ws.addEventListener('message', ({ data }) => {
      const message = JSON.parse(data);
      if (message.id) {
        const pending = this.pending.get(message.id);
        if (!pending) return;
        this.pending.delete(message.id);
        message.error ? pending.reject(new Error(message.error.message)) : pending.resolve(message.result);
      } else {
        for (const handler of this.events.get(message.method) || []) handler(message.params || {});
      }
    });
  }
  async open() {
    await new Promise((resolveOpen, rejectOpen) => {
      this.ws.addEventListener('open', resolveOpen, { once: true });
      this.ws.addEventListener('error', rejectOpen, { once: true });
    });
    await this.send('Page.enable');
    await this.send('Runtime.enable');
  }
  on(name, handler) {
    const list = this.events.get(name) || [];
    list.push(handler);
    this.events.set(name, list);
  }
  send(method, params = {}) {
    const id = ++this.id;
    this.ws.send(JSON.stringify({ id, method, params }));
    return new Promise((resolveSend, rejectSend) => this.pending.set(id, { resolve: resolveSend, reject: rejectSend }));
  }
  async evaluate(expression) {
    const response = await this.send('Runtime.evaluate', { expression, awaitPromise: true, returnByValue: true });
    if (response.exceptionDetails) throw new Error(response.exceptionDetails.exception?.description || response.exceptionDetails.text);
    return response.result.value;
  }
  async navigate(url) {
    const loaded = new Promise((resolveLoad) => this.on('Page.loadEventFired', resolveLoad));
    await this.send('Page.navigate', { url });
    await loaded;
  }
  close() { this.ws.close(); }
}

const server = createServer(async (request, response) => {
  const path = new URL(request.url, 'http://fixture').pathname;
  if (path === '/fixture.html') {
    const fixtureUrl = new URL(request.url, 'http://fixture');
    const realVideo = Boolean(videoPath && fixtureUrl.searchParams.get('real') === '1');
    const sourceUrl = fixtureUrl.searchParams.get('external') === '1' ? 'https://media.example.invalid/clip.mp4' : (realVideo ? '/video.mp4' : '/mock.mp4');
    const html = `<!doctype html><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{margin:0}.card{width:min(90vw,640px);margin:32px auto}.dvz-gallery-media{position:relative;width:100%;aspect-ratio:16/9}.dvz-gallery-media img,.dvz-gallery-media video{position:absolute;width:100%;height:100%;object-fit:cover}.dvz-gallery-video-control{position:absolute;right:8px;bottom:8px}</style><div class="card" data-gallery-lazy="video"><div class="dvz-gallery-media"><img class="dvz-gallery-poster" src="/poster.svg" alt="Preview poster"><video class="dvz-gallery-video" muted loop playsinline preload="none" poster="/poster.svg" aria-label="preview"><source data-src="${sourceUrl}" type="video/mp4"></video></div></div><script>(${fixtureSetup.toString()})(${JSON.stringify({ reduce: fixtureUrl.searchParams.get('reduce') === '1', saveData: fixtureUrl.searchParams.get('saveData') === '1', rejectPlay: fixtureUrl.searchParams.get('reject') === '1', realVideo })})</script><script src="/gallery-media.js"></script>`;
    response.writeHead(200, { 'content-type': 'text/html' }); response.end(html); return;
  }
  if (path === '/nojs.html') {
    response.writeHead(200, { 'content-type': 'text/html' });
    response.end('<!doctype html><img src="/poster.svg" alt="Preview poster"><video poster="/poster.svg"><source data-src="/mock.mp4" type="video/mp4"></video>'); return;
  }
  if (path === '/assets/gallery/v0.4/local-video-assets.json') { response.writeHead(200, { 'content-type': 'application/json' }); response.end(JSON.stringify({ available: [] })); return; }
  if (path === '/gallery-media.js') { response.writeHead(200, { 'content-type': 'text/javascript' }); response.end(script); return; }
  if (path === '/poster.svg') { response.writeHead(200, { 'content-type': 'image/svg+xml' }); response.end('<svg xmlns="http://www.w3.org/2000/svg" width="16" height="9"/>'); return; }
  if (path === '/video.mp4' && videoPath) {
    const info = await stat(videoPath); const range = request.headers.range;
    if (range) {
      const [start, endRaw] = range.replace(/bytes=/, '').split('-'); const from = Number(start); const to = Math.min(Number(endRaw || info.size - 1), info.size - 1);
      response.writeHead(206, { 'content-type': 'video/mp4', 'content-length': to - from + 1, 'content-range': `bytes ${from}-${to}/${info.size}`, 'accept-ranges': 'bytes' });
      createReadStream(videoPath, { start: from, end: to }).pipe(response); return;
    }
    response.writeHead(200, { 'content-type': 'video/mp4', 'content-length': info.size }); response.end(await readFile(videoPath)); return;
  }
  response.writeHead(404); response.end('not found');
});

function fixtureSetup(options) {
  window.matchMedia = (query) => ({ matches: query.includes('prefers-reduced-motion') && options.reduce, media: query, addListener() {}, removeListener() {} });
  Object.defineProperty(navigator, 'connection', { configurable: true, value: { saveData: options.saveData } });
  const observers = [];
  window.IntersectionObserver = class {
    constructor(callback, config) { this.callback = callback; this.config = config; this.targets = new Set(); observers.push(this); }
    observe(target) { this.targets.add(target); }
    trigger(target, isIntersecting) { this.callback([{ target, isIntersecting }]); }
  };
  window.__observers = observers;
  window.__visibility = false;
  Object.defineProperty(document, 'hidden', { configurable: true, get: () => window.__visibility });
  if (!options.realVideo) {
    HTMLMediaElement.prototype.load = function () { this.dataset.loadCalls = String(Number(this.dataset.loadCalls || 0) + 1); };
    HTMLMediaElement.prototype.play = function () {
      this.dataset.playCalls = String(Number(this.dataset.playCalls || 0) + 1);
      if (options.rejectPlay) return Promise.reject(new DOMException('Autoplay blocked', 'NotAllowedError'));
      this.__paused = false; this.dispatchEvent(new Event('play')); return Promise.resolve();
    };
    HTMLMediaElement.prototype.pause = function () {
      this.dataset.pauseCalls = String(Number(this.dataset.pauseCalls || 0) + 1);
      this.__paused = true; this.dispatchEvent(new Event('pause'));
    };
    Object.defineProperty(HTMLMediaElement.prototype, 'paused', { configurable: true, get() { return this.__paused !== false; } });
  }
  window.__trigger = (index, visible) => observers[index].trigger(document.querySelector('[data-gallery-lazy]'), visible);
}

const chromeArgs = ['--headless=new', '--no-sandbox', '--no-first-run', '--no-default-browser-check', '--disable-background-networking', '--host-resolver-rules=MAP docs.example.invalid 127.0.0.1', '--remote-allow-origins=*', '--remote-debugging-port=0', 'about:blank'];
const chrome = spawn(browserPath(), chromeArgs, { stdio: ['ignore', 'ignore', 'pipe'] });
let chromeError = '';
chrome.stderr.on('data', (chunk) => { chromeError += chunk.toString(); });
let page;
let serverPort;
try {
  server.listen(0, '127.0.0.1');
  await new Promise((resolveListen) => server.once('listening', resolveListen));
  serverPort = server.address().port;
  // Chrome chooses an ephemeral CDP port and prints it only when remote debugging is enabled.
  const existingPort = Number(chromeError.match(/DevTools listening on ws:\/\/[^:]+:(\d+)\//)?.[1] || 0);
  const debugPort = existingPort || await new Promise((resolvePort, rejectPort) => {
    const timer = setTimeout(() => rejectPort(new Error('Chrome did not expose its DevTools endpoint')), 15000);
    chrome.stderr.on('data', (chunk) => {
      const match = chromeError.match(/DevTools listening on ws:\/\/[^:]+:(\d+)\//);
      if (match) { clearTimeout(timer); resolvePort(Number(match[1])); }
    });
  });
  const target = await (await fetch(`http://127.0.0.1:${debugPort}/json/new?about:blank`, { method: 'PUT' })).json();
  page = new Cdp(target.webSocketDebuggerUrl);
  await page.open();
  const base = `http://docs.example.invalid:${serverPort}`;

  await page.navigate(`${base}/fixture.html`);
  await page.evaluate(`window.__trigger(0, true); window.__trigger(1, true)`);
  const autoplay = await page.evaluate(`(() => { const c=document.querySelector('[data-gallery-lazy]'),v=c.querySelector('video'),s=v.querySelector('source'); return {loaded:c.dataset.loaded,src:s.getAttribute('src'),plays:v.dataset.playCalls||'0',control:!!c.querySelector('button'),rootMargin:window.__observers[0].config.rootMargin}; })()`);
  check(autoplay.loaded === '1' && autoplay.src, 'near-viewport preload did not assign source/load');
  check(autoplay.plays === '1' && autoplay.control && autoplay.rootMargin === '400px 0px', `visible card did not autoplay with control and preload margin: ${JSON.stringify(autoplay)}`);

  await page.evaluate(`window.__trigger(1, false)`);
  await page.evaluate(`window.__trigger(1, true)`);
  const resume = await page.evaluate(`(() => {const c=document.querySelector('[data-gallery-lazy]'),v=c.querySelector('video');return {plays:Number(v.dataset.playCalls),pauses:Number(v.dataset.pauseCalls)};})()`);
  check(resume.plays === 2 && resume.pauses >= 1, 'scrolling out and back did not pause then resume');

  await page.evaluate(`document.querySelector('button').click()`);
  await page.evaluate(`window.__trigger(1, false); window.__trigger(1, true)`);
  const userPause = await page.evaluate(`(() => {const c=document.querySelector('[data-gallery-lazy]'),v=c.querySelector('video');return {paused:c.dataset.userPaused,plays:Number(v.dataset.playCalls)};})()`);
  check(userPause.paused === '1' && userPause.plays === 2, 'scrolling resumed a user-paused video');
  await page.evaluate(`document.querySelector('button').click()`);
  const userPlay = await page.evaluate(`document.querySelector('[data-gallery-lazy]').dataset.userPaused`);
  check(userPlay === '0', 'play control did not clear user pause');

  await page.evaluate(`window.__visibility=true; document.dispatchEvent(new Event('visibilitychange'))`);
  const background = await page.evaluate(`(() => {const v=document.querySelector('video');return {plays:Number(v.dataset.playCalls),pauses:Number(v.dataset.pauseCalls)};})()`);
  check(background.plays === 3 && background.pauses >= 2, 'background visibility policy did not pause visible playback');
  await page.evaluate(`window.__visibility=false; document.dispatchEvent(new Event('visibilitychange'))`);
  check(await page.evaluate(`Number(document.querySelector('video').dataset.playCalls)`) === 4, 'foreground visibility did not resume eligible playback');
  console.log('PASS homepage playback policy: preload, autoplay, visibility, user pause/resume');

  for (const query of ['?reduce=1', '?saveData=1']) {
    await page.navigate(`${base}/fixture.html${query}`);
    const disabled = await page.evaluate(`(() => {const c=document.querySelector('[data-gallery-lazy]'),v=c.querySelector('video'),s=v.querySelector('source');return {loaded:c.dataset.loaded||'',src:s.getAttribute('src'),plays:v.dataset.playCalls||'0',control:!!c.querySelector('button'),poster:!!v.poster};})()`);
    check(disabled.loaded === '' && disabled.src === null && disabled.plays === '0' && !disabled.control && disabled.poster, `${query} did not stay poster-only`);
  }
  console.log('PASS reduced-motion and save-data modes remain poster-only');

  await page.navigate(`${base}/fixture.html?reject=1`);
  await page.evaluate(`window.__trigger(0, true); window.__trigger(1, true)`);
  await new Promise((resolveDelay) => setTimeout(resolveDelay, 0));
  const rejected = await page.evaluate(`(() => {const c=document.querySelector('[data-gallery-lazy]');return {control:!!c.querySelector('button'),state:c.querySelector('button')?.dataset.state};})()`);
  check(rejected.control && rejected.state === 'paused', 'rejected autoplay did not retain a usable paused control');
  console.log('PASS rejected autoplay retains poster and play control');

  await page.send('Emulation.setDeviceMetricsOverride', { width: 375, height: 812, deviceScaleFactor: 1, mobile: true });
  await page.navigate(`${base}/fixture.html`);
  const mobile = await page.evaluate(`({viewport:innerWidth,document:document.documentElement.scrollWidth,card:document.querySelector('.card').getBoundingClientRect().width})`);
  check(mobile.document <= mobile.viewport && mobile.card <= mobile.viewport, `mobile fixture overflows horizontally: ${JSON.stringify(mobile)}`);
  console.log('PASS mobile viewport has no horizontal overflow');

  await page.navigate(`${base}/nojs.html`);
  const noJs = await page.evaluate(`({poster:!!document.querySelector('img[src="/poster.svg"]'),source:document.querySelector('source').getAttribute('src')})`);
  check(noJs.poster && noJs.source === null, 'no-JS fallback does not show the poster without loading video');
  console.log('PASS no-JavaScript fallback displays poster without loading video');

  for (const external of [false, true]) {
    await page.navigate(`http://localhost:${serverPort}/fixture.html${external ? '?external=1' : ''}`);
    check(await page.evaluate(`new Promise(resolve => { const started=Date.now(); const timer=setInterval(()=>{if(window.__observers?.length===2){clearInterval(timer);resolve(true);}else if(Date.now()-started>2000){clearInterval(timer);resolve(false);}},10); })`), 'local inventory did not initialize');
    await page.evaluate(`window.__trigger(0,true); window.__trigger(1,true)`);
    const actual = await page.evaluate(`({src:document.querySelector('source').getAttribute('src'),control:!!document.querySelector('button')})`);
    if (external) check(actual.src === 'https://media.example.invalid/clip.mp4' && actual.control, 'external source was inventory-gated or its URL was changed');
    else check(actual.src === null && !actual.control, 'missing local source was downloaded');
  }
  console.log('PASS localhost external URLs retain their identity; missing local files remain poster-only');

  if (videoPath) {
    await page.send('Emulation.setDeviceMetricsOverride', { width: 1280, height: 720, deviceScaleFactor: 1, mobile: false });
    await page.navigate(`${base}/fixture.html?real=1`);
    await page.evaluate(`window.__trigger(0, true); window.__trigger(1, true)`);
    const decoded = await page.evaluate(`new Promise((resolveDecode) => {const v=document.querySelector('video');const finish=()=>resolveDecode({readyState:v.readyState,duration:v.duration,videoWidth:v.videoWidth,videoHeight:v.videoHeight});if(v.readyState>=2)finish();else{v.addEventListener('loadeddata',finish,{once:true});v.addEventListener('error',()=>resolveDecode({error:v.error?.message||'media error'}),{once:true});setTimeout(()=>resolveDecode({timeout:true,readyState:v.readyState}),10000);}})`);
    check(decoded.readyState >= 2 && decoded.videoWidth > 0 && decoded.videoHeight > 0, `real MP4 decode failed: ${JSON.stringify(decoded)}`);
    console.log(`PASS real MP4 decode: ${decoded.videoWidth}x${decoded.videoHeight}, ${decoded.duration.toFixed(3)}s`);
  } else {
    console.log('INFO policy fixture uses mocked HTMLMediaElement play/load; pass --video=PATH for a real MP4 decode check');
  }
} catch (error) {
  console.error(`FAIL homepage playback check: ${error.message}`);
  if (chromeError) console.error(chromeError);
  process.exitCode = 1;
} finally {
  page?.close();
  chrome.kill('SIGTERM');
  server.close();
}
