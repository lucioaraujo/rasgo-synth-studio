# Rasgo Synth — Documento de Referência do Projeto

Instrumento generativo — conceito, histórico, arquitetura, funcionalidades, fluxo de uso, procedimentos realizados, validações, segurança, layout e pendências.

> **Sobre este documento**: é a referência técnica estável do projeto, atualizada conforme upgrades reais acontecem (não a cada sessão). Para o diário de bordo fase-a-fase, com o histórico completo de decisões e o motivo de cada uma, a fonte de verdade continua sendo [`Plano_de_Upgrade_Instrumento_Generativo.md`](../../Plano_de_Upgrade_Instrumento_Generativo.md) na raiz do projeto — em caso de divergência entre os dois, aquele documento é autoritativo.

Última atualização: 22 de agosto de 2026 (referência sincronizada com o trabalho operacional das fases 118–123).

---

## 1. Conceito

Rasgo Synth é um **instrumento generativo**: você define uma configuração inicial (uma semente numérica, um andamento) e aperta compor, o software escreve sozinho uma peça inteira, com desenvolvimento e variação contínua ao longo do tempo, não um loop que se repete. Um console de performance ao vivo continua possível como modo secundário (o app já toca áudio em tempo real via `juce_audio_devices`), mas o alvo principal sempre foi outro: servir de base para faixas de álbum.

A frase que resume o projeto, e que hoje aparece na própria interface do app, é: **"se arrisca, e soa"**. Cada execução é sonoramente única por natureza, o motor é construído sobre dinâmica caótica (nove mapas — Lorenz, Henon, Latoocarfian e outros seis do "Chaos Cookbook") e osciladores instáveis que se cruzam e se realimentam, a mesma qualidade sonora encontrada experimentando com os circuitos CMOS do projeto de hardware Antitotem que precede este software.

Um princípio de design que atravessa o projeto inteiro: **portar a ideia, não copiar o circuito**. Sempre que um esquema real (CBS, Buchla, TR-808, MFOS/Ray Wilson, Instruō, Qu-Bit Electronix e vários outros) foi estudado, o objetivo foi entender o comportamento sonoro e reimplementá-lo em C++ puro, nunca simular transistor por transistor.

O porquê filosófico por trás dessa técnica está registrado em [`Manifesto.md`](Manifesto.md).

Outro princípio, reafirmado várias vezes ao longo do desenvolvimento: quando um parâmetro pode ser decidido pela semente ou por controle manual do usuário, o projeto tende a escolher um dos dois, não uma mistura arbitrária. A escala em si deixou de ser um controle manual (fase 48) — hoje é sempre uma escolha da própria semente, dentro de um repertório de 39 escalas pesquisadas mais uma fatia de escalas geradas algoritmicamente (peneiras de Xenakis, fase 61). Controles manuais sobreviveram só onde fazem sentido como "afinação do instrumento" (o andamento, o volume de saída), não como sobreposição de decisões estruturais que a semente já toma.

Uma virada de fase explícita aconteceu na fase 80: de "adicionar módulos isolados" para "multiplicar encontros entre módulos já existentes". Os **pesos de jogo de Caillois** (Alea/Agon/Ilinx/Mimicry, ver seção 4.9) formalizam isso numa máquina de regras real, e a fase 84 (Curto-Circuito, seção 4.10) leva essa lógica a um extremo: um mesmo evento pode romper o próprio protocolo que a semente havia estabelecido.

---

## 2. Histórico

### 2.1 Kosmische (2013)

Uma das referências técnicas iniciais foi uma perícia sobre o Kosmische, projeto externo de Nicolas Steven Miller: um app Android que embarcava um servidor SuperCollider completo via NDK. O binário nativo do Kosmische trazia três famílias de UGens relevantes: `ChaosUGens` (os nove mapas caóticos), `GendynUGens` (síntese dinâmica estocástica de Xenakis) e `AY_UGen` (emulação do chip AY-3-8910). Essa investigação informou conceitos do Rasgo Synth, mas Kosmische não integra Rasgo e não constitui sua origem autoral.

### 2.2 Antitotem e os módulos físicos do Rasgo

O Antitotem é um instrumento de hardware construído à mão, de natureza modular e CMOS/discreta, que antecede Rasgo e foi posteriormente incorporado ao projeto como seu precursor e patrono. A expressão "Rasgo em hardware" se refere ao conjunto de módulos físicos autorais daquela fase (desde 2013), já concebidos visando ao padrão Eurorack.

Desde então, Rasgo evoluiu para algo maior: hoje é o projeto-guarda-chuva que reúne todos os instrumentos e softwares musicais autorais de Lucio de Araujo. Rasgo Synth é uma obra dentro desse conjunto, enquanto Antitotem permanece simultaneamente como obra incorporada e origem simbólica da linhagem. Rasgo Synth Performance é agora um protótipo standalone local e independente para DSP ao vivo; console embarcado ou hardware continuam possibilidades posteriores, não a definição desse instrumento de software.

### 2.3 Decisão de arquitetura

A decisão técnica foi não reviver o servidor SuperCollider como dependência de runtime. Os conceitos investigados foram implementados novamente em C++ como código próprio do Rasgo Synth, e depois amplamente expandidos — hoje dezenas de mecanismos além do que existia em 2013.

### 2.4 Licenciamento

O código autoral atual do Rasgo Synth Studio segue **AGPL-3.0-or-later**. Kosmische também é GPLv3, de forma independente. As condições específicas do aplicativo construído com JUCE estão registradas em `THIRD_PARTY_NOTICES.md`; toda atribuição por mecanismo está em `CREDITS.md`.

---

## 3. Arquitetura JUCE / C++

O projeto é dividido em dois subprojetos independentes.

### 3.1 `rasgo-synth-core` — motor DSP

C++17 puro, sem nenhuma dependência de plataforma. Compilado com g++ diretamente (sem CMake nesta máquina).

- **`src/dsp/`** — dezenas de classes DSP: osciladores clássicos com PolyBLEP; nove mapas caóticos; oscilador Walsh (base ortogonal ±1, fase 59); VOSIM (pips de seno² decaindo, fase 72); filtro ladder Moog, state-variable Chamberlin e Polivoks auto-oscilante (fase 75); waveshaper de diodo; ressoador modal; nuvem granular; divisor de clock XOR polirrítmico; oito vozes de percussão digitais; `RhythmSieve.hpp` (peneira rítmica de Xenakis, fase 82) e `DrumArchetypes.hpp` (7 arquétipos nomeados + slot de peneira); Rungler, Fracture Folder, Leibniz Rectifier; `ParenthesisGlitch.hpp` (fase 70); `SieveScale.hpp` (escala gerada por peneira de Xenakis, fase 61); Gendyn, AY-3-8910; fonte de tensão aleatória tipo Buchla.
- **`src/sequencer/`** — 39 escalas pesquisadas (`Scales.hpp`, fase 48) mais peneiras geradas; sequenciador de passos (`StepSequencer.hpp`) com condução de voz real por semitons, hocketing/cascata polifônica, progressões de acorde e transformações Neo-Riemannianas P/L/R (mutuamente exclusivas); arco narrativo por cadeia de Markov com gramáticas plugáveis (`GenerativeArc.hpp` + `ArcTemplates.hpp`).
- **`src/engine/`** — `PieceRenderer.hpp` (render principal), `CompositionIdentity.hpp`, `GamePlan.hpp`, `MaterialAgency.hpp`, `IntermodularOrchestra.hpp`, `OrganismStemAuditor.hpp` (22 mesas opcionais pós-espaço/pré-edição), `OrganismAudibility.hpp` (ecologia móvel de presença) e `AlbumRenderer.hpp` (trajetória de jogo entre faixas).
- **`src/io/`** — `WavWriter.hpp` (metadados via chunk RIFF LIST/INFO) e `Recorder.hpp`.
- **Demos CLI** — `rasgo_integrated_demo` (peça avulsa) e `rasgo_album_demo` (conjunto de faixas).
- **`regression_test.py`** — script de teste automatizado (seção 7).

### 3.2 `rasgo-synth-app` — aplicativo desktop JUCE

- **`src/MainComponent.h`** — a tela principal (seção 5).
- **`src/RasgoLookAndFeel.h`** — a identidade visual "Cobre" (seção 9), incluindo o `LogoMark` (vetor real do autor, carregado de um SVG em disco via `juce::Drawable`).
- **`src/ArcTimelineView.h`, `WaveformView.h`, `OscilloscopeView.h`** (agora estéreo), **`SpectrumView.h`** (banco de filtros Goertzel), **`LevelMetersView.h`** (RMS/peak horizontal), **`GameWeightsView.h`**, **`GranulationView.h`** (partículas) — as visualizações da tela principal.
- **`src/BufferPlaybackSource.h`** — fonte de playback JUCE a partir do buffer já renderizado, com filtro ao vivo e ganho master.
- **`src/Main.cpp`** — janela abre maximizada por padrão (`setFullScreen`); um `juce::Viewport` (`RasgoViewport`) garante scroll real em monitores menores do que o layout confortável exige, em vez de cortar ou sobrepor controles.
- **`build_juce_minimal.sh` / `build_app.sh`** — receita de compilação sem CMake/Projucer.
- **`autotest.sh`** — smoke test automatizado do app (seção 7).

Áudio ao vivo é servido por `juce_audio_devices`/`juce_audio_basics`, com ALSA como único caminho no Linux e o pseudo-dispositivo "pulse" preferido sobre o "default" bruto.

---

## 4. Funcionalidades

### 4.1 Núcleo sonoro

Seis princípios de síntese distintos coexistem: subtrativa (osciladores + ladder digital, Chamberlin ou Polivoks), aditiva (Walsh, VOSIM), ressonante (ressoador modal, incluindo a voz de tuba/sopro contínuo), granular (`GranularCloud`), estocástica (`GendynOscillator`) e vetorial (Vector Synthesis, fase 76 — crossfade 2D real entre um banco dedicado de Gendyn/Walsh/Atari Punk/ruído colorido, posição ditada por dois mapas de caos). Os nove mapas caóticos e quatro LFOs lentos seguem todos ativos.

Dois osciladores principais (`osc1`/`osc2`) podem se modular mutuamente em **cross-FM bidirecional**. Um seguidor de envelope lento pode realimentar a própria saída de áudio na taxa do mapa de Lorenz (**self-feedback**). Uma camada de **ruído colorido** soma-se à textura granular/Gendyn.

Camadas aditivas de acompanhamento: **Harmonàig** (fase 77, pad de acorde de 4 vozes reais — raiz/3ª/5ª/7ª, inspirado no módulo real da Instruō por Jason Lim), **polifonia de peneiras de Xenakis** (fase 78, 4 canais independentes — madeira/sopro, metal/vidro, água, plasma reativo) e **Bloom** (fase 79, melodia fractal Trunk/Branch/Path, inspirada no módulo real da Qu-Bit Electronix por Andrew Ikenberry).

### 4.2 Camada narrativa — o arco generativo

A duração e a forma de cada peça são uma **caminhada de Markov** sobre cinco papéis estruturais, terminando só na coda. Duas gramáticas plugáveis e misturáveis: **Dramatic** (Pirâmide de Freytag) e **Kishōtenketsu**. Um accelerando rumo ao clímax (fase 68) espelha o ritardando de coda já existente.

### 4.3 Sequenciamento e escalas

39 escalas pesquisadas (fase 48) mais peneiras geradas algoritmicamente por teoria de Xenakis (fase 61, rejeitadas contra o catálogo real). Condução de voz real por distância em semitons (fase 62), progressões de acorde de verdade (fase 34) e, como alternativa mutuamente exclusiva, transformações triádicas Neo-Riemannianas P/L/R (fase 65, voice leading parcimonioso verificado). Hocketing com cascata polifônica tipo shift-register (fase 63). Um divisor de clock XOR gera polirritmia independente da grade euclidiana principal; o ponteiro de leitura da melodia pode ser endereçado por uma fonte caótica dedicada em vez do clock linear (fase 64).

### 4.4 Percussão

Oito vozes digitais independentes (bumbo, caixa, chimbal, cowbell, clap, tom/conga, cymbal, maracas), inspiradas em parte pelo TR-808. Presença agora ligada à densidade do arco, não uma constante fixa (fase 66) — 12% das peças não têm bateria nenhuma, e uma fração tem falhas reais por batida. Oito opções de arquétipo rítmico por seção do arco (fase 67 + 82): Marcha, Soul-backbeat, Metal-blast-beat, Jazz-swing, Samba-inspirado, dois arquétipos aksak balcânicos reais (Brăiloiu/Dzhudzhev, 9+7 e 7+7+2) e uma peneira rítmica de Xenakis gerada (mesma matemática da seção 4.3, aplicada a pontos no tempo). Ritmo euclidiano real (Bjorklund/Toussaint) como alternativa pro cowbell (fase 57). Dinâmica expandida: hihat no contágio do caos, ratchet nos toms, sustain estendido no prato (fase 56).

### 4.5 Saída estéreo

Canais divergem de verdade a partir do delay/reverb (linhas independentes, levemente dessincronizadas). Um batimento binaural caótico (fase 73) soma uma segunda fonte de divergência real e verificável entre L/R, independente de pan/delay.

### 4.6 Modo álbum

Uma semente de álbum deriva deterministicamente N sementes de faixa. Até a fase 82, todas as faixas compartilhavam um centro tonal forçado (mesma escala/raiz). **A partir da fase 83**, isso mudou: o álbum percorre uma **trajetória em linha reta pelos quatro pesos de jogo de Caillois** (seção 4.9) — a primeira faixa nasce num ponto sorteado do simplex, a última termina noutro ponto também sorteado, cada faixa no meio ocupa seu próprio ponto interpolado, e cada faixa escolhe sua própria escala livremente, como uma peça avulsa. Um **tema** nomeado ainda define bpm/nota-raiz/viés de energia (mood), mas não mais a escala.

### 4.7 Qualidade sonora — higiene técnica e masterização

Dither TPDF, PCM24/48 kHz, `DcBlocker`, `LookaheadLimiter` true peak com FIR da
ITU-R BS.1770-5, condicionamento contínuo neutro abaixo do joelho, PolyBLEP
(~46dB de redução de aliasing medida), oversampling na mistura por diodo e
masterização de álbum em duas passadas. O Studio verifica o buffer pós-fade e usa
somente trim constante quando necessário para entregar até −1,8 dBTP; a regressão
formal mede o WAV codificado de modo independente com FFmpeg. Um bug real de
instabilidade contínua no `ModalResonator` da tuba/canal madeira (excitação
contínua empurrando o resonador pra perto do próprio teto de segurança, ~25x mais
alto que a peça) foi corrigido na fase 81 com um limitador suave `tanh` na saída,
mesmo padrão já usado no Polivoks.

Desde a fase 122, `OrganismStemAuditor` pode reconstituir 22 contribuições no
ponto pós-espaço/pré-edição, e `OrganismAudibility` move direitos de presença sem
normalizar nem disparar módulos. O alvo de −1,8 dBTP substituiu −1,5 depois de
uma seed extrema alcançar −0,9 dBTP na reconstrução externa do FFmpeg.

### 4.8 Captura, metadados e índice

Cada WAV grava metadados embutidos (chunk RIFF LIST/INFO) e um `.txt` companheiro legível como partitura (fase 54) com a trajetória completa da peça. Um relato em texto puro da própria peça (semente, seções) vive num chunk RIFF trailing legível em editor de texto/hex, invisível a qualquer player (self-epitaph, fase 46). Toda gravação acrescenta uma linha a `rasgo_index.jsonl`.

### 4.9 Pesos de jogo de Caillois (fase 80)

Quatro pesos contínuos — **Alea, Agon, Ilinx, Mimicry** — amostrados uniformemente do 4-simplex via "stick-breaking" (três pontos de corte sorteados e ordenados, os quatro vãos são os pesos), então cada peça tem sua própria mistura autoral em vez de escolher de uma lista fixa de "jogos". Alea não tem cluster de mecanismos próprio — é a ausência de viés temático, o comportamento-base do motor. Os outros três nudgeiam (nunca forçam) onze mecanismos já existentes: **Agon** (vozes competindo/coordenando) → Agon, Harmonàig, polifonia de peneiras; **Ilinx** (vertigem/perda de controle) → Rungler, colapso de Rossler, thermal fade, self-feedback, stochastic mute, Leibniz rectifier — e, desde a fase 84, também o Curto-Circuito (seção 4.10); **Mimicry** (simulacro) → arquivo apócrifo, self-epitaph, sinal estrangeiro, parênteses, elemento estranho. Atribuído honestamente a Roger Caillois (*Les Jeux et les Hommes*, 1958).

### 4.10 Curto-Circuito (fase 84)

Um evento único por peça, viesado por Ilinx, com duas funções na mesma instância. **Corte seco real**: um gate (~3ms de rampa linear, nunca fade) fecha pra silêncio verdadeiro por 100-400ms e reabre do mesmo jeito — a mesma assinatura identificada numa análise real de waveform/espectrograma de uma referência do próprio autor. Uma "faísca" de ruído cavalga só as bordas de transição, literalizando "curto circuito" como artefato elétrico, não um mudo comum. **Mecanismos de penetra**: no mesmo instante, seis mecanismos que a semente já tinha sorteado como desligados (arquivo apócrifo, entonação livre, modulação tonal, self-feedback, rungler, elemento estranho) ganham 40% de chance cada de ligar mesmo assim — reaproveitando o mesmo idioma de "ativação forçada" que a garantia de intensidade (fase 40) já estabelecia, disparado por uma ruptura no meio da peça em vez de uma rede de segurança no início.

### 4.11 Outras rupturas e glitches

Fracture Folder + Rungler (fase 41), Leibniz Rectifier (fase 42), contágio do caos (fase 44), thermal fade (fase 45), colapso de Rossler + self-epitaph (fase 46), stochastic mute (fase 47), elemento estranho (fase 50), sinal estrangeiro real via temperatura/carga do host (fase 51, a única exceção honesta de reprodutibilidade pura), overflow de ponto fixo Q1.15 (fase 52), spectral ghost via FFT/IFFT (fase 53), sample-hold glitch (fase 55), cross-domain contagion (pico de áudio empurrando o clock, fase 60), Parênteses (reroteamento pra loop de stutter, fase 70), chaos-addressed step reading (fase 64) e o Curto-Circuito (seção 4.10, fase 84) coroando a família.

---

### Estado operacional atual (22 de agosto de 2026)

O Studio agora inclui agência material, observação de audibilidade pós-espaço,
memória de orquestração relacional, contratos melódicos e de escalas ampliados,
articulação variada da percussão e Modo Selvagem auditável. O Modo Selvagem é o
padrão do Studio: entropia real re-semeia o gerador compartilhado nas fronteiras
de compasso. Desligá-lo restaura a reprodutibilidade por seed quando coincidem
andamento, configuração, versão do motor e estado externo relevante. Takes
recentes com a mesma seed em modo normal e selvagem tiveram hashes SHA-256
diferentes, embora preservassem o mesmo material macro. A base de verificação
atual é CTest 19/19, regressão normal 6/6 e regressão selvagem 3/3, sem clipping.

## 5. Fluxo do usuário

### 5.1 No aplicativo desktop

1. Abrir o app: `cd rasgo-synth-app && ./rasgo_synth_app` — abre maximizado.
2. Definir a semente no campo SEED, ou clicar ALEA para sortear uma nova.
3. Ajustar o andamento no slider TEMPO (20–220 bpm). A escala não é mais um controle manual — a semente escolhe.
4. Ajustar o volume de saída no slider MASTER (ao lado de NÍVEL, medidor RMS/peak horizontal).
5. Conferir/ajustar o DIRETÓRIO DE SAÍDA, TÍTULO e ARTISTA (mais estreitos agora, dividindo espaço com PESOS DE JOGO).
6. Clicar no botão circular COMPOR (ícone de colcheia ♪, vermelho) — a renderização roda numa thread separada; a linha do tempo do arco acende a seção ativa, a forma de onda se desenha.
7. Clicar PLAY (ícone play/pause, dourado) para ouvir o resultado, ou clicar diretamente sobre a forma de onda (com título WAVEFORM) para tocar a partir daquele ponto.
8. Arrastar sobre a forma de onda seleciona um trecho — habilita TOCAR TRECHO e SEED DO TRECHO (deriva uma nova seed via hash FNV-1a e já dispara uma nova composição).
9. Durante a reprodução: o slider BRILHO AO VIVO aplica um filtro passa-baixa real ajustável na hora; VARIAR SEED captura o instante exato e deriva uma nova seed.
10. Abaixo do waveform: ESPECTRO (banco Goertzel) e GRANULAÇÃO (partículas reativas à amplitude). PESOS DE JOGO mostra as quatro barras coloridas de Alea/Agon/Ilinx/Mimicry da peça atual. O painel de mecanismos mostra, em grade, quais dos ~50 recursos do motor se manifestaram nesta peça específica.
11. OSCILOSCÓPIO (ESTÉREO, dourado) e LOG (com COPIAR/LIMPAR abaixo da caixa) dividem a faixa inferior, 60%/40% de largura.
12. TUTORIAL e ABOUT (canto superior direito) abrem diálogos temáticos com instruções de uso e a descrição real do projeto.
13. MODO ALBUM (botão retangular, preenche de cobre quando ligado) troca o painel para TEMA/FAIXAS/OUVIR.

### 5.2 Via linha de comando

```bash
# Peça avulsa
./rasgo_integrated_demo [seed] [outDir] [bpm]

# Conjunto de faixas (álbum) -- trajetória de pesos de jogo, não mais escala fixa
./rasgo_album_demo [albumSeed] [trackCount] [outDir] [tema]

# Bateria de regressão
python3 regression_test.py --seeds 20

# Smoke test do app
./autotest.sh --skip-build --timeout=30
```

---

## 6. Procedimentos realizados (linha do tempo por fases)

O documento vivo do projeto ([`Plano_de_Upgrade_Instrumento_Generativo.md`](../../Plano_de_Upgrade_Instrumento_Generativo.md)) é a fonte de verdade completa, fase a fase, com o motivo e o teste de cada uma. Resumo cronológico:

- **Fases 1–23** — núcleo DSP validado, sequenciador, camada narrativa por cadeia de Markov, reskin visual "Sinal" (depois substituído por "Cobre", ver seção 9), oito vozes de percussão, saída estéreo real, modo álbum inicial, qualidade sonora audiófila (dither/DC-block/limiter/PolyBLEP), nove mapas caóticos ativos, módulos de circuito reais (Atari Punk, Envelope-oscillator, Noise Cornucopia), Turing Machine no sequenciador, self-feedback de áudio, cross-FM, ruído colorido.
- **Fases 24–48** — bateria real audível com jitter de mixagem por seed, arquivo apócrifo, interação real com a forma de onda, modo álbum na UI, variação de andamento, harmonia funcional inicial, performance ao vivo (filtro + VARIAR AGORA), modulação de centro tonal, modulação modal, progressões de acorde reais, humanização rítmica, efeitos por voz de percussão, hocketing, rede de influência cruzada, entonação livre, garantia de intensidade (todo piece se testa em ao menos um mecanismo de risco), Fracture Folder + Rungler, Leibniz Rectifier, voz de tuba, contágio do caos, thermal fade, Rossler collapse + self-epitaph, stochastic mute, e a expansão de escalas de 9 para 39 com a caixa ESCALA removida da UI.
- **Fases 49–68** — Agon (quatro vozes competindo por poder, com uma "rusga"), elemento estranho (interno e literal via leitura real do host), overflow de ponto fixo, spectral ghost, partitura .txt companheira, sample-hold glitch, dinâmica de bateria expandida, ritmo euclidiano real, oscilador Walsh, contágio cruzado audio→clock, escala gerada por peneira de Xenakis, condução de voz real, cascata polifônica, endereço de passo caótico, transformações Neo-Riemannianas, bateria como instrumento de verdade (a partir de escuta real de um álbum), arquétipos rítmicos com identidade real, accelerando rumo ao clímax.
- **Fases 69–76** — mercado de probabilidades (preços persistidos entre renders), Parênteses (reroteamento pra stutter loop), Heisenberg (precisão rítmica/de afinação acopladas inversamente), VOSIM, batimento binaural caótico, envelope de asfixia por sidechain, filtro Polivoks auto-oscilante, Vector Synthesis (crossfade 2D real).
- **Fases 77–82** — Harmonàig (pad de 4 vozes, atribuído a Jason Lim/Instruō), polifonia de peneiras de Xenakis (4 canais), Bloom (melodia fractal, atribuído a Andrew Ikenberry/Qu-Bit Electronix), pesos de jogo de Caillois (fase 80, virada de fase explícita), correção de um bug real de instabilidade contínua no ModalResonator da tuba/madeira (fase 81), pesquisa de ritmo aprofundada — dois arquétipos aksak reais + peneira rítmica de Xenakis gerada (fase 82).
- **Fase 83** — fim da afinidade tonal forçada do modo álbum, substituída por uma trajetória em linha reta pelos pesos de jogo de Caillois; cada faixa escolhe sua própria escala livremente.
- **Fase 84** — "Curto-Circuito": corte seco real (gate + faísca de ruído) que, no mesmo instante, dá uma segunda chance a mecanismos que a semente tinha deixado desligados de entrarem de penetra na peça.
- **Fases 85–117** — ritmo generativo em camadas e gesto/dinâmica, mudança real
  de métrica, montagem, theremin, Atropelo, síntese vocal/coro, ácido, arquivo
  apócrifo aprofundado, flauta FM com movimento harmônico/voicing/dissonância,
  phasing, microtonalidade, objeto espectral, polimetria, jogo por blefe e
  confiança, rasgo espectral e ruptura por excesso de coerência. A fonte de
  verdade detalhada permanece o Plano de Upgrade.
- **Fase 118** — identidade composicional antes dos módulos, jogo como política,
  protagonismo tímbrico/rítmico e saída PCM24/48 kHz true-peak auditável.
- **Fase 119** — módulos como arquivos abertos, arquiviagem com proveniência,
  topologia mutável, cismos interior/exterior e gesto de rasgar em seis
  movimentos, com consequências temporais, rítmicas, harmônicas e espaciais.
- **Fase 120** — ativação, presença e agência deixam de ser sinônimos; seis
  famílias sustentadas alternam primeiro plano/interlocução/fundo segundo
  identidade e jogo, enquanto acontecimentos raros interrompem sem ocupar
  seções vazias. Contribuição real por família é medida antes do espaço/master e
  confrontada pela regressão com o histórico de agência.
- **Fase 121** — primeira Orquestra Relacional RASGO: a bateria publica gesto
  realmente ouvido; o theremin interpreta com alvo, sensibilidade, resistência,
  memória e presença próprios; a harmonia aceita, tensiona, resiste ou se
  reconfigura; a consequência retorna ao ritmo. Continuidade, latência,
  retirada e retorno são possibilidades temporais, não roteiro obrigatório.
- **Redesenho de layout do app** (entre as fases 82 e 83) — identidade visual trocada de verde-fosforescente ("Sinal") para cobre; logo real do autor vetorizada e aplicada como emblema; painel de módulos expandido pra grade cobrindo ~50 mecanismos; osciloscópio estéreo; painéis ESPECTRO/NÍVEL/GRANULAÇÃO/PESOS DE JOGO novos; BPM e MASTER como sliders horizontais; ícones reais pra COMPOR/PLAY; ABOUT/TUTORIAL temáticos; janela maximizada com fallback de scroll.

---

## 7. Validações

### 7.1 O que existe

- **`regression_test.py`** — builda o motor, renderiza vinte seeds, mede duração,
  PCM24/48 kHz, clipping, seções, true peak externo, saúde do barramento,
  diversidade perceptual, jogo, rasgo, topologia, arquiviagem e causalidade
  relacional. Falha quando a arquitetura declarada não produz consequência
  observável, mas não força participantes raros a existir só para cobrir uma
  estatística.
- **`autotest.sh`** — builda o app, roda com `RASGO_AUTOTEST=1`, confirma a linha "gravado:" no log e console nativo limpo.

### 7.2 O que foi de fato verificado

Cada fase desde a 13 passou por `regression_test.py` (lotes de 15–20 seeds) com zero amostras saturadas, e o app foi recompilado e passou no `autotest.sh` depois de cada uma. Além disso, cada mecanismo seed-gated novo desde a fase 49 recebeu uma **auditoria empírica de disparo real** (renderizar um lote de seeds e conferir nos metadados que o mecanismo de fato ativa na taxa esperada, não só que compila) — disciplina que já pegou bugs reais (colisão de 11,6% na peneira de altura antes de corrigida, contador de gap em unidade errada no Agon, instabilidade do ModalResonator na tuba). Várias fases nasceram diretamente de **escuta real** do usuário, não só medição: a bateria (fase 66), a audibilidade da tuba (fase 81) e a própria análise de waveform/espectrograma que originou o Curto-Circuito (fase 84).

### 7.3 O que ainda não foi verificado

Nem toda combinação possível dos ~50 mecanismos foi ouvida manualmente — a cobertura é por auditoria empírica de metadados (confirma disparo real) mais escuta pontual guiada por relatos específicos do usuário, não uma varredura exaustiva de todas as combinações.

---

## 8. Segurança

- **Execução inteiramente local** — nenhuma rede, telemetria ou coleta de dados pessoais em nenhum componente do motor ou do app.
- **Sem credenciais nem dados sensíveis** em qualquer parte do código ou dos scripts de build/teste.
- **Cuidado com espaço em disco** e com `recordings/` — é o diretório real de saída do usuário (álbuns reais, índice acumulado); scripts de teste escrevem em subpastas isoladas ou limpam exatamente o arquivo que acabaram de criar, nunca uma varredura ampla.
- **Licença AGPL-3.0-or-later** do código autoral atual; texto integral em `LICENSE`.
- **Build sem privilégios elevados** — a compilação (g++ direto) não requer sudo.

---

## 9. Layout — identidade visual "Cobre"

A identidade visual mudou de "Sinal" (verde-fosforescente sobre quase-preto frio) para **"Cobre"**: a cor de destaque é um laranja-cobre oxidado (`#e8865f`), escolhida por referenciar diretamente o objeto físico real por trás do nome "rasgo" — placas de Eurorack corroídas por ácido que o autor construía à mão desde 2013. Fundo continua quase-preto, agora com viés quente em vez de frio.

- **Emblema** — o logo real do autor (vetorização própria em Inkscape) carregado via `juce::Drawable::createFromSVGFile`, tingido de cobre, aplicado ao lado do título.
- **Paleta ampliada** — `Palette::action` (dourado, `#f2c14e`) para os botões de ação primária (COMPOR/PLAY); `Palette::danger` (vermelho) reaproveitado como cor do COMPOR especificamente; um pequeno ciclo cobre/vermelho/dourado (`vizAccent`) colore as barras dos novos painéis (pesos de jogo, espectro por banda, medidores L/R) em vez de tudo em cobre monocromático.
- **Ícones reais** — COMPOR usa o glifo Unicode de colcheia (♪, mais confiável que um path desenhado à mão); PLAY usa um triângulo/duas barras conforme o estado de reprodução.
- **Osciloscópio estéreo** — duas raias independentes (L/R), dourado, janela de tempo mais longa que antes.
- **Tipografia** — monoespaçada em título, rótulos e log; tamanhos aumentados em relação à versão "Sinal".
- **Textura de fundo** — grade de graticule de osciloscópio por trás de toda a janela.
- **Janela** — abre maximizada por padrão (`setFullScreen`), com fallback de scroll real (`juce::Viewport`) em monitores menores do que o layout confortável (1100×760) exige.

**Tentativa revertida**: um efeito de "rasgo" visual (papel rasgado atravessando o painel de controles e/ou o topo da janela) foi implementado e depois removido por completo a pedido do usuário — não reintroduzir sem pedido explícito novo.

---

## 10. Pendências e próximos passos

Ver a lista viva completa em `CONTINUIDADE.md` ("Tarefas em aberto") e no Plano de Upgrade ("Em aberto"). Resumo do que segue genuinamente aberto:

- **"Nota de rodapé" e voz narrante** — o mecanismo de nota de rodapé existe; permanece aberta uma voz literária mais rica.
- **Síntese vocal mais física** — Scat e Coral existem como vozes originais por formantes; um subsistema completo de trato vocal continua uma fronteira de longo prazo.
- **Expansão dos papéis relacionais** — theremin, flauta FM, tuba, vozes, polifonias e arquivo ainda precisam de contratos causais mais profundos.
- **Mudança real de compasso/métrica** — conecta com a pesquisa de ritmo (fase 82), deliberadamente fora de escopo até agora (mudaria a espinha dorsal rítmica de 16 passos fixos).
- **Console embarcado da Versão Performance** — etapa posterior à Versão Studio, por decisão explícita do usuário.
- **Sincronização documental** — os quatro documentos públicos principais estão sincronizados em EN/FR/ES/PT; auditorias internas de fases e o plano vivo permanecem documentos de trabalho em português.

---

## 11. Referências e influências

Um vácuo não explica o Rasgo Synth; este é um projeto de síntese generativa que se declara, com honestidade, em diálogo direto com circuitos reais, instrumentos comerciais, artigos acadêmicos e a própria trajetória do autor. Nenhum código-fonte de outro instrumento nem layout elétrico exato foi copiado: os comportamentos foram implementados originalmente em C++ a partir de conceitos e documentação públicos. Ver `CREDITS.md` para a tabela completa por mecanismo.

### 11.1 Trajetória autoral e referência externa inicial

- **Antitotem e módulos físicos do Rasgo (2013–)** — obras construídas pelo autor. Antitotem antecede Rasgo, foi incorporado como precursor e patrono.
- **Kosmische (2013)** — instrumento generativo Android de Nicolas Steven Miller (github.com/zrnsm/kosmische, GPLv3), referência técnica e conceitual inicial, não integrante de Rasgo.

### 11.2 Hardware comercial e circuitos DIY publicamente documentados

- **Roland TR-808** — referência sonora pra parte das oito vozes de percussão.
- **Music Thing Modular — Turing Machine** (Tom Whitwell) — o personagem de trava/mutação por passo do sequenciador.
- **Mutable Instruments** (Émilie Gillet) — Rings, Clouds, Branches, reinterpretados.
- **Instruō — ochd** (Ben "DivKid" Wilson) e **Harmonàig** (Jason Lim) — banco de LFOs e pad de acorde de 4 vozes, respectivamente.
- **Qu-Bit Electronix — Bloom** (Andrew Ikenberry) — melodia fractal Trunk/Branch/Path.
- **Frap Tools — Sapèl** — quatro cores de ruído.
- **Make Noise — Maths** / no-input mixing — realimentação de áudio real.
- **Buchla** — leveler/lag processors.
- **Rob Hordijk — Benjolin** — cross-FM bidirecional.
- **Ken Stone — CGS22 "Master Divider"** — divisor de clock XOR polirrítmico.
- **Forrest Mims III — Stepped Tone Generator** ("Atari Punk Console") — textura stepped/glitch.
- **Ray Wilson — MFOS "Noise Cornucopia"** — ruído granulado e gatilho aleatório.
- **General Instrument AY-3-8910** — arpejo/textura chiptune.
- **Pantala Labs** — modo lo-fi de delay, saturadores de fita/válvula.
- **Robert Moog; Tim Stilson e Julius O. Smith** — filtro ladder.
- **Hal Chamberlin** — state-variable filter.
- **Polivoks (Formanta, URSS)** — terceira cor de filtro, auto-oscilação real.
- **Rob Hordijk/Benjolin, Xaoc Devices Leibniz-Rectifier** — Rungler e retificação/corrupção de bits.
- **Sequential Circuits Prophet VS** — crossfade 2D real (Vector Synthesis).
- **Formanta Polivoks; técnica de sidechain/ducking de produção techno/EDM** — envelope de asfixia.

### 11.3 Matemática, ciência da computação e síntese acadêmica

- **Iannis Xenakis** — Síntese Estocástica Dinâmica ("Gendyn"); e, em *Formalized Music* (capítulo XI, "Sieves: A User's Guide"), a teoria de peneiras por congruência de módulo/resíduo, aplicada tanto a classes de altura (fase 61) quanto a pontos no tempo (fase 82).
- **Joe Pritchard — *The Chaos Cookbook* (1992)**; **Lance Putnam — Chaos UGens do SuperCollider (2004)** — referências de pesquisa para os nove mapas caóticos.
- **Vesa Välimäki e Antti Huovilainen — PolyBLEP** — anti-aliasing dos osciladores.
- **Dither TPDF** — técnica padrão de processamento de áudio digital.
- **Murray Barbour — VOSIM** (Werner Kaegi e Stan Tempelaars, IEM Utrecht) — síntese por pips de seno².
- **Brăiloiu; Stoyan Dzhudzhev** — teoria do ritmo aditivo/aksak balcânico, dois arquétipos reais (9+7 e 7+7+2).
- **David Lewin; Neo-Riemannian theory** — transformações triádicas P/L/R.

### 11.4 Estruturas narrativas, ludológicas e composicionais

- **Gustav Freytag — a Pirâmide de Freytag** (1863) — a gramática narrativa "Dramatic".
- **Kishōtenketsu** — a estrutura narrativa de 4 partes chinesa/japonesa (qǐ-chéng-zhuǎn-hé).
- **Roger Caillois — *Les Jeux et les Hommes*** (1958) — as quatro categorias de jogo (Alea, Agon, Ilinx, Mimicry), formalizadas mecanicamente como pesos contínuos que enviesam onze mecanismos existentes (fase 80).

Quem quiser aprofundar qualquer um desses pontos de partida antes de portar a próxima ideia deve manter esse mesmo padrão de honestidade: citar a fonte real, entender o comportamento antes de reimplementar, e deixar claro nesta seção (ou na fase correspondente do `Plano_de_Upgrade_Instrumento_Generativo.md`) que a ideia dialoga com um trabalho alheio, não que nasceu isolada.
