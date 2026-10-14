// Progressive enhancement: all four tutorials remain readable without JS.
(function () {
  "use strict";
  var languages = ["en", "pt", "fr", "es"];
  function showLanguage() {
    var language = window.location.hash.slice(1).split("--")[0];
    language = ({portugues:"pt", english:"en", francais:"fr", espanol:"es"})[language] || language;
    if (languages.indexOf(language) === -1) language = "en";
    document.documentElement.lang = language === "pt" ? "pt-BR" : language;
    document.querySelectorAll("[data-tutorial-language]").forEach(function (article) {
      article.hidden = article.getAttribute("data-tutorial-language") !== language;
    });
    document.querySelectorAll("[data-language]").forEach(function (link) {
      if (link.getAttribute("data-language") === language) link.setAttribute("aria-current", "true");
      else link.removeAttribute("aria-current");
    });
    document.querySelectorAll("[data-family-language]").forEach(function (link) {
      link.hidden = link.getAttribute("data-family-language") !== language;
    });
    document.querySelectorAll("[data-navigation-language]").forEach(function (links) {
      links.hidden = links.getAttribute("data-navigation-language") !== language;
      links.setAttribute("data-active", links.hidden ? "false" : "true");
    });
  }
  function resetLanguageStart() {
    var hash = window.location.hash;
    if (languages.indexOf(hash.slice(1)) === -1 || !window.scrollTo) return;
    function reset() {
      // Do not override a newer lesson link or smooth-scroll behind a sticky header.
      if (window.location.hash === hash) window.scrollTo({ top: 0, left: 0, behavior: "instant" });
    }
    if (window.requestAnimationFrame) {
      // Native fragment navigation settles after the load/hashchange handler.
      window.requestAnimationFrame(function () { window.requestAnimationFrame(reset); });
    } else reset();
  }
  window.addEventListener("hashchange", function () {
    showLanguage();
    resetLanguageStart();
  });
  // Language-only URLs should open the tutorial with its family/header visible.
  // Lesson deep links still retain native anchor navigation.
  window.addEventListener("load", resetLanguageStart);
  showLanguage();
}());
