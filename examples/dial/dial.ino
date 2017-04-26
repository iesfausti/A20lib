#include <A20lib.h>


// Instantiate the library with TxPin, RxPin.
A20lib A20l = A20lib();

void setup() {
    delay(1000);
    A20l.blockUntilReady(9600,"");
}

void loop() {
    callInfo cinfo = A20l.checkCallStatus();

    int sigStrength = A20l.getSignalStrength();
    log("Signal strength percentage: ");
    logln(sigStrength);

    delay(5000);

    if (cinfo.number != NULL) {
        if (cinfo.direction == DIR_INCOMING && cinfo.number == "919999999999") {
            A20l.answer();
        } else {
            A20l.hangUp();
        }
        delay(1000);
    } else {
        logln("No number yet.");
        delay(1000);
    }
}
