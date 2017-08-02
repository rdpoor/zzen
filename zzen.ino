/*
zzen: An austere impedance / conductance meter

MIT License

Copyright (c) 2017 Robert Poor <rdpoor@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/**
Theory of operation
         R[DRV0]
DRV0 ----/\/\/\-----+
         R[DRV1]    |
DRV1 ----/\/\/\-----+
         R[DRV2]    |          R[ADCA]
DRV2 ----/\/\/\-----+-----+----/\/\/\---- ADCA
                          |
                         DUT
         R[DRVC]          |    R[ADCB]
DRVC ----/\/\/\-----------+----/\/\/\---- ADCB

To measure resistance of the DUT:
- DRV0 is driven HIGH (or LOW)
- DRV1 and DRV2 are tri-stated
- DRVC is driven LOW (or HIGH)
- ADCA and ADCB take a reading, store the difference
Assuming that the impedance presented by ADCA and ADCB is much greater than
R[DUT], then:
The current through DUT: I[DUT] = (V[DRV0]-V[DRVC])/(R[DRV0]+R[DUT]+R[DRVC])
We also know that        I[DUT] = (V[ADCA]-V[ADCB])/R[DUT]
Setting the two equations equal:
    (V[DRV0]-V[DRVC])/(R[DRV0]+R[DRVC]+R[DUT]) = (V[ADCA]-V[ADCB])/R[DUT]
Save some typing:
    Rdut := R[DUT]
    Rdrv := R[DRV0]+R[DRVC]
    Vdrv := V[DRV0]-V[DRVC]
    Vadc := V[ADCA]-V[ADCB]
Solving for Rdut:
    Vdrv / (Rdut + Rdrv) = vdac / Rdut
    Vdrv * Rdut = Vadc * (Rdut + Rdrv)
    Vdrv * Rdut = Vadc * Rdut + vdac * Rdrv
    Rdut * (Vdrv - Vadc) = Vadc * Rdrv
    Rdut = Rdrv * Vadc / (Vdrv - Vadc)

## On Autoscaling

If R[DUT] is much smaller than R[DRV0], the voltage (V[ADCA]-V[ADCB]) will be
diminishingly small, leading to inaccurate readings.  By the same token, if
R[DUT] is much larger than R[DRV0], the differential voltage will be near the
upper limits, also leading to inaccurate readings.  To get the most accurate
readings, we want R[DRV] to approximately equal R[DUT].  Thus, we can switch in
DRV0, DRV1, or DRV2 depending on what best matches RDUT.

## On Conductivity

In real-world applications, condictivity probes need to be driven with an AC
signal, specifically with an average of zero volts, in order to prevent
bulk ion transport and deposition on the sensor probes.  Our circuit manages
this by driving DRVn and DRVC opposite polarity half of the time.

We could simply alternate the two, but that could lead to artifacts from nearby
AC power signals.  Instead, we use a pseudo random sequence to determine the
polarity of one sample to the next.  The only requirement of the PSR is that it
display an average of zero.

*/

SYSTEM_MODE(MANUAL)

#define DRVC B0
#define DRV0 B1
#define DRV1 B2
#define DRV2 B3
#define DRV3 B4

#define ADCA A0
#define ADCB A1

#define LED D7
int state;

typedef enum {
  SCALE0, SCALE1, SCALE2, SCALE3
} drive_scale_t;

void zenDrive(bool polarity, drive_scale_t scale) {
  const int drive_pins[] = {DRV0, DRV1, DRV2, DRV3};
  int pin = drive_pins[scale];
  // someday this will be a pair of instructions that manipulate PORTA directly
  pinMode(DRV0, INPUT);
  pinMode(DRV1, INPUT);
  pinMode(DRV2, INPUT);
  pinMode(DRV3, INPUT);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, polarity);
  digitalWrite(DRVC, !polarity);
}

double zenRead(bool polarity) {
  double diff = (analogRead(ADCA) - analogRead(ADCB)) / 4096.0;
  return polarity ? diff : -diff;
}

void setupDrive() {
  pinMode(DRV0, INPUT);
  pinMode(DRV1, INPUT);
  pinMode(DRV2, INPUT);
  pinMode(DRV3, INPUT);
  pinMode(DRVC, OUTPUT);
}

void setup() {
  state = 0;
  pinMode(LED, OUTPUT);
  setupDrive();
}

void loop() {
  digitalWrite(LED, state ? HIGH : LOW);
  state = !state;
  for (int scale=0; scale<4; scale++) {
    for (int polarity=0; polarity<2; polarity++) {
      zenDrive(polarity, (drive_scale_t)scale);
      Serial.print(zenRead(polarity) * 100.0);
      if ((scale == 3) && (polarity == 1)) {
        Serial.println();
        delay(100);
      } else {
        Serial.print(", ");
      }
    }
  }
}
