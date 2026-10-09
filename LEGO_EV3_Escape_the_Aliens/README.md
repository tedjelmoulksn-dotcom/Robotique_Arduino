# Escape the Aliens — robot LEGO Mindstorms EV3

## Vue d'ensemble
Programme graphique LEGO Mindstorms EV3 pour un robot mobile à deux moteurs qui avance, évite les obstacles détectés par deux capteurs infrarouges et s'arrête sur appui d'un capteur tactile. Fichier daté de novembre 2024. Contexte (cours, club de robotique, animation) : À documenter.

## Objectifs
- Programmer un comportement d'évitement d'obstacles simple.
- Faire tourner deux tâches en parallèle sur la brique EV3.
- Prévoir un arrêt par capteur tactile.

## Architecture
Le bloc de départ lance **deux boucles infinies en parallèle**, une par capteur infrarouge :

```mermaid
flowchart TD
 S[Démarrage] --> L1[Boucle 1]
 S --> L2[Boucle 2]
 L1 --> T1{Capteur tactile port 1<br/>relâché ?}
 T1 -- oui --> IR4{Proximité IR port 4<br/>≤ 100 ?}
 IR4 -- oui --> A1[Note A5 1 s<br/>moteurs B+C : 0 / 50, 1 tour<br/>attente 1 s, arrêt]
 IR4 -- non --> F1[Avance B+C à 14 %]
 T1 -- non --> STOP1[Boucle infinie : arrêt des moteurs]
 L2 --> T2{Capteur tactile port 1<br/>relâché ?}
 T2 -- oui --> IR3{Proximité IR port 3<br/>≤ 100 ?}
 IR3 -- oui --> A2[Son 440 Hz 1 s<br/>moteurs B+C : 40 / 0, 1 tour<br/>attente 1 s, arrêt]
 IR3 -- non --> F2[Avance B+C à 14 %]
 T2 -- non --> STOP2[Boucle infinie : arrêt des moteurs]
```

Chaque boucle attend 1 s à la fin de chaque itération.

## Matériel
- Brique LEGO Mindstorms EV3.
- Moteurs sur les ports B et C (pilotage « Move Tank »).
- Capteur tactile sur le port 1.
- Deux capteurs infrarouges sur les ports 3 et 4.

## Logiciel
LEGO Mindstorms EV3 (environnement de programmation par blocs, fichier `.ev3`).

## Implémentation
Paramètres relevés dans `Program.ev3p` (contenu de l'archive `.ev3`) :

| Boucle | Capteur IR | Réaction quand la condition est vraie | Sinon |
|---|---|---|---|
| 1 | port 4 | note A5 (1 s), moteur gauche 0 % / droit 50 % pendant 1 tour → pivot d'un côté | avance à 14 % / 14 % |
| 2 | port 3 | son 440 Hz (1 s), moteur gauche 40 % / droit 0 % pendant 1 tour → pivot de l'autre côté | avance à 14 % / 14 % |

## Principes d'ingénierie
- Programmation multitâche (deux séquences parallèles).
- Évitement réactif : chaque capteur commande un pivot vers le côté opposé.
- Arrêt de sécurité par capteur tactile.

## Résultats
Aucune vidéo ni mesure conservée : À documenter.

## Difficultés / limites (observations sur le fichier)
- Le seuil de proximité est réglé à « ≤ 100 », alors que la proximité infrarouge EV3 varie de 0 à 100 : la condition est donc toujours vraie et le robot pivoterait à chaque itération. Le seuil prévu est à vérifier.
- Les deux boucles commandent les mêmes moteurs (B+C) en parallèle : leurs ordres peuvent se superposer.
- L'environnement EV3 enregistre l'année « 2018 » dans le modèle de projet ; ce n'est pas la date du travail.

## Structure
```
LEGO_EV3_Escape_the_Aliens/
├── README.md
├── .gitignore
└── src/
    └── escape_the_aliens.ev3   (archive du projet : Program.ev3p, Project.lvprojx…)
```

## Exécution
Ouvrir `src/escape_the_aliens.ev3` dans le logiciel LEGO Mindstorms EV3 (ou EV3 Classroom), brancher la brique et télécharger le programme.

## Médias
À documenter (photo du robot, vidéo).

## Compétences
Programmation de robot mobile, capteurs infrarouges et tactile, multitâche, LEGO Mindstorms EV3.
