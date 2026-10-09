# Arduino H-Bridge Experiment — Direction and PWM

A motor-control exercise using an LMD18200 H-bridge and an Arduino sketch.

## Control sequence

[`essai_lmd18200.ino`](essai_lmd18200.ino) uses pin 9 for direction and pin 10 for PWM. The fixed value `analogWrite(..., 200)` corresponds to approximately 78.4% of the 8-bit duty range.

The sequence applies one direction for five seconds, reverses for five seconds and stops for two seconds.

## Embedded concepts

Separate direction and PWM signals make actuator control explicit. Blocking delays implement a simple timed sequence; there is no encoder feedback, closed-loop speed regulation or acceleration profile.

## Hardware integration

Check the actual driver variant, supply, motor current, logic levels and common reference before connection. The source establishes control pins and a sequence, not a verified power-stage design or measured operating envelope.

Open the sketch in an Arduino-compatible environment, select the actual board and check PWM capability on the selected pin.

## Verification

Observe direction, PWM duty/frequency and stop behaviour on the assembled circuit. Review switching behaviour before changing direction under load.

Relate each direction/PWM command to the waveform at the driver input and the resulting motor response. This is the direct observation path for the open-loop sequence.

## Licence

No project-wide licence has been defined.
