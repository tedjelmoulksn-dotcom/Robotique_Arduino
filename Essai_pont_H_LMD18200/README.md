# Essai d'un pont en H LMD18200 avec Arduino

Croquis d'essai (octobre 2024) pour piloter un moteur à courant continu à travers un module à pont en H LMD18200.

- Broche 9 : entrée de direction (`DIR`).
- Broche 10 : PWM de vitesse (`analogWrite`, 200 sur 255, soit environ 80 %).
- Séquence : 5 s dans un sens, 5 s dans l'autre, arrêt 2 s (PWM à 0), puis reprise.

## À documenter

Moteur utilisé, tension d'alimentation, schéma de câblage et résultat de l'essai. Le fichier d'origine s'appelait `essaie_circuit_lmd1800T.txt` ; la référence exacte du module est à confirmer.

Classé dans `To_Review` : un seul fichier, à rattacher à un projet de robotique si le contexte se confirme.
