#include <Arduino.h>
#include "A20lib.h"


/////////////////////////////////////////////
// Public methods.
//

A20lib::A20lib() {
    Serial.begin(115200);
    Serial.setDebugOutput(0);
    Serial.setTimeout(100);
}


// Block until the module is ready.
byte A20lib::blockUntilReady(long baudRate, String PinCode) {

    byte response = A20_NOTOK;
    while (A20_OK != response) {
        response = begin(baudRate, PinCode);
        // This means the modem has failed to initialize and we need to reboot
        // it.
        if (A20_FAILURE == response) {
            return A20_FAILURE;
        }
        delay(1000);
        logln("Waiting for module to be ready...");
    }
    return A20_OK;
}


// Initialize the software serial connection and change the baud rate from the
// default (autodetected) to the desired speed.
byte A20lib::begin(long baudRate, String PinCode) {
    char buffer[50];

    Serial.flush();

    if (A20_OK != setRate(baudRate)) {
        return A20_NOTOK;
    }

    // Factory reset.
    A20command("AT&F", "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);

    // Echo off.
    A20command("ATE0", "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);
    // Register phone SIM
    if (PinCode.length() > 0)
    {
      logln("Registering pin code...");
      sprintf(buffer, "AT+CPIN=%s;", PinCode.c_str());
      A20command(buffer, "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);
    }

    logln("Dialing number...");

 


    A20command("AT+CLIP=1", "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);
    


    // Set caller ID on.
    A20command("AT+CLIP=1", "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);

    // Set SMS to text mode.
    A20command("AT+CMGF=1", "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);

    // Turn SMS indicators off.
    A20command("AT+CNMI=1,0", "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);

    // Set SMS storage to the GSM modem.
    if (A20_OK != A20command("AT+CPMS=ME,ME,ME", "OK", "yy", A20_CMD_TIMEOUT, 2, NULL))
        // This may sometimes fail, in which case the modem needs to be
        // rebooted.
    {
        return A20_FAILURE;
    }

    // Set SMS character set.
    setSMScharset("UCS2");

    return A20_OK;
}

// Dial a number.
void A20lib::dial(String number) {
    char buffer[50];

    logln("Dialing number...");

    sprintf(buffer, "ATD%s;", number.c_str());
    A20command(buffer, "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);
}


// Redial the last number.
void A20lib::redial() {
    logln("Redialing last number...");
    A20command("AT+DLST", "OK", "CONNECT", A20_CMD_TIMEOUT, 2, NULL);
}


// Answer a call.
void A20lib::answer() {
    A20command("ATA", "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);
}


// Hang up the phone.
void A20lib::hangUp() {
    A20command("ATH", "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);
}


// Check whether there is an active call.
callInfo A20lib::checkCallStatus() {
    char number[50];
    String response = "";
    uint32_t respStart = 0, matched = 0;
    callInfo cinfo = (const struct callInfo) {
        0
    };

    // Issue the command and wait for the response.
    A20command("AT+CLCC", "OK", "+CLCC", A20_CMD_TIMEOUT, 2, &response);

    // Parse the response if it contains a valid +CLCC.
    respStart = response.indexOf("+CLCC");
    if (respStart >= 0) {
        matched = sscanf(response.substring(respStart).c_str(), "+CLCC: %d,%d,%d,%d,%d,\"%s\",%d", &cinfo.index, &cinfo.direction, &cinfo.state, &cinfo.mode, &cinfo.multiparty, number, &cinfo.type);
        cinfo.number = String(number);
    }

    uint8_t comma_index = cinfo.number.indexOf('"');
    if (comma_index != -1) {
        logln("Extra comma found.");
        cinfo.number = cinfo.number.substring(0, comma_index);
    }

    return cinfo;
}


// Get the strength of the GSM signal.
int A20lib::getSignalStrength() {
    String response = "";
    uint32_t respStart = 0;
    int strength, error  = 0;

    // Issue the command and wait for the response.
    A20command("AT+CSQ", "OK", "+CSQ", A20_CMD_TIMEOUT, 2, &response);

    respStart = response.indexOf("+CSQ");
    if (respStart < 0) {
        return 0;
    }

    sscanf(response.substring(respStart).c_str(), "+CSQ: %d,%d",
           &strength, &error);

    // Bring value range 0..31 to 0..100%, don't mind rounding..
    strength = (strength * 100) / 31;
    return strength;
}


// Send an SMS.
byte A20lib::sendSMS(String number, String text) {
    char ctrlZ[2] = { 0x1a, 0x00 };
    char buffer[100];

    if (text.length() > 159) {
        // We can't send messages longer than 160 characters.
        return A20_NOTOK;
    }

    log("Sending SMS to ");
    log(number);
    logln("...");

    sprintf(buffer, "AT+CMGS=\"%s\"", number.c_str());
    A20command(buffer, ">", "yy", A20_CMD_TIMEOUT, 2, NULL);
    delay(100);
    Serial.println(text.c_str());
    Serial.println(ctrlZ);
    Serial.println();

    return A20_OK;
}


// Retrieve the number and locations of unread SMS messages.
int A20lib::getUnreadSMSLocs(int* buf, int maxItems) {
    return getSMSLocsOfType(buf, maxItems, "REC UNREAD");
}

// Retrieve the number and locations of all SMS messages.
int A20lib::getSMSLocs(int* buf, int maxItems) {
    return getSMSLocsOfType(buf, maxItems, "ALL");
}

// Retrieve the number and locations of all SMS messages.
int A20lib::getSMSLocsOfType(int* buf, int maxItems, String type) {
    String seqStart = "+CMGL: ";
    String response = "";

    String command = "AT+CMGL=\"";
    command += type;
    command += "\"";

    // Issue the command and wait for the response.
    byte status = A20command(command.c_str(), "\xff\r\nOK\r\n", "\r\nOK\r\n", A20_CMD_TIMEOUT, 2, &response);

    int seqStartLen = seqStart.length();
    int responseLen = response.length();
    int index, occurrences = 0;

    // Start looking for the +CMGL string.
    for (int i = 0; i < (responseLen - seqStartLen); i++) {
        // If we found a response and it's less than occurrences, add it.
        if (response.substring(i, i + seqStartLen) == seqStart && occurrences < maxItems) {
            // Parse the position out of the reply.
            sscanf(response.substring(i, i + 12).c_str(), "+CMGL: %u,%*s", &index);

            buf[occurrences] = index;
            occurrences++;
        }
    }
    return occurrences;
}

// Return the SMS at index.
SMSmessage A20lib::readSMS(int index) {
    String response = "";
    char buffer[30];

    // Issue the command and wait for the response.
    sprintf(buffer, "AT+CMGR=%d", index);
    A20command(buffer, "\xff\r\nOK\r\n", "\r\nOK\r\n", A20_CMD_TIMEOUT, 2, &response);

    char message[200];
    char number[50];
    char date[50];
    char type[10];
    int respStart = 0, matched = 0;
    SMSmessage sms = (const struct SMSmessage) {
        "", "", ""
    };

    // Parse the response if it contains a valid +CLCC.
    respStart = response.indexOf("+CMGR");
    if (respStart >= 0) {
        // Parse the message header.
        matched = sscanf(response.substring(respStart).c_str(), "+CMGR: \"REC %s\",\"%s\",,\"%s\"\r\n", type, number, date);
        sms.number = String(number);
        sms.date = String(date);
        // The rest is the message, extract it.
        sms.message = response.substring(strlen(type) + strlen(number) + strlen(date) + 24, response.length() - 8);
    }
    return sms;
}

// Delete the SMS at index.
byte A20lib::deleteSMS(int index) {
    char buffer[20];
    sprintf(buffer, "AT+CMGD=%d", index);
    return A20command(buffer, "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);
}


// Set the SMS charset.
byte A20lib::setSMScharset(String charset) {
    char buffer[30];

    sprintf(buffer, "AT+CSCS=\"%s\"", charset.c_str());
    return A20command(buffer, "OK", "yy", A20_CMD_TIMEOUT, 2, NULL);
}

/////////////////////////////////////////////
// Private methods.
//


// Autodetect the connection rate.

long A20lib::detectRate() {
    unsigned long rate = 0;
    unsigned long rates[] = {9600, 115200};

    // Try to autodetect the rate.
    logln("Autodetecting connection rate...");
    for (int i = 0; i < countof(rates); i++) {
        rate = rates[i];

        Serial.begin(rate);
        log("Trying rate ");
        log(rate);
        logln("...");

        delay(100);
        if (A20command("\rAT", "OK", "+CME", 2000, 2, NULL) == A20_OK) {
            return rate;
        }
    }

    logln("Couldn't detect the rate.");

    return A20_NOTOK;
}


// Set the A20 baud rate.
char A20lib::setRate(long baudRate) {
    int rate = 0;

    rate = detectRate();
    if (rate == A20_NOTOK) {
        return A20_NOTOK;
    }

    // The rate is already the desired rate, return.
    //if (rate == baudRate) return OK;

    logln("Setting baud rate on the module...");

    // Change the rate to the requested.
    char buffer[30];
    sprintf(buffer, "AT+IPR=%d", baudRate);
    A20command(buffer, "OK", "+IPR=", A20_CMD_TIMEOUT, 3, NULL);

    logln("Switching to the new rate...");
    // Begin the connection again at the requested rate.
    Serial.begin(baudRate);
    logln("Rate set.");

    return A20_OK;
}


// Read some data from the A20 in a non-blocking manner.
String A20lib::read() {
    String reply = "";
    if (Serial.available()) {
        reply = Serial.readString();
    }

    // XXX: Replace NULs with \xff so we can match on them.
    for (int x = 0; x < reply.length(); x++) {
        if (reply.charAt(x) == 0) {
            reply.setCharAt(x, 255);
        }
    }
    return reply;
}


// Issue a command.
byte A20lib::A20command(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response) {
    byte returnValue = A20_NOTOK;
    byte count = 0;

    // Get rid of any buffered output.
    Serial.flush();

    while (count < repetitions && returnValue != A20_OK) {
        log("Issuing command: ");
        logln(command);

        Serial.write(command);
        Serial.write('\r');

        if (A20waitFor(resp1, resp2, timeout, response) == A20_OK) {
            returnValue = A20_OK;
        } else {
            returnValue = A20_NOTOK;
        }
        count++;
    }
    return returnValue;
}


// Wait for responses.
byte A20lib::A20waitFor(const char *resp1, const char *resp2, int timeout, String *response) {
    unsigned long entry = millis();
    int count = 0;
    String reply = "";
    byte retVal = 99;
    do {
        reply += read();
        yield();
    } while (((reply.indexOf(resp1) + reply.indexOf(resp2)) == -2) && ((millis() - entry) < timeout));

    if (reply != "") {
        log("Reply in ");
        log((millis() - entry));
        log(" ms: ");
        logln(reply);
    }

    if (response != NULL) {
        *response = reply;
    }

    if ((millis() - entry) >= timeout) {
        retVal = A20_TIMEOUT;
        logln("Timed out.");
    } else {
        if (reply.indexOf(resp1) + reply.indexOf(resp2) > -2) {
            logln("Reply OK.");
            retVal = A20_OK;
        } else {
            logln("Reply NOT OK.");
            retVal = A20_NOTOK;
        }
    }
    return retVal;
}
