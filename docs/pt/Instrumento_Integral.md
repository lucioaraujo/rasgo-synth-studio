# Rasgo Synth — explicação integral do instrumento

Este documento explica o Rasgo Synth como um instrumento inteiro: o que entra,
como a peça é construída, o que cada painel mostra, como os módulos se
relacionam e o que é registrado no final. O [catálogo de mecanismos](../../website/assets/mechanisms.pt.json)
é a referência módulo por módulo; aqui o objetivo é compreender o organismo
completo.

## Studio e Performance

O Rasgo Synth Studio é o instrumento desktop existente para composição,
renderização, escuta e pesquisa. Rasgo Synth Performance é um instrumento irmão
de DSP ao vivo, com contratos, motor, host standalone e primeira superfície
local de quatro cenas e cinco gestos. Ainda não é publicado: validação gráfica,
de dispositivo, musical e de build portátil permanecem abertas. Os controles ao
vivo do Studio não constituem uma release do Performance.

## 1. O que é o Rasgo Synth

O Rasgo Synth é um instrumento generativo de software. Ele não funciona como
um teclado que recebe uma nota humana a cada instante, nem como um sequenciador
que repete um loop fixo. A pessoa fornece uma seed, um andamento e algumas
condições de saída; o instrumento percorre um espaço autoral de possibilidades
e sintetiza uma peça inteira.

Essa peça contém, conforme as decisões da seed:

- melodia, baixo, harmonia e condução de vozes;
- bateria, arquétipos rítmicos, swing, microtiming e polimetria;
- osciladores, ressonadores, ruídos, granularidade, FM e filtros;
- narrativa de seções, memória, agência, relações e rupturas;
- espacialização, atenção pós-espaço, mixagem limitada e masterização;
- metadata, telemetria e um relato textual da própria trajetória.

Nem todo mecanismo atua em toda peça. Isso é parte do instrumento. A seed pode
selecionar um corpo, deixar outro dormente, ativar uma textura rara ou preservar
silêncio. Por isso distinguimos quatro estados:

| Estado | Significado |
|---|---|
| Disponível | o mecanismo existe no repertório do motor |
| Manifestado | a seed o selecionou ou produziu uma condição para ele |
| Audível | há sinal mensurável depois do processamento e do espaço |
| Influente | o acontecimento alterou perceptivelmente outro fluxo ou a forma final |

Um módulo aparecer no inventário não significa que ele deva ser ouvido em todas
as seeds. A presença da flauta, da tuba ou de qualquer voz individual também não
é um indicador da cobertura total do instrumento.

## 2. O percurso completo: da seed ao arquivo

### 2.1 Origem e identidade

A seed é um ponto de partida, não uma peça congelada. Ela escolhe uma identidade
composicional: foco material, família de escala, centro tonal, modelo rítmico,
corpo de voz, jogo, risco e probabilidades. A mesma seed em modo normal tende a
ser reproduzível quando tempo, configuração, versão do motor e estado externo
são os mesmos.

O Modo Selvagem introduz entropia adicional em pontos controlados do percurso.
Ele pode abrir uma trajetória diferente a partir da mesma seed e registra essa
condição no resultado. Não é um defeito de reprodução; é outro regime do
instrumento.

### 2.2 Plano de jogo

O plano de jogo deriva quatro forças de Caillois:

- **Alea**: acaso e distribuição de possibilidades;
- **Agon**: disputa, mérito e competição entre vozes ou funções;
- **Ilinx**: vertigem, instabilidade, deformação e perda de controle;
- **Mimicry**: imitação, personagem, memória e simulacro.

Essas forças formam pesos contínuos. Eles não escolhem sozinhos “o estilo” da
peça; enviesam mecanismos que já existem. A estratégia e a regra exibidas no
painel são a leitura compacta desse campo.

### 2.3 Arquitetura temporal

O relógio do Rasgo possui várias camadas:

1. **grade** — posições de referência;
2. **pulso e subdivisão** — o que organiza a percepção do batimento;
3. **métrica** — acentos, ciclos e hierarquias;
4. **swing** — deformação sistemática da grade;
5. **microtiming** — avanço, atraso e relação entre eventos;
6. **humanização** — variação correlacionada, não erro branco independente;
7. **gate e articulação** — quanto tempo cada evento ocupa;
8. **probabilidade e ratchet** — presença, repetição e subdivisão;
9. **fase, polimetria e deriva** — vozes com ciclos e referências diferentes;
10. **memória** — o comportamento atual pode depender do que acabou de ocorrer.

O `TemporalGrooveImprint` transporta parte desse contexto para o baixo
expressivo e para a percussão de arquétipo. Sua influência é limitada para que
o groove preserve a identidade do padrão original.

### 2.4 Harmonia, melodia e baixo

O sequenciamento escolhe escalas de um repertório pesquisado e também pode usar
peneiras de Xenakis, misturas nota a nota, modulação de centro, modulação modal,
progressões e transformações neo-riemannianas.

A melodia não é somente uma lista de alturas. Ela pode receber condução de voz,
contorno, voicing, dissonância, afinação expressiva, memória gestual e uma
interpretação própria.

O baixo possui três camadas separadas:

1. **função** — raiz, pedal, caminho, tensão, contraponto ou espaço;
2. **frase** — registro, direção, repetição, aproximação, relação com o kick e
   densidade;
3. **corpo** — tuba sintetizada, acid sintetizado ou `core-sub` de fallback.

O corpo não decide a linha. Uma tuba pode estar ativa sem produzir uma linha
grave perceptível; o `core-sub` pode sustentar a fundação quando tuba e acid não
foram selecionados. Essa separação evita atribuir toda a música a uma única
voz.

### 2.5 Matéria sonora

Os módulos de matéria transformam os planos em sinal. O motor combina, conforme
a seed:

- síntese subtrativa, aditiva, ressonante, granular e estocástica;
- osciladores clássicos, morph, hard sync, FM, cross-FM, VOSIM e Walsh;
- ressonadores modais, filtros, saturadores, delay, reverb e espaço estéreo;
- texturas de ruído, mapas caóticos, glitches, rasgos e objetos espectrais;
- oito famílias de voz percussiva sintetizada, sem samples de bateria.

As referências de TR-808, Buchla, Mutable Instruments, Polivoks, Atari Punk
Console e outras são referências de comportamento e pesquisa. O código próprio
é uma reimplementação autoral; não é uma cópia de circuito ou firmware.

### 2.6 Bateria e relações

A bateria pode usar o caminho gerado tradicional, arquétipos rítmicos,
Euclidiano, aksak, sieve, interlocking, humanização, variação tímbrica e
articulação por golpe. Kick, snare, hihat, cowbell, clap, tom, cymbal e maracas
possuem decisões e corpos próprios.

O baixo pode coincidir com o kick, alternar com ele ou ocupar os espaços. A
melodia, a textura e a percussão também podem responder ou ceder. O instrumento
não aplica uma hierarquia fixa em que a flauta sempre é protagonista.

### 2.7 Forma e narrativa

O arco narrativo é uma caminhada sobre papéis como abertura, emergência,
desenvolvimento, clímax, ruptura, decantação e fechamento. Densidade, energia,
registro, filtro e presença mudam ao longo da caminhada.

O arco pode:

- repetir uma região;
- mudar de estratégia;
- abrir ou fechar espaço;
- criar uma ruptura;
- retornar a uma ideia com diferença;
- chegar à coda e encerrar a peça.

Assim, a duração da peça é consequência de uma trajetória narrativa, não apenas
de um número fixo de segundos.

### 2.8 Performance e nota como gesto

No A/B expressivo, o instrumento trata um evento como trajetória, não como
`pitch + velocity + duration`. A nota pode ter preparação, ataque, aproximação
de afinação, curva dinâmica, brilho, ruído, articulação, release, falha e
recuperação.

`PerformerState`, `NoteMorphology` e os planos gestuais já possuem contratos e
consumidores concretos em partes do baixo e da percussão. Eles ainda não devem
ser descritos como uma performance humana universal em todas as vozes.

### 2.9 Atenção, stems e mixagem

Quando a auditoria de stems está ligada, o instrumento observa o sinal depois
do espaço e calcula sobrevivência, soterramento, atenção e contribuição por
família. A mixagem narrativa pode aplicar ganhos pequenos, com limite e
histerese, valorizando acontecimentos sem reescrever a composição.

A auditoria não é um botão de “som melhor” e não é obrigatória para ouvir. Ela é
uma lente de medição e, quando habilitada, uma camada limitada de atenção.

### 2.10 Render e memória do arquivo

O `PieceRenderer` entrega:

- WAV PCM24, normalmente a 48 kHz;
- metadata com seed, configuração, identidade e mecanismos efetivos;
- um arquivo `.txt` companheiro, legível como partitura descritiva;
- uma linha no índice JSONL de gravações, quando o caminho de saída está ativo.

O `.txt` não pretende substituir uma partitura tradicional. Ele registra seções,
tempo, decisões, vozes, mecanismos e acontecimentos para que a peça possa ser
estudada sem abrir o binário.

## 3. Como ler o Studio

### 3.1 Painel superior e controles de composição

- **SEED**: define ou exibe o ponto de partida numérico.
- **ALEA / nova seed**: escolhe outro ponto de partida.
- **TEMPO**: define o andamento inicial; o arco pode produzir deriva e mudanças
  internas.
- **MASTER**: controla o nível final de saída, não a energia composicional.
- **MODO SELVAGEM**: permite trajetórias entrópicas adicionais.
- **AUDITORIA STEMS**: liga a medição pós-espaço e a atenção narrativa.
- **A/B EXPRESSIVO**: liga os comportamentos temporais e gestuais experimentais.
- **COMPOR**: inicia o render completo em segundo plano.
- **ABOUT**: mostra identidade, autoria e licença.
- **TUTORIAL**: abre a explicação operacional dentro do app.
- **idioma**: alterna inglês, português, francês e espanhol; o inglês é o
  idioma inicial.

As cores e o desenho do botão COMPOR pertencem à identidade do instrumento e
não são alterados pela tradução.

### 3.2 Painéis de leitura do resultado

- **WAVEFORM**: visão geral do áudio e seleção de regiões.
- **OSCILOSCÓPIO**: forma instantânea do sinal estéreo durante a reprodução.
- **ESPECTRO**: distribuição de energia por faixas de frequência.
- **ESPECTROGRAMA**: evolução espectral ao longo do tempo.
- **GONIÔMETRO**: relação estéreo, largura e correlação entre canais.
- **RETRATO DE FASE**: leitura geométrica do comportamento do sinal.
- **GRANULAÇÃO**: leitura visual dos eventos granulares e da atividade de textura.
- **GENEALOGIA DO ARCO**: seções e transformações da forma narrativa.
- **JOGO / ESTRATÉGIA**: pesos Alea/Agon/Ilinx/Mimicry, estratégia, regra e
  recursos da peça.
- **PAINEL DE MÓDULOS**: mecanismos manifestados naquela seed, sem afirmar
  hierarquia perceptiva.
- **LOG**: acontecimentos com horário, diagnóstico, vozes, módulos e arquivo.
  **COPIAR** leva o texto para a área de transferência; **LIMPAR** remove a
  apresentação do log, sem alterar o arquivo de áudio já criado.

### 3.3 Modo Álbum

O **MODO ÁLBUM** troca a composição única por uma coleção de faixas. A pessoa
escolhe um tema, a quantidade de faixas e uma seed de álbum. O `AlbumRenderer`
deriva seeds de faixa, conserva uma continuidade tonal ampla e permite que cada
faixa tenha sua própria identidade, energia, narrativa e seleção de módulos.

O álbum não é uma peça longa artificialmente cortada. É uma trajetória de várias
peças relacionadas. O painel de faixas permite escolher e ouvir cada resultado;
os fluxos de arco e genealogia são reiniciados para que cada faixa tenha sua
própria história.

### 3.4 Seleção, reprodução e variação

Depois de renderizar, **PLAY** reproduz o arquivo interno. Clicar na waveform
reposiciona a escuta; arrastar seleciona uma região. **TOCAR TRECHO** reproduz
somente essa região e **SEED DO TRECHO** calcula uma seed derivada dos bytes das
amostras selecionadas.

**VARIAR SEED** usa uma janela ao redor do instante escolhido durante a
reprodução, deriva outra seed e inicia uma nova composição. A variação nasce do
gesto de escuta, não de uma mutação arbitrária escondida.

## 4. Como ler o fluxograma e a partitura

O fluxograma da página é uma visão de dependências. Setas contínuas representam
fluxos principais; setas pontilhadas representam enviesamento, observação ou
influência limitada. Ele não significa que todos os módulos estejam ativos em
todas as peças.

A partitura textual é um relato causal. Ela deve ser lida perguntando:

1. qual era a identidade inicial da peça;
2. que seções o arco percorreu;
3. que vozes e corpos foram selecionados;
4. quais eventos realmente aconteceram;
5. que mecanismos foram somente possíveis;
6. que consequências foram observadas no áudio e nos stems.

O catálogo técnico complementa essa leitura com categoria, descrição e
referência de cada mecanismo. A página web apresenta primeiro o fluxo integral
para que uma lista extensa de módulos não seja confundida com a própria música.

## 5. Limites atuais e próximos desenvolvimentos

O Rasgo Synth já possui uma arquitetura ampla, mas nem todos os contratos
conceituais chegam ao PCM de todas as vozes. Performance, memória, atenção e
mixagem precisam continuar sendo avaliadas por consumidores concretos e por
escuta humana.

As próximas melhorias prioritárias são:

- tornar a linha grave mais legível sem obrigar tuba, acid ou sub a aparecerem;
- aprofundar o baixo como contraponto quando a forma permitir;
- conectar morfologia de nota e estado performativo a mais de uma família de
  voz, com testes e limites claros;
- manter a tradução integral do Studio, incluindo log e mensagens dinâmicas;
- preservar a distinção entre presença técnica, audibilidade e valor musical.

## 6. Referência técnica

- [Manual do usuário](manual.md)
- [Documento de referência](Documento_de_Referencia.md)
- [Catálogo técnico dos mecanismos](_catalogo_mecanismos.md)
- [Catálogo web filtrável](../../website/assets/mechanisms.pt.json)
- [Plano de Upgrade](../../Plano_de_Upgrade_Instrumento_Generativo.md)
- [Continuidade](../../CONTINUIDADE.md)
