# zzen
Impedance / admittance meter with autoscaling and very few parts

## To run

1. Connect to Particle Electron via USB
2. Compile and load zzen.ino into Particle Electron
3. In shell window, run `p5serial`
4. Open `display/index.html` in a browser window.

The browser window will continually display the resistance and conductance
of the Device Under Test connected to the Electron.

## Next Steps

* Add op-amps to improve performance
* Support PT1000 temperature probe
* Develop calibration process
* Bulletproof DUT inputs
* Lay out circuit board
* Reduce Power Consumption
* Incorporate Telemetry
* Simple smartphone app (just use Tinker initially)
* Waterproof housing

## Future Work

### Can this be adapted to be a network (RLC) analyzer?

By applying a step function through a series resistor and watching the shape of
the resulting waveform, it might be possible to compute not only the resistance
of the DUT, but also the equivalent two-terminal RLC device.

### Can this be adapted to be a voltmeter or ammeter?

Does Vadc still register a voltage when none of the drive resistors are enabled?
If so, DUT is voltage (or current) source.  Could we use the Rdrv[n] arrangement
to create a voltage divider?  Or would that be a separate design?  How much in
common could such a system share with this ohmmeter?
