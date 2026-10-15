#!/usr/bin/env python3
"""Bounded public website export, not publication/source or binary release.

Only explicitly selected files. Markdown becomes derived HTML; references to
internal development documents are visibly marked, not replaced by manuals.
Requires Python-Markdown at build time; default --check does not write.
"""
import argparse
import hashlib
import html
import json
import posixpath
import re
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

import markdown
from build_studio_tutorial import ROOT, FAMILY, PAGES, STRIPS, generate, navigation

MAX_FILE = 2 * 1024 * 1024
MAX_TOTAL = 8 * 1024 * 1024
INTERNAL = {"CONTINUIDADE.md", "Plano_de_Upgrade_Instrumento_Generativo.md",
            "TAREFAS_STUDIO.md", "docs/INTERMODULAR_LAB.md", "packaging/PORTABILIDADE.md"}
ASSETS = ("app_screenshot.png", "mechanisms.pt.json", "quickstart.js", "rasgo_logo.svg",
          "site.css", "site.js", "studio.css", "architecture.svg", "genealogy.svg",
          "fonts/BigShouldersDisplay.ttf", "fonts/IBMPlexMono-Regular.ttf",
          "fonts/IBMPlexMono-Bold.ttf", "fonts/LICENSE-BigShoulders.txt",
          "fonts/LICENSE-IBMPlexMono.txt")
OVERVIEWS = {"pt": "Instrumento_Integral.md", "en": "Instrument_Overview.md",
             "fr": "Instrument_Integral.md", "es": "Instrumento_Integral.md"}


def selection():
    paths = [f"website/{name}" for name in (*PAGES.values(), "quickstart.html", "catalogo.html")]
    paths += [f"website/assets/{name}" for name in ASSETS]
    for code, overview in OVERVIEWS.items():
        paths += [f"docs/{code}/{name}" for name in
                  ("manual.md", overview, "Manifesto.md", "Arquitetura_Fluxograma.md", "Documento_de_Referencia.md")]
    return paths + ["docs/pt/_catalogo_mecanismos.md", "docs/GROOVE_RHYTHM.md",
                    "CREDITS.md", "THIRD_PARTY_NOTICES.md", "DEPENDENCIES.md", "LICENSE", "INSTALL.md",
                    "packaging/linux/README.md", "packaging/appimage/RUNTIME_SOURCES.md",
                    "rasgo-synth-app/assets/README.md"]


def destination(source):
    if source.startswith("website/"):
        return source[len("website/"):]
    return source[:-3] + ".html" if source.endswith(".md") else "LICENSE.txt"


def safe_read(root, name):
    path = root / name
    for ancestor in (path, *path.parents):
        if ancestor == root:
            break
        if ancestor.is_symlink():
            raise ValueError("symlink input: " + name)
    if not path.is_file() or path.stat().st_size > MAX_FILE:
        raise ValueError("missing or oversized input: " + name)
    with path.open("rb") as stream:
        raw = stream.read(MAX_FILE + 1)
    if len(raw) > MAX_FILE:
        raise ValueError("input grew past file budget: " + name)
    return raw


class Links(HTMLParser):
    def __init__(self, text):
        super().__init__()
        self.refs, self.ids = [], set()
        self.feed(text)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs:
            self.ids.add(attrs["id"])
        for key in ("href", "src"):
            if key in attrs:
                self.refs.append((tag, key, attrs[key]))


def validate(files):
    pages = {name: Links(raw.decode()) for name, raw in files.items() if name.endswith(".html")}
    for name, page in pages.items():
        for tag, key, raw in page.refs:
            url = urlsplit(raw)
            if url.scheme or url.netloc:
                if tag == "a" and url.scheme in ("http", "https", "mailto"):
                    continue
                if tag == "link" and url.scheme == "data" and raw.startswith("data:image/svg+xml,"):
                    continue
                raise ValueError("remote/unsafe runtime reference: " + raw)
            target = posixpath.normpath(posixpath.join(posixpath.dirname(name), unquote(url.path))) if url.path else name
            if target not in files:
                raise ValueError(f"export link missing: {name} -> {target}")
            if url.fragment and target in pages and unquote(url.fragment) not in pages[target].ids:
                raise ValueError(f"export anchor missing: {name} -> {raw}")
    css = files["assets/site.css"].decode()
    for target in re.findall(r'url\("([^"]+)"\)', css):
        if posixpath.normpath("assets/" + target) not in files:
            raise ValueError("CSS asset missing: " + target)
    for code, overview in OVERVIEWS.items():
        if f"docs/{code}/{overview[:-3]}.html" not in files:
            raise ValueError("language guide missing")


def prepare(root=ROOT):
    mapping = {name: destination(name) for name in selection()}
    inputs, total = {}, 0
    for name in mapping:
        raw = safe_read(root, name)
        total += len(raw)
        if total > MAX_TOTAL:
            raise ValueError("input exceeds total budget")
        inputs[name] = raw
    if inputs["website/quickstart.html"].decode() != generate(root):
        raise ValueError("tutorial is stale; generate it before export")
    marked, files, provenance = [], {}, []
    for source, raw in inputs.items():
        target = mapping[source]
        text = None
        if source.endswith(".md"):
            code = source.split("/")[1] if source.startswith("docs/") and source.split("/")[1] in PAGES else "pt"
            body = markdown.markdown(raw.decode(), extensions=["tables", "fenced_code", "toc"])
            stylesheet = posixpath.relpath("assets/site.css", posixpath.dirname(target) or ".")
            style = posixpath.relpath("assets/studio.css", posixpath.dirname(target) or ".")
            prefix = posixpath.relpath(".", posixpath.dirname(target) or ".")
            prefix = "" if prefix == "." else prefix + "/"
            context = "installation" if source == "INSTALL.md" else "manual"
            script = (f'<script src="{prefix}assets/quickstart.js" defer></script>'
                      if context == "installation" else "")
            text = ('<!doctype html><html lang="' + ("pt-BR" if code == "pt" else code)
                    + '"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">'
                    + '<title>' + html.escape(raw.decode().splitlines()[0].lstrip("# ")) + '</title>'
                    + f'<link rel="stylesheet" href="{stylesheet}"><link rel="stylesheet" href="{style}">'
                    + script + '</head><body class="studio-public"><div class="rasgo-strip">'
                    + f'<a href="{FAMILY}{"" if code == "pt" else PAGES[code]}">{STRIPS[code]}</a></div>'
                    + navigation(code, prefix, public=True, context=context)
                    + '<main class="quickstart">'
                    + body + '</main></body></html>')
        elif source.endswith(".html"):
            text = raw.decode()
        if text is not None:
            def rewrite(match):
                key, quote, value = match.group(1, 2, 3)
                url = urlsplit(html.unescape(value))
                if url.scheme or url.netloc or not url.path:
                    return match[0]
                original = posixpath.normpath(posixpath.join(posixpath.dirname(source), unquote(url.path)))
                if original in INTERNAL:
                    if key != "href":
                        raise ValueError("internal runtime asset: " + original)
                    marked.append({"source": source, "reference": original})
                    output = "publication-notes.html"
                    fragment = "#internal-reference"
                    annotation = ' data-internal-reference="true" title="Internal reference — not distributed"'
                elif original in mapping:
                    output = mapping[original]
                    fragment = "#" + url.fragment if url.fragment else ""
                    annotation = ""
                else:
                    raise ValueError(f"unreviewed local reference: {source} -> {original}")
                relative = posixpath.relpath(output, posixpath.dirname(target) or ".")
                return key + "=" + quote + html.escape(relative + fragment, quote=True) + quote + annotation
            # Rendered Markdown paths are still relative to their canonical source.
            # Generated navigation/styles above already use public paths.
            if source.endswith(".md"):
                public_nav, content = text.split("<main class=\"quickstart\">", 1)
                text = public_nav + '<main class="quickstart">' + re.sub(
                    r'(href|src)=(["\x27])(.*?)\2', rewrite, content)
            else:
                text = re.sub(r'(href|src)=(["\x27])(.*?)\2', rewrite, text)
            raw_out = text.encode()
        elif source == "website/assets/site.js":
            js = raw.decode()
            for name, output in mapping.items():
                if name.endswith(".md"):
                    js = js.replace("../" + name, output)
            raw_out = js.encode()
        else:
            raw_out = raw
        files[target] = raw_out
        provenance.append({"source": source, "output": target,
                           "source_sha256": hashlib.sha256(raw).hexdigest(),
                           "output_sha256": hashlib.sha256(raw_out).hexdigest(),
                           "transformed": raw != raw_out})
    files["publication-notes.html"] = (
        '<!doctype html><html lang="pt-BR"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">'
        '<title>Studio — notas editoriais</title><link rel="stylesheet" href="assets/site.css"><link rel="stylesheet" href="assets/studio.css">'
        '</head><body class="studio-public">' + navigation("pt", public=True)
        + '<main class="quickstart"><h1 id="internal-reference">PT / EN / FR / ES</h1>'
        '<p lang="pt-BR">Manuais são derivados HTML de fontes Markdown. '
        'Links marcados como referência interna apontam a esta nota: diários de trabalho não são distribuídos. '
        'Os nomes/citações permanecem por transparência; esta nota não substitui seu conteúdo.</p>'
        '<p lang="en">Manuals are HTML derivatives of Markdown sources. '
        'Links marked as internal references lead here because working logs are not distributed. This notice does not replace those documents.</p>'
        '<p lang="fr">Manuels dérivés HTML de sources Markdown. '
        'Les références internes signalées mènent ici ; les journaux de travail ne sont pas distribués. Cette note ne les remplace pas.</p>'
        '<p lang="es">Manuales derivados HTML de fuentes Markdown. '
        'Las referencias internas señaladas llevan aquí; no se distribuyen diarios de trabajo. Esta nota no los sustituye.</p>'
        '<p>Rasgo Synth Studio · 0.1.0 experimental</p>'
        '<p><a href="index.html">PT</a> · <a href="en.html">EN</a> · <a href="fr.html">FR</a> · <a href="es.html">ES</a></p>'
        '<p><a href="CREDITS.html">Créditos</a> · <a href="THIRD_PARTY_NOTICES.html">Notices</a> · <a href="LICENSE.txt">AGPL-3.0-or-later</a></p>'
        '</main></body></html>\n').encode()
    validate(files)
    if len(files) > 64 or sum(map(len, files.values())) > MAX_TOTAL:
        raise ValueError("output exceeds budget")
    manifest = {"kind": "reviewed website export, not binary release approval", "date": "2026-10-16",
                "builder": "tools/build_studio_website.py", "markdown_version": markdown.__version__,
                "files": provenance, "marked_internal_references": marked,
                "source_selection": "explicit allowlist; no audio, logs, other instruments or app code"}
    files["public-manifest.json"] = (json.dumps(manifest, ensure_ascii=False, indent=2) + "\n").encode()
    files["SHA256SUMS"] = "".join(f"{hashlib.sha256(raw).hexdigest()}  {name}\n"
                                for name, raw in sorted(files.items())).encode()
    return files, manifest, inputs


def export(output, root=ROOT):
    output = Path(output).absolute()
    allowed = (root / ".local-build").resolve()
    if not output.resolve().is_relative_to(allowed) or output.exists():
        raise ValueError("output must be fresh and inside Studio/.local-build")
    for ancestor in (output, *output.parents):
        if ancestor.is_symlink():
            raise ValueError("symlink output path")
        if ancestor == root:
            break
    files, manifest, inputs = prepare(root)
    for name, raw in inputs.items():
        if safe_read(root, name) != raw:
            raise ValueError("source changed during export: " + name)
    if not output.parent.is_dir():
        raise ValueError("output parent must exist")
    output.mkdir()
    for name, raw in files.items():
        path = output / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("xb") as stream:
            stream.write(raw)
    return files, manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--check", action="store_true")
    modes.add_argument("--output", type=Path)
    args = parser.parse_args()
    files, manifest = export(args.output) if args.output else prepare()[:2]
    print(json.dumps({"files": len(files), "bytes": sum(map(len, files.values())),
                      "marked_references": len(manifest["marked_internal_references"]),
                      "output": str(args.output) if args.output else None}))


if __name__ == "__main__":
    main()
