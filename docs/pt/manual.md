# Rasgo Synth Studio — Manual

Versão 0.1.0 experimental — 2026

Revisado em 2026-10-12 com base nos controles e contratos atuais. [Tutorial didático](../../website/quickstart.html#pt).

## 1. Conceito

O Rasgo Synth Studio é um instrumento musical generativo: você escolhe condições de composição, e ele sintetiza uma peça inteira. A seed é o ponto de partida; andamento, seleção de módulos, opções de expressão, direção musical e estado pertinente também interferem. Ele não recebe cada nota de um teclado nem funciona como um rack com cabos manuais.

Os sons são sintetizados pelo instrumento, não montados de uma biblioteca externa de samples gravados. Escalas, ritmos, vozes, texturas e forma narrativa integram um espaço de possibilidades autoral. Uma peça pode repetir, transformar, interromper ou deixar material em silêncio; variação não significa novidade garantida em cada evento.

MODO SELVAGEM vem ligado no Studio e acrescenta entropia durante o render. Para comparação controlada, desligue-o e conserve seed, andamento, escolhas, versão e estado externo pertinente. A seed sozinha não garante áudio idêntico.

A duração de uma peça avulsa emerge da narrativa. No álbum, você pode manter essa duração emergente ou ativar um planejamento de duração. O Studio primeiro compõe/renderiza e depois reproduz; seus controles de escuta não transformam o WAV salvo em tempo real. Studio e Performance são instrumentos separados, com publicações independentes.

## 2. História

Rasgo Synth é uma obra dentro de Rasgo, projeto-guarda-chuva de Lucio de Araujo para seus instrumentos e softwares musicais autorais. Antitotem antecede o Rasgo e depois foi incorporado como seu precursor e patrono. Os módulos físicos do Rasgo já eram concebidos visando ao padrão Eurorack, desde 2013; hoje o nome abrange um projeto de instrumentos musicais mais amplo.

Kosmische é um instrumento Android GPLv3 separado, de Nicolas Steven Miller. Foi uma das referências conceituais estudadas no início do Rasgo Synth. Não integra o Rasgo, e o Rasgo Synth não é um fork nem sua continuação autoral.

A implementação específica do projeto foi reescrita do zero em C++17 e JUCE, sob direção de Lucio de Araujo, com o Anthropic Claude usado como ferramenta de desenvolvimento. Nenhum código-fonte do Kosmische ou do Antitotem foi copiado. O instrumento recebeu posteriormente muitos sistemas, vozes e mecanismos composicionais além dessas referências iniciais -- da última contagem, mais de oitenta fases de desenvolvimento documentadas.

## 3. Pesquisa e proveniência

Os nove mapas caóticos são implementações independentes de recorrências matemáticas publicadas. Kosmische e os Chaos UGens do SuperCollider, criados por Lance Putnam, foram examinados como contexto técnico; o livro *The Chaos Cookbook* é de Joe Pritchard, publicado em 1992. Essas referências não tornam as implementações do Rasgo portes de código-fonte.

Os instrumentos de hardware e software citados pelo Rasgo são referências de projeto, não reivindicações de autoria ou emulação exata. O ressoador modal, a textura granular, as fontes aleatórias, os filtros e os módulos inspirados em circuitos são interpretações comportamentais originais baseadas em conceitos públicos, literatura, documentação e esquemas. Os criadores e colaboradores históricos são nomeados em [CREDITS.md](../../CREDITS.md).

As oito vozes de percussão são estilizações digitais originais, inspiradas em parte pelo comportamento documentado do Roland TR-808. Não usam samples, não copiam firmware e não são réplicas de circuitos em nível de transistores. Cada peça escolhe entre oito arquétipos rítmicos possíveis: Soul, Jazz, Metal, Marcha, Samba, dois arquétipos aksak balcânicos reais (documentados por Brăiloiu e Stoyan Dzhudzhev), e uma peneira rítmica gerada algoritmicamente por teoria de Xenakis.

A lista autoritativa de proveniência está em [CREDITS.md](../../CREDITS.md). Bibliotecas incorporadas e seus termos de licença são listados separadamente em [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## 4. Referências selecionadas

- Lucio de Araujo: Rasgo e Antitotem.
- Nicolas Steven Miller: Kosmische (GPLv3), referência conceitual externa.
- Joe Pritchard: *The Chaos Cookbook* (1992).
- Lance Putnam: Chaos UGens do SuperCollider (2004).
- Forrest M. Mims III: Stepped-Tone Generator, popularmente conhecido como Atari Punk Console.
- Iannis Xenakis: síntese estocástica dinâmica e teoria de peneiras (*Formalized Music*, cap. XI).
- Instruo, Ben "DivKid" Wilson (ochd) e Jason Lim (Harmonàig).
- Qu-Bit Electronix, Andrew Ikenberry: Bloom.
- Emilie Gillet / Mutable Instruments: conceitos publicados de DSP Eurorack.
- Roland Corporation: documentação do TR-808 e referência de desenho sonoro.
- Roger Caillois: *Les Jeux et les Hommes* (1958), as quatro categorias de jogo (Alea/Agon/Ilinx/Mimicry).
- Brăiloiu, Stoyan Dzhudzhev: teoria do ritmo aditivo/aksak balcânico.
- Os demais criadores listados em [CREDITS.md](../../CREDITS.md).

## 5. Licença

O código autoral do Rasgo Synth Studio é distribuído sob a GNU Affero General Public License versão 3 ou posterior (AGPL-3.0-or-later); consulte `LICENSE`. A licença GPLv3 do Kosmische se aplica ao Kosmische e não estabelece autoria compartilhada nem cópia de código.

O aplicativo desktop usa JUCE. O código local do JUCE 9 oferece AGPLv3 ou uma licença comercial JUCE; portanto, quem distribuir uma compilação combinada deve cumprir os termos aplicáveis do JUCE além da licença própria do Rasgo Synth. Outros componentes incorporados ou vinculados mantêm suas respectivas licenças; consulte [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## 6. Tutorial

### Antes de começar

Este guia descreve o Studio 0.1.0 experimental, não o Rasgo Modular ou o Performance. Use volume de escuta baixo, uma pasta com permissão de escrita e uma peça por vez. O ícone de colcheia ♪ é o botão de composição; PLAY reproduz o áudio já gerado. Os nomes abaixo correspondem à interface em português.

### Sua primeira peça — seis passos

1. Confira a saída de áudio, abaixe MASTER e escolha a pasta de saída. Preencha título e artista se desejar; os arquivos serão gravados nessa pasta.
2. Defina SEED e TEMPO. Desligue MODO SELVAGEM para o primeiro exercício. Deixe DIREÇÃO MUSICAL e EXPRESSIVO desligados e módulos em Automático como referência inicial.
3. Aperte COMPOR (♪). Aguarde o render e a confirmação de gravação; não é necessário apertar PLAY para começar a composição.
4. Aperte PLAY. Ouça primeiro sem mudar nada: perceba entradas, saídas, pausas, registro grave, percussão e texturas.
5. Arraste na forma de onda para selecionar uma região e use TOCAR TRECHO. SEED DO TRECHO deriva outra origem; não exporta um sample. Componha de novo para ouvi-la.
6. Preserve o WAV e os arquivos acompanhantes. Use ALEA para outra seed ou siga os exercícios abaixo mudando uma escolha de cada vez.

### Seed e andamento

O que é: SEED escolhe um ponto de partida; TEMPO define o andamento inicial da peça.

Como pensar: a seed é uma condição de composição, não o nome de um preset que explica tudo. O motor pode transformar o andamento durante a peça. A escala é escolhida pelo motor; não há um seletor manual de escala neste painel.

Controles: SEED, ALEA, TEMPO e MODO SELVAGEM. No modo álbum, o tema fornece o BPM-base; o slider da peça fica desabilitado.

Experimente: anote a seed e o tempo, gere uma peça com MODO SELVAGEM desligado e escute. Mantenha a seed, mude apenas TEMPO e gere novamente. Compare duração dos gestos, espaçamento e percepção de energia; não espere apenas uma mudança de velocidade no mesmo WAV.

### Escolher quem pode tocar

O que é: ESCOLHER MÓDULOS abre 34 participantes entre fontes, texturas e bateria.

Como pensar: você escolhe elegibilidade, não um solo ou mute do áudio atual. Incluir autoriza participação; entradas, notas, pausas e dependências ainda determinam a execução. Outros mecanismos e efeitos continuam automáticos. A grade de atividade não é essa lista de controles.

Controles: Automático preserva a escolha generativa; Incluir permite a fonte designada; Excluir a bloqueia. Confirme em APLICAR AO PRÓXIMO RENDER. Fechar sem aplicar descarta o rascunho; TODOS AUTOMÁTICOS redefine o rascunho antes da confirmação. Reiniciar o app redefine as escolhas.

Experimente: mantenha seed, tempo e modos. Primeiro inclua Camada de ruído colorido; aplique e gere. Depois exclua essa mesma camada e gere de novo. Compare textura e espaço ocupado pelo ruído. Outros geradores ainda podem produzir ruído: Excluir não promete silêncio de toda uma família sonora.

### Bateria e groove

O que é: o conjunto reúne bumbo, caixa, chimbal, cowbell, palmas, toms, prato e maracas, sintetizados em vez de samples de bateria.

Como pensar: os participantes dividem função rítmica, mas não têm knobs individuais de afinação ou volume no Studio. Excluir Conjunto de bateria bloqueia suas peças, mesmo se você marcou um bumbo como Incluir. Incluir um participante não obriga ataques constantes.

Controles: ESCOLHER MÓDULOS; para explorar os estudos adicionais de groove e interpretação, ligue DIREÇÃO MUSICAL e EXPRESSIVO antes de compor. Famílias e respostas são escolhidas pela seed e pelas seções; não há seletor manual de gênero ou polirritmia 3:2.

Experimente: inclua Conjunto de bateria, Bumbo, Caixa e Chimbal e gere. Mantendo tudo igual, exclua Cowbell e Maracas e gere novamente. Observe se a pulsação fica mais legível. Não aumente MASTER para compensar apenas uma peça: o desequilíbrio da mixagem é uma limitação conhecida, não um erro seu.

### Registro grave e linha de baixo

O que é: Baixo acid e Tuba são fontes selecionáveis; o grave core-sub é uma função interna distinta da Voz principal.

Como pensar: grave, baixo musical e volume são coisas diferentes. Uma linha de baixo pede eventos, registro e articulação; o nome do módulo não garante presença clara na mixagem. O fallback core-sub não tem item próprio no seletor.

Controles: seleção de Baixo acid/Tuba e DIREÇÃO MUSICAL + EXPRESSIVO. Não existe aqui um fader individual de baixo ou um botão de solo.

Experimente: com os dois modos ligados, inclua Baixo acid e exclua Tuba; aplique e gere. Depois faça a escolha inversa. Escute ataque, sustentação, pausas e relação com o bumbo. Excluir ambas pode deixar o fallback disponível conforme o plano musical; não garante que você o reconhecerá. Se o grave estiver mascarado, registre seed e escolhas para revisão.

### Melodia, timbre e relações

O que é: vozes como Flauta FM, Theremin, Coral e texturas como Granular oferecem matérias sonoras diferentes.

Como pensar: selecionar não equivale a girar knobs de um rack. As trajetórias são calculadas no render. Coral, Scat e Harmonaig dependem da progressão de acordes gerada; uma dependência ausente pode impedir a participação e aparecer como `blockedBy` nos metadados.

Controles: ESCOLHER MÓDULOS, DIREÇÃO MUSICAL e EXPRESSIVO. Direção organiza entradas, retiradas e relações; Expressivo acrescenta interpretação experimental. São opções distintas, desligadas por padrão, e precisam de novo render.

Experimente: gere uma referência com os dois modos desligados. Ligue somente Direção e gere; depois ligue também Expressivo e gere. Conserve seed, tempo, módulos e Auditoria Stems. Compare pausas, respostas, ataques e timbre, não apenas volume. O contraste pode ser discreto; não há promessa de maior emoção ou diálogo audível em toda peça.

### Escuta e visualizações

O que é: forma de onda, espectro, espectrograma e outros visores ajudam a observar o áudio.

Como pensar: um indicador de módulo mostra atividade, não uma prova de audibilidade ou qualidade musical. As animações do website são ilustrações, não áudio do seu aplicativo.

Controles: PLAY, posição na forma de onda, TOCAR TRECHO, MASTER e brilho de escuta. Estes controles não reescrevem o WAV gravado. Em janelas menores, use a rolagem vertical para alcançar log e visores.

Experimente: ouça um trecho e relacione um ataque da percussão com a forma de onda; depois procure uma textura sustentada no espectro. Um desenho com amplitude pequena não prova, sozinho, que o arquivo esteja mal masterizado. Se não identificar uma voz, registre o caso em vez de aumentar o volume sem limite.

### Seu primeiro álbum

O que é: MODO ÁLBUM cria uma coleção de faixas a partir da seed, do tema e do planejamento.

Como pensar: o tema fornece o BPM-base; o planejamento muda os próximos renders, não faixas já salvas. Não há ainda memória composicional que reaprenda temas entre faixas.

Controles: tema, FAIXAS (1–40; padrão 8), VARIAR BPM (desligado por padrão; amplitude 0–35%, padrão 15%), percurso equilibrado/contrastante/crescente/arco. PLANEJAR DURAÇÃO também começa desligado. Quando ligado, permite 1–45 minutos de alvo total e distribuição equilibrada/contrastante/livre. O plano limita cada faixa a 15 segundos–8 minutos e o total a 45 minutos; a prévia distingue total solicitado e planejado quando o pedido não cabe.

Experimente: escolha 2 faixas, ative PLANEJAR DURAÇÃO e peça 2 minutos no total. Confira a prévia; ligue VARIAR BPM para comparar andamentos iniciais. Componha o álbum e escute pelo seletor OUVIR. PLAY retorna aos visores; CONFIGURAR ÁLBUM reabre as opções. O tempo musical planejado não é o tempo de processamento nem um corte posterior do WAV.

O álbum usa calibrações antes dos renders finais. `album_plan.json` registra planejamento e arquivos efetivos; álbuns interrompidos não equivalem a álbuns concluídos. A seleção de módulos e os modos expressivos valem para todas as faixas seguintes. Auditoria Stems é herdada da opção escolhida no modo peça, embora o controle fique desabilitado no modo álbum: defina-a antes de entrar. Sua dica de tela antiga ainda não descreve corretamente essa herança.

### Progresso, arquivos e problemas

O que é: o percentual acompanha frames musicais da passagem atual, não o tempo real decorrido.

Como pensar: 100% de composição ainda pode ser seguido de finalização e gravação. No álbum, percentual e calibração não representam uma estimativa única do tempo restante do conjunto. Auditoria Stems acrescenta análise e processamento e pode alterar o resultado, não somente a duração do render.

Controles: log, COPIAR, pasta de saída, SOBRE e TUTORIAL. SOBRE mostra versão e data/hora da build. Ao fechar durante um render, o app solicita cancelamento e aguarda um checkpoint; uma gravação em curso pode precisar terminar. Isso não garante recuperação após crash ou falta de energia.

Experimente: após a primeira peça, encontre o WAV e seus acompanhantes na pasta. Se algo falhar, copie o log e anote versão, sistema, dispositivo, seed, modos, seleção e ação anterior à falha. Não envie gravações pessoais automaticamente. Evite renders/builds paralelos se a máquina estiver sob pressão de memória.

### Limitações desta versão

A publicação experimental não significa mixagem aprovada: o baixo pode ficar discreto, algumas percussões sobressaem e timbres podem se repetir. A mixagem generativa e a diversidade interpretativa seguirão em desenvolvimento após a publicação. Noise, silêncio e ruptura são possibilidades, não obrigação estética. A revisão das instruções não substitui nova validação de escuta, de acessibilidade ou de todos os teclados/idiomas.

[Guia integral](Instrumento_Integral.md) · [Créditos](../../CREDITS.md) · [Método rítmico e referências](../GROOVE_RHYTHM.md)
