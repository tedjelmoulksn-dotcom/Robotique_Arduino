# Robotics and Arduino — Embedded Interaction and Motor Control

Three practical projects exploring sensor-driven behaviour, interrupt-based user input and actuator control.

## Project map

| Project | Implementation | Technical focus |
|---|---|---|
| [LCD runner](Arduino_LCD_Runner/) | Arduino sketch and 16×2 LCD | Custom characters, game state, button interrupts and collision detection |
| [H-bridge experiment](Essai_pont_H_LMD18200/) | Arduino direction and PWM control | Motor direction, duty cycle and timed sequencing |
| [EV3 obstacle avoidance](LEGO_EV3_Escape_the_Aliens/) | Native LEGO Mindstorms EV3 program | Parallel control paths, infrared sensing and touch-stop input |

The larger competition robot is documented separately in [Farming Mars](https://github.com/tedjelmoulksn-dotcom/Farming_Mars_CFR).

## Embedded design perspective

These exercises connect observable behaviour to low-level mechanisms: a button interrupt changes shared game state, custom LCD characters occupy controller memory, PWM controls average actuator drive, and independent robot-control loops can compete for the same motors.

Each module README explains the archived implementation and its constraints. The Arduino sketches are individual programs rather than a shared firmware framework.

## Getting started

```bash
git clone https://github.com/tedjelmoulksn-dotcom/Robotique_Arduino.git
cd Robotique_Arduino
```

Open the selected `.ino` file in an Arduino-compatible environment and select the actual board. Check its pin assignments and peripheral wiring before upload. Open the `.ev3` file using a compatible legacy EV3 environment.

## Validation and limitations

The three exercises illustrate different control models: a timed open-loop motor sequence, an interrupt-driven interactive display and concurrent sensor-driven motor commands. Evaluate each against its own state transitions, input handling and output ownership.

The H-bridge exercise uses a fixed-duty open-loop sequence. EV3 combines parallel sensor responses, requiring an explicit rule for motor ownership. The LCD program connects an interrupt-driven input to a delay-paced animation loop; its latency depends on when the main loop consumes the input request.

Follow input events through the program state to the commanded output when checking each module.

## Licence and attribution

Existing source attribution remains in place. No project-wide licence has been defined.
