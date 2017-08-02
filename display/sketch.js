/*
Serial read and animate example

Reads an ASCII-encoded string from a seiral port via a webSocket server.
Animates the text on the screen with the received value

You can use this with the included Arduino example called AnalogReadSerial.
Works with P5 editor as the socket/serial server, version 0.5.5 or later.

written 2 Oct 2015
by Tom Igoe
*/

// Declare a "SerialPort" object
var serial;
// fill in the name of your serial port here:
var portName = "/dev/cu.usbmodem1411";
var sensorString = "mine turtle";
var autorangeValue = 0;
var options = {
  baudrate: 115200
};

function setup() {
  createCanvas(windowWidth, windowHeight);
  println("setup A");

  // make an instance of the SerialPort object
  serial = new p5.SerialPort();
  // Get a list the ports available (call back into gotList())
  serial.list();
  serial.open(portName, options);
  // When you get a list of serial ports that are available
  serial.on('list', gotList);
  // When you some data from the serial port
  serial.on('data', gotData);
  println("setup B");
}


// Got the list of ports
function gotList(thelist) {
  println("List of Serial Ports:");
  // theList is an array of their names
  for (var i = 0; i < thelist.length; i++) {
    // Display in the console
    println(i + " " + thelist[i]);
  }
}

// Called when there is data available from the serial port
function gotData() {
  var currentString = serial.readLine();  // read the incoming data
  trim(currentString);
  if (!currentString) return;
  // console.log(currentString);
  sensorString = currentString;
  // expecting:
  //  1234.56, 1
  // where the first number is a float representing resistance in ohms,
  // the second is the autoranging range, 0 to 2
  // const re = /^(.*),\s+(.*)/;
  // const match = re.exec(currentString);
  // if (!match) return;
  // // console.log('sensor = ' + match[1] + ' range = ' + match[2]);
  // if (!isNaN(match[1])) {
  //   sensorValue = match[1];
  //   autorangeValue = match[2];
  // }
}

function draw() {
  background(40, 40, 40);
  if (autorangeValue == 0) {
    fill(255, 200, 200);
  } else if (autorangeValue == 1) {
    fill(200, 255, 200);
  } else {
    fill(200, 200, 255);
  }
  textStyle(BOLD);
  textSize(24);
  text(sensorString, 10, 100);
}
