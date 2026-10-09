# Arduino LCD Runner — Interrupts and Character Graphics

A small obstacle-avoidance game on a 16×2 character LCD. A button requests a jump while the terrain scrolls; collision ends the run and travelled distance drives the score.

## Embedded implementation

| Function | Role |
|---|---|
| `initializeGraphics()` | Loads custom character patterns |
| `advanceTerrain()` | Shifts upper/lower terrain buffers |
| `drawHero()` | Draws the character and checks collisions |
| `loop()` | Advances the game state and display |

The implementation uses the HD44780-style controller's custom-character memory, limited to eight slots. Two line buffers represent terrain, and discrete hero states represent running/jumping phases. Button input uses an external interrupt.

## Hardware and setup

The original project uses an Arduino Uno-compatible board, a 16×2 LCD and a push-button. Review [`arduino_lcd_runner.ino`](arduino_lcd_runner.ino) for the exact library constructor and pin definitions; match those definitions to the wiring before upload.

Open the sketch in an Arduino-compatible IDE, select the board/port and compile before uploading.

## Timing and extension

The main loop uses `delay()`, so animation timing is blocking. This is an interactive embedded example rather than a measured hard real-time application. A continuation could introduce `millis()` scheduling, explicit button debouncing and a review of ISR-shared variables.

No new hardware demonstration or execution-time measurement was performed for this README update.

## Source attribution

The sketch was adapted from a third-party LCD-game tutorial. Its exact original reference and redistribution terms have not yet been documented. Preserve this provenance issue when reusing the source; do not assume unrestricted licensing.

## Licence

No project-wide licence has been defined.
