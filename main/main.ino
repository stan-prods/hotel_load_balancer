const int smallBreak = 60;
const int bigBreak = 900;
const int attemptMinutes = 3;
const int activationInterval = 15;
const int genRestoreSeconds = 5;
const int voltageNorm = 230;
const int allowedVoltageDeviationPercent = 15;

unsigned int lastShotdownTimestamp;
unsigned int lastActivationTimestamp;

struct ControlLine {
    int inputPin;
    int outputPin;
    bool isActive;
    int currentValue;
    int averageValue;
    int readingsInAverage;
    int valueTimestamp;
    unsigned int timeoutUntil;
    int attempts;
    int lastMinute[60];
    int lastHour[60];
};

ControlLine controlLines[6] {
    {A1, 8},
    {A2, 7},
    {A3, 6},
    {A4, 5},
    {A5, 4},
    {A6, 3}
};

ControlLine activateConrolLine (ControlLine cl) {
    if (cl.timeoutUntil < millis()) {
        cl.isActive = true;
        digitalWrite(cl.outputPin, HIGH);
    }

    return cl;
}

ControlLine deactivateControlLine (ControlLine cl) {
    cl.isActive = false;
    digitalWrite(cl.outputPin, LOW);

    if (cl.timeoutUntil - millis() < 60000 * attemptMinutes) {
        cl.attempts++;
    } else {
        cl.attempts = 0;
    }

    cl.timeoutUntil = millis() + ((cl.attempts < 3 ? smallBreak : bigBreak) * 1000);

    return cl;
}

void setup(void) {
    Serial.begin(9600);
    for (byte i = 0; i < 6; i++) {
        pinMode(controlLines[i].inputPin, INPUT);
        pinMode(controlLines[i].outputPin, OUTPUT);
    }
}


ControlLine shutdownLine () {
    int biggest = -1;
    for (byte i = 0; i < 6; i++) {
        if (!controlLines[i].isActive) {
            continue;
        }

        if (biggest < 0) {
            biggest = i;
            continue;
        }

        if (controlLines[i].currentValue > controlLines[biggest].currentValue) {
            biggest = i;
        }
    }

    deactivateControlLine(controlLines[biggest]);
    lastShotdownTimestamp = millis();
}

int currentVoltage;
int averageVoltage;
int readingsInAverageVoltage;
int voltageTimestamp;

void updateVoltage() {
    int value = abs(analogRead(A0) - 512);

    if (voltageTimestamp < millis() / 1000) {
        voltageTimestamp = millis() / 1000;

        currentVoltage = averageVoltage / readingsInAverageVoltage;
        averageVoltage = 0;
        readingsInAverageVoltage = 0;
    }

    averageVoltage += value;
    readingsInAverageVoltage++;
}

int getVoltageDropPerc() {
    return 0;
}

void monitorVoltate() {
    updateVoltage();

    if (getVoltageDropPerc() > allowedVoltageDeviationPercent && millis() > lastShotdownTimestamp * (genRestoreSeconds * 1000)) {
        shutdownLine();
    }
}

void populateSingleLineData (ControlLine cl) {
    int value = analogRead(cl.inputPin);


    if (cl.valueTimestamp < millis() / 1000) {
        cl.valueTimestamp = millis() / 1000;

        cl.currentValue = cl.averageValue / cl.readingsInAverage;
        cl.averageValue = 0;
        cl.readingsInAverage = 0;
    }

    cl.averageValue += value;
    cl.readingsInAverage++;

    //TODO create history
}

void monitorControlLines () {
    for (byte i = 0; i < 6; i++) {
        populateSingleLineData(controlLines[i]);

        if (getVoltageDropPerc() < allowedVoltageDeviationPercent / 2 && (lastActivationTimestamp - millis() / 1000) > activationInterval) {
            if (!controlLines[i].isActive && controlLines[i].timeoutUntil < millis()) {
                activateConrolLine(controlLines[i]);
                lastActivationTimestamp = millis();
            }
        }
    }
}

void loop(void) {
    monitorControlLines();
    monitorVoltate();
}
