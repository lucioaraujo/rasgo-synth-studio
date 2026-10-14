# Rasgo Synth — Document de Référence du Projet

Instrument génératif — concept, histoire, architecture, fonctionnalités, flux d'utilisation, travail effectué, validations, sécurité, layout et points en suspens.

> **À propos de ce document** : c'est la référence technique stable du projet, mise à jour au fil des vraies évolutions (pas à chaque session). Pour le journal de bord phase par phase, avec l'historique complet des décisions et la raison de chacune, la source de vérité reste [`Plano_de_Upgrade_Instrumento_Generativo.md`](../../Plano_de_Upgrade_Instrumento_Generativo.md) (portugais uniquement) à la racine du projet — en cas de divergence entre les deux, ce document fait autorité.

Dernière mise à jour : 22 août 2026 (référence synchronisée avec le travail opérationnel des phases 118–123).

---

## 1. Concept

Rasgo Synth est un **instrument génératif** : vous définissez une configuration initiale (une graine numérique, un tempo) et appuyez sur composer, le logiciel écrit seul une pièce entière, avec un développement et une variation continus dans le temps, pas une boucle qui se répète. Une console de performance en direct reste possible comme mode secondaire (l'application joue déjà de l'audio en temps réel via `juce_audio_devices`), mais la cible principale a toujours été autre chose : servir de base à des pistes d'album.

La phrase qui résume le projet, et qui apparaît aujourd'hui dans l'interface même de l'application, est : **« il prend un risque, et il sonne »** (« se arrisca, e soa »). Chaque exécution est sonorement unique par nature, le moteur est construit sur une dynamique chaotique (neuf cartes — Lorenz, Hénon, Latoocarfian et six autres du « Chaos Cookbook ») et des oscillateurs instables qui se croisent et se rebouclent, la même qualité sonore trouvée en expérimentant avec les circuits CMOS du projet matériel Antitotem qui précède ce logiciel.

Un principe de conception qui traverse tout le projet : **porter l'idée, pas copier le circuit**. Chaque fois qu'un schéma réel (CBS, Buchla, TR-808, MFOS/Ray Wilson, Instruō, Qu-Bit Electronix et plusieurs autres) a été étudié, l'objectif était de comprendre le comportement sonore et de le réimplémenter en C++ pur, jamais de simuler transistor par transistor.

Le pourquoi philosophique derrière cette technique est consigné dans [`Manifesto.md`](Manifesto.md).

Autre principe, réaffirmé plusieurs fois au cours du développement : quand un paramètre peut être décidé soit par la graine soit par un contrôle manuel de l'utilisateur, le projet tend à choisir l'un ou l'autre, jamais un mélange arbitraire. La gamme elle-même a cessé d'être un contrôle manuel (phase 48) — aujourd'hui c'est toujours un choix de la graine elle-même, parmi un répertoire de 39 gammes étudiées plus une part de gammes générées algorithmiquement (cribles de Xenakis, phase 61). Les contrôles manuels n'ont survécu que là où ils ont du sens comme « accordage de l'instrument » (le tempo, le volume de sortie), pas comme une couche superposée à des décisions structurelles que la graine prend déjà.

Un tournant explicite s'est produit à la phase 80 : passer de « ajouter des modules isolés » à « multiplier les rencontres entre des modules déjà existants ». Les **poids de jeu de Caillois** (Alea/Agon/Ilinx/Mimicry, voir section 4.9) formalisent cela en une véritable machine à règles, et la phase 84 (Curto-Circuito/Court-Circuit, section 4.10) pousse cette logique à l'extrême : un même événement peut rompre le protocole même que la graine avait établi.

---

## 2. Histoire

### 2.1 Kosmische (2013)

L'une des références techniques initiales fut une expertise sur Kosmische, projet externe de Nicolas Steven Miller : une application Android qui embarquait un serveur SuperCollider complet via le NDK. Le binaire natif de Kosmische comportait trois familles d'UGens pertinentes : `ChaosUGens` (les neuf cartes chaotiques), `GendynUGens` (synthèse stochastique dynamique de Xenakis) et `AY_UGen` (émulation de la puce AY-3-8910). Cette investigation a informé des concepts de Rasgo Synth, mais Kosmische ne fait pas partie de Rasgo et ne constitue pas son origine d'auteur.

### 2.2 Antitotem et les modules physiques de Rasgo

Antitotem est un instrument matériel construit à la main, de nature modulaire et CMOS/discrète, qui précède Rasgo et fut ensuite incorporé au projet comme son précurseur et son patron. L'expression « Rasgo en matériel » désigne l'ensemble des modules physiques d'auteur de cette phase (depuis 2013), déjà conçus en visant le standard Eurorack.

Depuis, Rasgo a évolué vers quelque chose de plus grand : c'est aujourd'hui le projet-cadre qui rassemble tous les instruments et logiciels musicaux d'auteur de Lucio de Araujo. Rasgo Synth est une œuvre au sein de cet ensemble, tandis qu'Antitotem demeure simultanément une œuvre incorporée et l'origine symbolique de la lignée. Rasgo Synth Performance est désormais un prototype standalone local et indépendant pour le DSP live ; une console embarquée ou du matériel restent des possibilités ultérieures, non la définition de cet instrument logiciel.

### 2.3 Décision d'architecture

La décision technique fut de ne pas relancer le serveur SuperCollider comme dépendance d'exécution. Les concepts investigués ont été réimplémentés en C++ comme code propre de Rasgo Synth, puis largement étendus — aujourd'hui, des dizaines de mécanismes au-delà de ce qui existait en 2013.

### 2.4 Licence

Le code original actuel de Rasgo Synth Studio utilise **AGPL-3.0-or-later**. Kosmische est aussi en GPLv3, de manière indépendante. Les conditions spécifiques de l'application construite avec JUCE sont consignées dans `THIRD_PARTY_NOTICES.md` ; toute l'attribution par mécanisme est dans `CREDITS.md`.

---

## 3. Architecture JUCE / C++

Le projet est divisé en deux sous-projets indépendants.

### 3.1 `rasgo-synth-core` — le moteur DSP

C++17 pur, sans aucune dépendance de plateforme. Compilé directement avec g++ (pas de CMake sur cette machine).

- **`src/dsp/`** — des dizaines de classes DSP : oscillateurs classiques avec PolyBLEP ; neuf cartes chaotiques ; un oscillateur Walsh (base orthogonale ±1, phase 59) ; VOSIM (impulsions sin² décroissantes, phase 72) ; filtre ladder Moog, filtre state-variable Chamberlin et filtre Polivoks auto-oscillant (phase 75) ; waveshaper à diode ; résonateur modal ; nuage granulaire ; diviseur d'horloge XOR polyrythmique ; huit voix de percussion numériques ; `RhythmSieve.hpp` (crible rythmique de Xenakis, phase 82) et `DrumArchetypes.hpp` (7 archétypes nommés + un emplacement de crible) ; Rungler, Fracture Folder, Leibniz Rectifier ; `ParenthesisGlitch.hpp` (phase 70) ; `SieveScale.hpp` (gamme générée par crible de Xenakis, phase 61) ; Gendyn, AY-3-8910 ; une source de tension aléatoire de type Buchla.
- **`src/sequencer/`** — 39 gammes étudiées (`Scales.hpp`, phase 48) plus des cribles générés ; un séquenceur pas à pas (`StepSequencer.hpp`) avec une conduite de voix réelle par demi-tons, hocketing/cascade polyphonique, progressions d'accords et transformations néo-riemanniennes P/L/R (mutuellement exclusives) ; un arc narratif par chaîne de Markov avec des grammaires enfichables (`GenerativeArc.hpp` + `ArcTemplates.hpp`).
- **`src/engine/`** — `PieceRenderer.hpp` (la fonction de rendu principale, ~50 mécanismes contrôlés par la graine) et `AlbumRenderer.hpp` (phase 83 : chaque piste d'album occupe un point interpolé sur une trajectoire de poids de jeu de Caillois, choisissant librement sa propre gamme — plus de centre tonal forcé).
- **`src/io/`** — `WavWriter.hpp` (métadonnées via un chunk RIFF LIST/INFO) et `Recorder.hpp`.
- **Démos CLI** — `rasgo_integrated_demo` (pièce isolée) et `rasgo_album_demo` (ensemble de pistes).
- **`regression_test.py`** — script de test automatisé (section 7).

### 3.2 `rasgo-synth-app` — l'application de bureau JUCE

- **`src/MainComponent.h`** — l'écran principal (section 5).
- **`src/RasgoLookAndFeel.h`** — l'identité visuelle « Cobre »/Cuivre (section 9), incluant le `LogoMark` (le vrai logo vectorisé par l'auteur lui-même, chargé au moment de l'exécution depuis un fichier SVG via `juce::Drawable`).
- **`src/ArcTimelineView.h`, `WaveformView.h`, `OscilloscopeView.h`** (désormais stéréo), **`SpectrumView.h`** (banc de filtres Goertzel), **`LevelMetersView.h`** (RMS/crête horizontal), **`GameWeightsView.h`**, **`GranulationView.h`** (particules) — les visualisations de l'écran principal.
- **`src/BufferPlaybackSource.h`** — la source de lecture JUCE construite à partir du buffer déjà rendu, avec un filtre en direct et un gain master.
- **`src/Main.cpp`** — la fenêtre s'ouvre maximisée par défaut (`setFullScreen`) ; un `juce::Viewport` (`RasgoViewport`) garantit un vrai défilement sur les moniteurs plus petits que ce que le layout confortable exige, au lieu de couper ou de superposer les contrôles.
- **`build_juce_minimal.sh` / `build_app.sh`** — la recette de compilation sans CMake/Projucer.
- **`autotest.sh`** — le test de fumée automatisé de l'application (section 7).

L'audio en direct est servi par `juce_audio_devices`/`juce_audio_basics`, avec ALSA comme seul chemin sous Linux, et le pseudo-périphérique « pulse » préféré au « default » brut.

---

## 4. Fonctionnalités

### 4.1 Noyau sonore

Six principes de synthèse distincts coexistent : soustractive (oscillateurs + ladder numérique, Chamberlin ou Polivoks), additive (Walsh, VOSIM), résonante (résonateur modal, incluant la voix de tuba/souffle continu), granulaire (`GranularCloud`), stochastique (`GendynOscillator`) et vectorielle (Vector Synthesis, phase 76 — un vrai crossfade 2D à travers un banc dédié de Gendyn/Walsh/Atari Punk/bruit coloré, position pilotée par deux cartes chaotiques). Les neuf cartes chaotiques et les quatre LFO lents restent tous actifs.

Les deux oscillateurs principaux (`osc1`/`osc2`) peuvent se moduler mutuellement en **cross-FM bidirectionnel**. Un suiveur d'enveloppe lent peut reboucler la propre sortie audio de la pièce dans le taux de la carte de Lorenz (**self-feedback**). Une couche de **bruit coloré** s'ajoute à la texture granulaire/Gendyn.

Couches additives d'accompagnement : **Harmonàig** (phase 77, un vrai pad d'accord à 4 voix — fondamentale/3ce/5te/7me, inspiré du vrai module Instruō de Jason Lim), **polyphonie de cribles de Xenakis** (phase 78, 4 canaux indépendants — bois/souffle, métal/verre, eau, plasma réactif) et **Bloom** (phase 79, une mélodie fractale Trunk/Branch/Path, inspirée du vrai module Qu-Bit Electronix d'Andrew Ikenberry).

### 4.2 La couche narrative — l'arc génératif

La durée et la forme de chaque pièce sont une **marche de Markov** sur cinq rôles structurels, ne se terminant qu'à la coda. Deux grammaires enfichables et mélangeables : **Dramatic** (pyramide de Freytag) et **Kishōtenketsu**. Un accelerando vers le climax (phase 68) reflète le ritardando de coda déjà existant.

### 4.3 Séquençage et gammes

39 gammes étudiées (phase 48) plus des gammes générées algorithmiquement par la théorie des cribles de Xenakis (phase 61, rejetées par échantillonnage contre le catalogue réel). Conduite de voix réelle par distance en demi-tons (phase 62), vraies progressions d'accords (phase 34), et, comme alternative mutuellement exclusive, transformations triadiques néo-riemanniennes P/L/R (phase 65, conduite de voix parcimonieuse vérifiée). Hocketing avec une cascade polyphonique de type registre à décalage (phase 63). Un diviseur d'horloge XOR génère une polyrythmie indépendante de la grille euclidienne principale ; le pointeur de lecture de la mélodie peut être adressé par une source chaotique dédiée au lieu de l'horloge linéaire (phase 64).

### 4.4 Percussion

Huit voix numériques indépendantes (grosse caisse, caisse claire, charleston, cowbell, clap, tom/conga, cymbale, maracas), inspirées en partie par la TR-808. La présence est désormais liée à la densité de l'arc, pas une constante fixe (phase 66) — 12 % des pièces n'ont aucune batterie, et une fraction a de vrais échecs par coup. Huit options d'archétype rythmique par section de l'arc (phases 67 + 82) : Marche, Soul-backbeat, Metal-blast-beat, Jazz-swing, Samba-inspiré, deux archétypes aksak balkaniques réels (Constantin Brăiloiu/Stoyan Dzhudzhev, 9+7 et 7+7+2), et un crible rythmique de Xenakis généré (même mathématique que la section 4.3, appliquée à des points dans le temps). Vrai rythme euclidien (Bjorklund/Toussaint) comme alternative pour le cowbell (phase 57). Dynamique étendue : charleston dans la contagion du chaos, ratchets sur les toms, sustain de cymbale étendu (phase 56).

### 4.5 Sortie stéréo

Les canaux divergent réellement à partir de l'étage de delay/réverbération (lignes indépendantes, légèrement désynchronisées). Un battement binaural chaotique (phase 73) ajoute une seconde source réelle et vérifiable de divergence L/R, indépendante du panoramique/delay.

### 4.6 Mode album

Une graine d'album dérive de manière déterministe N graines de piste. Jusqu'à la phase 82, toutes les pistes partageaient un centre tonal forcé (même gamme/tonique). **À partir de la phase 83**, cela a changé : l'album parcourt désormais une **trajectoire en ligne droite à travers les quatre poids de jeu de Caillois** (section 4.9) — la première piste naît en un point tiré au hasard, la dernière se termine en un autre point également tiré au hasard, chaque piste entre les deux occupe son propre point interpolé, et chaque piste choisit librement sa propre gamme, comme le ferait une pièce isolée. Un **thème** nommé définit toujours le tempo/la tonique/le biais d'énergie (l'ambiance), mais plus la gamme.

### 4.7 Qualité sonore — hygiène technique et mastering

Dither TPDF, `DcBlocker`, `LookaheadLimiter` + saturateur de sécurité, PolyBLEP (~46dB de réduction d'aliasing mesurée), suréchantillonnage dans le mélange à diode, mastering d'album en deux passes. Un vrai bug d'instabilité soutenue dans le `ModalResonator` du canal tuba/bois (excitation continue poussant le résonateur près de son propre plafond de sécurité, ~25x plus fort que la pièce) a été corrigé en phase 81 avec un limiteur de sortie doux `tanh`, le même motif déjà utilisé pour Polivoks.

### 4.8 Capture, métadonnées et index

Chaque WAV enregistre des métadonnées intégrées (chunk RIFF LIST/INFO) et un fichier `.txt` compagnon lisible comme une partition (phase 54) avec la trajectoire complète de la pièce. Un récit en texte brut de la pièce elle-même (graine, sections) vit dans un chunk RIFF final lisible dans un éditeur de texte/hexadécimal, invisible pour tout lecteur (self-epitaph, phase 46). Chaque enregistrement réussi ajoute une ligne à `rasgo_index.jsonl`.

### 4.9 Poids de jeu de Caillois (phase 80)

Quatre poids continus — **Alea, Agon, Ilinx, Mimicry** — échantillonnés uniformément sur le 4-simplexe via le « stick-breaking » (trois points de coupure aléatoires triés, les quatre écarts sont les poids), de sorte que chaque pièce obtient son propre mélange d'auteur au lieu de choisir dans une liste fixe de « jeux ». Alea n'a pas de groupe de mécanismes propre — il EST l'absence de biais thématique, le comportement de base du moteur. Les trois autres influencent (sans jamais forcer) onze mécanismes déjà existants : **Agon** (voix en compétition/coordination) → Agon lui-même, Harmonàig, polyphonie de cribles ; **Ilinx** (vertige/perte de contrôle) → Rungler, effondrement de Rössler, thermal fade, self-feedback, stochastic mute, Leibniz rectifier — et, depuis la phase 84, aussi Curto-Circuito (section 4.10) ; **Mimicry** (simulacre) → déchirure apocryphe, self-epitaph, signal étranger, parenthèse, élément étrange. Honnêtement attribué à Roger Caillois (*Les Jeux et les Hommes*, 1958).

### 4.10 Curto-Circuito / « Court-Circuit » (phase 84)

Un événement unique par pièce, influencé par Ilinx, avec deux fonctions dans la même instance. **Une vraie coupure sèche** : une porte (~3ms de rampe linéaire, jamais une courbe de fondu) se ferme sur un silence véritable pendant 100-400ms puis se rouvre de la même manière — exactement la signature identifiée dans une vraie analyse de forme d'onde/spectrogramme d'un des propres morceaux de référence de l'auteur. Une « étincelle » de bruit ne chevauche que les bords de transition, littéralisant le « court-circuit » comme un artefact électrique, pas une simple coupure de son. **Mécanismes intrus** : au même instant, six mécanismes que la graine avait déjà tirés comme DÉSACTIVÉS pour cette pièce (déchirure apocryphe, intonation libre, modulation tonale, self-feedback, rungler, élément étrange) obtiennent chacun une vraie chance élevée de 40 % de s'activer quand même — réutilisant exactement le même idiome d'« activation forcée » que la garantie d'intensité de la phase 40 avait déjà établi, simplement déclenché par une rupture au milieu de la pièce plutôt qu'un filet de sécurité en début de pièce.

### 4.11 Autres ruptures et glitchs

Fracture Folder + Rungler (phase 41), Leibniz Rectifier (phase 42), contagion du chaos (phase 44), thermal fade (phase 45), effondrement de Rössler + self-epitaph (phase 46), stochastic mute (phase 47), élément étrange (phase 50), un vrai signal étranger via la température/charge de l'hôte (phase 51, la seule exception honnête à la reproductibilité pure), débordement en virgule fixe Q1.15 (phase 52), spectral ghost via FFT/IFFT (phase 53), sample-hold glitch (phase 55), contagion inter-domaines (un pic audio poussant l'horloge, phase 60), Parenthèse (redirection vers une boucle de bégaiement, phase 70), lecture de pas adressée par le chaos (phase 64), et Curto-Circuito (section 4.10, phase 84) couronnant la famille.

---

### État opérationnel actuel (22 août 2026)

Le Studio comprend désormais l'agence matérielle, l'observation d'audibilité
après espace, la mémoire d'orchestration relationnelle, les contrats mélodiques
et de gammes élargis, une articulation de percussion variée et un Mode Sauvage
auditable. Le Mode Sauvage est le réglage par défaut du Studio : une entropie
réelle réensemence le générateur partagé aux frontières de mesure. Le désactiver
rétablit la reproductibilité par graine si le tempo, la configuration, la version
du moteur et l'état externe pertinent sont identiques. Des prises récentes avec
la même graine en mode normal et sauvage avaient des hashes SHA-256 différents
tout en conservant le même matériau macro. La base de vérification actuelle est
CTest 19/19, régression normale 6/6 et régression sauvage 3/3, sans clipping.

## 5. Flux d'utilisation

### 5.1 Dans l'application de bureau

1. Ouvrir l'application : `cd rasgo-synth-app && ./rasgo_synth_app` — s'ouvre maximisée.
2. Définir la graine dans le champ SEED, ou cliquer sur ALEA pour en tirer une nouvelle.
3. Ajuster le tempo sur le curseur TEMPO (20–220 bpm). La gamme n'est plus un contrôle manuel — la graine la choisit.
4. Ajuster le volume de sortie sur le curseur MASTER (à côté de NÍVEL, un vumètre RMS/crête horizontal).
5. Vérifier/ajuster le RÉPERTOIRE DE SORTIE, le TITRE et l'ARTISTE (plus étroits maintenant, partageant l'espace avec PESOS DE JOGO/poids de jeu).
6. Cliquer sur le bouton rond COMPOR (icône de croche ♪, rouge) — le rendu s'exécute sur un thread séparé ; la timeline de l'arc éclaire la section active, la forme d'onde se dessine.
7. Cliquer sur PLAY (icône lecture/pause, doré) pour entendre le résultat, ou cliquer directement sur la forme d'onde (intitulée WAVEFORM) pour lire à partir de ce point.
8. Glisser sur la forme d'onde sélectionne un passage — active TOCAR TRECHO et SEED DO TRECHO (dérive une nouvelle graine via un hash FNV-1a et déclenche immédiatement une nouvelle composition).
9. Pendant la lecture : le curseur BRILHO AO VIVO applique un vrai filtre passe-bas ajustable en direct ; VARIAR SEED capture l'instant exact et dérive une nouvelle graine.
10. Sous la forme d'onde : ESPECTRO (banc Goertzel) et GRANULAÇÃO (particules réactives à l'amplitude). PESOS DE JOGO montre les quatre barres colorées d'Alea/Agon/Ilinx/Mimicry de la pièce actuelle. Le panneau de mécanismes montre, en grille, lesquels des ~50 fonctionnalités du moteur se sont manifestées dans cette pièce spécifique.
11. OSCILOSCÓPIO (ESTÉREO) (doré) et LOG (avec COPIAR/LIMPAR sous la boîte) se partagent la bande inférieure, 60%/40% de largeur.
12. TUTORIAL et ABOUT (coin supérieur droit) ouvrent des dialogues thématisés avec les instructions d'utilisation et la vraie description du projet.
13. MODO ALBUM (bouton rectangulaire, se remplit de cuivre lorsqu'activé) fait passer le panneau à TEMA/FAIXAS/OUVIR.

### 5.2 Par ligne de commande

```bash
# Pièce isolée
./rasgo_integrated_demo [seed] [outDir] [bpm]

# Ensemble de pistes (album) -- trajectoire de poids de jeu, plus une gamme fixe
./rasgo_album_demo [albumSeed] [trackCount] [outDir] [theme]

# Batterie de régression
python3 regression_test.py --seeds 20

# Test de fumée de l'application
./autotest.sh --skip-build --timeout=30
```

---

## 6. Travail effectué (chronologie par phases)

Le document vivant du projet ([`Plano_de_Upgrade_Instrumento_Generativo.md`](../../Plano_de_Upgrade_Instrumento_Generativo.md), portugais uniquement) est la source de vérité complète, phase par phase. Résumé chronologique :

- **Phases 1–23** — noyau DSP validé, séquenceur, couche narrative par chaîne de Markov, refonte visuelle « Sinal » (remplacée ensuite par « Cobre »/Cuivre, voir section 9), huit voix de percussion, vraie sortie stéréo, mode album initial, qualité sonore audiophile (dither/DC-block/limiter/PolyBLEP), les neuf cartes chaotiques actives, vrais modules de circuits (Atari Punk, envelope-oscillator, Noise Cornucopia), caractère Turing Machine dans le séquenceur, self-feedback audio réel, cross-FM, bruit coloré.
- **Phases 24–48** — batterie réelle audible avec gigue de mixage par graine, déchirure apocryphe, interaction réelle avec la forme d'onde, mode album dans l'UI, variation de tempo, première harmonie fonctionnelle, performance en direct (filtre + VARIAR AGORA), modulation de centre tonal, modulation modale, vraies progressions d'accords, humanisation rythmique, effets par voix de percussion, hocketing, réseau d'influence croisée, intonation libre, garantie d'intensité (chaque pièce se teste contre au moins un mécanisme de risque), Fracture Folder + Rungler, Leibniz Rectifier, voix de tuba, contagion du chaos, thermal fade, effondrement de Rössler + self-epitaph, stochastic mute, et l'expansion des gammes de 9 à 39 avec le contrôle ESCALA retiré de l'UI.
- **Phases 49–68** — Agon (quatre voix en compétition pour le pouvoir, avec une « rusga »), élément étrange (interne et littéral via une vraie lecture de l'hôte), débordement en virgule fixe, spectral ghost, partition .txt compagnon, sample-hold glitch, dynamique de batterie étendue, vrai rythme euclidien, oscillateur Walsh, contagion inter-domaines (audio→horloge), gamme générée par crible de Xenakis, vraie conduite de voix, cascade polyphonique, lecture de pas adressée par le chaos, transformations néo-riemanniennes, batterie comme un vrai instrument (à partir d'une vraie écoute d'un album complet), archétypes rythmiques avec une identité réelle, accelerando vers le climax.
- **Phases 69–76** — marché de probabilités (prix persistés entre les rendus), Parenthèse (redirection vers une boucle de bégaiement), Heisenberg (précision rythmique/de hauteur couplées inversement), VOSIM, battement binaural chaotique, enveloppe d'asphyxie par sidechain, filtre Polivoks auto-oscillant, Vector Synthesis (vrai crossfade 2D).
- **Phases 77–82** — Harmonàig (pad à 4 voix, attribué à Jason Lim/Instruō), polyphonie de cribles de Xenakis (4 canaux), Bloom (mélodie fractale, attribuée à Andrew Ikenberry/Qu-Bit Electronix), poids de jeu de Caillois (phase 80, un tournant explicite), correction d'un vrai bug d'instabilité soutenue dans le `ModalResonator` du tuba/bois (phase 81), recherche rythmique approfondie — deux vrais archétypes aksak + un crible rythmique de Xenakis généré (phase 82).
- **Phase 83** — fin de l'affinité tonale forcée de l'album, remplacée par une trajectoire en ligne droite à travers les poids de jeu de Caillois ; chaque piste choisit librement sa propre gamme.
- **Phase 84** — « Curto-Circuito » (« Court-Circuit ») : une vraie coupure sèche (porte + étincelle de bruit) qui, au même instant, donne aux mécanismes que la graine avait laissés désactivés une seconde chance de s'imposer dans la pièce.
- **Refonte du layout de l'application** (entre les phases 82 et 83) — identité visuelle changée du vert phosphorescent (« Sinal ») au cuivre ; le vrai logo de l'auteur vectorisé et appliqué comme emblème ; le panneau de modules étendu en une grille couvrant ~50 mécanismes ; oscilloscope stéréo ; nouveaux panneaux ESPECTRO/NÍVEL/GRANULAÇÃO/PESOS DE JOGO (bancs Goertzel, vumètre RMS/crête, particules, barres colorées) ; BPM et MASTER en curseurs horizontaux ; vraies icônes pour COMPOR/PLAY ; ABOUT/TUTORIAL thématisés ; la fenêtre s'ouvre maximisée avec un repli de défilement.

---

## 7. Validations

### 7.1 Ce qui existe

- **`regression_test.py`** — compile le moteur, rend un lot de graines, mesure la durée/le pic/la fraction d'écrêtage/l'historique des sections. Critères : durée 15–300s, écrêtage < 0,05%, historique non vide.
- **`autotest.sh`** — compile l'application, l'exécute avec `RASGO_AUTOTEST=1`, confirme la ligne « gravado : » dans le journal et une console native propre.

### 7.2 Ce qui a réellement été vérifié

Chaque phase depuis la 13 est passée par `regression_test.py` (lots de 15–20 graines) avec zéro échantillon saturé, et l'application a été recompilée et a passé `autotest.sh` après chacune. Au-delà de cela, chaque nouveau mécanisme contrôlé par la graine depuis la phase 49 a reçu un **audit empirique de taux de déclenchement** (rendre un lot de graines et confirmer dans les métadonnées que le mécanisme se déclenche réellement au taux attendu, pas seulement qu'il compile) — une discipline qui a déjà attrapé de vrais bugs (un taux de collision de 11,6% dans le crible de hauteur avant sa correction, un compteur d'écart dans la mauvaise unité dans Agon, l'instabilité du `ModalResonator` du tuba). Plusieurs phases sont nées directement d'une **vraie écoute** de l'utilisateur, pas seulement d'une mesure : la batterie (phase 66), l'audibilité du tuba (phase 81), et l'analyse même de forme d'onde/spectrogramme qui a donné naissance à Curto-Circuito (phase 84).

### 7.3 Ce qui n'a pas encore été vérifié

Toutes les combinaisons possibles des ~50 mécanismes du moteur n'ont pas été écoutées à la main — la couverture vient d'audits empiriques des métadonnées (confirmant un vrai déclenchement) plus une écoute ciblée guidée par des retours spécifiques de l'utilisateur, pas un balayage exhaustif de toutes les combinaisons.

---

## 8. Sécurité

- **Exécution entièrement locale** — aucun réseau, télémétrie ou collecte de données personnelles dans aucun composant du moteur ou de l'application.
- **Aucune donnée sensible ni identifiant** nulle part dans le code ou les scripts de build/test.
- **Attention à l'espace disque** et à `recordings/` — c'est le vrai répertoire de sortie de l'utilisateur (vrais albums, index accumulé) ; les scripts de test écrivent dans des sous-dossiers isolés ou nettoient exactement le fichier qu'ils viennent de créer, jamais un balayage large.
- **Licence AGPL-3.0-or-later** du code original actuel ; texte intégral dans `LICENSE`.
- **Compilation sans privilèges élevés** — la compilation (g++ direct) ne nécessite pas sudo.

---

## 9. Layout — l'identité visuelle « Cobre » (Cuivre)

L'identité visuelle est passée de « Sinal » (vert phosphorescent sur presque-noir froid) à **« Cobre »/« Cuivre »** : la couleur d'accent est un orange cuivré oxydé (`#e8865f`), choisi parce qu'il fait directement référence à l'objet physique réel derrière le nom « rasgo » — des panneaux Eurorack gravés à l'acide que l'auteur construit à la main depuis 2013. Le fond reste presque noir, désormais avec un biais chaud plutôt que froid.

- **Emblème** — le vrai logo de l'auteur (sa propre vectorisation Inkscape) chargé via `juce::Drawable::createFromSVGFile`, teinté de cuivre, appliqué à côté du titre.
- **Palette étendue** — `Palette::action` (doré, `#f2c14e`) pour les boutons d'action primaire (COMPOR/PLAY) ; `Palette::danger` (rouge) réutilisé spécifiquement comme couleur propre de COMPOR ; un petit cycle cuivre/rouge/doré (`vizAccent`) colore les barres des nouveaux panneaux (poids de jeu, spectre par bande, vumètres L/R) au lieu de tout en cuivre monochrome.
- **Vraies icônes** — COMPOR utilise le glyphe Unicode de croche (♪, plus fiable qu'un tracé dessiné à la main) ; PLAY utilise un triangle/deux barres selon l'état de lecture.
- **Oscilloscope stéréo** — deux voies indépendantes (L/R), doré, une fenêtre temporelle plus longue qu'avant.
- **Typographie** — monospace partout dans le titre, les étiquettes et le journal ; tailles augmentées par rapport à la version « Sinal ».
- **Texture de fond** — une grille de graticule d'oscilloscope derrière toute la fenêtre.
- **Fenêtre** — s'ouvre maximisée par défaut (`setFullScreen`), avec un vrai repli de défilement (`juce::Viewport`) sur les moniteurs plus petits que ce que le layout confortable (1100×760) exige.

**Tentative annulée** : un effet visuel de « déchirure » (papier déchiré traversant le panneau de contrôle et/ou le haut de la fenêtre) a été implémenté puis complètement supprimé à la demande de l'utilisateur — ne pas le réintroduire sans une nouvelle demande explicite.

---

## 10. Points en suspens et prochaines étapes

Voir la liste vivante complète dans `CONTINUIDADE.md` (« Tarefas em aberto », portugais uniquement) et dans le Plano de Upgrade (« Em aberto »). Résumé de ce qui reste réellement ouvert :

- **Une « note de bas de page » et une voix narrante** — le mécanisme de note de bas de page existe ; une voix littéraire plus riche reste ouverte.
- **Une synthèse vocale plus physique** — Scat et Chœur existent comme voix originales à formants ; un sous-système complet de conduit vocal reste une frontière à long terme.
- **L'expansion des rôles relationnels** — thérémine, flûte FM, tuba, voix, polyphonies et archive nécessitent encore des contrats causaux plus profonds.
- **De vrais changements de mesure/signature rythmique** — se connecte à la recherche rythmique (phase 82), délibérément hors de portée jusqu'à présent (changerait l'ossature rythmique fixe à 16 pas).
- **La console embarquée de la Version Performance** — une étape après la Version Studio, par décision explicite de l'utilisateur.
- **Synchronisation documentaire** — les quatre documents publics principaux sont synchronisés en EN/FR/ES/PT ; les audits internes de phases et le plan vivant restent des documents de travail en portugais.

---

## 11. Références et influences

Un vide n'explique pas Rasgo Synth ; c'est un projet de synthèse générative qui se déclare, honnêtement, en dialogue direct avec de vrais circuits, des instruments commerciaux, des articles académiques et la trajectoire propre de l'auteur. Aucun code source d'un autre instrument ni aucun schéma électrique exact n'a été copié : les comportements ont été implémentés originalement en C++ à partir de concepts et de documentation publics. Voir `CREDITS.md` pour le tableau complet par mécanisme.

### 11.1 Trajectoire d'auteur et référence externe initiale

- **Antitotem et modules physiques de Rasgo (2013–)** — œuvres construites par l'auteur. Antitotem précède Rasgo, fut incorporé comme son précurseur et son patron.
- **Kosmische (2013)** — instrument génératif Android de Nicolas Steven Miller (github.com/zrnsm/kosmische, GPLv3), référence technique et conceptuelle initiale, ne fait pas partie de Rasgo.

### 11.2 Matériel commercial et circuits DIY publiquement documentés

- **Roland TR-808** — référence sonore pour une partie des huit voix de percussion.
- **Music Thing Modular — Turing Machine** (Tom Whitwell) — le caractère de verrouillage/mutation par pas du séquenceur.
- **Mutable Instruments** (Émilie Gillet) — Rings, Clouds, Branches, réinterprétés.
- **Instruō — ochd** (Ben « DivKid » Wilson) et **Harmonàig** (Jason Lim) — le banc de LFO et le pad d'accord à 4 voix, respectivement.
- **Qu-Bit Electronix — Bloom** (Andrew Ikenberry) — la mélodie fractale Trunk/Branch/Path.
- **Frap Tools — Sapèl** — quatre couleurs de bruit.
- **Make Noise — Maths** / mixage sans entrée — vrai self-feedback audio.
- **Buchla** — processeurs leveler/lag.
- **Rob Hordijk — Benjolin** — cross-FM bidirectionnel.
- **Ken Stone — CGS22 « Master Divider »** — le diviseur d'horloge XOR polyrythmique.
- **Forrest Mims III — Stepped Tone Generator** (« Atari Punk Console ») — texture stepped/glitch.
- **Ray Wilson — MFOS « Noise Cornucopia »** — bruit granuleux et porte aléatoire.
- **General Instrument AY-3-8910** — arpège/texture chiptune.
- **Pantala Labs** — mode delay lo-fi, saturateurs bande/lampe.
- **Robert Moog ; Tim Stilson et Julius O. Smith** — le filtre ladder.
- **Hal Chamberlin** — le filtre state-variable.
- **Polivoks (Formanta, URSS)** — la troisième couleur de filtre, vraie auto-oscillation.
- **Rob Hordijk/Benjolin, Xaoc Devices Leibniz-Rectifier** — Rungler et rectification/corruption de bits.
- **Sequential Circuits Prophet VS** — vrai crossfade 2D (Vector Synthesis).
- **Technique de sidechain/ducking de la production techno/EDM** — l'enveloppe d'asphyxie.

### 11.3 Mathématiques, informatique et synthèse académique

- **Iannis Xenakis** — Synthèse Stochastique Dynamique (« Gendyn ») ; et, dans *Formalized Music* (chapitre XI, « Sieves: A User's Guide »), la théorie des cribles par congruence de module/résidu, appliquée aussi bien aux classes de hauteur (phase 61) qu'aux points dans le temps (phase 82).
- **Joe Pritchard — *The Chaos Cookbook* (1992)** ; **Lance Putnam — Chaos UGens de SuperCollider (2004)** — références de recherche pour les neuf cartes chaotiques.
- **Vesa Välimäki et Antti Huovilainen — PolyBLEP** — l'anti-aliasing des oscillateurs.
- **Dither TPDF** — une technique standard de traitement audio numérique.
- **Werner Kaegi et Stan Tempelaars, Institut de Sonologie d'Utrecht — VOSIM** — synthèse par impulsions sin² décroissantes.
- **Constantin Brăiloiu ; Stoyan Dzhudzhev** — la théorie du rythme additif/aksak balkanique, deux archétypes réels (9+7 et 7+7+2).
- **David Lewin ; théorie néo-riemannienne** — transformations triadiques P/L/R.

### 11.4 Structures narratives, ludologiques et compositionnelles

- **Gustav Freytag — la Pyramide de Freytag** (1863) — la grammaire narrative « Dramatic ».
- **Kishōtenketsu** — la structure narrative chinoise/japonaise en 4 parties (qǐ-chéng-zhuǎn-hé).
- **Roger Caillois — *Les Jeux et les Hommes*** (1958) — les quatre catégories de jeu (Alea, Agon, Ilinx, Mimicry), formalisées mécaniquement comme des poids continus influençant onze mécanismes existants (phase 80).

Quiconque souhaite approfondir l'un de ces points de départ avant de porter la prochaine idée doit maintenir le même standard d'honnêteté : citer la vraie source, comprendre le comportement avant de réimplémenter, et indiquer clairement dans cette section (ou dans la phase correspondante du `Plano_de_Upgrade_Instrumento_Generativo.md`) que l'idée dialogue avec le travail d'autrui, pas qu'elle est née isolée.
