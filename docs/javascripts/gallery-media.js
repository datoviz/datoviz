(() => {
  const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  const saveData = navigator.connection && navigator.connection.saveData;
  if (reduceMotion || saveData) return;

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
    video.play().catch(() => {});
  };

  if (!("IntersectionObserver" in window)) {
    document.querySelectorAll("[data-gallery-lazy]").forEach(loadCard);
    return;
  }

  const observer = new IntersectionObserver((entries) => {
    for (const entry of entries) {
      if (!entry.isIntersecting) continue;
      loadCard(entry.target);
      observer.unobserve(entry.target);
    }
  }, { rootMargin: "400px 0px" });

  document.querySelectorAll("[data-gallery-lazy]").forEach((card) => observer.observe(card));
})();
