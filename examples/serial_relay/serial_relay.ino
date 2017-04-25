#include <SoftwareSerial.h>
SoftwareSerial swSer(15, 13, false, 256);

#include <A20lib.h>


// Instantiate the library with TxPin, RxPin.
A6lib A6l(D6, D5);

A20lib A20l();

void setup() {
    swSer.begin(9600);
    delay(1000);

    A20l.blockUntilReady(9600);
}


void loop() {
    // Relay things between Serial and the module's SoftSerial.
    while (A20l.A6conn->available() > 0) {
        swSer.write(A20l.A6conn->read());
    }
    while (Serial.available() > 0) {
        A20l.A20conn->write(Serial.read());
    }
}
