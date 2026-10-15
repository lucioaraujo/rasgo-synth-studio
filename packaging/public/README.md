# Rasgo Synth Studio

[English](#english) · [Português](#português) · [Français](#français) · [Español](#español)

## English

A generative musical instrument by Lucio de Araujo, part of the RASGO family.
Developed in C++/JUCE, Studio composes pieces and albums from seeds, module
choices and musical direction, exploring rhythm, timbre, form and chance.

- [Presentation](site/en.html) · [Tutorial](site/quickstart.html#en)
- [Manual](docs/en/manual.md) · [Installation and requirements](INSTALL.md#english)
- [Build and dependencies](DEPENDENCIES.md) · [Credits](CREDITS.md)

Version 0.1.0 is an [experimental release](https://github.com/lucioaraujo/rasgo-synth-studio/releases/tag/v0.1.0)
with Linux AppImage/.deb/.tar.gz, Windows installer/ZIP and one Universal 2
Mac DMG. Linux startup/layout/close checks passed in virtual Debian12,
Ubuntu24.04, Fedora and Arch, alongside local Ubuntu22.04/Mint22 checks.
Windows and the same final Mac DMG passed automated render, Unicode paths and
normal close tests. AppImage includes runtime notices and a separate full
runtime source kit; FUSE mounting remains untested. These tests do not establish
manual installer or hardware audio acceptance. Website files above are source
previews, not a deployed website. Builds/publication are manual.

Mix balance, timbral variety and expressivity are still being developed.
Start at a low volume. Recordings/preferences stay outside the application.
Authorial code: AGPL-3.0-or-later; JUCE uses its AGPLv3 option, fonts SIL OFL1.1.
Other components retain their own notices. This repository contains Studio
only, no sibling instruments, generated audio or private research history.

## Português

Instrumento generativo da família RASGO, desenvolvido em C++/JUCE por Lucio
de Araujo. Compõe peças e álbuns a partir de seeds, escolhas de módulos e
direção musical. Explora relações entre ritmo, timbre, forma e acaso.

Generative composition instrument / instrument de composition générative /
instrumento de composición generativa. Interface, tutorial e manual:
Português, English, Français, Español.

- [Apresentação e tutorial](site/index.html)
- [Manual PT](docs/pt/manual.md) · [EN](docs/en/manual.md) ·
  [FR](docs/fr/manual.md) · [ES](docs/es/manual.md)
- [Instalação e requisitos](INSTALL.md#portugues)
- [Compilar / dependency lock](DEPENDENCIES.md)
- [Créditos](CREDITS.md) e [avisos de terceiros](THIRD_PARTY_NOTICES.md)

### Estado

Versão0.1.0 experimental. O candidato Linux x86_64 foi compilado em Ubuntu22.04,
testado nessa base isolada e no Mint22, com exportação WAV e fechamento.
Os [pacotes 0.1.0 estão públicos](https://github.com/lucioaraujo/rasgo-synth-studio/releases/tag/v0.1.0):
Linux AppImage/.deb/.tar.gz, Windows instalador/ZIP e Mac DMG Universal 2.
Windows e o mesmo DMG Mac passaram render Unicode e fechamento automatizados.
AppImage inclui avisos e kit completo de fontes do runtime. Instalação
manual/escuta física Windows/Mac e montagem FUSE continuam sem teste.
Os links do site acima apontam às fontes, não a um website já implantado.

Não há lançamento automático por push. Mistura, diversidade tímbrica e
expressividade continuam em desenvolvimento; esta versão não é apresentada
como produto estável. As composições geradas e suas preferências ficam fora
da pasta instalada.

### Código livre

Código autoral: [AGPL-3.0-or-later](LICENSE). JUCE é utilizado pela opção
AGPLv3; fontes tipográficas conservam a SIL OFL1.1 e demais componentes seus
próprios avisos. Este repositório contém somente o Studio, não o Performance,
os outros instrumentos RASGO ou o arquivo privado de pesquisa.

SOURCE_MANIFEST.json registra a seleção desta exportação. O histórico Git
privado não foi republicado; os créditos e avisos dos arquivos foram mantidos.

## Français

Instrument musical génératif de Lucio de Araujo, de la famille RASGO.
Développé en C++/JUCE, Studio compose pièces et albums à partir de seeds,
de choix de modules et d'une direction musicale : rythme, timbre, forme et hasard.

- [Présentation](site/fr.html) · [Tutoriel](site/quickstart.html#fr)
- [Manuel](docs/fr/manual.md) · [Installation et configuration](INSTALL.md#francais)
- [Compilation et dépendances](DEPENDENCIES.md) · [Crédits](CREDITS.md)

Version 0.1.0 [expérimentale disponible](https://github.com/lucioaraujo/rasgo-synth-studio/releases/tag/v0.1.0) :
Linux AppImage/.deb/.tar.gz, Windows installateur/ZIP et Mac DMG Universal 2.
Tests Linux de démarrage/disposition/fermeture sous Debian12,
Ubuntu24.04, Fedora et Arch virtuels, et tests locaux Ubuntu22.04/Mint22.
Windows et le même DMG Mac ont passé rendu Unicode et fermeture automatisés.
AppImage contient les notices et un kit complet des sources du runtime.
Installation manuelle/écoute Windows/Mac et montage FUSE non testés.
Les liens web montrent les
sources, pas un site déployé. Builds/publication manuels.

Mixage, variété des timbres et expressivité restent en développement.
Commencer à faible volume. Enregistrements/préférences restent hors de
l'application. Code original AGPL-3.0-or-later, JUCE via AGPLv3 et polices
SIL OFL1.1 ; autres composants sous leurs propres licences. Studio seul,
sans instruments frères, audio généré ou historique de recherche privé.

## Español

Instrumento musical generativo de Lucio de Araujo, de la familia RASGO.
Desarrollado en C++/JUCE, Studio compone piezas y álbumes a partir de seeds,
elecciones de módulos y dirección musical: ritmo, timbre, forma y azar.

- [Presentación](site/es.html) · [Tutorial](site/quickstart.html#es)
- [Manual](docs/es/manual.md) · [Instalación y requisitos](INSTALL.md#espanol)
- [Compilar y dependencias](DEPENDENCIES.md) · [Créditos](CREDITS.md)

Versión 0.1.0 [experimental disponible](https://github.com/lucioaraujo/rasgo-synth-studio/releases/tag/v0.1.0):
Linux AppImage/.deb/.tar.gz, Windows instalador/ZIP y Mac DMG Universal 2.
Pruebas Linux de inicio/disposición/cierre en Debian12,
Ubuntu24.04, Fedora y Arch virtuales, además de Ubuntu22.04/Mint22 locales.
Windows y el mismo DMG Mac pasaron render Unicode y cierre automatizados.
AppImage incluye avisos y un kit completo de fuentes del runtime.
Instalación manual/escucha Windows/Mac y montaje FUSE todavía sin probar.
Los enlaces web muestran fuentes, no un sitio desplegado. Builds/publicación
manuales.

Mezcla, variedad tímbrica y expresividad siguen en desarrollo. Empezar con
volumen bajo. Grabaciones/preferencias quedan fuera de la aplicación.
Código propio AGPL-3.0-or-later, JUCE mediante AGPLv3 y fuentes SIL OFL1.1;
otros componentes mantienen sus licencias. Solo Studio: sin instrumentos
hermanos, audio generado ni historial privado de investigación.
