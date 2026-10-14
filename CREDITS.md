# Créditos e linhagens — Rasgo / Rasgo Synth

Revisão: 12 de outubro de 2026.

## Licença atual do Studio — 12/10/2026

Por decisão expressa de Lucio de Araujo, o código autoral atual do Studio,
incluindo core, host e ferramentas próprias, passa a GNU AGPL-3.0-or-later.
Texto integral em [LICENSE](LICENSE) e rasgo-synth-core/LICENSE. JUCE é
usado na opção AGPLv3 nesta distribuição; seus titulares e os das demais
bibliotecas/fontes continuam identificados nos avisos próprios.
Registros datados abaixo conservam GPLv3 como licença vigente naquelas
etapas; não são o aviso da fonte atual. Cópias anteriores e direitos já
concedidos permanecem preservados. Licenças de referências não foram trocadas.

Este documento registra autoria, influências conceituais e referências
históricas do Rasgo e, especificamente, do software Rasgo Synth. Ele não
transfere autoria de terceiros para o projeto e não afirma que o código de uma
obra citada tenha sido incorporado. Dependências de software e suas licenças
estão separadas em [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

Entrada de texto Linux —10/10/2026: fallback latino autoral de teclas mortas,
consulta XKB da disposição ativa e preservação UTF-8 em pastas. Pesquisa
da API [X.Org/Xlib](https://www.x.org/releases/current/doc/libX11/libX11/libX11.html)
e headers instalados `XKBlib.h`/`XKB.h`; inspeção do backend JUCE9.0.0
fixado, sem copiar/modificar sua implementação. libX11 já vinculada;
nenhuma tabela Compose de terceiros incorporada. Não constitui IME completo
nem mudança global de locale/disposição. Build/testes e smokes gráficos10/10
passaram; digitação real PT/FR/AltGr e export acentuado pela GUI ainda pendentes.

## O que é Rasgo

Estado persistido seguro —11/10/2026: correção original Studio GPLv3 no
`ProbabilityMarket.hpp`, reutilizando os contratos locais de lease/snapshot/
staging. Sem nova dependência, referência externa ou importação de irmão.
Save completo e erro explícito preservam o estado precedente; não mescla
renders concorrentes nem promete durabilidade/recuperação automática.

Índice seguro —11/10/2026: `PublishedIndex.hpp` e testes são código original
Studio, GPLv3 existente, sem código externo/irmão ou dependência nova.
Referência de contrato da API: [C++ draft, filesystem rename](https://eel.is/c++draft/fs.op.rename).
Não implica fsync, transação, recuperação automática ou segurança contra
escritores não cooperantes. Uso offline, limites e evidência na continuidade.

Publicação de manifesto —11/10/2026: `PublishedText.hpp` é código original
Studio sob a licença GPLv3 existente; adapta o contrato local
`RecordingPublication.hpp`, sem código externo ou de instrumento irmão
importado. Publicação offline sem substituição não implica fsync/transação
de múltiplos arquivos. Validações e limites na [continuidade](CONTINUIDADE.md).

Diagnóstico de recuperação —11/10/2026: adaptação autoral Studio do
`RenderSession.h`, GPLv3 existente. Buffer fixo8 KiB e indicação explícita
de truncamento UTF-8, sem biblioteca/código externo ou dependência nova.
Reproduziu perda de pasta de recuperação no limite anterior192 bytes;
testes/integrador nativo e limitações na continuidade. Não altera o DSP.

Partitura segura —11/10/2026: `PublishedScore.hpp` e seu teste são código
original Studio, GPLv3 existente; reutilizam somente o primitivo local
`PublishedText.hpp`. Não há código de terceiro, instrumento irmão, samples
ou dependência nova. Publicação sem substituição, não transação WAV/índice/
market nem recuperação automática. Fonte/core validados; app pendente,
evidências e limites na [continuidade](CONTINUIDADE.md).

Interação bumbo/baixo —10/10/2026: `BassKickInteraction.hpp` é código
original Studio, GPLv3, motivado pela escuta do autor e pela inspeção da
atenuação constante no core-sub. Envelope temporal por disparo aceito,
com limites próprios; sem samples, código ou modelo externo incorporados.
Não reivindica interpretação humana ou estilo cultural. Método e estado no
[laboratório intermodular](docs/INTERMODULAR_LAB.md).

Retorno observado do baixo —10/10/2026: `BassExecutionFeedback.hpp` é código
original do Studio sob sua licença existente GPLv3. Reutiliza os contratos
locais `PerformerState`/`BassPerformancePlan` e os três adaptadores de corpo;
nenhum código externo ou de instrumento irmão foi importado. Finitude do sinal
não é medida de qualidade interpretativa humana. Método e limites em
[laboratório intermodular](docs/INTERMODULAR_LAB.md).

**Rasgo** é o projeto geral criado e dirigido por **Lucio de Araujo** para
reunir todos os seus instrumentos e softwares musicais autorais. Cada obra pode
conservar nome e identidade próprios dentro desse conjunto. **Rasgo Synth** é
um desses softwares, não o nome do projeto inteiro.

Essa dimensão de guarda-chuva é a forma atual e ampliada do projeto. Em sua
linhagem física, Rasgo já compreendia módulos concebidos visando ao padrão
Eurorack. Hoje, porém, Rasgo não se limita a essa realização em hardware: passou
a acolher o conjunto mais amplo da produção autoral de instrumentos e softwares
musicais de Lucio.

O hardware continua pertencendo ao horizonte de Rasgo. Lucio pretende retomar,
em outro momento, o desenvolvimento de **consoles embarcados, pedais e módulos**.
Esses caminhos são planos futuros do projeto-guarda-chuva e não devem ser
apresentados como produtos concluídos nem como parte da fase atual do Rasgo
Synth.

Entre as possibilidades previstas para uma etapa posterior está uma **versão
console embarcada do Rasgo Synth**. Ela permanece como direção de pesquisa e
desenvolvimento futuro, sem implementação ou prazo anunciados no estado atual
do projeto.

Essa definição não mistura Rasgo com as outras áreas de criação de Lucio e não
incorpora automaticamente obras de terceiros. Um trabalho de upgrade ou
manutenção também não passa a integrar Rasgo apenas por ter sido realizado por
Lucio quando sua autoria original pertence a outra pessoa.

## Autoria do Rasgo Synth

Verificação técnica —09/10/2026: o helper original `tools/studio_x11_close.py`
usa a API da libX11 instalada; consulta do [manual primário X.Org](https://www.x.org/releases/current/doc/libX11/libX11/libX11.html),
seções ClientMessage, XSendEvent e propriedades. Autores e licença permissiva
do manual na própria fonte; nenhuma implementação Xlib/texto foi incorporado.
Somente diagnóstico de filho isolado, não dependência nova do core.

O detector local de pico e seus coeficientes preexistentes são confrontados
com [FFmpeg6.1.1, f_ebur128.c](https://raw.githubusercontent.com/FFmpeg/FFmpeg/n6.1.1/libavfilter/f_ebur128.c),
Clément Bœsch/FFmpeg, LGPL-2.1-or-later: resampling libswresample a192 kHz.
Consulta de algoritmo/metodologia, sem copiar código ou vincular FFmpeg ao
instrumento. Diagnóstico por stdin e piso de sample peak são código próprio;
a diferença de interpoladores não certifica qualquer um como verdade exata.

Proteção offline —10/10/2026: `DeliveryPeakGuard` é implementação autoral
sinc/janela Hann,8 fases/64 taps, banco gerado por fórmula e normalizado DC.
Consulta da [ITU-R BS.1770-5, Anexo2 e seu attachment](https://www.itu.int/rec/R-REC-BS.1770-5-202311-I/en)
sobre erro de sobreamostragem e da [documentação primária do resampler FFmpeg](https://www.ffmpeg.org/ffmpeg-resampler.html)
sobre tamanho de filtro/formato/oversampling. Também consultado
[resample.c do FFmpeg6.1](https://www.ffmpeg.org/doxygen/6.1/resample_8c_source.html),
Michael Niedermayer/FFmpeg, LGPL-2.1-or-later, para entender a reflexão nas
bordas: início em torno da primeira amostra, final repetindo a última.
O modelo finito de bordas do guard foi escrito localmente para medição,
nunca inserido no áudio entregue. Sem copiar código, bancos ou
texto de terceiros; FFmpeg não é vinculado ao app. O FIR normativo preexistente
permanece medidor separado; o guard adicional não é certificação BS.1770.
Três reconstruções independentes (32/64/128 taps,4x/4x/8x) confrontam o
mesmo PCM. Método/projeção/ganho explícitos na metadata; não normaliza LUFS.
Estado dos testes/build e limites em [continuidade](CONTINUIDADE.md).

Pesquisa rítmica —08/10/2026: `GrooveDialogue` implementa seis estudos autorais
de pulso, síncope, acento e relação bateria/baixo. Referências consultadas:
Friberg/Sundström ([KTH](https://www.speech.kth.se/music/performance/Texts/ensemble_swing.htm)),
Naveda/Gouyon/Guedes/Leman ([INESC, samba](https://repositorio.inesctec.pt/server/api/core/bitstreams/06ca3366-5cfb-44a1-8b1d-32550456e1ac/content)),
Witek et al. ([PLOS, síncope/groove](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0094446))
e Datseris et al. ([microtiming/swing](https://arxiv.org/abs/1904.03442)).
Não foram copiados código, transcrições, samples, tabelas empíricas ou áudio.
São estudos originais, não emulações de tradições inteiras. Métodos, usos
concretos e limites: [groove e malandragem rítmica](docs/GROOVE_RHYTHM.md).

**Rasgo Synth** foi concebido e é mantido por **Lucio de Araujo**, dentro do
projeto Rasgo.

Copyright © 2026 Lucio de Araujo.

O código específico do Rasgo Synth, sua arquitetura, integração, sistema de
composição por seed, motor de renderização, interface e implementações DSP
atuais foram escritos para este projeto. Os créditos abaixo reconhecem ideias,
obras, instrumentos, circuitos, algoritmos e tradições que ajudaram a formar o
vocabulário do instrumento; eles não indicam cópia literal de código, firmware,
esquemas, amostras ou recursos gráficos.

## Antitotem — precursor e patrono

Consulta rítmica de8 out. 2026: ratchets2–4 e groove de
`ANTITOTEM/src/core/SimpleSequencer.cpp`, commit
`eabde9f5cbadf7b1ba3254354002cb5e723cba40` (AGPL-3.0), arquivo limpo,
SHA-256 `dc3dc4fdfb097c22d3451062c33d909ee420d9c3599d961ba8950e3beaaefec5`.
Referência interna para respostas curtas/decrescendo do Studio; não houve
cópia/port desse código, inclusão de headers irmãos ou alteração do Antitotem.
`GrooveRepeat.hpp` foi escrito para a temporização e as vozes do Studio.
Método/limites e estado de validação: [Groove v2](docs/GROOVE_RHYTHM.md).

**Antitotem** é obra de **Lucio de Araujo**. Ele antecede a formalização do
Rasgo e é seu precursor histórico, mas não permaneceu fora do projeto: foi
posteriormente incorporado ao próprio Rasgo.

Dentro dessa linhagem, Antitotem ocupa uma posição semelhante à de um
**patrono**: é a obra fundadora que precede, inspira e simbolicamente ampara o
desenvolvimento posterior do projeto. Portanto, deve ser apresentado ao mesmo
tempo como precursor de Rasgo e como parte integrante de Rasgo — nunca como
obra de terceiro ou mera referência externa.

Neste documento, **Rasgo em hardware** designa o conjunto de módulos físicos
autorais daquela fase, que já eram concebidos visando ao padrão Eurorack. A
expressão não designa um único aparelho chamado “Rasgo Synth de hardware” e não
descreve apenas uma intenção futura.

### Assistência de desenvolvimento

A reescrita integral do código e numerosas implementações foram realizadas com
o **Anthropic Claude** como ferramenta de desenvolvimento, sob direção de Lucio
de Araujo. A ferramenta é registrada como assistência, não como autora dos
projetos, circuitos ou algoritmos citados abaixo e não como biblioteca
incorporada ao executável.

Incrementos posteriores também utilizam Codex como ferramenta assistiva,
sob direção de Lucio de Araujo, incluindo a experiência do baixo por frase de
8 out. 2026. Assistência não equivale a autoria dos referenciais ou biblioteca
incluída no instrumento.

## Origem conceitual do Rasgo Synth

O desenvolvimento inicial do Rasgo Synth, como uma obra dentro de Rasgo,
reuniu duas referências principais de naturezas diferentes:

- **Antitotem e os módulos físicos do Rasgo**, de Lucio de Araujo: o primeiro é
  precursor, patrono e parte incorporada à própria linhagem; os módulos já eram
  concebidos visando ao padrão Eurorack e constituem sua origem material,
  estética e autoral;
- **[Kosmische](https://github.com/zrnsm/kosmische)**, de **Nicolas Steven
  Miller** (`zrnsm`), distribuído sob GNU GPL v3: projeto externo usado como
  referência conceitual para o instrumento generativo portátil, os mapas
  caóticos e caminhos de síntese que motivaram a investigação inicial.

Kosmische é um projeto independente, de outro autor. O Rasgo Synth não é uma
continuação autoral de Kosmische, e Kosmische não integra Rasgo. O código do
Rasgo Synth foi reescrito e depois ampliado com numerosas implementações sem
precedente naquele projeto.

## Instrumentos, circuitos e módulos de referência

Pesquisa local — 8 out. 2026: distribuição-fonte DISTRHO Cardinal26.02,
consultada localmente, com85 manifestos e1.422 entradas
declaradas (não número certificado de módulos disponíveis no runtime).
Catálogos Surge XT/Bogaudio/Audible/Valley e demais coleções foram referências
para possibilidades tímbricas; também foi consultado pontualmente
`BogaudioModules/src/FMOp.cpp`, sem port/adaptação desse código. A experiência
`BassPhraseTimbre` usa implementação original de espectros finitos e identidades
trigonométricas, não o algoritmo FMOp. Nenhum código, painel ou dependência do
Cardinal foi incorporado nesta etapa; futuras cópias exigem auditoria própria
de licença/autoria/dependências. Fonte desta pesquisa: README, Makefile,
manifestos e `docs/LICENSES.md` da cópia local. Primeiro experimento testado no
Studio; escuta pendente. Isso não audita todos os módulos dessas coleções.

As implementações do Rasgo Synth reproduzem ou reinterpretam comportamentos
musicais por código original. Salvo indicação expressa em
`THIRD_PARTY_NOTICES.md`, nenhum firmware ou código dos projetos desta tabela é
incluído no repositório.

| Área ou módulo do Rasgo Synth | Crédito e referência | Natureza da relação |
| --- | --- | --- |
| `AtariPunkConsole` | **Forrest M. Mims III** — *Stepped Tone Generator*, posteriormente popularizado como Atari Punk Console | Reimplementação comportamental original do oscilador astável/monoestável |
| Circuitos CMOS/Lunetta | **Stanley Lunetta** e a comunidade DIY de instrumentos CMOS | Linhagem técnica e estética |
| Percussão eletrônica | **Roland Corporation**, esquemas de serviço da TR-808; análises públicas de **Eric Archer** | Vozes sintetizadas originalmente e inspiradas em comportamentos da TR-808; não são amostras nem réplicas completas dos circuitos |
| `BuchlaRandomSource` | **Don Buchla** — Music Easel/Model 208 Random Voltage Source; redesenho de referência de **Scott Stites** | Reimplementação conceitual original |
| `SignalLeveler` e `LagProcessor` | **Buchla Associates / Don Buchla**, documentos de Dual Signal Leveler e Dual Control Voltage Integrator | Interpretação funcional original de documentos de circuito |
| `KSRandomizerCore` | **Scott Stites / KS Synthesis** — 4006 Randomizer Core | Reimplementação conceitual original |
| `Rungler` | **Rob Hordijk** — Rungler/Benjolin | Reimplementação conceitual original |
| Sequenciamento com repetição probabilística | **Tom Whitwell / Music Thing Modular** — [Turing Machine](https://github.com/TomWhitwell/TuringMachine) | Lógica original inspirada no equilíbrio entre sequência travada e mutação |
| `PolyrhythmDivider` | **Ken Stone / Cat Girl Synth (CGS)** — CGS22 Master Divider | Reimplementação conceitual original |
| `NoiseGateGenerator` e `GrainyNoise` | **Ray Wilson / Music From Outer Space** — Noise Cornucopia | Reimplementações conceituais originais |
| `BernoulliGate` | **Émilie Gillet / Mutable Instruments** — Branches | Reimplementação comportamental original |
| `ModalResonator` | **Émilie Gillet / Mutable Instruments** — Rings | Banco ressonante original inspirado no conceito publicado |
| `GranularCloud` | **Émilie Gillet / Mutable Instruments** — Clouds | Motor granular original inspirado no conceito publicado |
| `BreathExciter` | **Émilie Gillet / Mutable Instruments** — Elements | Excitador original inspirado na distinção entre sopro e impacto |
| Saltos melódicos probabilísticos | **Émilie Gillet / Mutable Instruments** — Marbles | Referência comparativa para distribuição de alturas; implementação original |
| `OchdBank` | **Instruō**, em colaboração com **Ben “DivKid” Wilson** — [øchd](https://www.instruomodular.com/product/ochd/) | Banco original de LFOs relacionado ao conceito de velocidades organicamente acopladas; øchd não é um produto da Mutable Instruments |
| `ColoredNoise` | **Frap Tools** — [Sapèl](https://frap.tools/products/sapel/); aproximação de ruído rosa de **Paul Kellet** | Banco original de ruídos branco, rosa, vermelho e azul |
| `MoogFilter` | **Robert Moog**, filtro ladder; **Tim Stilson** e **Julius O. Smith**, aproximação digital Stilson/Smith | Implementação independente da técnica digital documentada |
| `SallenKeyFilter` e núcleo de `PolivoksFilter` | **Hal Chamberlin** — filtro de estado variável, descrito em *Musical Applications of Microprocessors* | O algoritmo implementado é Chamberlin; não deve ser apresentado como implementação da topologia eletrônica Sallen–Key |
| Nome histórico Sallen–Key | **R. P. Sallen** e **E. L. Key** | Referência à topologia eletrônica histórica, distinta do algoritmo usado atualmente |
| `PolivoksFilter` | **Vladimir Kuzmin**, projetista do sintetizador Polivoks | Filtro digital original inspirado no caráter instável do Polivoks; não é modelo do circuito original |
| `FractureFolder` | **Don Buchla**, Buchla 259; **Serge Tcherepnin**, Serge Wave Multiplier; **Émilie Gillet / Mutable Instruments**, Plaits e Tides | Linhagem de wavefolding; implementação e extensão “fracture” originais |
| `LeibnizRectifier` | **Xaoc Devices** — [Leibniz Binary Subsystem](https://xaocdevices.com/main/drezno/), especialmente Drezno/Lipsk | Corrupção digital de bits reinterpretada em código original |
| Referência de retificação dinâmica | **Nonlinearcircuits / Andrew Fitch** — [Difference Rectifier / Neuron](https://www.nonlinearcircuits.com/blog/2013/01/difference-rectifier-neuron.html) | Referência contextual; o nome correto não é “Dynamic Rectifier” e o módulo Rasgo não deve ser descrito como port desse circuito |
| `EnvelopeOscillator` | **CBS Musical Instruments**, documento *Dual Attack Generator No. 180*; tradição Buchla/Serge de envelopes em frequência de áudio | Reimplementação conceitual original |
| `RingModulator` | **CBS Musical Instruments**, documento *Dual Ring Modulator No. 111* | Referência documental; implementação DSP original |
| `MorphOscillator` e `DiodeShaper` | Documentação DIY de **fluxmonkey** e **Beavis Audio Research** | Referências de forma e mistura por diodos; implementação DSP original |
| `Effects` em modo lo-fi | **Princeton Technology Corporation**, circuito integrado PT2399; **Pantala Labs**, Echo 2399 | Emulação comportamental original, sem código ou modelo de silício incorporado |
| `TapeSaturator` e `TubeSaturator` | Instrumentos e módulos da **Pantala Labs** como referências comparativas | Curvas DSP genéricas e originais; não são emulações oficiais dos produtos |
| `AY8910Chip` | **General Instrument** — AY-3-8910; família compatível Yamaha YM2149 | Modelo simplificado e original da arquitetura publicamente documentada |
| Autorrealimentação em frequência de áudio | **Make Noise** — Maths; tradição de módulos Serge | Referência de patch e comportamento; implementação original |
| Modulação cruzada bidirecional | **Vlad Kreimer / SOMA Laboratory** — Lyra-8 | Referência de comportamento; implementação original |
| Cascata polifônica/hocketing | **Intellijel Designs** — Shifty | Referência de registrador de deslocamento melódico; implementação original |
| Leitura endereçável de passos | **Bastl Instruments** — Popcorn | Referência de sequenciamento endereçável; implementação original |
| Síntese vetorial | **Chris Meyer** e a equipe da **Sequential Circuits** — Prophet VS; o nome “Vector Synthesis” foi proposto pelo gerente do projeto **Josh Jeffe**, segundo o [relato de Meyer](https://learningmodular.com/the-story-of-the-prophet-vs/) | Crossfade bidimensional original entre quatro fontes já existentes no Rasgo Synth |
| Pad de acorde de 4 vozes | **Jason Lim**, fundador da **Instruō** (Glasgow, Escócia) — [harmonàig](https://www.instruomodular.com/product/harmonaig/), quantizador de tensão de 4 vozes com saída raiz/3ª/5ª/7ª | Banco original de 4 osciladores/envelopes dedicado, soando o acorde real da peça como pad sustentado; não é modelagem do circuito nem porte de firmware |

Marcas e nomes de produtos são usados apenas para identificar referências
históricas ou comparativas. Rasgo Synth não é afiliado, patrocinado nem
endossado por essas pessoas ou organizações.

## Síntese, DSP, matemática e teoria musical

| Técnica usada ou estudada | Crédito |
| --- | --- |
| Chaos UGens do SuperCollider | **Lance Putnam** (2004). Serviram como referência histórica junto à análise de Kosmische; não há código desses UGens incorporado às implementações atuais |
| *The Chaos Cookbook* | **Joe Pritchard** (1992). O livro não é de Lance Putnam |
| Mapa de Hénon | **Michel Hénon** |
| Atrator de Lorenz | **Edward N. Lorenz** |
| Mapas Latoocarfian/Clifford | **Clifford A. Pickover** |
| Standard map/Chirikov map | **Boris Chirikov** |
| Atrator de Rössler | **Otto E. Rössler** |
| GENDYN e teoria de crivos (*sieves*) | **Iannis Xenakis** |
| VOSIM | Trabalho desenvolvido no Instituto de Sonologia de Utrecht, associado a **Werner Kaegi**; análise espectral por **Stan Tempelaars**. A atribuição deve separar essas contribuições em vez de tratá-las como uma coautoria genérica “Kaegi & Tempelaars, 1978” |
| PolyBLEP/antialiasing de osciladores | **Vesa Välimäki** e **Antti Huovilainen**, [“Antialiasing Oscillators in Subtractive Synthesis”](https://doi.org/10.1109/MSP.2007.323276) |
| Funções de Walsh | **Joseph L. Walsh** |
| Matrizes de Hadamard | **Jacques Hadamard** |
| Código Gray | **Frank Gray** |
| Reverberação por combs/allpasses | **Manfred R. Schroeder**. A implementação atual é do tipo Schroeder e não deve ser chamada de Freeverb |
| Ritmos euclidianos | **E. Bjorklund**, algoritmo; **Godfried Toussaint**, aplicação e análise musical |
| Transformações neo-riemannianas P/L/R | **Hugo Riemann**, **David Lewin** e **Richard Cohn** |
| Cadeias de Markov | **Andrey Markov** |
| Arco dramático/pirâmide | **Gustav Freytag** |
| Kishōtenketsu | Estrutura narrativa de origem chinesa, desenvolvida em tradições do Leste Asiático; não é atribuída a um único autor |
| As quatro categorias de jogo (Alea/Agon/Ilinx/Mimicry) | **Roger Caillois**, *Les Jeux et les Hommes* (1958) |
| Ritmo aditivo/aksak balcânico | **Constantin Brăiloiu**, pesquisa etnomusicológica sobre ritmo aksak; **Stoyan Dzhudzhev**, teoria e nomenclatura do ritmo aksak búlgaro |

## Agradecimento às comunidades

O projeto reconhece a contribuição duradoura das comunidades de síntese DIY,
Lunetta/CMOS, música eletroacústica, software livre e modular Eurorack, cujos
documentos, discussões e experimentos mantêm essas técnicas acessíveis para
estudo e novas criações.

Correções documentais de nomes, autoria ou proveniência são bem-vindas quando
acompanhadas de uma fonte primária ou bibliográfica verificável.
