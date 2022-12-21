const int controlLinesAmount = 5;

const byte smallBreak = 6;
const byte bigBreak = 90;
const byte attemptMinutes = 3;
const byte activationInterval = 15;
const byte genRestoreSeconds = 5;
const byte voltageNorm = 230;
const byte allowedVoltageDeviationPercent = 13;

const byte measureIntervalSec = 2;

unsigned int lastShotdownTimestamp;
unsigned int lastActivationTimestamp;

struct Measure {
    int value;
    int averageValue;
    byte readingsInAverage;
    unsigned int valueTimestamp;
};

Measure voltage;

struct ControlLine {
    byte inputPin;
    byte outputPin;
    Measure current;
    bool isActive;
    unsigned int timeoutUntil;
    byte attempts;
};

ControlLine controlLines[controlLinesAmount] {
    {A1, 8},
    {A2, 7},
    {A3, 6},
    {A4, 5},
    {A5, 4}
};

ControlLine activateConrolLine (ControlLine &cl) {
    if (cl.timeoutUntil < millis()) {
        cl.isActive = true;
        digitalWrite(cl.outputPin, HIGH);
        lastActivationTimestamp = millis();
    }

    return cl;
}

ControlLine deactivateControlLine (ControlLine &cl) {
    cl.isActive = false;
    digitalWrite(cl.outputPin, LOW);

    if (cl.timeoutUntil - millis() < 60000 * attemptMinutes) {
        cl.attempts++;
    } else {
        cl.attempts = 0;
    }

    cl.timeoutUntil = millis() + ((cl.attempts < 3 ? smallBreak : bigBreak) * 10 * 1000);

    return cl;
}

void setup(void) {
    Serial.begin(9600);
    for (byte i = 0; i < controlLinesAmount; i++) {
        pinMode(controlLines[i].inputPin, INPUT);
        pinMode(controlLines[i].outputPin, OUTPUT);
    }
}


ControlLine shutdownLine () {
    int biggest = -1;
    for (byte i = 0; i < controlLinesAmount; i++) {
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

bool isDataPrinted;
bool isVoltageMeasured;
void measure(Measure &m, int value) {
    unsigned int sec = millis() / 1000;

    if (m.valueTimestamp + (measureIntervalSec - 1) < sec) {
        m.valueTimestamp = sec;


        m.value = m.averageValue / m.readingsInAverage;
        if (isVoltageMeasured) {
            Serial.print(String(m.value));
            Serial.print(" ");
            isDataPrinted = true;
        }
        m.averageValue = 0;
        m.readingsInAverage = 0;

    }

    m.averageValue += value;
    m.readingsInAverage++;
}

void updateVoltage() {
    int reading = analogRead(A0);
    int value = abs(reading - 512);

    Serial.print(value);
    Serial.print(" ");
    Serial.println(reading - 512);

    if (value > 260) {
        //TODO emergency shotdown
    }

    //isVoltageMeasured = true;

    measure(voltage, value);

    isVoltageMeasured = false;
    if (isDataPrinted) {
        isDataPrinted = false;
        Serial.println(" ");
    }
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

void populateSingleLineData (ControlLine &cl) {
    int value = abs(analogRead(cl.inputPin) - 768); //for base voltage 3.3

    measure(cl.current, value);

    //TODO create history
}

void monitorControlLines () {
    for (byte i = 0; i < controlLinesAmount; i++) {
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
