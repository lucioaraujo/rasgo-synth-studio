// Rasgo Synth Studio — site apresentacional
(function () {
  "use strict";

  /* ---------- catálogo de mecanismos ---------- */
  var state = { category: null, query: "" };
  var items = [];
  var grid = document.getElementById("catalog-grid");
  var countLine = document.getElementById("count-line");
  var chipRow = document.getElementById("chip-row");
  var searchInput = document.getElementById("search");
  var catalogTotal = document.getElementById("catalog-total");
  var emptyLine = document.getElementById("catalog-empty");
  var retryButton = document.getElementById("catalog-retry");
  var loading = false;

  function textElement(tag, className, value) {
    var element = document.createElement(tag);
    element.className = className;
    element.textContent = value;
    return element;
  }

  function normalizeSearch(value) {
    return value.normalize("NFD").replace(/[\u0300-\u036f]/g, "").toLowerCase();
  }

  function validateCatalog(data) {
    if (!Array.isArray(data) || data.length > 512) throw new Error("catálogo inválido");
    data.forEach(function (item) {
      if (!item || ["name", "category", "description", "reference"].some(function (key) {
        return typeof item[key] !== "string" || !item[key].trim() || item[key].length > 16384;
      })) throw new Error("entrada inválida no catálogo");
    });
    return data;
  }

  function categoryOrder(items) {
    var counts = Object.create(null);
    items.forEach(function (it) {
      counts[it.category] = (counts[it.category] || 0) + 1;
    });
    return Object.keys(counts).sort(function (a, b) { return counts[b] - counts[a]; }).map(function (c) {
      return { name: c, count: counts[c] };
    });
  }

  function renderChips() {
    if (!chipRow) return;
    var cats = categoryOrder(items);
    chipRow.textContent = "";
    function appendChip(name, category) {
      var button = textElement("button", "chip", name);
      button.setAttribute("type", "button");
      if (category !== null) button.setAttribute("data-cat", category);
      button.setAttribute("aria-pressed", category === state.category ? "true" : "false");
      chipRow.appendChild(button);
    }
    appendChip("Todos (" + items.length + ")", null);
    cats.forEach(function (c) {
      appendChip(c.name + " (" + c.count + ")", c.name);
    });
    chipRow.querySelectorAll(".chip").forEach(function (btn) {
      btn.addEventListener("click", function () {
        state.category = btn.getAttribute("data-cat");
        chipRow.querySelectorAll(".chip").forEach(function (b) {
          b.setAttribute("aria-pressed", b === btn ? "true" : "false");
        });
        applyFilter();
      });
    });
  }

  function renderCards() {
    grid.textContent = "";
    items.forEach(function (it, idx) {
      var card = textElement("article", "mech-card", "");
      card.setAttribute("data-idx", idx);
      card.setAttribute("data-cat", it.category);
      card.appendChild(textElement("div", "cat", it.category));
      card.appendChild(textElement("h3", "", it.name));
      card.appendChild(textElement("p", "desc", it.description));
      card.appendChild(textElement("p", "ref", it.reference));
      grid.appendChild(card);
    });
  }

  function applyFilter() {
    if (!grid || loading) return;
    var q = normalizeSearch(state.query.trim());
    var visible = 0;
    grid.querySelectorAll(".mech-card").forEach(function (card) {
      var idx = parseInt(card.getAttribute("data-idx"), 10);
      var it = items[idx];
      var matchesCat = state.category === null || it.category === state.category;
      var haystack = normalizeSearch(it.name + " " + it.description + " " + it.reference);
      var matchesQuery = q === "" || haystack.indexOf(q) !== -1;
      var show = matchesCat && matchesQuery;
      card.classList.toggle("hidden", !show);
      card.hidden = !show;
      if (show) visible++;
    });
    if (countLine) countLine.textContent = visible + " de " + items.length + " registros no catálogo" +
      (state.category !== null ? " · categoria " + state.category : "") +
      (q ? " · busca “" + state.query + "”" : "");
    if (emptyLine) emptyLine.hidden = visible !== 0;
  }

  function loadCatalog() {
    if (!grid || loading) return;
    loading = true;
    state.category = null;
    items = [];
    grid.setAttribute("aria-busy", "true");
    grid.textContent = "Carregando catálogo…";
    if (chipRow) chipRow.textContent = "";
    if (countLine) countLine.textContent = "Carregando catálogo…";
    if (searchInput) searchInput.disabled = true;
    if (retryButton) { retryButton.hidden = true; retryButton.disabled = true; }
    if (emptyLine) emptyLine.hidden = true;
    var controller = typeof AbortController === "function" ? new AbortController() : null;
    var timer;
    var deadline = new Promise(function (_, reject) {
      timer = setTimeout(function () {
        reject(new Error("tempo limite de carregamento"));
        if (controller) controller.abort();
      }, 10000);
    });
    var request = Promise.resolve().then(function () {
      return fetch("assets/mechanisms.pt.json", controller ? { signal: controller.signal } : {});
    }).then(function (response) {
      if (!response.ok) throw new Error("HTTP " + response.status);
      return response.json();
    });
    Promise.race([request, deadline]).then(function (data) {
      items = validateCatalog(data);
      renderChips();
      renderCards();
      loading = false;
      if (catalogTotal) catalogTotal.textContent = String(items.length);
      if (searchInput) searchInput.disabled = false;
      applyFilter();
    }).catch(function (err) {
      loading = false;
      grid.textContent = "Não foi possível carregar o catálogo (" + err.message + ").";
      if (countLine) countLine.textContent = "Catálogo indisponível; consulte a versão textual abaixo.";
      if (catalogTotal) catalogTotal.textContent = "—";
      if (retryButton) { retryButton.hidden = false; retryButton.disabled = false; }
    }).finally(function () {
      clearTimeout(timer);
      grid.setAttribute("aria-busy", "false");
    });
  }
  if (retryButton) retryButton.addEventListener("click", loadCatalog);
  loadCatalog();

  if (searchInput) {
    searchInput.addEventListener("input", function () {
      state.query = searchInput.value;
      applyFilter();
    });
  }

  /* ---------- documentação por idioma ---------- */
  var languageSelect = document.getElementById("language-select");
  var languageTargets = {
    pt: "../docs/pt/Instrumento_Integral.md",
    en: "../docs/en/Instrument_Overview.md",
    fr: "../docs/fr/Instrument_Integral.md",
    es: "../docs/es/Instrumento_Integral.md"
  };
  if (languageSelect) {
    var savedLanguage = "pt";
    try { savedLanguage = localStorage.getItem("rasgo-site-language") || "pt"; } catch (_) {}
    var knownLanguage = function (language) {
      return Object.prototype.hasOwnProperty.call(languageTargets, language);
    };
    languageSelect.value = knownLanguage(savedLanguage) ? savedLanguage : "pt";
    languageSelect.addEventListener("change", function () {
      var language = languageSelect.value;
      try { localStorage.setItem("rasgo-site-language", language); } catch (_) {}
      if (knownLanguage(language)) window.location.href = languageTargets[language];
    });
  }

  /* ---------- tema claro/escuro ---------- */
  var themeBtn = document.getElementById("theme-toggle");
  function currentTheme() {
    return document.documentElement.getAttribute("data-theme");
  }
  function setTheme(t) {
    if (t) {
      document.documentElement.setAttribute("data-theme", t);
      try { localStorage.setItem("rasgo-theme", t); } catch (_) {}
    } else {
      document.documentElement.removeAttribute("data-theme");
      try { localStorage.removeItem("rasgo-theme"); } catch (_) {}
    }
    updateThemeLabel();
  }
  function updateThemeLabel() {
    if (!themeBtn) return;
    var t = currentTheme();
    themeBtn.textContent = t === "dark" ? "☀ CLARO" : t === "light" ? "◑ SISTEMA" : "☾ ESCURO";
  }
  var saved = null;
  try { saved = localStorage.getItem("rasgo-theme"); } catch (_) {}
  if (saved) document.documentElement.setAttribute("data-theme", saved);
  updateThemeLabel();
  if (themeBtn) {
    themeBtn.addEventListener("click", function () {
      var t = currentTheme();
      if (!t) setTheme("dark");
      else if (t === "dark") setTheme("light");
      else setTheme(null);
    });
  }

  /* ---------- osciloscópio decorativo no hero ---------- */
  var canvas = document.getElementById("scope-canvas");
  if (canvas && canvas.getContext) {
    var ctx = canvas.getContext("2d");
    var reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    var w, h, dpr = Math.min(window.devicePixelRatio || 1, 2);

    function resize() {
      var rect = canvas.getBoundingClientRect();
      w = rect.width; h = rect.height;
      canvas.width = w * dpr; canvas.height = h * dpr;
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }
    resize();
    window.addEventListener("resize", resize);

    // Henon-map-derived trace, a nod to the engine's own nine chaotic
    // maps -- not decorative noise, a real (if simplified) recurrence.
    var x = 0.1, y = 0.1;
    var trail = [];
    var maxTrail = 220;

    function step() {
      var a = 1.4, b = 0.3;
      var xn = 1 - a * x * x + y;
      var yn = b * x;
      x = xn; y = yn;
      trail.push({ x: x, y: y });
      if (trail.length > maxTrail) trail.shift();
    }

    function draw() {
      var accent = getComputedStyle(document.documentElement).getPropertyValue("--copper").trim();
      var dim = getComputedStyle(document.documentElement).getPropertyValue("--brass").trim();
      ctx.clearRect(0, 0, w, h);
      if (trail.length < 2) return;
      ctx.lineWidth = 1.3;
      for (var i = 1; i < trail.length; i++) {
        var p0 = trail[i - 1], p1 = trail[i];
        var alpha = i / trail.length;
        ctx.strokeStyle = i === trail.length - 1 ? accent : dim;
        ctx.globalAlpha = alpha * 0.9 + 0.1;
        ctx.beginPath();
        ctx.moveTo(w / 2 + p0.x * (w * 0.28), h / 2 + p0.y * (h * 0.85));
        ctx.lineTo(w / 2 + p1.x * (w * 0.28), h / 2 + p1.y * (h * 0.85));
        ctx.stroke();
      }
      ctx.globalAlpha = 1;
    }

    if (reduceMotion) {
      for (var i = 0; i < maxTrail; i++) step();
      draw();
    } else {
      (function loop() {
        step();
        draw();
        requestAnimationFrame(loop);
      })();
    }
  }

  /* ---------- faixa de osciloscópio: atrator de Lorenz, X(t) ---------- */
  var lorenzCanvas = document.getElementById("lorenz-strip");
  if (lorenzCanvas && lorenzCanvas.getContext) {
    var lctx = lorenzCanvas.getContext("2d");
    var lReduce = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    var lw, lh, ldpr = Math.min(window.devicePixelRatio || 1, 2);
    function lresize() {
      var r = lorenzCanvas.getBoundingClientRect();
      lw = r.width; lh = r.height;
      lorenzCanvas.width = lw * ldpr; lorenzCanvas.height = lh * ldpr;
      lctx.setTransform(ldpr, 0, 0, ldpr, 0, 0);
    }
    lresize();
    window.addEventListener("resize", lresize);

    // Sistema real de Lorenz (sigma=10, rho=28, beta=8/3), integrado por
    // Euler simples -- o mesmo atrator usado no motor pra modular o
    // corte do filtro; aqui só o componente X vira um traço tipo
    // osciloscópio (amplitude x tempo), não um retrato de fase.
    var lx = 0.6, ly = 0, lz = 0, ldt = 0.006;
    var lhist = [];
    var lmax = 900;
    function lstep() {
      var sigma = 10, rho = 28, beta = 8 / 3;
      var dx = sigma * (ly - lx);
      var dy = lx * (rho - lz) - ly;
      var dz = lx * ly - beta * lz;
      lx += dx * ldt; ly += dy * ldt; lz += dz * ldt;
      lhist.push(lx);
      if (lhist.length > lmax) lhist.shift();
    }
    function ldraw() {
      var accent = getComputedStyle(document.documentElement).getPropertyValue("--amber").trim();
      lctx.clearRect(0, 0, lw, lh);
      if (lhist.length < 2) return;
      lctx.beginPath();
      lctx.strokeStyle = accent;
      lctx.lineWidth = 1.3;
      lctx.globalAlpha = 0.55;
      for (var i = 0; i < lhist.length; i++) {
        var px = (i / lmax) * lw;
        var py = lh / 2 - (lhist[i] / 26) * (lh * 0.44);
        if (i === 0) lctx.moveTo(px, py); else lctx.lineTo(px, py);
      }
      lctx.stroke();
      lctx.globalAlpha = 1;
    }
    for (var lpre = 0; lpre < lmax; lpre++) lstep(); // nasce já preenchida, não crescendo do zero
    ldraw();
    if (!lReduce) {
      (function lloop() {
        for (var s = 0; s < 3; s++) lstep();
        ldraw();
        requestAnimationFrame(lloop);
      })();
    }
  }

})();
