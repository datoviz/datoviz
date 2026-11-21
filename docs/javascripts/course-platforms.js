(() => {
  const PLATFORMS = ["Linux", "macOS", "Windows"];

  const labelsOf = (set) => [...set.querySelectorAll(":scope > .tabbed-labels > label")];
  const inputsOf = (set) => [...set.querySelectorAll(":scope > input")];
  const isPlatformSet = (set) => {
    const names = labelsOf(set).map((label) => label.textContent.trim());
    return names.length === PLATFORMS.length && names.every((name, i) => name === PLATFORMS[i]);
  };

  const init = () => {
    // Opt-in: only pages that mark their primary platform choice.
    const primary = document.querySelector("#course-platform .tabbed-set");
    if (!primary || !isPlatformSet(primary)) return;
    const article = primary.closest("article");
    if (!article) return;

    const currentPlatform = () => {
      const checked = inputsOf(primary).find((input) => input.checked);
      if (!checked) return null;
      const label = labelsOf(primary).find((item) => item.htmlFor === checked.id);
      return label ? label.textContent.trim() : null;
    };

    // Material's linked tabs match tabs by their rendered label text, which is empty inside a closed
    // <details>, so platform tabs in collapsed boxes miss the sync. Once a box opens and its labels
    // are visible, click the matching native tab and let Material sync and remember it as usual.
    article.addEventListener("toggle", (event) => {
      const details = event.target;
      if (!(details instanceof HTMLDetailsElement) || !details.open) return;
      const platform = currentPlatform();
      if (!platform) return;
      for (const set of details.querySelectorAll(".tabbed-set")) {
        if (!isPlatformSet(set) || set.closest("details:not([open])") || !set.getClientRects().length) continue;
        const label = labelsOf(set).find((item) => item.textContent.trim() === platform);
        const input = label && inputsOf(set).find((item) => item.id === label.htmlFor);
        if (input && !input.checked) input.click();
      }
    }, true);
  };

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
