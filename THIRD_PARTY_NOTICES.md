# Third-Party Notices

Revisão: 14 de outubro de 2026.

Este arquivo identifica software de terceiros usado para construir ou executar
o Rasgo Synth. Ele complementa, mas não substitui, os textos integrais das
licenças aplicáveis. Créditos históricos e influências conceituais estão em
[`CREDITS.md`](CREDITS.md).

## 1. Código próprio do Rasgo Synth

O código específico do Rasgo Synth é copyright © 2026 Lucio de Araujo e é
distribuído sob a GNU Affero General Public License, versão 3 ou posterior
(SPDX: AGPL-3.0-or-later), conforme
[`LICENSE`](LICENSE).

Alinhamento autorizado pelo titular em12/10/2026, somente nas fontes atuais
do Studio (core, host e ferramentas próprias). Cópias de versões anteriores
e direitos concedidos sob GPLv3 não são revogados. Performance e código
preservado/copiado em outros instrumentos mantêm seus avisos de origem.
Históricos de trabalho conservam a licença vigente na data registrada.

O motor em `rasgo-synth-core/` usa apenas a biblioteca padrão de C++ e APIs
básicas do sistema operacional. A auditoria do código atual não encontrou
firmware, código-fonte, amostras de áudio, imagens ou outros ativos copiados de
Kosmische, SuperCollider, Mutable Instruments ou dos circuitos e instrumentos
citados em `CREDITS.md`.

Equações matemáticas, comportamentos musicais, topologias descritas
publicamente e referências conceituais não são aqui classificados como
componentes de software incorporados. As implementações correspondentes foram
escritas para o Rasgo Synth.

## 2. JUCE — aplicativo gráfico

O aplicativo em `rasgo-synth-app/` é construído com o
**[JUCE](https://github.com/juce-framework/JUCE)**. Os arquivos locais
examinados declaram **JUCE 9.0.0**; `JUCE-master/` é ignorado pelo Git e não faz
parte do código publicado neste repositório. A referência reproduzível adotada
para novos checkouts é a tag `9.0.0`, commit
`f8f8864172464b9adf9eba6101e1f784838d1597`, registrada em
[`DEPENDENCIES.md`](DEPENDENCIES.md).

O script `rasgo-synth-app/build_juce_minimal.sh` compila os seguintes módulos:

- `juce_core`;
- `juce_events`;
- `juce_data_structures`;
- `juce_graphics`;
- `juce_gui_basics`;
- `juce_audio_basics`;
- `juce_audio_devices`.

Os módulos JUCE 9 são oferecidos sob licença dupla:

- **GNU Affero General Public License v3 (AGPLv3)**; ou
- uma **licença comercial do JUCE**.

Para a distribuição livre do Studio, a opção escolhida pelo autor é AGPLv3,
não a opção comercial. Consulte o [aviso oficial do
JUCE](https://github.com/juce-framework/JUCE/blob/master/LICENSE.md). Quando o JUCE é utilizado sob
a opção AGPLv3, a distribuição e o uso do aplicativo combinado precisam
observar as obrigações aplicáveis da AGPLv3. Uma licença comercial do JUCE segue
os termos comerciais correspondentes. A licença do motor Rasgo Synth, por si
só, não altera a licença do JUCE.

O nome local `JUCE-master` não fixa uma revisão por si só. Builds de release
devem partir do commit registrado em `DEPENDENCIES.md` e conferir novamente o
arquivo `LICENSE.md` daquela revisão.

## 3. Componentes compilados através do JUCE

O procedimento de build atual também compila componentes mantidos dentro da
árvore do JUCE. As cópias dos textos integrais abaixo se encontram no checkout
do JUCE indicado em cada linha e devem acompanhar qualquer pacote binário na
forma exigida pelas respectivas licenças.

| Componente | Titular/autoria indicada no código examinado | Licença | Texto no checkout do JUCE |
| --- | --- | --- | --- |
| [HarfBuzz](https://harfbuzz.github.io/) | Vários contribuidores e organizações; relação completa no arquivo de licença | Old MIT | `modules/juce_graphics/fonts/harfbuzz/COPYING` |
| [SheenBidi](https://github.com/Tehreer/SheenBidi) 2.9.0 | Copyright © 2014–2025 Muhammad Tayyab Akram | Apache License 2.0 | `modules/juce_graphics/unicode/sheenbidi/LICENSE` |
| [LunaSVG](https://github.com/sammycage/lunasvg) | Copyright © 2020–2025 Samuel Ugochukwu | MIT | `modules/juce_graphics/drawables/lunasvg/LICENSE` |
| [PlutoVG](https://github.com/sammycage/plutovg) | Copyright © 2020–2025 Samuel Ugochukwu | MIT | `modules/juce_graphics/drawables/lunasvg/plutovg/LICENSE` |
| zlib incorporada pela receita CMake | Copyright © 1995–2026 Jean-loup Gailly e Mark Adler, na cópia fixada | zlib | `modules/juce_core/zip/zlib/LICENSE` |

Esses componentes não são contribuições de Lucio de Araujo e não são
relicenciados pela licença do Rasgo Synth. Devem ser preservados seus avisos de
copyright, permissões, condições e exclusões de garantia.

O próprio arquivo de licença da revisão escolhida do JUCE é a fonte de verdade
para outros componentes que possam passar a ser compilados por uma atualização
do framework ou do script de build.

## 4. Bibliotecas fornecidas pelo sistema

Preparação Linux,12/10: a próxima receita compila os codecs JPEG/PNG que
acompanham o JUCE fixado, em vez de vincular explicitamente jpeg8 do host.
Avisos integrais: `modules/juce_graphics/image_formats/jpglib/README`
(Independent JPEG Group) e `modules/juce_graphics/image_formats/pnglib/LICENSE`
(libpng). Eles devem acompanhar a próxima build. O candidato AGPL atual foi
compilado antes desta mudança e ainda usa os codecs do sistema; não confundir
os dois. Nenhum DSP/módulo musical foi alterado por essa preparação.

Seguimento13–14/10: a build Ubuntu22.04 e a build CMake do Mint já incorporam
os codecs JPEG/PNG fixados, com seus avisos. O trecho anterior descreve a
preparação de12/10, não todas as builds atuais. A receita CMake também compila
a zlib da revisão JUCE; seu `LICENSE` integral entra na árvore nativa e nos
novos pacotes. O empacotador exige esse aviso quando não há dependência direta
libz. A receita direta anterior pode usar a zlib do sistema; não relabelar
candidatos antigos. Esses avisos selecionados não constituem uma auditoria
completa de todo o binário ou fechamento das fontes correspondentes.

Os candidatos AppImage usam linuxdeploy com revisão/hash fixados e runtime
AppImage `8f39b89e2ac31e1640b3d3f7e9a5108e6ce805fa`, cujo aviso MIT e relação
de dependências embarcadas acompanham o pacote. As bibliotecas copiadas do
host recebem inventário de hashes, pacote/versão e textos de copyright da
distribuição. Isso não substitui a revisão de fontes correspondentes nem
certifica todas as obrigações de redistribuição; essas verificações continuam
portões de publicação. Pins e referências em `packaging/linux/README.md`.

No GNU/Linux, o script de build referencia bibliotecas fornecidas pelo sistema,
incluindo:

- FreeType e Fontconfig;
- X11, Xext e Xrender;
- libpng, libjpeg e zlib;
- ALSA (`libasound`);
- bibliotecas de execução do compilador e do sistema, como `libstdc++`,
  `libgcc`, `libm`, `libdl`, `pthread` e a biblioteca C.

Essas bibliotecas não são armazenadas neste repositório. Elas são resolvidas no
sistema de build e de execução, e mantêm seus próprios titulares e licenças. A
lista efetiva pode variar por plataforma, distribuição, configuração do linker
e versão do JUCE. O responsável por um pacote binário deve gerar novamente a
lista de dependências do artefato final e incluir os avisos exigidos pelas
versões realmente distribuídas.

## 4.1 Fontes distribuídas com o aplicativo

| Família | Titular indicado no aviso oficial | Licença | Arquivos e texto integral |
|---|---|---|---|
| Big Shoulders | The Big Shoulders Project Authors | SIL Open Font License 1.1 | `rasgo-synth-app/assets/fonts/BigShouldersDisplay.ttf` e `LICENSE-BigShoulders.txt` |
| IBM Plex Mono | Copyright © 2017 IBM Corp.; nome reservado “Plex” | SIL Open Font License 1.1 | `rasgo-synth-app/assets/fonts/IBMPlexMono-Regular.ttf`, `IBMPlexMono-Bold.ttf` e `LICENSE-IBMPlexMono.txt` |

O inventário e a proveniência dos demais assets do aplicativo estão em
`rasgo-synth-app/assets/README.md`. A revisão e os hashes exatos dos binários de
fonte foram verificados em14/10/2026 por igualdade SHA256 com cópias oficiais:
Big Shoulders em google/fonts `7d86539c3e24d8ea2cae7b3ff04c64250dca99c5`;
IBM Plex Mono Regular/Bold em `633f3200539c52ee0aba2dfd7f46921417a81877`.
Isso identifica as cópias idênticas publicadas, não a data original de download.

## 5. Projetos citados, mas não incorporados

Os itens seguintes são referências conceituais ou históricas, não dependências
embarcadas no código atual:

- Kosmische, de Nicolas Steven Miller — GNU GPL v3;
- Chaos UGens do SuperCollider, de Lance Putnam — GPL v2 ou posterior na fonte
  examinada;
- firmware Eurorack da Mutable Instruments, de Émilie Gillet — licenças que
  variam por módulo e revisão;
- esquemas, módulos e instrumentos identificados em `CREDITS.md`.

A licença de uma referência não deve ser descrita como “herdada” pelo Rasgo
Synth quando nenhum código daquela referência foi incorporado. Do mesmo modo,
a compatibilidade entre licenças não transforma uma influência conceitual em
dependência ou coautoria.

## 6. Checklist para distribuição

Antes de publicar código ou binários do aplicativo:

1. conferir o checkout contra o commit do JUCE fixado em `DEPENDENCIES.md`;
2. registrar a opção AGPLv3 do JUCE e conferir os avisos da revisão usada no build;
3. incluir `LICENSE`, este arquivo e os textos integrais das licenças de todos
   os componentes compilados ou redistribuídos;
4. conferir as dependências do binário final na plataforma de lançamento;
5. preservar os avisos de copyright de terceiros;
6. atualizar este inventário sempre que módulos, bibliotecas, fontes, imagens,
   amostras ou outros ativos externos forem adicionados.

Este inventário descreve o estado do repositório e do procedimento de build na
data indicada no início do arquivo; não é aconselhamento jurídico.
