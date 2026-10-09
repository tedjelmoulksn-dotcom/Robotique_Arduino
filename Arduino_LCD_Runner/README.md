# Arduino LCD Runner

Mini-jeu de type « runner » sur écran LCD 16×2 avec Arduino : le personnage court automatiquement et doit éviter des obstacles en sautant grâce à un bouton.

## Vue d'ensemble

Projet personnel (avril 2026) : adaptation et modification d'un jeu existant pour Arduino et LCD, dans l'esprit du « Chrome Dino ».

> **Origine du code** : le programme part d'un code de jeu LCD pour Arduino diffusé dans un tutoriel tiers. Avant publication, citer précisément cette source ici, décrire les modifications apportées et vérifier que sa redistribution est permise.

## Objectifs

Comprendre et modifier un programme temps réel simple : affichage par caractères personnalisés, machine à états, entrée par interruption.

## Matériel

- Arduino Uno (ou compatible)
- Écran LCD 16×2 (contrôleur HD44780)
- Bouton poussoir

## Logiciel

IDE Arduino, C++. Bibliothèque et brochage : **à documenter** à partir du croquis.

## Implémentation

| Fonction | Rôle |
|---|---|
| `initializeGraphics()` | Définit les sprites dans la mémoire de caractères du LCD |
| `advanceTerrain()` | Fait défiler le décor |
| `drawHero()` | Affiche le personnage, détecte les collisions, met à jour le score |
| `loop()` | Logique principale du jeu |

## Principes d'ingénierie

- **Caractères personnalisés (CGRAM)** du LCD, limités à 8.
- **Deux tampons** (ligne haute et ligne basse) décalés vers la gauche pour simuler le défilement.
- **Obstacles pseudo-aléatoires.**
- **Machine à états** pour le personnage : course, puis saut en plusieurs phases.
- **Collision** par comparaison des tuiles.
- **Bouton sur interruption externe INT0.**

## Résultats

Le terrain défile, un appui déclenche un saut, une collision termine la partie et le score suit la distance parcourue. Photo ou vidéo du montage : **à ajouter**.

## Difficultés et limites

- Animation discrète, sans physique réelle.
- Boucle cadencée par `delay()`.

Améliorations envisagées : cadencement non bloquant avec `millis()`, difficulté progressive, sauvegarde du score en EEPROM, version I2C, buzzer.

## Structure du dépôt

```
arduino_lcd_runner.ino    Croquis (à exporter depuis le Google Doc en texte brut)
```

## Exécution

Ouvrir le croquis dans l'IDE Arduino, sélectionner la carte, téléverser.

## Compétences démontrées

- Pilotage d'un LCD HD44780 et caractères personnalisés.
- Interruptions externes sur AVR.
- Machine à états et logique de jeu en C++ embarqué.

## Licence

Aucune licence définie ; dépend de la licence du code d'origine.
