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

We could simply alternate the two, but that could lead to heterodyning with
AC power signals.  Instead, we use a pseudo random sequence to determine the
polarity of one sample to the next.  The only requirement of the PSR is that it
display an average of zero.

## On the selection of Rdrv[n]

For a given Rdrv, the system can take accurate readings over two decades, with
DUT ranging from Rdrv/10 to Rdv*10.  If we use four drive resistors, what's the
best set of values?

Propose:
     31.6 ohms (to read from 3 ohms to 300 ohms) [15 + 15]
     1.0K ohms (to read 100 ohms to 10K ohms) [1K]
    31.6K ohms (to read from 3K ohms to 310K ohms) [33K]
     1.0M ohms (to read from 100K ohms to 10M ohms) [1M]

This gives good overlap in each range and provides a very wide range of values.

## On ADC input impedance

The maximum external input impedance allowed for errors below 1/4 LSB is given
by:

     Rin = ((k - 0.5) / (Fadc * Cadc * ln(2^(N+2)))) - Radc

We also know that:

     Radc = 1.5 (min)
     N = 12 (bits)
     k = # sampling periods = ADC_SMPR1 = ?
     Fadc = ADC clock freq = 15MHz?
     Cadc = internal sample and hold capacitor = 4pF

Our best guess:

    Rin = 1 / (15 * 10^6 * 6 * 10^-12 * 9.7) = 1150

The documents also mention 1µA of leakage on the input.  Across 1M, that could
be a one volt error.

So, this all tells us that we'll want an opamp follower for any drive resistor
greater than 1K.

Replace R[ADCA] and R[ADCB] in the above schematic with a good dual rail-to-rail
opamp.  The TI OPA344 looks like a good option, as does the LMV342 (though that
is only SMT).

milli -3
micro -6
nano  -9
pico  -12

*/

SYSTEM_MODE(MANUAL)

#define ADCA A0
#define ADCB A1

#define DRVC B0
#define DRV0 B1
#define DRV1 B2
#define DRV2 B3
#define DRV3 B4

#define N_RANGES 4

#define LED D7
int state;

// drive_params_t encapsulates the calibration parameters for a drive circuit.
// At present, the calibration parameters are derived from scipy curve fitting.
typedef struct {
  double vdrv;
  double rdrv;
} drive_params_t;

// From 20170803b.ipynb
drive_params_t g_drives[] = {
  {1.00027687,  76.00513493},
  {9.97222635e-01,   1.05333803e+03},
  {1.00225376e+00,   3.39380768e+04},
  {4.75945804e-01,   2.54828619e+04}
};

// Set of of the N_RANGES drivers as outputs, the others tri-stated
void zenSetDrive(int drive_pin) {
  // someday this will be a pair of instructions that manipulate PORTA directly
  pinMode(DRV0, INPUT);
  pinMode(DRV1, INPUT);
  pinMode(DRV2, INPUT);
  pinMode(DRV3, INPUT);
  pinMode(drive_pin, OUTPUT);
}

// take a reading with the drive currently in effect and the
// specified polarity.  Returns a value between 0.0 and 1.0
// (but it can go slightly outside of that range).
double zenRawRead(bool polarity, int drive_pin) {
  digitalWrite(drive_pin, polarity);
  digitalWrite(DRVC, !polarity);
  double diff = (analogRead(ADCA) - analogRead(ADCB)) / 4096.0;
  return polarity ? diff : -diff;
}

// take a pair of readings with opposite drive polarity,
// return the average.
double zenSample(bool polarity, int range) {
  const int drive_pins[] = {DRV0, DRV1, DRV2, DRV3};
  double a, b, c;
  int drive_pin = drive_pins[range];
  zenSetDrive(drive_pin);
  a = zenRawRead(polarity, drive_pin);
  a = zenRawRead(polarity, drive_pin);
  a = zenRawRead(polarity, drive_pin);
  a = zenRawRead(polarity, drive_pin); // does repeating the read make a diff?
  b = zenRawRead(!polarity, drive_pin);
  b = zenRawRead(!polarity, drive_pin);
  b = zenRawRead(!polarity, drive_pin);
  b = zenRawRead(!polarity, drive_pin); // ditto...
  c = (a + b) / 2.0;
  return c;
}

// Given samples acquired at each range, pick the range that will give us the
// best accuracy, i.e. the sample closest to 0.5.
int zenChooseBestRange(double samples[]) {
  int best_range = -1;
  double best_err = 2.0;
  for (int range=0; range<N_RANGES; range++) {
    double sample = samples[range];
    double err = (sample >= 0.5) ? (sample - 0.5) : (0.5 - sample);
    if (err < best_err) {
      best_err = err;
      best_range = range;
    }
  }
  return best_range;
}

// Convert a raw ADC reading into ohms.
double zenSampleToOhms(double vadc, int range) {
  double rdrv = g_drives[range].rdrv;
  double vdrv = g_drives[range].vdrv;
  return rdrv * vadc / (vdrv - vadc);
}

// Activate each N_RANGES drive in turn, read samples.  Find the
// best reading of the N_RANGES and output results for that range.
void zenRead(bool polarity) {
  double samples[N_RANGES];
  for (int i=0; i<N_RANGES; i++) {
    samples[i] = zenSample(polarity, i);
  }
  int range = zenChooseBestRange(samples);
  double ohms = zenSampleToOhms(samples[range], range);
  zenReport(ohms, range);
}

void zenReport(double ohms, int range) {
  Serial.print(ohms);
  Serial.print(", ");
  Serial.println(range);
}

void zenCalibrate(bool polarity) {
  for (int i=0; i<N_RANGES; i++) {
    double vadc = zenSample(polarity, i);
    double ohms = zenSampleToOhms(vadc, i);
    Serial.print(vadc * 1000.0); // bc print() limits to two decimal places
    Serial.print(", ");
    Serial.print(ohms);
    if (i == N_RANGES-1) {
      Serial.println();
    } else {
      Serial.print(", ");
    }
  }
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
  for (int polarity=0; polarity<2; polarity++) {
    zenRead(polarity);
    // zenCalibrate(polarity);
    delay(250);
  }
}
