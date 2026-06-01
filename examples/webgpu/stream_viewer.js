import {
  STREAMS,
  WebGpuDemoSession,
  initWebGPU,
  streamConfigByName,
  streamSourceName,
} from "../../web/drp2/webgpu.js";

const statusEl = document.querySelector("#status");
const canvas = document.querySelector("#viewport");
const streamNameEl = document.querySelector("#stream-name");
const streamSelectEl = document.querySelector("#stream-select");
const playToggleEl = document.querySelector("#play-toggle");
const frameInfoEl = document.querySelector("#frame-info");

function requireElement(element, name) {
  if (element === null) {
    throw new Error(`missing ${name}`);
  }
  return element;
}

function setStatus(message, isError = false) {
  statusEl.textContent = message;
  statusEl.style.color = isError ? "#ff8f8f" : "#9be59b";
}

function applyStreamCanvasAspect(stream) {
  const width = Number(stream.canvas?.width);
  const height = Number(stream.canvas?.height);
  if (Number.isFinite(width) && Number.isFinite(height) && width > 0 && height > 0) {
    canvas.style.setProperty("--stream-aspect", `${width / height}`);
  } else {
    canvas.style.removeProperty("--stream-aspect");
  }
}

async function main() {
  requireElement(statusEl, "#status");
  requireElement(canvas, "#viewport");
  requireElement(streamNameEl, "#stream-name");
  requireElement(streamSelectEl, "#stream-select");
  requireElement(playToggleEl, "#play-toggle");
  requireElement(frameInfoEl, "#frame-info");

  try {
    const { device, context, format, capabilities } = await initWebGPU(canvas);
    const params = new URLSearchParams(window.location.search);
    let streamName = params.get("stream") ?? "indexed_quad_wgsl";
    let stream = null;
    let streamConfig = streamConfigByName(streamName);
    let runtime = null;
    let session = null;
    let frameIndex = 0;
    let playing = false;
    let playbackRequest = 0;
    let playbackStartMs = 0;

    for (const item of STREAMS) {
      const option = document.createElement("option");
      option.value = item.name;
      option.textContent = item.label;
      streamSelectEl.appendChild(option);
    }
    streamName = streamConfig.name;
    streamSelectEl.value = streamName;

    const frameCount = () => session?.frameCount() ?? 0;
    const frameTime = (index) => session?.frameTime(index) ?? 0;

    const frameIndexAtTime = (timeSeconds) => {
      const frames = runtime?.frames ?? [];
      if (frames.length <= 1) {
        return 0;
      }
      const duration = frameTime(frames.length - 1);
      const t = duration > 0 ? timeSeconds % duration : 0;
      let lo = 0;
      let hi = frames.length - 1;
      while (lo < hi) {
        const mid = Math.ceil((lo + hi) / 2);
        if (frameTime(mid) <= t) {
          lo = mid;
        } else {
          hi = mid - 1;
        }
      }
      return lo;
    };

    const updatePlaybackUi = () => {
      const count = frameCount();
      playToggleEl.disabled = count <= 1;
      playToggleEl.textContent = playing ? "Pause" : "Play";
      frameInfoEl.textContent = count > 0 ? `${frameIndex + 1}/${count}` : "0/0";
    };

    const stopPlayback = () => {
      playing = false;
      if (playbackRequest !== 0) {
        cancelAnimationFrame(playbackRequest);
        playbackRequest = 0;
      }
      updatePlaybackUi();
    };

    const loadStream = async (name) => {
      if (session === null) {
        session = new WebGpuDemoSession(device, context, format, capabilities, {
          canvas,
          onStreamLoaded: applyStreamCanvasAspect,
        });
      }
      await session.loadStream(name);
      streamConfig = session.config;
      stream = session.stream;
      runtime = session.runtime;
      streamName = session.streamName;
      const streamPath = `./streams/${streamSourceName(streamConfig)}.json`;
      streamNameEl.textContent = streamPath.slice(2);
      frameIndex = 0;
      stopPlayback();
      updatePlaybackUi();
      const url = new URL(window.location.href);
      url.searchParams.set("stream", streamName);
      window.history.replaceState(null, "", url);
    };

    let rendering = false;
    let rerenderRequested = false;

    const render = async () => {
      if (rendering) {
        rerenderRequested = true;
        return;
      }

      rendering = true;
      try {
        do {
          rerenderRequested = false;
          if (stream === null) {
            await loadStream(streamName);
          }
          const count = frameCount();
          const result = await session.render({
            frameIndex: count > 0 ? frameIndex : null,
            reloadOnResize: true,
          });
          runtime = session.runtime;
          if (result.readbacks.length > 0) {
            const readback = result.readbacks[0];
            setStatus(`Rendered ${stream.name}; readback nonzero=${readback.summary.nonzero}`);
          } else {
            const frameSuffix = count > 0 ? `; frame=${frameIndex + 1}/${count}` : "";
            setStatus(`Rendered ${streamName}; readbacks=0${frameSuffix}`);
          }
          updatePlaybackUi();
        } while (rerenderRequested);
      } finally {
        rendering = false;
      }
    };

    const schedulePlayback = () => {
      playbackRequest = requestAnimationFrame(async () => {
        playbackRequest = 0;
        if (!playing) {
          return;
        }
        const count = frameCount();
        if (count <= 1) {
          stopPlayback();
          return;
        }
        frameIndex = frameIndexAtTime((performance.now() - playbackStartMs) / 1000);
        try {
          await render();
        } catch (error) {
          stopPlayback();
          setStatus(error.message, true);
          return;
        }
        if (playing) {
          schedulePlayback();
        }
      });
    };

    playToggleEl.addEventListener("click", () => {
      if (playing) {
        stopPlayback();
        return;
      }
      if (frameCount() <= 1) {
        return;
      }
      playing = true;
      playbackStartMs = performance.now() - frameTime(frameIndex) * 1000;
      updatePlaybackUi();
      schedulePlayback();
    });

    streamSelectEl.addEventListener("change", () => {
      stopPlayback();
      loadStream(streamSelectEl.value)
        .then(render)
        .catch((error) => setStatus(error.message, true));
    });

    await loadStream(streamName);
    await render();
    new ResizeObserver(() => {
      stopPlayback();
      render().catch((error) => setStatus(error.message, true));
    }).observe(canvas);
  } catch (error) {
    setStatus(error.message, true);
    console.error(error);
  }
}

main();
