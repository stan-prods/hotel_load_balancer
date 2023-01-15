/*
    Outputs:
    1 - status led(s)
    2 - servo
    3 - main contactor control voltage

        Relays:
        1 - power source select generator/line
        2 - gas valve
        3 - gas valve powering from battery (for egnition)
        4 - egnition key position

    Inputs:
    1 - 5v when line is present
    2 - 5v when generator is started working
    3 - manual gas valve control switch
    4 - manual servo control pot


    powering from usb
    if generator is powerking, then switch to charge from gen. Else charge from mains
*/

//inputs
#define mainsPresentCV 4
#define generatorBasedPowerPin 5

//outputs
#define powerSourceSelectRelayPin 6
#define gasValveRelayPin 7
#define gasValvePowerSourceSelectPin 8
#define starterRelayPin 9

//constants
#define gasValveOpenDelaySecs 5
#define starterActiveDelaySecs 10
#define generatorCooldownDelaySecs 10



bool isStarterRunning, isGeneratorWorking, isMainsPresent;
uint8_t attempts;

unsigned long secs;
unsigned long starterEnabledTimestampSecs;
unsigned long generatorCooldownStartedTimestampSecs;


void setup() {

}

void checkStatus() {
    isGeneratorWorking = digitalRead(generatorBasedPowerPin);
    isMainsPresent = digitalRead(mainsPresentCV);
}

void loop() {
    secs = millis() / 1000;

    checkStatus();
    checkStarter();

    if (!isGeneratorWorking && !isMainsPresent) {
        startGenerator();
    }

    if (isGeneratorWorking && isMainsPresent) {
        stopGenerator();
    }


}

void powerSelect() {
    digitalWrite(powerSourceSelectRelayPin, isGeneratorWorking)
}


void startGenerator() {
    if (attempts >= 3) {
        return;
    }
    //open gas valve
    digitalWrite(gasValvePowerSourceSelectPin, HIGH);
    digitalWrite(gasValveRelayPin, HIGH);

    //if first attempt, let gas flow
    if (attempts == 0) {
        delay(gasValveOpenDelaySecs * 1000);
    }

    runStarter();
}

void stopGenerator() {
    if (!generatorCooldownStartedTimestampSecs) {
        generatorCooldownDelaySecs = secs;
    }

    if (secs - generatorCooldownStartedTimestampSecs >= generatorCooldownDelaySecs) {
        digitalWrite(gasValveRelayPin, LOW);
        generatorCooldownStartedTimestampSecs = 0;
    }
}

void runStarter() {
    if (!isStarterRunning) {
        digitalWrite(starterRelayPin, HIGH);
        starterEnabledTimestampSecs = secs;
        isStarterRunning = true;
    }
}

void stopStarter() {
    digitalWrite(starterRelayPin, LOW);
    starterEnabledTimestampSecs = 0;
    isStarterRunning = false;
}

void checkStarter() {
    if (!isStarterRunning) {
        return;
    }

    if (isGeneratorWorking) {
        stopStarter();
        attempts = 0;

        digitalWrite(gasValvePowerSourceSelectPin, LOW);
    }

    if (secs - starterEnabledTimestampSecs >= starterActiveDelaySecs) {
        stopStarter();
        attempts++;

        digitalWrite(gasValvePowerSourceSelectPin, LOW);
        digitalWrite(gasValveRelayPin, LOW);
    }
}
