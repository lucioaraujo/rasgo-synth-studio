// Rasgo Synth Studio — local UI vocabulary.
//
// Same RASGO convention used by Navalha 2 and Antitotem: English is the
// default, with Portuguese, French and Spanish available at runtime.

#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <utility>

namespace rasgo::ui {

enum class Language { english = 0, portuguese, french, spanish };

struct LocalizedText { const char* en; const char* pt; const char* fr; const char* es; };

inline int languageIndex(Language value) noexcept { return static_cast<int>(value); }

inline Language languageFromCode(const juce::String& code) noexcept {
    if (code.equalsIgnoreCase("pt")) return Language::portuguese;
    if (code.equalsIgnoreCase("fr")) return Language::french;
    if (code.equalsIgnoreCase("es")) return Language::spanish;
    return Language::english;
}

inline juce::String languageCode(Language value) {
    constexpr std::array<const char*, 4> codes { "en", "pt", "fr", "es" };
    return codes[static_cast<std::size_t>(languageIndex(value))];
}

inline juce::String languageLabel(Language value) {
    constexpr std::array<const char*, 4> labels { "EN", "PT", "FR", "ES" };
    return labels[static_cast<std::size_t>(languageIndex(value))];
}

inline Language nextLanguage(Language value) noexcept {
    return static_cast<Language>((languageIndex(value) + 1) % 4);
}

inline juce::String text(const LocalizedText& value, Language language) {
    const std::array<const char*, 4> options { value.en, value.pt, value.fr, value.es };
    return juce::String::fromUTF8(options[static_cast<std::size_t>(languageIndex(language))]);
}

namespace strings {
inline const LocalizedText chooseModules { "CHOOSE MODULES", "ESCOLHER MÓDULOS", "CHOISIR LES MODULES", "ELEGIR MÓDULOS" };
inline const LocalizedText moduleAutomatic { "Automatic", "Automático", "Automatique", "Automático" };
inline const LocalizedText moduleInclude { "Include", "Incluir", "Inclure", "Incluir" };
inline const LocalizedText moduleExclude { "Exclude", "Excluir", "Exclure", "Excluir" };
inline const LocalizedText moduleApply { "APPLY TO NEXT RENDER", "APLICAR AO PRÓXIMO RENDER", "APPLIQUER AU PROCHAIN RENDU", "APLICAR AL PRÓXIMO RENDER" };
inline const LocalizedText moduleReset { "ALL AUTOMATIC", "TODOS AUTOMÁTICOS", "TOUS AUTOMATIQUES", "TODOS AUTOMÁTICOS" };
inline const LocalizedText moduleSources { "Sound sources", "Fontes sonoras", "Sources sonores", "Fuentes sonoras" };
inline const LocalizedText moduleTextures { "Textures", "Texturas", "Textures", "Texturas" };
inline const LocalizedText modulePercussion { "Percussion", "Percussão", "Percussion", "Percusión" };
inline const LocalizedText moduleSelectionHelp {
    "34 selectable participants. Other mechanisms stay automatic. Choices apply before the next piece or every album track, not to the current WAV. Include permits participation, not continuous playing. Excluded drums block kit parts; chord-dependent voices may be blocked (recorded in metadata).",
    "34 participantes selecionáveis. Outros mecanismos continuam automáticos. Escolhas valem antes da próxima peça ou de cada faixa do álbum, não no WAV atual. Incluir permite participação, não execução contínua. Bateria excluída bloqueia suas peças; vozes que dependem de acordes podem ser bloqueadas (registro nos metadados).",
    "34 participants sélectionnables. Les autres mécanismes restent automatiques. Choix appliqués au prochain morceau ou à chaque piste, pas au WAV actuel. Inclure autorise la participation, sans jeu continu. Batterie exclue : éléments bloqués ; voix dépendantes d’accords parfois bloquées (métadonnées).",
    "34 participantes seleccionables. Otros mecanismos siguen automáticos. Elecciones para la próxima pieza o cada pista, no el WAV actual. Incluir permite participación, no ejecución continua. Batería excluida bloquea sus piezas; voces que necesitan acordes pueden quedar bloqueadas (metadatos)."
};
inline const LocalizedText albumConfigure { "CONFIGURE ALBUM", "CONFIGURAR ÁLBUM", "CONFIGURER L’ALBUM", "CONFIGURAR ÁLBUM" };
inline const LocalizedText albumListenView { "LISTENING VIEW", "VISUALIZAR ESCUTA", "VUE D’ÉCOUTE", "VISTA DE ESCUCHA" };
inline const LocalizedText albumPlanTitle { "ALBUM — TEMPO AND DURATION", "ÁLBUM — ANDAMENTO E DURAÇÃO", "ALBUM — TEMPO ET DURÉE", "ÁLBUM — TEMPO Y DURACIÓN" };
inline const LocalizedText albumTempoTitle { "Tempo between tracks", "Andamento entre faixas", "Tempo entre les pistes", "Tempo entre pistas" };
inline const LocalizedText albumVaryBpm { "VARY BPM", "VARIAR BPM", "VARIER LE BPM", "VARIAR BPM" };
inline const LocalizedText albumBpmSpread { "Variation around theme BPM", "Variação ao redor do BPM do tema", "Variation autour du BPM du thème", "Variación alrededor del BPM del tema" };
inline const LocalizedText albumTempoContour { "Tempo trajectory", "Percurso do andamento", "Trajectoire du tempo", "Trayectoria del tempo" };
inline const LocalizedText albumBalanced { "Balanced", "Equilibrada", "Équilibrée", "Equilibrada" };
inline const LocalizedText albumContrasting { "Contrasting", "Contrastante", "Contrastée", "Contrastante" };
inline const LocalizedText albumRising { "Rising", "Crescente", "Croissante", "Creciente" };
inline const LocalizedText albumArch { "Arch", "Arco", "Arc", "Arco" };
inline const LocalizedText albumFree { "Free", "Livre", "Libre", "Libre" };
inline const LocalizedText albumDurationTitle { "Album and track durations", "Duração do álbum e das faixas", "Durée de l’album et des pistes", "Duración del álbum y de las pistas" };
inline const LocalizedText albumDurationEnabled { "PLAN DURATION", "PLANEJAR DURAÇÃO", "PLANIFIER LA DURÉE", "PLANIFICAR DURACIÓN" };
inline const LocalizedText albumDurationTarget { "Album target (minutes)", "Alvo do álbum (minutos)", "Objectif de l’album (minutes)", "Objetivo del álbum (minutos)" };
inline const LocalizedText albumDurationDistribution { "Track duration distribution", "Distribuição da duração das faixas", "Répartition des durées des pistes", "Distribución de duraciones de pistas" };
inline const LocalizedText albumPlannedTotal { "Planned total", "Total planejado", "Total prévu", "Total previsto" };
inline const LocalizedText albumRequestedTotal { "Requested total", "Total solicitado", "Total demandé", "Total solicitado" };
inline const LocalizedText albumEmergentDuration { "Emergent duration", "Duração emergente", "Durée émergente", "Duración emergente" };
inline const LocalizedText albumRenderFailed { "Album generation failed", "Falha ao gerar o álbum", "Échec de la génération de l’album", "Error al generar el álbum" };
inline const LocalizedText pieceRenderFailed { "Piece generation failed", "Falha ao gerar a peça", "Échec de la génération de la pièce", "Error al generar la pieza" };
inline const LocalizedText musicalDirection { "MUSICAL DIRECTION", "DIREÇÃO MUSICAL", "DIRECTION MUSICALE", "DIRECCIÓN MUSICAL" };
inline const LocalizedText grooveSummary { "Original groove studies — bar counts", "Estudos autorais de groove — contagem de compassos", "Études originales de groove — nombre de mesures", "Estudios originales de groove — número de compases" };
inline const LocalizedText groovePlans { "Hit plans kick/snare/hat; bass imprints", "Planos bumbo/caixa/chimbal; interpretações do baixo", "Plans grosse caisse/caisse claire/charleston ; empreintes de basse", "Planes bombo/caja/charles; interpretaciones del bajo" };
inline const LocalizedText grooveExtraTriggers { "Extra response triggers kick/snare/hat (not an audibility measure)", "Disparos extras de resposta bumbo/caixa/chimbal (não medem audibilidade)", "Déclenchements supplémentaires grosse caisse/caisse claire/charleston (pas une mesure d’audibilité)", "Disparos adicionales bombo/caja/charles (no miden audibilidad)" };
inline const LocalizedText grooveSwing { "Sixteenth swing", "Swing de semicolcheias", "Swing de doubles croches", "Swing de semicorcheas" };
inline const LocalizedText grooveRelaxed { "Relaxed backbeat", "Backbeat relaxado", "Backbeat détendu", "Backbeat relajado" };
inline const LocalizedText grooveInterlock { "Interlocking offbeats", "Síncope entrelaçada", "Contretemps entrelacés", "Contratiempos entrelazados" };
inline const LocalizedText grooveThree { "3–3–2", "3–3–2", "3–3–2", "3–3–2" };
inline const LocalizedText grooveDisplaced { "Displaced accents", "Acentos deslocados", "Accents déplacés", "Acentos desplazados" };
inline const LocalizedText grooveBroken { "Broken answer", "Resposta quebrada", "Réponse brisée", "Respuesta quebrada" };
inline const LocalizedText musicalDirectionHelp {
    "Optional in pieces/albums: arrangement and kit/bass/FM relations. With EXPRESSIVE, adds functional bass and six original groove studies: sixteenth swing, relaxed backbeat, interlocking offbeats, 3-3-2, displaced accents and broken answers. Some phrase endings use 2-4 decreasing snare or hat attacks, depending on density; this is not a polyrhythm selector. Applies on 16-step bars; include drums to hear the kit. Seed/sections choose families, not a manual style selector. Next render only; listening acceptance required.",
    "Opcional em peça/álbum: arranjo e relações kit/baixo/FM. Com EXPRESSIVO, acrescenta baixo funcional e seis estudos autorais de groove: swing de semicolcheias, backbeat relaxado, síncope entrelaçada, 3-3-2, acentos deslocados e respostas quebradas. Alguns finais de frase usam 2-4 ataques decrescentes de caixa ou chimbal, conforme a densidade; não é seletor de polirritmia. Atua em compassos de16 passos; inclua bateria para ouvir o kit. Seed/seções escolhem famílias, sem seletor manual de estilo. Próximo render; exige escuta.",
    "Facultatif en pièce/album : arrangement et relations batterie/basse/FM. Avec EXPRESSIF, ajoute basse fonctionnelle et six études originales de groove : swing de doubles croches, backbeat détendu, contretemps entrelacés, 3-3-2, accents déplacés et réponses brisées. Certaines fins de phrase utilisent 2-4 attaques décroissantes de caisse claire ou charleston selon la densité ; ce n’est pas un sélecteur de polyrythmie. Mesures de16 pas ; incluez la batterie pour l’entendre. Familles choisies par seed/sections, sans sélecteur manuel de style. Prochain rendu ; écoute requise.",
    "Opcional en pieza/álbum: arreglo y relaciones batería/bajo/FM. Con EXPRESIVO, añade bajo funcional y seis estudios originales de groove: swing de semicorcheas, backbeat relajado, contratiempos entrelazados, 3-3-2, acentos desplazados y respuestas quebradas. Algunos finales de frase usan 2-4 ataques decrecientes de caja o charles según la densidad; no es un selector de polirritmia. Compases de16 pasos; incluya batería para oír el kit. Seed/secciones eligen familias, sin selector manual de estilo. Próximo render; requiere escucha."
};
inline const LocalizedText albumPlanHelp {
"Options off preserve the existing path. Targets shape musical sections, not a cut at the end. Limits: 15s–8min per track, 45min planned total. Adjusted targets are shown below. PLAY restores listening views; settings remain available. Musical direction is optional; cross-track memory and further compositional controls remain pending.",
    "Opções desligadas preservam o percurso atual. Alvos dimensionam as seções musicais, não cortam o final. Limites: 15s–8min por faixa, 45min no total planejado. Ajustes aparecem abaixo. PLAY restaura a escuta; escolhas permanecem disponíveis. Direção musical é opcional; memória entre faixas e outros controles composicionais continuam pendentes.",
    "Options désactivées : parcours existant préservé. Les objectifs dimensionnent les sections, sans couper la fin. Limites : 15s–8min par piste, 45min au total. Les ajustements figurent ci-dessous. PLAY restaure l’écoute ; les choix restent disponibles. La direction musicale est facultative ; mémoire entre pistes et autres commandes restent à développer.",
    "Opciones apagadas: recorrido actual conservado. Los objetivos dimensionan las secciones, sin cortar el final. Límites: 15s–8min por pista, 45min en total. Los ajustes aparecen abajo. PLAY restaura la escucha; las elecciones se conservan. La dirección musical es opcional; memoria entre pistas y otros controles siguen pendientes."
};
inline const LocalizedText tutorial { "TUTORIAL", "TUTORIAL", "TUTORIEL", "TUTORIAL" };
inline const LocalizedText about { "ABOUT", "SOBRE", "À PROPOS", "ACERCA DE" };
inline const LocalizedText subtitle { "generative instrument — takes risks, then sounds", "instrumento generativo — se arrisca, e soa", "instrument génératif — il prend des risques, puis sonne", "instrumento generativo — se arriesga y suena" };
inline const LocalizedText seed { "SEED", "SEED", "SEED", "SEED" };
inline const LocalizedText randomize { "ALEA", "ALEA", "ALEA", "ALEA" };
inline const LocalizedText tempo { "TEMPO", "TEMPO", "TEMPO", "TEMPO" };
inline const LocalizedText closing {
    "Closing: stopping playback and waiting for the render to stop...",
    "Fechando: interrompendo a reprodução e aguardando o render parar...",
    "Fermeture : arrêt de la lecture et attente de l’arrêt du rendu...",
    "Cerrando: deteniendo la reproducción y esperando que pare el render..."
};
inline const LocalizedText master { "MASTER", "MASTER", "MASTER", "MASTER" };
inline const LocalizedText outputDirectory { "OUTPUT DIRECTORY", "DIRETÓRIO DE SAÍDA", "DOSSIER DE SORTIE", "DIRECTORIO DE SALIDA" };
inline const LocalizedText title { "TITLE", "TÍTULO", "TITRE", "TÍTULO" };
inline const LocalizedText automatic { "(automatic)", "(automático)", "(automatique)", "(automático)" };
inline const LocalizedText artist { "ARTIST", "ARTISTA", "ARTISTE", "ARTISTA" };
inline const LocalizedText artistPlaceholder { "artist name", "nome do artista", "nom de l’artiste", "nombre del artista" };
inline const LocalizedText wildOn { "WILD MODE", "MODO SELVAGEM", "MODE SAUVAGE", "MODO SALVAJE" };
inline const LocalizedText wildOff { "NORMAL MODE", "MODO NORMAL", "MODE NORMAL", "MODO NORMAL" };
inline const LocalizedText wildTip { "On: the seed starts the path, but entropy can open new routes. Off: deterministic, reproducible render.", "Ligado: a seed inicia o percurso, mas a entropia pode abrir novos caminhos. Desligado: render determinístico e reproduzível.", "Actif : la seed lance le parcours, mais l’entropie peut ouvrir de nouvelles voies. Inactif : rendu déterministe et reproductible.", "Activo: la seed inicia el recorrido, pero la entropía puede abrir caminos nuevos. Inactivo: render determinista y reproducible." };
inline const LocalizedText stemAudit { "STEM AUDIT", "AUDITORIA STEMS", "AUDIT DES STEMS", "AUDITORÍA DE STEMS" };
inline const LocalizedText stemAuditTip { "On by default: measures post-space stems and enables narrative attention and limited narrative mixing. Turn off for a faster render.", "Ligado por padrão: mede stems pós-espaço e habilita atenção e mixagem narrativa limitada. Desligue para um render mais rápido.", "Actif par défaut : mesure les stems post-espace et active l’attention et le mixage narratif limité. Désactivez pour un rendu plus rapide.", "Activo por defecto: mide stems posespacio y habilita atención y mezcla narrativa limitada. Apáguelo para un render más rápido." };
inline const LocalizedText expressiveOn { "EXPRESSIVE A/B", "A/B EXPRESSIVO", "A/B EXPRESSIF", "A/B EXPRESIVO" };
inline const LocalizedText expressiveOff { "EXPRESSIVE OFF", "EXPRESSIVO OFF", "EXPRESSIF OFF", "EXPRESIVO OFF" };
inline const LocalizedText expressiveTip { "Experimental A/B: applies temporal memory and articulation to four drum voices. Off by default.", "A/B experimental: aplica memória temporal e articulação a quatro vozes de bateria. Desligado por padrão.", "A/B expérimental : applique mémoire temporelle et articulation à quatre voix de batterie. Désactivé par défaut.", "A/B experimental: aplica memoria temporal y articulación a cuatro voces de batería. Desactivado por defecto." };
inline const LocalizedText albumMode { "ALBUM MODE", "MODO ÁLBUM", "MODE ALBUM", "MODO ÁLBUM" };
inline const LocalizedText theme { "THEME", "TEMA", "THÈME", "TEMA" };
inline const LocalizedText tracks { "TRACKS", "FAIXAS", "PISTES", "PISTAS" };
inline const LocalizedText listen { "LISTEN", "OUVIR", "ÉCOUTER", "ESCUCHAR" };
inline const LocalizedText playRegion { "PLAY REGION", "TOCAR TRECHO", "LIRE LA RÉGION", "REPRODUCIR REGIÓN" };
inline const LocalizedText seedFromRegion { "SEED FROM REGION", "SEED DO TRECHO", "SEED DE LA RÉGION", "SEED DE LA REGIÓN" };
inline const LocalizedText varySeed { "VARY SEED", "VARIAR SEED", "VARIER LA SEED", "VARIAR SEED" };
inline const LocalizedText liveBrightness { "LIVE BRIGHTNESS", "BRILHO AO VIVO", "BRILLANCE EN DIRECT", "BRILLO EN VIVO" };
inline const LocalizedText waveform { "WAVEFORM", "FORMA DE ONDA", "FORME D’ONDE", "FORMA DE ONDA" };
inline const LocalizedText scope { "OSCILLOSCOPE (STEREO)", "OSCILOSCÓPIO (ESTÉREO)", "OSCILLOSCOPE (STÉRÉO)", "OSCILOSCOPIO (ESTÉREO)" };
inline const LocalizedText goniometer { "GONIOMETER", "GONIÔMETRO", "GONIOMÈTRE", "GONIÓMETRO" };
inline const LocalizedText phasePortrait { "PHASE PORTRAIT", "RETRATO DE FASE", "PORTRAIT DE PHASE", "RETRATO DE FASE" };
inline const LocalizedText spectrum { "SPECTRUM", "ESPECTRO", "SPECTRE", "ESPECTRO" };
inline const LocalizedText level { "LEVEL", "NÍVEL", "NIVEAU", "NIVEL" };
inline const LocalizedText granulation { "GRANULATION", "GRANULAÇÃO", "GRANULATION", "GRANULACIÓN" };
inline const LocalizedText genealogy { "ARC GENEALOGY", "GENEALOGIA DO ARCO", "GÉNÉALOGIE DE L’ARC", "GENEALOGÍA DEL ARCO" };
inline const LocalizedText spectrogram { "SPECTROGRAM", "ESPECTROGRAMA", "SPECTROGRAMME", "ESPECTROGRAMA" };
inline const LocalizedText game { "GAME / STRATEGY", "JOGO / ESTRATÉGIA", "JEU / STRATÉGIE", "JUEGO / ESTRATEGIA" };
inline const LocalizedText strategy { "STRATEGY", "ESTRATÉGIA", "STRATÉGIE", "ESTRATEGIA" };
inline const LocalizedText rule { "RULE", "REGRA", "RÈGLE", "REGLA" };
inline const LocalizedText resourceEnergy { "EN", "EN", "ÉN", "EN" };
inline const LocalizedText resourceMemory { "MEM", "MEM", "MÉM", "MEM" };
inline const LocalizedText resourceRisk { "RISK", "RISCO", "RISQUE", "RIESGO" };
inline const LocalizedText resourceFreedom { "FREE", "LIVRE", "LIBRE", "LIBRE" };
inline const LocalizedText resourceHidden { "HIDDEN", "OCULTO", "CACHÉ", "OCULTO" };
inline const LocalizedText clear { "CLEAR", "LIMPAR", "EFFACER", "LIMPIAR" };
inline const LocalizedText copy { "COPY", "COPIAR", "COPIER", "COPIAR" };
inline const LocalizedText compose { "COMPOSE", "COMPOR", "COMPOSER", "COMPONER" };
inline const LocalizedText tutorialTitle { "HOW TO USE", "COMO USAR", "MODE D’EMPLOI", "CÓMO USAR" };
inline const LocalizedText tutorialBody {
    "1  GENERATE\nSEED starts the path. WILD MODE is on by default; turn it off for a reproducible normal render. ALEA chooses another starting point. COMPOSE creates the piece.\n\n2  ADJUST\nTEMPO changes pace. MASTER controls final output level.\n\n3  LISTEN AND RETURN\nPLAY listens to the last piece. Click the waveform to begin there; drag to select a region and use PLAY REGION or SEED FROM REGION.\n\n4  READ THE PIECE\nThe grid shows manifested mechanisms. GAME / STRATEGY shows ALEA, AGON, ILINX and MIMICRY.\n\n5  ALBUM\nALBUM MODE opens track planning from a theme. Each track chooses its own scale. Optional BPM and duration choices appear in the preview; PLAY returns to the listening view.",
    "1  GERAR\nSEED inicia o percurso. MODO SELVAGEM vem ligado; desligue-o para um render normal reproduzível. ALEA sorteia outro ponto de partida. COMPOR cria a peça.\n\n2  AJUSTAR\nTEMPO altera o andamento. MASTER controla o nível final.\n\n3  ESCUTAR E RETOMAR\nPLAY toca a última peça. Clique na forma de onda para iniciar dali; arraste para selecionar um trecho e use TOCAR TRECHO ou SEED DO TRECHO.\n\n4  LER A PEÇA\nA grade mostra mecanismos manifestados. JOGO / ESTRATÉGIA mostra ALEA, AGON, ILINX e MIMICRY.\n\n5  ÁLBUM\nMODO ÁLBUM abre o planejamento de faixas a partir de um tema. Cada faixa escolhe sua escala. As opções de BPM e duração aparecem na prévia; PLAY retorna à escuta.",
    "1  GÉNÉRER\nSEED lance le parcours. MODE SAUVAGE est actif par défaut ; désactivez-le pour un rendu normal reproductible. ALEA choisit un autre point de départ. COMPOSER crée la pièce.\n\n2  AJUSTER\nTEMPO modifie l’allure. MASTER contrôle le niveau final.\n\n3  ÉCOUTER ET REVENIR\nPLAY écoute la dernière pièce. Cliquez la forme d’onde pour commencer là ; glissez pour choisir une région et utilisez LIRE LA RÉGION ou SEED DE LA RÉGION.\n\n4  LIRE LA PIÈCE\nLa grille montre les mécanismes manifestés. JEU / STRATÉGIE montre ALEA, AGON, ILINX et MIMICRY.\n\n5  ALBUM\nMODE ALBUM ouvre la planification des pistes à partir d’un thème. Chaque piste choisit sa gamme. Les options de tempo et de durée apparaissent dans l’aperçu ; PLAY revient à l’écoute.",
    "1  GENERAR\nSEED inicia el recorrido. MODO SALVAJE está activo por defecto; apáguelo para un render normal reproducible. ALEA elige otro punto de partida. COMPONER crea la pieza.\n\n2  AJUSTAR\nTEMPO cambia el pulso. MASTER controla el nivel final.\n\n3  ESCUCHAR Y VOLVER\nPLAY escucha la última pieza. Haga clic en la forma de onda para empezar allí; arrastre para elegir una región y use REPRODUCIR REGIÓN o SEED DE LA REGIÓN.\n\n4  LEER LA PIEZA\nLa cuadrícula muestra mecanismos manifestados. JUEGO / ESTRATEGIA muestra ALEA, AGON, ILINX y MIMICRY.\n\n5  ÁLBUM\nMODO ÁLBUM abre la planificación de pistas desde un tema. Cada pista elige su escala. Las opciones de BPM y duración aparecen en la vista previa; PLAY vuelve a la escucha."
};
inline const LocalizedText aboutBody {
    "Generative sound-synthesis instrument.\n\nCopyright © 2026 Lucio de Araujo\nGNU AGPL-3.0-or-later\nJUCE: AGPLv3\n\nFree software. You may modify and redistribute it under the license terms. No warranty. Read LICENSE and THIRD_PARTY_NOTICES.md in the distribution.",
    "Instrumento generativo de síntese sonora.\n\nCopyright © 2026 Lucio de Araujo\nGNU AGPL-3.0-or-later\nJUCE: AGPLv3\n\nSoftware livre. Você pode modificar e redistribuir nos termos da licença. Sem garantia. Consulte LICENSE e THIRD_PARTY_NOTICES.md na distribuição.",
    "Instrument génératif de synthèse sonore.\n\nCopyright © 2026 Lucio de Araujo\nGNU AGPL-3.0-or-later\nJUCE : AGPLv3\n\nLogiciel libre. Vous pouvez le modifier et le redistribuer selon les conditions de la licence. Sans garantie. Consultez LICENSE et THIRD_PARTY_NOTICES.md dans la distribution.",
    "Instrumento generativo de síntesis sonora.\n\nCopyright © 2026 Lucio de Araujo\nGNU AGPL-3.0-or-later\nJUCE: AGPLv3\n\nSoftware libre. Puedes modificarlo y redistribuirlo según los términos de la licencia. Sin garantía. Consulta LICENSE y THIRD_PARTY_NOTICES.md en la distribución."
};
inline const LocalizedText close { "CLOSE", "FECHAR", "FERMER", "CERRAR" };
inline const LocalizedText appVersionLabel { "Version", "Versão", "Version", "Versión" };
inline const LocalizedText appBuildLabel { "Built", "Compilado em", "Compilé le", "Compilado el" };
inline const LocalizedText appStarted { "Rasgo Synth started.", "Rasgo Synth iniciado.", "Rasgo Synth démarré.", "Rasgo Synth iniciado." };
inline const LocalizedText composingWait { "composing, please wait...", "compondo, aguarde...", "composition en cours, veuillez patienter...", "componiendo, espere..." };
inline const LocalizedText albumWait { "creating album, please wait...", "criação de álbum em andamento, aguarde...", "création de l’album en cours, veuillez patienter...", "creando álbum, espere..." };
inline const LocalizedText renderElapsed { "Render elapsed", "Tempo de render", "Temps de rendu", "Tiempo de render" };
inline const LocalizedText generatedAudio { "Audio generated", "Áudio gerado", "Audio généré", "Audio generado" };
inline const LocalizedText renderCalibration { "Calibration", "Calibração", "Étalonnage", "Calibración" };
inline const LocalizedText renderTrack { "Track", "Faixa", "Piste", "Pista" };
inline const LocalizedText renderComposition { "Composition", "Composição", "Composition", "Composición" };
inline const LocalizedText renderFinalizing { "Finalizing file", "Finalizando arquivo", "Finalisation du fichier", "Finalizando archivo" };
inline const LocalizedText renderActivityTip {
    "Composition percentage counts generated frames against a read-only preview of the same musical arc, including abrupt thermal endings. It is not elapsed-time percentage or an ETA. 100% composition is followed by file finalization. Album percentage refers to the current track/pass.",
    "O percentual de composição conta amostras geradas sobre uma leitura antecipada do mesmo arco musical, incluindo encerramentos térmicos abruptos. Não é percentual do tempo decorrido nem tempo restante estimado. Após 100% da composição há finalização do arquivo. No álbum, o percentual é da faixa/passagem atual.",
    "Le pourcentage de composition compte les échantillons générés sur une prélecture du même arc musical, y compris les fins thermiques abruptes. Ce n’est ni un pourcentage du temps écoulé ni une estimation du temps restant. À 100 %, le fichier doit encore être finalisé. Pour l’album, le pourcentage concerne la piste/passe actuelle.",
    "El porcentaje de composición cuenta muestras generadas respecto a una lectura previa del mismo arco musical, incluidos los finales térmicos abruptos. No es porcentaje del tiempo transcurrido ni estimación del tiempo restante. Tras el 100 % se finaliza el archivo. En el álbum, corresponde a la pista/pasada actual."
};
inline const LocalizedText saved { "saved.", "gravado.", "enregistré.", "guardado." };
inline const LocalizedText ready { "ready to compose.", "pronto para compor.", "prêt à composer.", "listo para componer." };
inline const LocalizedText logCopied { "log copied to clipboard.", "log copiado para a área de transferência.", "log copié dans le presse-papiers.", "registro copiado al portapapeles." };
inline const LocalizedText trackItem { "track", "faixa", "piste", "pista" };
inline const LocalizedText tutorialCardGenerate {
    "SEED starts the path. WILD MODE allows entropy to open different routes; turn it off for a reproducible render. ALEA chooses another starting point. COMPOSE creates the piece.",
    "SEED inicia o percurso. MODO SELVAGEM permite que a entropia abra caminhos diferentes; desligue-o para um render reproduzível. ALEA escolhe outro ponto de partida. COMPOR cria a peça.",
    "SEED lance le parcours. MODE SAUVAGE permet à l’entropie d’ouvrir d’autres voies ; désactivez-le pour un rendu reproductible. ALEA choisit un autre départ. COMPOSER crée la pièce.",
    "SEED inicia el recorrido. MODO SALVAJE permite que la entropía abra rutas distintas; apáguelo para un render reproducible. ALEA elige otro punto de partida. COMPONER crea la pieza."
};
inline const LocalizedText tutorialCardAdjust {
    "TEMPO changes pace. MASTER controls the final level. Level changes listening, not composition.",
    "TEMPO altera o andamento. MASTER controla o nível final. O nível muda a escuta, não a composição.",
    "TEMPO modifie l’allure. MASTER contrôle le niveau final. Le niveau change l’écoute, pas la composition.",
    "TEMPO cambia el pulso. MASTER controla el nivel final. El nivel cambia la escucha, no la composición."
};
inline const LocalizedText tutorialCardListen {
    "PLAY listens to the last piece. Click the waveform to start there; drag to select a region and use PLAY REGION or SEED FROM REGION.",
    "PLAY toca a última peça. Clique na forma de onda para iniciar dali; arraste para selecionar um trecho e use TOCAR TRECHO ou SEED DO TRECHO.",
    "PLAY écoute la dernière pièce. Cliquez la forme d’onde pour commencer là ; glissez pour choisir une région et utilisez LIRE LA RÉGION ou SEED DE LA RÉGION.",
    "PLAY escucha la última pieza. Haga clic en la forma de onda para empezar allí; arrastre para elegir una región y use REPRODUCIR REGIÓN o SEED DE LA REGIÓN."
};
inline const LocalizedText tutorialCardRead {
    "The mechanism grid shows what manifested. GAME / STRATEGY shows ALEA, AGON, ILINX and MIMICRY with their values and resources.",
    "A grade de mecanismos mostra o que se manifestou. JOGO / ESTRATÉGIA mostra ALEA, AGON, ILINX e MIMICRY com seus valores e recursos.",
    "La grille montre les mécanismes manifestés. JEU / STRATÉGIE montre ALEA, AGON, ILINX et MIMICRY avec leurs valeurs et ressources.",
    "La cuadrícula muestra los mecanismos manifestados. JUEGO / ESTRATEGIA muestra ALEA, AGON, ILINX y MIMICRY con sus valores y recursos."
};
inline const LocalizedText tutorialCardAlbum {
    "ALBUM MODE opens track planning from a theme. Each track chooses its own scale. Optional BPM and duration choices appear in the preview; PLAY returns to the listening view.",
    "MODO ÁLBUM abre o planejamento de faixas a partir de um tema. Cada faixa escolhe sua escala. As opções de BPM e duração aparecem na prévia; PLAY retorna à escuta.",
    "MODE ALBUM ouvre la planification des pistes à partir d’un thème. Chaque piste choisit sa gamme. Les options de tempo et de durée apparaissent dans l’aperçu ; PLAY revient à l’écoute.",
    "MODO ÁLBUM abre la planificación de pistas desde un tema. Cada pista elige su escala. Las opciones de BPM y duración aparecen en la vista previa; PLAY vuelve a la escucha."
};
inline const LocalizedText tutorialGenerateTitle { "1  GENERATE", "1  GERAR", "1  GÉNÉRER", "1  GENERAR" };
inline const LocalizedText tutorialAdjustTitle { "2  ADJUST", "2  AJUSTAR", "2  AJUSTER", "2  AJUSTAR" };
inline const LocalizedText tutorialListenTitle { "3  LISTEN AND RETURN", "3  ESCUTAR E RETOMAR", "3  ÉCOUTER ET REVENIR", "3  ESCUCHAR Y VOLVER" };
inline const LocalizedText tutorialReadTitle { "4  READ THE PIECE", "4  LER A PEÇA", "4  LIRE LA PIÈCE", "4  LEER LA PIEZA" };
inline const LocalizedText tutorialAlbumTitle { "5  ALBUM", "5  ÁLBUM", "5  ALBUM", "5  ÁLBUM" };
inline const LocalizedText tutorialSubtitle { "a path of listening, reading and recombination", "um fluxo de escuta, leitura e recombinação", "un parcours d’écoute, de lecture et de recombinaison", "un flujo de escucha, lectura y recombinación" };
inline const LocalizedText saveFailed { "could not save file.", "falha ao gravar arquivo.", "échec de l’enregistrement du fichier.", "no se pudo guardar el archivo." };
inline const LocalizedText logRecorded { "recorded:", "gravado:", "enregistré :", "grabado:" };
inline const LocalizedText logIdentity { "identity:", "identidade:", "identité :", "identidad:" };
inline const LocalizedText logModules { "manifested modules:", "módulos manifestados:", "modules manifestés :", "módulos manifestados:" };
inline const LocalizedText logGame { "game:", "jogo:", "jeu :", "juego:" };
inline const LocalizedText logOutput { "output:", "saída:", "sortie :", "salida:" };
inline const LocalizedText logDrums { "drums:", "bateria:", "batterie :", "batería:" };
inline const LocalizedText logDrumSurvival { "drums survival:", "sobrevivência bateria:", "survie de la batterie :", "supervivencia batería:" };
inline const LocalizedText logAttention { "narrative attention:", "atenção narrativa:", "attention narrative :", "atención narrativa:" };
inline const LocalizedText logPlaying { "playing:", "tocando:", "lecture :", "reproduciendo:" };
inline const LocalizedText logStopped { "stopped.", "parado.", "arrêté.", "detenido." };
inline const LocalizedText logSelected { "selected region:", "trecho selecionado:", "région sélectionnée :", "región seleccionada:" };
inline const LocalizedText logEntering { "entering", "entrando em", "entrée dans", "entrando en" };
inline juce::String albumThemeName(const juce::String& original, Language language) {
    static const std::array<std::pair<const char*, LocalizedText>, 4> themes {
        std::pair<const char*, LocalizedText>{ "Livre", { "Free", "Livre", "Libre", "Libre" } },
        { "Contido", { "Restrained", "Contido", "Contenu", "Contenido" } },
        { "Luminoso", { "Bright", "Luminoso", "Lumineux", "Luminoso" } },
        { "Denso", { "Dense", "Denso", "Dense", "Denso" } }
    };
    for (const auto& entry : themes)
        if (original.equalsIgnoreCase(entry.first)) return text(entry.second, language);
    return original;
}
inline const LocalizedText emptyGoniometer { "(goniometer)", "(goniômetro)", "(goniomètre)", "(goniómetro)" };
inline const LocalizedText emptyModulePanel { "(no piece rendered)", "(sem peça renderizada)", "(aucune pièce rendue)", "(sin pieza renderizada)" };
inline const LocalizedText emptyWaveform { "(no render yet)", "(nenhuma renderização ainda)", "(aucun rendu pour l’instant)", "(aún no hay render)" };
inline const LocalizedText emptyOscilloscope { "(play to see the signal)", "(toque para ver o sinal)", "(lancez la lecture pour voir le signal)", "(reproduzca para ver la señal)" };
inline const LocalizedText emptyGenealogy { "(arc genealogy)", "(genealogia do arco)", "(généalogie de l’arc)", "(genealogía del arco)" };
inline const LocalizedText emptyPhasePortrait { "(phase portrait)", "(retrato de fase)", "(portrait de phase)", "(retrato de fase)" };
inline const LocalizedText emptySpectrum { "(spectrum)", "(espectro)", "(spectre)", "(espectro)" };
inline const LocalizedText emptySpectrogram { "(spectrogram)", "(espectrograma)", "(spectrogramme)", "(espectrograma)" };
inline const LocalizedText emptyGranulation { "(granulation)", "(granulação)", "(granulation)", "(granulación)" };

inline juce::String sectionName(const juce::String& original, Language language) {
    struct Entry { const char* key; LocalizedText value; };
    static const Entry entries[] {
        { "abertura", { "opening", "abertura", "ouverture", "apertura" } },
        { "emergencia", { "emergence", "emergência", "émergence", "emergencia" } },
        { "deriva inicial", { "initial drift", "deriva inicial", "dérive initiale", "deriva inicial" } },
        { "premissa", { "premise", "premissa", "prémisse", "premisa" } },
        { "contexto", { "context", "contexto", "contexte", "contexto" } },
        { "desdobramento", { "unfolding", "desdobramento", "déploiement", "desarrollo" } },
        { "elaboracao", { "elaboration", "elaboração", "élaboration", "elaboración" } },
        { "continuidade", { "continuity", "continuidade", "continuité", "continuidad" } },
        { "desenvolvimento", { "development", "desenvolvimento", "développement", "desarrollo" } },
        { "acumulo", { "buildup", "acúmulo", "accumulation", "acumulación" } },
        { "adensamento", { "thickening", "adensamento", "densification", "densificación" } },
        { "espiral", { "spiral", "espiral", "spirale", "espiral" } },
        { "climax", { "climax", "clímax", "climax", "clímax" } },
        { "ruptura", { "rupture", "ruptura", "rupture", "ruptura" } },
        { "vertice", { "vertex", "vértice", "sommet", "vértice" } },
        { "colapso", { "collapse", "colapso", "effondrement", "colapso" } },
        { "virada", { "turn", "virada", "tournant", "giro" } },
        { "reviravolta", { "reversal", "reviravolta", "revirement", "reviravuelta" } },
        { "digressao", { "digression", "digressão", "digression", "digresión" } },
        { "quebra de expectativa", { "expectation break", "quebra de expectativa", "rupture d’attente", "quiebre de expectativa" } },
        { "decantacao", { "settling", "decantação", "décantation", "decantación" } },
        { "arrefecimento", { "cooling", "arrefecimento", "refroidissement", "enfriamiento" } },
        { "eco", { "echo", "eco", "écho", "eco" } },
        { "reconciliacao", { "reconciliation", "reconciliação", "réconciliation", "reconciliación" } },
        { "religacao", { "reconnection", "religação", "reconnexion", "reconexión" } },
        { "reflexo", { "reflection", "reflexo", "reflet", "reflejo" } },
        { "resolucao", { "resolution", "resolução", "résolution", "resolución" } },
        { "repouso", { "rest", "repouso", "repos", "reposo" } },
        { "saldo", { "balance", "saldo", "bilan", "balance" } },
        { "silencio parcial", { "partial silence", "silêncio parcial", "silence partiel", "silencio parcial" } },
        { "sintese", { "synthesis", "síntese", "synthèse", "síntesis" } },
        { "desfecho", { "outcome", "desfecho", "dénouement", "desenlace" } },
        { "fechamento", { "closure", "fechamento", "clôture", "cierre" } },
        { "eco distante", { "distant echo", "eco distante", "écho lointain", "eco distante" } }
    };
    for (const auto& entry : entries)
        if (original.equalsIgnoreCase(entry.key)) return text(entry.value, language);
    return original;
}
} // namespace strings
} // namespace rasgo::ui
