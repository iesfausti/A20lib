#include <SoftwareSerial.h>
SoftwareSerial swSer(15, 13, false, 256);

#include <A20lib.h>


// Instantiate the library with TxPin, RxPin.
A20lib A20l();

int unreadSMSLocs[30] = {0};
int unreadSMSNum = 0;
SMSmessage sms;

void setup() {
    swSer.begin(9600);
    delay(1000);

    A20l.blockUntilReady(9600);
}


void loop() {
    callInfo cinfo = A20l.checkCallStatus();
    if (cinfo.direction == DIR_INCOMING) {
        if ("+1132352890".endsWith(cinfo.number)) {
            // If the number that sent the SMS is ours, reply.
            A20l.sendSMS(new_number, "I can't come to the phone right now, I'm a machine.");
            A20l.hangUp();
        }

        // Get the memory locations of unread SMS messages.
        unreadSMSNum = A20l.getUnreadSMSLocs(unreadSMSLocs, 30);

        for (int i = 0; i < unreadSMSNum; i++) {
            swSer.print("New message at index: ");
            swSer.println(unreadSMSLocs[i], DEC);

            sms = A20l.readSMS(unreadSMSLocs[i]);
            swSer.println(sms.number);
            swSer.println(sms.date);
            swSer.println(sms.message);
        }
        delay(1000);
    }
