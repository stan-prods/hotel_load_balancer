/*
    Outputs:
    1 - status led(s) //not implemented yet
    2 - servo //not implemented yet
    3 - main contactor control voltage //not implemented yet

        Relays:
        1 - power source select generator/line
        2 - gas valve
        3 - gas valve powering from battery (for egnition)
        4 - starter

    Inputs:
    1 - 5v when line is present
    2 - 5v when generator is started working
    3 - manual gas valve control switch //not implemented yet
    4 - manual servo control pot //not implemented yet


    powering from usb
    if generator is powerking, then switch to charge from gen. Else charge from mains
*/

#include "states.h"

//inputs
#define generatorBasedPowerPin 34
#define mainsPresentCV 35

//outputs
#define powerSourceSelectRelayPin 27
#define gasValveRelayPin 26
#define gasValvePowerSourceSelectPin 25
#define starterRelayPin 33

//constants
#define gasValveOpenDelaySecs 5
#define starterActiveDelaySecs 10
#define starterPauseDelaySecs 20
#define generatorCooldownDelaySecs 10
#define allowedAttempts 4

enum Activity {enable, disable};

bool isStarterRunning, isGeneratorWorking, isMainsPresent, isValveOpen, isValveOnBattery;
uint8_t attempts;

unsigned long secs;
unsigned long msecs;
unsigned long starterEnabledTimestampSecs;
unsigned long generatorCooldownStartedTimestampSecs;
unsigned long starterPauseStartedTimestampSecs;
unsigned long inputValueChangedTimestampMsecs;
unsigned long gasValveOpenOnStartTimestampSecs;


void setup() {
    Serial.begin(115200);
    pinMode(mainsPresentCV, INPUT);
    pinMode(generatorBasedPowerPin, INPUT);

    pinMode(powerSourceSelectRelayPin, OUTPUT);
    digitalWrite(powerSourceSelectRelayPin, disable);

    pinMode(gasValveRelayPin, OUTPUT);
    digitalWrite(gasValveRelayPin, disable);

    pinMode(gasValvePowerSourceSelectPin, OUTPUT);
    digitalWrite(gasValvePowerSourceSelectPin, disable);

    pinMode(starterRelayPin, OUTPUT);
    digitalWrite(starterRelayPin, disable);

}

void checkStatus() {
    //TODO add debounce
    bool genPowerVal = digitalRead(generatorBasedPowerPin);
    bool mainsPowerVal = digitalRead(mainsPresentCV);

    if (genPowerVal != isGeneratorWorking || mainsPowerVal != isMainsPresent) {
        if (!inputValueChangedTimestampMsecs) {
            inputValueChangedTimestampMsecs = msecs;
        }
    }

    if (inputValueChangedTimestampMsecs > 0 && msecs - inputValueChangedTimestampMsecs > 15) {
        attempts = 0;
        inputValueChangedTimestampMsecs = 0;
        isGeneratorWorking = genPowerVal;
        isMainsPresent = mainsPowerVal;
    }

    Serial.println("");
    Serial.println("");
    Serial.print(" Generator:");
    Serial.print(isGeneratorWorking);
    Serial.print(" Mains:");
    Serial.print(isMainsPresent);
    Serial.print(" Valve:");
    Serial.print(isValveOpen);
    Serial.print(" ValvePS:");
    Serial.print(isValveOnBattery);
    Serial.print(" Starter:");
    Serial.print(isStarterRunning);
    Serial.print(" Attempts:");
    Serial.print(attempts);
    Serial.println("");
}

void powerValveFromBattery(bool isValveShouldBePoweredFromBattery = true) {
    isValveOnBattery = isValveShouldBePoweredFromBattery;
    digitalWrite(gasValvePowerSourceSelectPin, isValveOnBattery ? enable : disable);
}

void enableValve(bool isValveShouldBeOpen = true) {
    isValveOpen = isValveShouldBeOpen;
    digitalWrite(gasValveRelayPin, isValveShouldBeOpen ? enable : disable);

    if (!isValveOpen) {
        powerValveFromBattery(false);
    }
}

void powerSelect() {
    digitalWrite(powerSourceSelectRelayPin, !isGeneratorWorking || isMainsPresent);
}


void startGenerator() {
    Serial.print(", Start generator");
    if (attempts > allowedAttempts - 1) {
        return;
    }
    //open gas valve
    powerValveFromBattery();
    enableValve();

    //if first attempt, let gas flow
    if (attempts == 0 && gasValveOpenOnStartTimestampSecs == 0) {
        gasValveOpenOnStartTimestampSecs = secs;
    }

    if (secs - gasValveOpenOnStartTimestampSecs > gasValveOpenDelaySecs) {
        gasValveOpenOnStartTimestampSecs = 0;
        runStarter();
    }

}

void stopGenerator() {
    Serial.print(", Stop generator");
    if (!generatorCooldownStartedTimestampSecs) {
        generatorCooldownStartedTimestampSecs = secs;
    }

    if (secs - generatorCooldownStartedTimestampSecs >= generatorCooldownDelaySecs) {
        digitalWrite(gasValveRelayPin, disable);
        isValveOpen = false;
        generatorCooldownStartedTimestampSecs = 0;
    }
}

void runStarter() {
    if (isGeneratorWorking) {
        return;
    }
    Serial.print(", Run Starter");
    if (!isStarterRunning && secs - starterPauseStartedTimestampSecs > starterPauseDelaySecs) {
        digitalWrite(starterRelayPin, enable);
        starterEnabledTimestampSecs = secs;
        isStarterRunning = true;
    }
}

void stopStarter() {
    Serial.print(", Stop Starter");
    digitalWrite(starterRelayPin, disable);
    starterEnabledTimestampSecs = 0;
    gasValveOpenOnStartTimestampSecs = 0;
    isStarterRunning = false;
}

void checkStarter() {
    if (!isStarterRunning) {
        return;
    }

    if (isMainsPresent) {
        stopStarter();
        attempts = 0;

        enableValve(false);
    }

    if (isGeneratorWorking) {
        Serial.print(", Generator started");
        stopStarter();
        attempts = 0;

        powerValveFromBattery(false);
    } else if (secs - starterEnabledTimestampSecs >= starterActiveDelaySecs) {
        Serial.print(", Generator start failed");
        stopStarter();
        attempts++;

        enableValve(false);

        starterPauseStartedTimestampSecs = secs;
    }
}

void loop() {
    msecs = millis();
    secs = msecs / 1000;

    checkStatus();
    checkStarter();

    if (!isGeneratorWorking && !isMainsPresent) {
        startGenerator();
    }

    if (isGeneratorWorking && isMainsPresent) {
        stopGenerator();
    }

    if (!isGeneratorWorking && isMainsPresent) {
        enableValve(false);

        stopStarter();
    }

    if (isGeneratorWorking && isValveOnBattery) {
        powerValveFromBattery(false);
    }

    powerSelect();

    delay(30);
}

