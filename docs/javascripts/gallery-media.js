(() => {
  const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  const saveData = navigator.connection && navigator.connection.saveData;
  if (reduceMotion || saveData) return;

  const updateControl = (card, video) => {
    const control = card.querySelector(".dvz-gallery-video-control");
    if (!control) return;
    const paused = video.paused;
    control.textContent = paused ? "Play preview" : "Pause preview";
    control.setAttribute("aria-label", paused ? "Play video preview" : "Pause video preview");
    control.setAttribute("aria-pressed", paused ? "false" : "true");
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
      source.src = source.dataset.src;
    }
    video.addEventListener("canplay", () => ready(video), { once: true });
    video.load();
  };

  const setActive = (card, active) => {
    const video = card.querySelector("video.dvz-gallery-video");
    if (!video) return;
    if (!card.querySelector(".dvz-gallery-video-control")) addControl(card, video);
    if (active) {
      loadCard(card);
      if (card.dataset.userPaused !== "1") {
        video.play().catch(() => updateControl(card, video));
      }
    } else {
      video.pause();
    }
  };

  if (!("IntersectionObserver" in window)) {
    document.querySelectorAll("[data-gallery-lazy]").forEach((card) => setActive(card, true));
    return;
  }

  const observer = new IntersectionObserver((entries) => {
    for (const entry of entries) {
      setActive(entry.target, entry.isIntersecting);
    }
  }, { rootMargin: "400px 0px" });

  document.querySelectorAll("[data-gallery-lazy]").forEach((card) => observer.observe(card));
})();
