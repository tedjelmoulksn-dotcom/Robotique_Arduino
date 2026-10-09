# LEGO EV3 — Sensor-Based Obstacle Avoidance

A native Mindstorms EV3 exercise combining infrared input, touch-stop control and parallel program paths.

## Available program

[`src/escape_the_aliens.ev3`](src/escape_the_aliens.ev3) is the archived graphical project. Use an EV3 environment compatible with this legacy file format.

The documented configuration uses motors B/C, a touch sensor on port 1 and infrared inputs on ports 3/4.

## Control design

The intended behaviour is mobile obstacle avoidance with a touch-controlled stop. Two parallel control paths act on the same motor pair, illustrating why concurrent sensor responses require explicit arbitration.

Review the infrared comparison using a threshold of 100 against the actual sensor-mode range. A comparison spanning the entire usable range cannot distinguish nearby obstacles meaningfully.

## Reproducing

Inspect all motor commands and sensor modes in the graphical editor before transferring the program. Establish one ownership rule for motor output, then test sensor branches separately before combining them.

The repository does not provide a text-source equivalent or a recorded demonstration.

## Validation and licence

No robot test was rerun for this README update. The intended behaviour should be distinguished from verified runtime behaviour. No project-wide licence has been defined.
