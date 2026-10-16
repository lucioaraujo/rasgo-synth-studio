# Rasgo Synth Studio — Manuel

Version expérimentale 0.1.1 — 2026

Révisé le 2026-10-12 selon les contrôles et contrats actuels. [Tutoriel pas à pas](../../website/quickstart.html#fr).

## 1. Concept

Rasgo Synth Studio est un instrument musical génératif : vous choisissez des conditions de composition, puis il synthétise une pièce entière. La seed est un point de départ ; tempo, sélection des modules, expression, direction musicale et état pertinent interviennent aussi. Ce n’est ni un clavier joué note par note ni un rack câblé manuellement.

Les sons sont synthétisés, et non assemblés à partir d’une bibliothèque externe d’échantillons enregistrés. Gammes, rythmes, voix, textures et forme narrative occupent un espace de possibilités d’auteur. Une pièce peut répéter, transformer, interrompre ou laisser un matériau silencieux ; la variation ne garantit pas un nouveau son à chaque événement.

MODE SAUVAGE est activé par défaut et ajoute de l’entropie pendant le rendu. Pour comparer de manière contrôlée, désactivez-le et conservez seed, tempo, choix, version et état externe pertinent. La seed seule ne garantit pas un audio identique.

La durée d’une pièce seule émerge du récit. En mode album, vous pouvez la conserver ou activer une planification de durée. Studio compose/calcule d’abord et lit ensuite ; les contrôles d’écoute ne réécrivent pas le WAV sauvegardé en temps réel. Studio et Performance sont des instruments distincts, avec des publications indépendantes.

## 2. Histoire

Rasgo Synth est une œuvre au sein de Rasgo, le projet-cadre de Lucio de Araujo pour ses instruments et logiciels musicaux d'auteur. Antitotem précède Rasgo et a ensuite été incorporé comme son précurseur et son patron. Les modules physiques de Rasgo étaient déjà conçus en visant le standard Eurorack, depuis 2013 ; aujourd'hui le nom couvre un projet d'instruments musicaux plus large.

Kosmische est un instrument Android GPLv3 distinct, de Nicolas Steven Miller. Ce fut l'une des références conceptuelles étudiées au début de Rasgo Synth. Il ne fait pas partie de Rasgo, et Rasgo Synth n'en est ni un fork ni une continuation d'auteur.

L'implémentation spécifique au projet a été réécrite de zéro en C++17 et JUCE, sous la direction de Lucio de Araujo, avec Anthropic Claude utilisé comme outil de développement. Aucun code source de Kosmische ou d'Antitotem n'a été copié. L'instrument a ensuite reçu de nombreux systèmes, voix et mécanismes compositionnels au-delà de ces références initiales -- au dernier décompte, plus de quatre-vingts phases de développement documentées.

## 3. Recherche et provenance

Les neuf cartes chaotiques sont des implémentations indépendantes de récurrences mathématiques publiées. Kosmische et les Chaos UGens de SuperCollider, créés par Lance Putnam, ont été examinés comme contexte technique ; *The Chaos Cookbook* est le livre de Joe Pritchard publié en 1992. Ces références ne font pas des implémentations de Rasgo des ports de code source.

Les instruments matériels et logiciels cités par Rasgo sont des références de conception, non des revendications de paternité ou d'émulation exacte. Le résonateur modal, la texture granulaire, les sources aléatoires, les filtres et les modules inspirés de circuits sont des interprétations comportementales originales basées sur des concepts publics, de la littérature, de la documentation et des schémas. Les créateurs et collaborateurs historiques sont nommés dans [CREDITS.md](../../CREDITS.md).

Les huit voix de percussion sont des stylisations numériques originales, inspirées en partie du comportement documenté de la Roland TR-808. Elles n'utilisent aucun échantillon, ne copient aucun firmware et ne sont pas des répliques de circuits au niveau transistor. Chaque pièce choisit parmi huit archétypes rythmiques possibles : Soul, Jazz, Metal, Marche, Samba, deux archétypes aksak balkaniques réels (documentés par Constantin Brăiloiu et Stoyan Dzhudzhev), et un crible rythmique généré algorithmiquement par la théorie des cribles de Xenakis.

Pour la liste faisant autorité en matière de provenance, voir [CREDITS.md](../../CREDITS.md). Les bibliothèques incorporées et leurs conditions de licence sont répertoriées séparément dans [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## 4. Références sélectionnées

- Lucio de Araujo : Rasgo et Antitotem.
- Nicolas Steven Miller : Kosmische (GPLv3), référence conceptuelle externe.
- Joe Pritchard : *The Chaos Cookbook* (1992).
- Lance Putnam : Chaos UGens de SuperCollider (2004).
- Forrest M. Mims III : le Stepped-Tone Generator, popularisé sous le nom d'Atari Punk Console.
- Iannis Xenakis : synthèse stochastique dynamique et théorie des cribles (*Formalized Music*, chap. XI).
- Instruō, Ben « DivKid » Wilson (ochd) et Jason Lim (Harmonàig).
- Qu-Bit Electronix, Andrew Ikenberry : Bloom.
- Émilie Gillet / Mutable Instruments : concepts DSP Eurorack publiés.
- Roland Corporation : documentation de la TR-808 et référence de conception sonore.
- Roger Caillois : *Les Jeux et les Hommes* (1958), les quatre catégories de jeu (Alea/Agon/Ilinx/Mimicry).
- Constantin Brăiloiu, Stoyan Dzhudzhev : théorie du rythme additif/aksak balkanique.
- Les autres créateurs listés dans [CREDITS.md](../../CREDITS.md).

## 5. Licence

Le code original de Rasgo Synth Studio est distribué sous la GNU Affero General Public License version 3 ou ultérieure (AGPL-3.0-or-later) ; voir `LICENSE`. La licence GPLv3 de Kosmische s'applique à Kosmische et n'établit ni paternité partagée ni code copié.

L'application de bureau utilise JUCE. Le code source local de JUCE 9 propose l'AGPLv3 ou une licence commerciale JUCE ; quiconque distribue une build combinée doit donc respecter les conditions JUCE applicables en plus de la licence propre de Rasgo Synth. Les autres composants intégrés ou liés conservent leurs licences respectives ; voir [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## 6. Tutoriel

### Avant de commencer

Ce guide décrit Studio 0.1.1 expérimental, et non Rasgo Modular ou Performance. Gardez un volume faible, choisissez un dossier accessible en écriture et travaillez sur une pièce à la fois. L’icône de croche ♪ lance la composition ; PLAY lit l’audio déjà calculé. Les noms ci-dessous correspondent à l’interface française.

### Votre première pièce — six étapes

1. Vérifiez la sortie audio, baissez MASTER et choisissez le dossier de sortie. Saisissez éventuellement titre et artiste ; les fichiers y seront sauvegardés.

    Le champ artiste est facultatif : vide, il n'attribue aucun artiste aux
    fichiers audio. Le nom saisi est conservé dans vos préférences. Le dossier
    initial est `Rasgo Synth` dans votre propre dossier Musique ; il est modifiable.
2. Réglez SEED et TEMPO. Désactivez MODE SAUVAGE pour cet exercice. Laissez DIRECTION MUSICALE et EXPRESSIF désactivés et les modules en Automatique comme référence.
3. Appuyez sur COMPOSER (♪). Attendez le rendu et la confirmation de sauvegarde ; PLAY ne lance pas la composition.
4. Appuyez sur PLAY. Écoutez sans modifier de paramètres : repérez entrées, sorties, silences, registre grave, percussion et textures.
5. Glissez sur la forme d’onde pour sélectionner une région, puis utilisez LIRE LA RÉGION. SEED DE LA RÉGION dérive un nouveau départ, sans exporter d’échantillon. Recomposez pour l’entendre.
6. Gardez le WAV et les fichiers compagnons. Utilisez ALEA pour une autre seed, ou suivez les exercices en changeant un choix à la fois.

### Seed et tempo

Ce que c’est : SEED choisit un point de départ ; TEMPO définit le tempo initial d’une pièce.

Comment le comprendre : la seed est une condition de composition, pas un nom de preset expliquant tout. Le moteur peut transformer le tempo dans la pièce. La gamme est choisie par le moteur ; ce panneau n’a pas de sélecteur manuel de gamme.

Contrôles : SEED, ALEA, TEMPO et MODE SAUVAGE. En mode album, le thème fournit le BPM de base ; le curseur de la pièce est désactivé.

Essayez : notez seed et tempo, calculez avec MODE SAUVAGE désactivé et écoutez. Conservez la seed, changez seulement TEMPO et recalculez. Comparez durée des gestes, espacement et énergie perçue ; ce n’est pas seulement un changement de vitesse du même WAV.

### Choisir qui peut jouer

Ce que c’est : CHOISIR LES MODULES ouvre 34 participants parmi sources, textures et batterie.

Comment le comprendre : vous choisissez une possibilité de participation, pas un solo ou mute de l’audio actuel. Inclure autorise la participation ; notes, entrées, silences et dépendances déterminent toujours le jeu. Les autres mécanismes et effets restent automatiques. La grille d’activité n’est pas cette liste de contrôles.

Contrôles : Automatique conserve le choix génératif ; Inclure autorise la source désignée ; Exclure la bloque. Confirmez avec APPLIQUER AU PROCHAIN RENDU. Fermer sans appliquer abandonne les modifications ; TOUS AUTOMATIQUES réinitialise le brouillon avant confirmation. Redémarrer l’application réinitialise les choix.

Essayez : conservez seed, tempo et modes. Incluez Couche de bruit coloré, appliquez et calculez. Excluez ensuite cette même couche et recalculez. Comparez texture et espace occupé par le bruit. D’autres générateurs peuvent produire du bruit : Exclure ne rend pas silencieuse toute une famille sonore.

### Batterie et groove

Ce que c’est : grosse caisse, caisse claire, charleston, cloche, clap, toms, cymbale et maracas sont synthétisés, sans samples de batterie.

Comment le comprendre : les participants ont des rôles rythmiques, mais Studio n’offre pas de réglage individuel d’accordage ou de volume. Exclure Batterie bloque ses éléments même si Grosse caisse est marquée Inclure. Inclure n’impose pas des frappes constantes.

Contrôles : CHOISIR LES MODULES ; activez DIRECTION MUSICALE et EXPRESSIF avant de composer pour explorer les études supplémentaires de groove et d’interprétation. Seed et sections choisissent familles et réponses ; aucun sélecteur manuel de genre ou de polyrythmie 3:2.

Essayez : incluez Batterie, Grosse caisse, Caisse claire et Charleston, puis calculez. Sans rien changer d’autre, excluez Cloche et Maracas et recalculez. Écoutez si la pulsation devient plus lisible. Ne montez pas MASTER pour compenser un seul élément : le déséquilibre du mixage est une limite connue, pas votre erreur.

### Registre grave et ligne de basse

Ce que c’est : Basse acid et Tuba sont des sources sélectionnables ; la basse interne core-sub est une fonction distincte de Voix principale.

Comment le comprendre : fréquence grave, ligne de basse musicale et volume sont différents. Une basse nécessite événements, registre et articulation ; le nom du module ne garantit pas sa présence claire dans le mixage. Le repli core-sub n’a pas d’entrée propre dans le sélecteur.

Contrôles : sélection Basse acid/Tuba et DIRECTION MUSICALE + EXPRESSIF. Il n’y a pas de fader individuel de basse ni de bouton solo.

Essayez : avec les deux modes actifs, incluez Basse acid et excluez Tuba ; appliquez et calculez. Inversez ensuite ce choix. Écoutez attaque, tenue, silences et relation à la grosse caisse. Exclure les deux peut laisser le repli disponible selon le plan musical ; sa reconnaissance n’est pas garantie. Signalez seed et choix si le grave est masqué.

### Mélodie, timbre et relations

Ce que c’est : des voix comme Flûte FM, Thérémine et Chœur, et des textures comme Granulaire, offrent différentes matières sonores.

Comment le comprendre : sélectionner n’est pas tourner les knobs d’un rack. Les trajectoires sont calculées au rendu. Chœur, Scat et Harmonaig dépendent de la progression d’accords générée ; une dépendance absente peut bloquer leur participation, indiquée par `blockedBy` dans les métadonnées.

Contrôles : CHOISIR LES MODULES, DIRECTION MUSICALE et EXPRESSIF. Direction organise entrées, retraits et relations ; Expressif ajoute une interprétation expérimentale. Les deux options sont distinctes, désactivées par défaut et nécessitent un nouveau rendu.

Essayez : calculez une référence avec les deux modes désactivés. Activez seulement Direction et calculez ; activez ensuite aussi Expressif et recalculez. Gardez seed, tempo, modules et Audit des stems identiques. Comparez silences, réponses, attaques et timbre, pas seulement volume. Le contraste peut être discret ; émotion accrue ou dialogue audible ne sont pas garantis dans chaque pièce.

### Écoute et visualisations

Ce que c’est : forme d’onde, spectre, spectrogramme et autres vues aident à observer l’audio.

Comment le comprendre : un indicateur de module montre une activité, pas une preuve d’audibilité ou de qualité musicale. Les animations du website sont illustratives, pas l’audio de votre application.

Contrôles : PLAY, position dans la forme d’onde, LIRE LA RÉGION, MASTER et brillance d’écoute. Ils ne réécrivent pas le WAV sauvegardé. Dans une petite fenêtre, faites défiler verticalement pour atteindre log et vues.

Essayez : écoutez une région et reliez une frappe à la forme d’onde ; cherchez ensuite une texture tenue dans le spectre. Une faible amplitude dessinée ne prouve pas à elle seule un mauvais mastering. Si vous n’identifiez pas une voix, signalez-le au lieu d’augmenter le volume sans limite.

### Votre premier album

Ce que c’est : MODE ALBUM crée une collection à partir d’une seed, d’un thème et d’un plan.

Comment le comprendre : le thème fournit le BPM de base ; les choix concernent les prochains rendus, pas les pistes déjà sauvegardées. Il n’y a pas encore de mémoire compositionnelle qui réapprend les thèmes entre pistes.

Contrôles : thème, PISTES (1–40 ; défaut 8), VARIER LE BPM (désactivé ; amplitude 0–35 %, défaut 15 %), trajectoire équilibrée/contrastante/croissante/arc. PLANIFIER LA DURÉE est aussi désactivé au départ. Activé, il offre un objectif total de 1–45 minutes et une répartition équilibrée/contrastante/libre. Le plan limite les pistes à 15 secondes–8 minutes et le total à 45 minutes ; l’aperçu distingue total demandé et prévu si la demande ne tient pas.

Essayez : choisissez 2 pistes, activez PLANIFIER LA DURÉE et demandez 2 minutes au total. Lisez l’aperçu ; activez VARIER LE BPM pour comparer les tempos initiaux. Composez et écoutez avec le sélecteur ÉCOUTER. PLAY rétablit les vues ; CONFIGURER L’ALBUM rouvre les options. La durée musicale prévue n’est ni le temps de calcul ni une coupe appliquée après coup au WAV.

Les passes de calibration précèdent les rendus finaux. `album_plan.json` enregistre le plan et les fichiers effectifs ; un album interrompu n’est pas terminé. Sélection et modes expressifs s’appliquent aux pistes suivantes. Audit des stems hérite du choix fait en mode pièce, malgré son contrôle désactivé en mode album : réglez-le avant d’y entrer. Son ancienne infobulle ne décrit pas correctement cet héritage.

### Progression, fichiers et problèmes

Ce que c’est : le pourcentage suit les frames musicales de la passe actuelle, pas le temps réel écoulé.

Comment le comprendre : 100 % de composition peut encore être suivi du traitement final et de la sauvegarde. Pour l’album, pourcentage et calibration ne constituent pas une estimation unique du temps restant. Audit des stems ajoute analyse et traitement et peut modifier le résultat, pas seulement le temps de calcul.

Contrôles : log, COPIER, dossier de sortie, À PROPOS et TUTORIEL. À PROPOS indique version et date/heure de compilation. Fermer pendant un rendu demande l’annulation et attend un checkpoint ; une écriture en cours peut devoir finir. Cela ne garantit pas la récupération après crash ou coupure de courant.

Essayez : après la première pièce, trouvez le WAV et ses compagnons. En cas d’échec, copiez le log et indiquez version, système, appareil, seed, modes, sélection et action précédente. Ne publiez pas automatiquement vos enregistrements personnels. Évitez rendus/builds simultanés sous pression mémoire.

### Limites de cette version

Une publication expérimentale n’est pas un mixage approuvé : basse discrète, percussion dominante et timbres récurrents restent possibles. Mixage génératif et diversité interprétative restent à développer après publication. Bruit, silence et rupture sont des possibilités, pas une esthétique imposée. Réviser les instructions ne remplace pas validation d’écoute, d’accessibilité ou de tous les claviers/langues.

[Vue d’ensemble](Instrument_Integral.md) · [Crédits](../../CREDITS.md) · [Méthode rythmique et références](../GROOVE_RHYTHM.md)
