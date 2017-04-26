#include <A20lib.h>


// Instantiate the library with TxPin, RxPin.
A20lib A20l = A20lib();

int unreadSMSLocs[30] = {0};
int unreadSMSNum = 0;
SMSmessage sms;

void setup() {
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
            logln("New message at index: ");
            logln(unreadSMSLocs[i]);

            sms = A20l.readSMS(unreadSMSLocs[i]);
            logln(sms.number);
            logln(sms.date);
            logln(sms.message);
        }
        delay(1000);
    }
