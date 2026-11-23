(() => {
  const KEY = "datoviz.docs.language.v1";
  const LANGUAGES = ["Python", "C"];
  const CODE_LANGUAGES = {
    python: "Python", py: "Python", python3: "Python", pycon: "Python",
    c: "C", cpp: "C", "c++": "C", h: "C", hpp: "C",
  };
  let selected = "Python";
  const controllers = new WeakMap();

  const read = () => {
    try {
      const saved = localStorage.getItem(KEY);
      if (LANGUAGES.includes(saved)) selected = saved;
    } catch (_) {
      // Native tabs and in-page synchronization still work without browser storage.
    }
  };

  const init = () => {
    const article = document.querySelector("article");
    if (!article) return;
    const existing = controllers.get(article);
    if (existing) {
      read();
      existing(selected);
      return;
    }
    const sets = [...article.querySelectorAll(".tabbed-set")].map((set) => {
      const labels = [...set.querySelectorAll(":scope > .tabbed-labels > label")];
      if (!LANGUAGES.every((name) =>
        labels.filter((label) => label.textContent.trim() === name).length === 1)) return null;
      const branches = [...set.querySelectorAll(":scope > .tabbed-content > .tabbed-block")];
      return labels.map((label, index) => {
        const name = label.textContent.trim();
        const families = new Set();
        for (const block of branches[index]?.querySelectorAll("[class*='language-']") || []) {
          for (const token of block.classList) {
            const family = CODE_LANGUAGES[token.replace(/^language-/, "")];
            if (token.startsWith("language-") && family) families.add(family);
          }
        }
        return {
          name,
          language: LANGUAGES.includes(name) ? name :
            families.size === 1 ? [...families][0] : null,
          input: document.getElementById(label.htmlFor),
        };
      });
    }).filter((set) => set && set.every((tab) => tab.input));
    if (!sets.length) return;
    article.dataset.dvzLanguagesReady = "true";
    let syncing = false;
    let activating = null;
    const tabs = new Map(sets.flat().map((tab) => [tab.input, tab]));
    read();

    const sync = (language, persist = false, origin = null, scope = null) => {
      if (syncing) return;
      syncing = true;
      selected = language;
      for (const set of sets) {
        if (scope && !scope.contains(set[0].input)) continue;
        // Keep an explicitly opened helper source visible while updating the other choices.
        if (origin && set.some((tab) => tab.input === origin)) continue;
        const tab = set.find((item) => item.name === language);
        if (!tab.input.checked) {
          tab.input.checked = true;
          tab.input.dispatchEvent(new Event("change", { bubbles: true }));
        }
      }
      syncing = false;
      if (persist) {
        try {
          localStorage.setItem(KEY, language);
        } catch (_) {
          // The current page keeps its selection even when storage is unavailable.
        }
      }
    };

    controllers.set(article, sync);
    article.addEventListener("click", (event) => {
      if (event.button !== 0 || event.altKey || event.ctrlKey || event.metaKey || event.shiftKey) return;
      const direct = tabs.get(event.target);
      if (direct) {
        // Native radio clicks include keyboard arrows/Space. Scripted clicks are not user intent.
        if (event.isTrusted && direct.language) sync(direct.language, true, direct.input);
        return;
      }
      const label = event.target instanceof Element && event.target.closest(".tabbed-labels > label");
      const tab = label && tabs.get(document.getElementById(label.htmlFor));
      if (!tab || !tab.language || tab.input.disabled) return;
      // Material wraps labels in anchors. Own their activation even if another component failed.
      event.preventDefault();
      event.stopPropagation();
      if (!event.isTrusted) {
        read();
        sync(selected);
        return;
      }
      tab.input.focus({ preventScroll: true });
      if (tab.input.checked) {
        sync(tab.language, true, tab.input);
      } else {
        activating = tab.input;
        try {
          tab.input.click();
        } finally {
          activating = null;
        }
      }
    }, true);
    article.addEventListener("change", (event) => {
      if (syncing) return;
      const tab = tabs.get(event.target);
      if (!tab || !tab.input.checked || !tab.language) return;
      if (activating === tab.input) {
        sync(tab.language, true, tab.input);
      } else {
        // History and Material can emit trusted changes after a scripted click; do not persist them.
        read();
        if (tab.language !== selected) sync(selected);
      }
    }, true);
    article.addEventListener("toggle", (event) => {
      if (event.target instanceof HTMLDetailsElement && event.target.open) {
        sync(selected, false, null, event.target);
      }
    }, true);
    sync(selected);
  };

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init);
  else init();
  if (typeof document$ !== "undefined") document$.subscribe(init);
  window.addEventListener("pageshow", init);
  window.addEventListener("storage", (event) => {
    if (event.key !== KEY || !LANGUAGES.includes(event.newValue)) return;
    selected = event.newValue;
    const sync = controllers.get(document.querySelector("article"));
    if (sync) sync(selected);
    else init();
  });
})();
