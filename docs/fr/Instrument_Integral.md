# Rasgo Synth — vue d’ensemble de l’instrument

Ce guide explique l’instrument entier : ce qui entre dans le système, comment
une pièce est construite, comment lire le Studio et comment le catalogue des
modules se rapporte à la musique.

## Studio et Performance

Rasgo Synth Studio est l’instrument de bureau existant pour composer, rendre,
écouter et faire de la recherche. Rasgo Synth Performance est un instrument
frère de DSP live, avec contrats, moteur, hôte standalone et une première
surface locale de quatre scènes et cinq gestes. Il n’est pas encore publié : les
validations graphique, de périphérique, musicale et de build portable restent à
faire. Les contrôles live du Studio ne constituent pas une release Performance.

## De la seed à la pièce

Rasgo Synth est un instrument logiciel génératif, pas une boucle fixe, un
lecteur de samples ni un clavier joué note par note. Une seed choisit une
trajectoire dans un espace d’identité, d’échelles, de rythmes, de voix, de
probabilités, de formes narratives et de matières sonores. Le moteur rend
ensuite une pièce complète.

Le parcours comprend neuf couches : identité; jeu (Alea, Agon, Ilinx, Mimicry);
temps; matière; forme; relations; expression; attention et mixage; puis
enregistrement. Le temps comprend grille, pulsation, subdivision, swing,
microtiming, humanisation, gate, probabilité, phase, polymétrie, dérive et
mémoire. La forme suit une marche de Markov à travers ouverture,
développement, climax, rupture, décantation et clôture.

Tous les modules ne sont pas actifs dans chaque seed. `Disponible`,
`manifesté`, `audible` et `influent` sont des états différents. La présence de
la flûte, de la tuba ou d’un corps de basse ne résume jamais l’instrument.

## Basse, mélodie et percussion

La basse est séparée en fonction, phrase et corps. La fonction peut être une
fondamentale, une pédale, un chemin, une tension, un contrepoint ou un espace.
La phrase règle registre, direction, répétition, relation avec le kick et
densité. Le corps peut être une tuba synthétique, une voix acid ou un
`core-sub` de secours. Le corps ne décide pas seul de la ligne.

La mélodie utilise échelles, conduite de voix, contour, voicing, dissonance,
intonation expressive et mémoire. La percussion possède des voix indépendantes,
des archétypes rythmiques, des structures euclidiennes et aksak, de l’articulation,
des variations timbrales et de l’interlocking. `TemporalGrooveImprint` alimente
la basse expressive et la percussion d’archétype avec une influence bornée.

## Lire le Studio

- **SEED / ALEA** : choisir ou dériver le point de départ.
- **TEMPO** : définir l’allure initiale; l’arc peut dériver ensuite.
- **MASTER** : régler le niveau final, pas l’énergie compositionnelle.
- **MODE SAUVAGE** : autoriser des trajectoires entropiques supplémentaires.
- **AUDIT DES STEMS** : mesurer survie post-espace et attention narrative.
- **A/B EXPRESSIF** : activer les comportements temporels et gestuels expérimentaux.
- **COMPOSER** : rendre la pièce en arrière-plan.
- **LANG** : passer entre anglais, portugais, français et espagnol.

Les panneaux lisent un même résultat : forme d’onde, oscilloscope, spectre,
spectrogramme, goniomètre, portrait de phase, granulation, généalogie de l’arc,
poids du jeu, grille des modules et log. Le fichier `.txt` compagnon est un
récit lisible des sections, décisions, voix et mécanismes; ce n’est pas une
partition traditionnelle.

L’audit des stems est une lentille de mesure, pas une condition d’écoute.
`COPIER` copie le log et `EFFACER` en efface l’affichage sans supprimer un
audio. Une région de la forme d’onde peut être écoutée ou servir à dériver une
nouvelle seed. `VARIER LA SEED` part d’un instant de lecture pour composer une
nouvelle pièce.

## Mode Album

Le Mode Album crée plusieurs pistes liées à partir d’une seed d’album.
`AlbumRenderer` dérive les seeds des pistes, conserve une continuité tonale
large et laisse à chaque piste sa propre narration et ses propres modules. Ce
n’est pas un long fichier découpé : c’est une famille de pièces.

## Flux et catalogue

Le flux d’architecture montre la causalité : la seed alimente identité,
séquençage, arc et DSP; l’arc alimente densité, registre et énergie; jeu,
relations et mix observent ou influencent dans des limites. Les flèches pleines
sont les flux principaux; les flèches pointillées indiquent une observation ou
une influence bornée.

Le catalogue web contient 142 mécanismes avec catégorie, description et
référence. Il décrit des possibilités, pas une promesse d’audibilité dans
chaque pièce. Voir le [guide portugais intégral](../pt/Instrumento_Integral.md)
et le [catalogue](../../website/assets/mechanisms.pt.json).

## Limites et suite

Certains contrats de performance, de mémoire, d’attention et de mixage restent
locaux ou observationnels. L’acceptation musicale demande donc télémétrie et
écoute humaine. La suite vise une basse plus lisible sans imposer un corps, un
contrepoint plus indépendant lorsque la forme le permet et une morphologie de
note étendue à d’autres familles de voix.
