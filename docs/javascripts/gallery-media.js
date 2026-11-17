(() => {
  const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  const saveData = navigator.connection && navigator.connection.saveData;
  const localPreview = ["localhost", "127.0.0.1", "::1"].includes(window.location.hostname);
  const localAssetRoot = new URL("../", document.currentScript.src);
  const localVideoManifest = new URL("../assets/gallery/v0.4/local-video-assets.json", document.currentScript.src).href;
  let localVideoUrls = null;
  const localCacheKey = Date.now().toString(36);
  const mediaUrl = (value) => {
    if (!localPreview) return value;
    const url = new URL(value, window.location.href);
    url.searchParams.set("dvz-local", localCacheKey);
    return url.href;
  };

  if (localPreview) {
    for (const action of document.querySelectorAll(".dvz-local-webgpu-action[hidden]")) {
      action.hidden = false;
    }
    for (const fallback of document.querySelectorAll(".dvz-public-webgpu-fallback")) {
      fallback.hidden = true;
    }
    for (const preview of document.querySelectorAll(".dvz-local-webgpu-tabs[hidden]")) {
      const tabs = preview.nextElementSibling;
      if (!tabs || !tabs.classList.contains("tabbed-set")) continue;
      preview.hidden = false;
      const iframe = tabs.querySelector("iframe[data-src]");
      if (iframe) iframe.src = iframe.dataset.src;
    }
    for (const poster of document.querySelectorAll("img.dvz-gallery-poster[src]")) {
      poster.src = mediaUrl(poster.getAttribute("src"));
    }
    for (const video of document.querySelectorAll("video.dvz-gallery-video[poster]")) {
      video.poster = mediaUrl(video.getAttribute("poster"));
    }
  }
  if (reduceMotion || saveData) return;

  const updateControl = (card, video) => {
    const control = card.querySelector(".dvz-gallery-video-control");
    if (!control) return;
    const paused = video.paused;
    control.dataset.state = paused ? "paused" : "playing";
    control.setAttribute("aria-label", paused ? "Play video preview" : "Pause video preview");
  };

  const addControl = (card, video) => {
    const control = document.createElement("button");
    control.type = "button";
    control.className = "dvz-gallery-video-control";
    control.addEventListener("click", () => {
      if (video.paused) {
        card.dataset.userPaused = "0";
        video.play().catch(() => updateControl(card, video));
      } else {
        card.dataset.userPaused = "1";
        video.pause();
      }
    });
    video.addEventListener("play", () => updateControl(card, video));
    video.addEventListener("pause", () => updateControl(card, video));
    card.appendChild(control);
    updateControl(card, video);
  };

  const ready = (video) => {
    const media = video.closest(".dvz-gallery-media");
    if (media) media.classList.add("is-ready");
  };

  const loadCard = (card) => {
    if (card.dataset.loaded === "1") return;
    card.dataset.loaded = "1";

    const video = card.querySelector("video.dvz-gallery-video");
    if (!video) return;

    for (const source of video.querySelectorAll("source[data-src]")) {
      const available = !localPreview || localVideoUrls?.has(new URL(source.dataset.src, window.location.href).pathname);
      const supported = !source.type || video.canPlayType(source.type) !== "";
      if (available && supported) source.src = mediaUrl(source.dataset.src);
    }
    video.addEventListener("canplay", () => ready(video), { once: true });
    video.load();
  };

  const visibleCards = new Set();

  const setActive = (card, active) => {
    const video = card.querySelector("video.dvz-gallery-video");
    if (!video) return;
    const sources = [...video.querySelectorAll("source[data-src]")];
    if (localPreview && !sources.some(source => localVideoUrls?.has(new URL(source.dataset.src, window.location.href).pathname))) return;
    if (!card.querySelector(".dvz-gallery-video-control")) addControl(card, video);
    if (active && !document.hidden) {
      loadCard(card);
      if (card.dataset.userPaused !== "1") {
        video.play().catch(() => updateControl(card, video));
      }
    } else {
      video.pause();
    }
  };

  const observeCards = () => {
    if (!("IntersectionObserver" in window)) {
      document.querySelectorAll("[data-gallery-lazy]").forEach((card) => {
        visibleCards.add(card);
        setActive(card, true);
      });
      return;
    }

    // Load near the viewport, but animate only while actually visible.
    const preloadObserver = new IntersectionObserver((entries) => {
      for (const entry of entries) {
        if (entry.isIntersecting) {
          const card = entry.target;
          const sources = [...card.querySelectorAll("source[data-src]")];
          if (!localPreview || sources.some(source => localVideoUrls?.has(new URL(source.dataset.src, window.location.href).pathname))) loadCard(card);
        }
      }
    }, { rootMargin: "400px 0px" });
    const playbackObserver = new IntersectionObserver((entries) => {
      for (const entry of entries) {
        if (entry.isIntersecting) visibleCards.add(entry.target);
        else visibleCards.delete(entry.target);
        setActive(entry.target, entry.isIntersecting);
      }
    });
    document.querySelectorAll("[data-gallery-lazy]").forEach((card) => {
      preloadObserver.observe(card);
      playbackObserver.observe(card);
    });
  };

  document.addEventListener("visibilitychange", () => {
    for (const card of visibleCards) setActive(card, true);
  });

  if (localPreview) {
    fetch(mediaUrl(localVideoManifest))
      .then((response) => response.json())
      .then((payload) => {
        localVideoUrls = new Set((payload.available || []).map(value => new URL(value.replace(/^\//, ""), localAssetRoot).pathname));
        observeCards();
      })
      .catch(() => {
        localVideoUrls = new Set();
        observeCards();
      });
  } else {
    observeCards();
  }
})();
