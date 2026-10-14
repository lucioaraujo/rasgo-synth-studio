# Groove, swing e malandragem rítmica — Studio

Atualizado: 2026-10-09. Direção autoral: Lucio de Araujo.

Candidato técnico atual: `linux-20261009-diagnostics.6e1Xp8`, com piso de
sample peak no detector e método/alvo/teto explícitos na metadata. Peça exportou
e fechou normalmente;10 CTests/29 Python/integração de gravação passaram.
Não é incremento musical de groove, nem aceitação por escuta. O comparativo
de medidores confirmou a diferença de reconstrução e o teto público−1 dBTP
na amostra;−1,8 é margem interna. Álbum/playback desta build ainda abertos.
Evidências abaixo pertencem à coorte v2 precedente; detalhes em continuidade.

Estado: v2 testada no core e compilada no candidato `linux-20261009-groove-v2-app`.
Teste puro128 seeds/6 famílias, contraste/repetição/bypass, álbum/calibração
e8 CTests focados passaram. O contraste isolado v2 inclui um disparo extra
real na seed4/24 s; seeds1/3 sem extras conservaram PCM idêntico.
Idle, duas peças após estabilização, álbum e reprodução silenciada fecharam
normalmente;8 WAVs v2 decodificados. Uma tentativa de fechar imediatamente
após render falhou, sem registro de chegada do pedido. Causa não isolada;
não declarar correção pela repetição bem-sucedida. Confronto dos estimadores
de true peak também pendente (seed4 interno−1,8/FFmpeg−1,6 dBTP).
Os smokes completos e6 WAVs validados anteriormente pertencem ao app v1,
não certificam v2. Evidências/hash e cobertura final em CONTINUIDADE.
Não é aceitação por escuta.

## Respostas subdivididas — v2, fonte de 8 out. 2026

Estudo do Antitotem confirmou groove, subdivisões e microeventos já ligados
ao áudio. Consulta somente leitura: `src/core/SimpleSequencer.cpp`, commit
`eabde9f5cbadf7b1ba3254354002cb5e723cba40`, arquivo limpo, SHA-256
`dc3dc4fdfb097c22d3451062c33d909ee420d9c3599d961ba8950e3beaaefec5`.
Autoria: Lucio de Araujo; fonte sob AGPL-3.0. Referência interna de ratchets
2–4 com decrescendo, não port, cópia de código ou dependência de build.
`GrooveRepeat.hpp` é implementação própria do Studio. Antitotem não alterado.

Alguns últimos compassos de frases de quatro compassos recebem uma resposta
no último passo: uma única voz, caixa OU chimbal, divide aquele passo em
2/3/4 ataques conforme densidade (>0,45 / >0,60 / >0,80). Mantém o primeiro
ataque/ganho; os seguintes têm decrescendo relativo ao ataque inicial.
O bumbo e o imprint do baixo não ganham repetições obrigatórias. Chimbal
repetido é fechado. A mesma seed/contexto produz a mesma decisão, sem RNG
novo compartilhado. Não é rolo contínuo, novo gênero ou polirritmia3:2:
ter três ataques dentro de um passo não cria duas pulsações simultâneas.

Dois estados fixos, no máximo quatro ataques por grupo, sem fila/alocação.
Agendamento usa a janela que resta APÓS o atraso expressivo do ataque,
na duração efetiva do passo Rasgar. Exige intervalo mínimo6 ms; reduz o número
de ataques se não couberem, ou conserva apenas o primeiro. Posições absolutas
arredondadas evitam acumular erro das divisões por três. Não altera BPM,
relógio técnico ou duração do compasso. Nova cabeça substitui grupo anterior.

`PieceConfig/AlbumConfig::grooveSubdivisionsEnabled=false` isola as respostas
sem desligar a primeira coorte de groove. Flag propagada a calibração/final
e manifesto; ainda depende de Expressivo + Direção + grooveDialogue.
Sem seletor manual novo nem alteração de layout. JSON/partitura versão2
separam planos extras solicitados (antes dos limites) de disparos extras
efetivamente realizados. Log4 idiomas mostra disparos, não audibilidade.
Um plano pode não soar por espaço temporal, corte do render ou decisão de voz.
Os microeventos não têm registros individuais no ledger expressivo nesta
coorte; o ledger anterior descreve a cabeça do grupo, não todos os ataques.

Testes puros ampliados: decisões128 seeds, bypass específico, contagem2/3/4,
fim de frase, uma voz por resposta, espaçamento, ganho inicial, arredondamento,
limites e substituição. Integrador opt-in ganhou modo `subdivisions` para
repetição PCM/contrafactual, além da propagação de álbum. Executados em09/10:
seed4 teve1 plano/1 extra, RMS de diferença0,00106032; seeds1/3 sem extras
foram exatamente neutras. Exclusão caixa+chimbal e bypass também passaram.
PieceInfo expõe planos além de disparos, sem mudança do DSP. Um probe de6
seeds explicou a falta de cobertura no teste inicial de12 s; o teste aceito
usa24 s, sem forçar ataques ou densidade. Não exige resposta em toda faixa.
Álbum2 faixas confirmou flags e correspondência calibração/final; não prova
audibilidade. Novo app compilado com JUCE9.0.0 fixado; runtime segue aberto.

Próximo: resolver o fechamento pós-render, concluir smokes/decodificação do
candidato e escuta em volume equivalente. Presença perceptiva do baixo
continua uma investigação própria, não resolvida por subdivisões de bateria.
Polirritmia sincronizada, intensidade manual e agrupamentos permanecem abertos.

## Diretriz musical

Groove não se reduz a jitter ou a atrasar todos os instrumentos igualmente.
O instrumento deve relacionar pulso, síncope, acento, duração, silêncio,
antecipação, sustentação e resposta. Uma frase precisa conservar identidade
para poder ser reconhecida, contrariada e retomada. A malandragem solicitada
pelo autor orienta esse jogo entre expectativa e desvio, não uma caricatura
cultural nem uma categoria de ruído aleatório.

As culturas musicais, especialmente as afro-diaspóricas, não são redutíveis a
uma tabela de passos. Jazz, samba, choro, funk e outras práticas devem ter
pesquisa e crédito próprios quando houver estudo específico; nomes de gênero
não certificam uma emulação, pertencimento ou competência de performance.

## Primeira coorte: GrooveDialogue

Código C++ original do Studio, seis estudos, sem samples, transcrição ou código
externo incorporado. Não amplia o catálogo legado de sete DrumArchetypes:
com a experiência ligada, substitui a fonte primária de bumbo/caixa/chimbal
nos compassos de16 passos. Outros compassos seguem o caminho precedente.
Outras cinco vozes de percussão e FM ainda não recebem esta nova gramática.

| Estudo | Organização nesta implementação |
|---|---|
| Swing de semicolcheias | células longo-curto de semicolcheias; deslocamento suavemente menor em BPM alto; não é swing de colcheias do jazz |
| Backbeat relaxado | caixa atrasada em relação ao bumbo, aproximação de caixa mais leve e baixo com outro posicionamento |
| Síncope entrelaçada | ataques em posições fracas, silêncio complementar e baixo com offsets assinados |
| 3–3–2 | âncora em agrupamento de colcheias3+3+2, acentos distribuídos; não é transcrição de clave |
| Acentos deslocados | posições da caixa deslocadas, contraposição com baixo e acentos distintos |
| Resposta quebrada | célula mais espaçada, resposta leve e condicionada no último compasso da frase |

Seed escolhe uma família inicial. Seções introduzem uma família diferente;
compassos repetidos conservam essa identidade. A resposta final ocorre em
parte dos grupos de quatro compassos e depende da densidade; não é fill
obrigatório. Não há seletor manual de família nem memória entre faixas.

Baixo recebe TemporalGrooveImprint com acento, duração, ataque e offsets
próprios. O planner de frase continua decidindo quais notas/pausas existem;
a experiência não força uma linha contínua. Antecipações reais do baixo usam
o lookahead existente, com clamps no início/emendas registrados anteriormente.
Bateria mantém atrasos causais positivos: seus pickups/síncopes são posições
na grade, não antecipações sub-grade. Lookahead de percussão continua pendente.

Swing legado não é somado ao novo swing. Para as três vozes primárias, o
sorteio legado de humanização continua sendo consumido para preservar o fluxo
RNG, mas seu jitter independente não é aplicado. Gestos de frase, Rasgar,
relações, elegibilidade e mixagem permanecem ativos; não é isolamento de todas
as causas do ritmo. Não tenta corrigir aqui a presença tímbrica do baixo.

## Uso e auditoria

Na nova build que contiver esta fonte: ativar EXPRESSIVO e DIREÇÃO MUSICAL,
selecionar Bateria/Bumbo/Caixa/Chimbal em Incluir e gerar de novo. Incluir é
elegibilidade, não som contínuo. Com Acid/tuba excluídos, o baixo funcional usa
core-sub nos planos não silenciosos. Áudios anteriores não são alterados.

`PieceConfig` e `AlbumConfig::grooveDialogueEnabled=false` fornecem o
contrafactual específico. Desligar Direção ou Expressivo também desliga esta
coorte, mas compara mais camadas. Calibração/final do álbum recebem a mesma
opção. Ajuda/log são traduzidos nos quatro idiomas, sem controles menores ou
mudança na geometria do painel.

WAV/partitura registram versão, família final, compassos por família, mudanças,
planos de ataques/ghosts e imprints do baixo, além de compassos não suportados.
Log mostra compassos e planos. Contagens são de planejamento, não prova de
ataque reproduzido, presença no mix ou groove percebido. Dados do archetype
legado identificam a fonte selecionada anteriormente; a nova coorte pode
substituí-la nos compassos suportados. Não deduzir fonte ouvida só desse nome.

Aceitação: determinismo, limites e bypass; variedade real das seis células;
contraste no PCM final; peças/álbum com bateria presente; calibração/final;
escuta em volumes equivalentes para sentir pulso e diálogo. BPM, padrão,
acento e timbre precisam ser considerados juntos. Ganho, presença do baixo,
articulação das demais vozes, compasso variável e repertório cultural mais
profundo continuam como próximas frentes, não tarefas concluídas.

## Referências e limites de derivação

- Friberg e Sundström, 2002: [Ensemble Swing — KTH](https://www.speech.kth.se/music/performance/Texts/ensemble_swing.htm).
  Referência para relações diferentes de timing entre intérpretes; não foram
  copiadas regressões, exemplos sonoros ou tabelas de execução.
- Naveda, Gouyon, Guedes e Leman, 2011: [Microtiming Patterns and Interactions
  with Musical Properties in Samba Music — INESC](https://repositorio.inesctec.pt/server/api/core/bitstreams/06ca3366-5cfb-44a1-8b1d-32550456e1ac/content).
  Fundamenta pesquisa futura multidimensional de timing, intensidade e
  registro; a coorte atual não reproduz os microtimings medidos do samba.
- Witek et al., 2014: [Syncopation, Body-Movement and Pleasure in Groove Music](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0094446).
  Referência sobre síncope e experiência corporal; não garante prazer em uma
  saída do Studio nem define um nível universal de complexidade desejável.
- Datseris et al., 2019: [Does it Swing?](https://arxiv.org/abs/1904.03442).
  Motiva cautela com a ideia de que mais desvios necessariamente melhoram
  swing. Os limites locais são escolhas autorais, não parâmetros validados
  por esse estudo ou uma simulação da execução dos participantes.

Pesquisa online de08/10/2026. PDFs/áudios externos não foram incorporados nem
redistribuídos; nenhum motor de outro instrumento RASGO foi alterado.
