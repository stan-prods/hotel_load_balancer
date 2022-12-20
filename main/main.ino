const int smallBreak = 60;
const int bigBreak = 900;
const int attemptMinutes = 3;
const int activationInterval = 15;
const int genRestoreSeconds = 5;
const int voltageNorm = 230;
const int allowedVoltageDeviationPercent = 13;

unsigned int lastShotdownTimestamp;
unsigned int lastActivationTimestamp;

struct Measure {
    int value;
    int averageValue;
    int readingsInAverage;
    int valueTimestamp;
};

Measure voltage;

struct ControlLine {
    int inputPin;
    int outputPin;
    Measure current;
    bool isActive;
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
        lastActivationTimestamp = millis();
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

        if (controlLines[i].current.value > controlLines[biggest].current.value) {
            biggest = i;
        }
    }

    deactivateControlLine(controlLines[biggest]);
    lastShotdownTimestamp = millis();
}

void measure(Measure m, int value) {
    if (m.valueTimestamp < millis() / 1000) {
        m.valueTimestamp = millis() / 1000;

        m.value = m.averageValue / m.readingsInAverage;
        m.averageValue = 0;
        m.readingsInAverage = 0;
    }

    m.averageValue += value;
    m.readingsInAverage++;
}

void updateVoltage() {
    int value = abs(analogRead(A0) - 512);

    if (value > 260) {
        //TODO emergency shotdown
    }

    measure(voltage, value);
}

int getVoltageDropPerc() {
    if (voltage.value > voltageNorm) {
        return 0;
    }

    return 100 - voltage.value / voltageNorm * 100;
}

void monitorVoltage() {
    updateVoltage();

    if (getVoltageDropPerc() > allowedVoltageDeviationPercent && millis() > lastShotdownTimestamp * (genRestoreSeconds * 1000)) {
        shutdownLine();
    }
}

void populateSingleLineData (ControlLine cl) {
    int value = abs(analogRead(cl.inputPin) - 768); //for base voltage 3.3

    measure(cl.current, value);

    //TODO create history
}

void monitorControlLines () {
    for (byte i = 0; i < 6; i++) {
        populateSingleLineData(controlLines[i]);

        if (getVoltageDropPerc() < allowedVoltageDeviationPercent / 2 && (lastActivationTimestamp - millis() / 1000) > activationInterval) {
            if (!controlLines[i].isActive && controlLines[i].timeoutUntil < millis()) {
                activateConrolLine(controlLines[i]);
            }
        }
    }
}

void loop(void) {
    monitorControlLines();
    monitorVoltage();
}
