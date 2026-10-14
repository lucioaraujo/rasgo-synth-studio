# Rasgo Synth Studio

Instrumento generativo da família RASGO, desenvolvido em C++/JUCE por Lucio
de Araujo. Compõe peças e álbuns a partir de seeds, escolhas de módulos e
direção musical. Explora relações entre ritmo, timbre, forma e acaso.

Generative composition instrument / instrument de composition générative /
instrumento de composición generativa. Interface, tutorial e manual:
Português, English, Français, Español.

- [Apresentação e tutorial](site/index.html)
- [Manual PT](docs/pt/manual.md) · [EN](docs/en/manual.md) ·
  [FR](docs/fr/manual.md) · [ES](docs/es/manual.md)
- [Instalação e requisitos](INSTALL.md)
- [Compilar / dependency lock](DEPENDENCIES.md)
- [Créditos](CREDITS.md) e [avisos de terceiros](THIRD_PARTY_NOTICES.md)

## Estado

Versão0.1.0 experimental. O candidato Linux x86_64 foi compilado em Ubuntu22.04,
testado nessa base isolada e no Mint22, com exportação WAV e fechamento.
Os workflows manuais preparam Windows x64 e macOS Intel/Apple Silicon.
Ter um workflow não significa que os pacotes ou seu uso musical já estejam
validados; consulte o resultado da execução e as notas de cada release.

Não há lançamento automático por push. Mistura, diversidade tímbrica e
expressividade continuam em desenvolvimento; esta versão não é apresentada
como produto estável. As composições geradas e suas preferências ficam fora
da pasta instalada.

## Código livre

Código autoral: [AGPL-3.0-or-later](LICENSE). JUCE é utilizado pela opção
AGPLv3; fontes tipográficas conservam a SIL OFL1.1 e demais componentes seus
próprios avisos. Este repositório contém somente o Studio, não o Performance,
os outros instrumentos RASGO ou o arquivo privado de pesquisa.

SOURCE_MANIFEST.json registra a seleção desta exportação. O histórico Git
privado não foi republicado; os créditos e avisos dos arquivos foram mantidos.
