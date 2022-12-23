const int smallBreak = 60;
const int bigBreak = 900;
const byte attemptMinutes = 10;
const byte activationInterval = 8;
const byte genRestoreSeconds = 5;
const byte voltageNorm = 230;
const byte allowedVoltageDeviationPercent = 13;

const byte measureSecFracture = 2;

unsigned long sec;
unsigned long lastShotdownTimestamp;
unsigned long lastActivationTimestamp;

struct Measure {
    int value;
    int minAverageValue;
    int maxAverageValue;
    int sinCenter;
    int measuresCount;
    int lastValueMeasuresCount;
    unsigned long valueTimestamp;
};

Measure voltage;

struct ControlLine {
    byte inputPin;
    byte outputPin;
    Measure current;
    bool isActive;
    unsigned long timeoutUntil;
    byte attempts;
};


const int controlLinesAmount = 6;
ControlLine controlLines[controlLinesAmount] {
    {A1, 8},
    {A2, 7},
    {A3, 6},
    {A4, 5},
    {A5, 4},
    {A6, 3}
};

ControlLine activateConrolLine (ControlLine &cl) {
    if (cl.timeoutUntil < sec) {
        cl.isActive = true;
        digitalWrite(cl.outputPin, HIGH);
        lastActivationTimestamp = sec;
    }

    return cl;
}

ControlLine deactivateControlLine (ControlLine &cl) {
    cl.isActive = false;
    digitalWrite(cl.outputPin, LOW);
}

ControlLine banControlLine (ControlLine &cl) {
    if (cl.timeoutUntil - sec < 60 * attemptMinutes) {
        cl.attempts++;
    } else {
        cl.attempts = 0;
    }

    cl.timeoutUntil = sec + (cl.attempts < 3 ? smallBreak : bigBreak);

    deactivateControlLine(cl);

    return cl;
}

void setup(void) {
    Serial.begin(9600);
    for (byte i = 0; i < controlLinesAmount; i++) {
        pinMode(controlLines[i].inputPin, INPUT);
        pinMode(controlLines[i].outputPin, OUTPUT);
        digitalWrite(controlLines[i].outputPin, LOW);
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

    banControlLine(controlLines[biggest]);
    lastShotdownTimestamp = sec;
}

int measure(Measure &m, int value) {
    if (m.valueTimestamp < sec) {
        m.valueTimestamp = sec;

        if (!m.sinCenter) {
            m.sinCenter = m.maxAverageValue - m.minAverageValue;
        } else {
            m.sinCenter = (m.sinCenter + (m.maxAverageValue - m.minAverageValue)) / 2;
        }

        m.value = max(m.maxAverageValue, m.sinCenter + m.minAverageValue) / 3;

        m.lastValueMeasuresCount = m.measuresCount;
        m.minAverageValue = 0;
        m.maxAverageValue = 0;
        m.measuresCount = 0;

    }

    if (value < m.minAverageValue) {
        m.minAverageValue = value;
    } else if (value > m.maxAverageValue) {
        m.maxAverageValue = value;
    }
    m.measuresCount++;
    return m.value;
}

void updateVoltage() {
    int value = measure(voltage, analogRead(A0));

}

int getVoltageDropPerc() {
    if (voltage.value > voltageNorm) {
        return 0;
    }

    return 100 - voltage.value * 100 / voltageNorm;
}

void monitorVoltage() {
    updateVoltage();

    if (voltage.value > 260 || voltage.value < 150) {
        for (byte i = 0; i < controlLinesAmount; i++) {
            deactivateControlLine(controlLines[i]);
        }
    }

    if (getVoltageDropPerc() > allowedVoltageDeviationPercent && (sec - lastShotdownTimestamp > genRestoreSeconds)) {
        shutdownLine();
    }
}

void populateSingleLineData (ControlLine &cl) {
    int value = analogRead(cl.inputPin);

    measure(cl.current, value);

    //TODO create history
}

void monitorControlLines () {
    for (byte i = 0; i < controlLinesAmount; i++) {
        populateSingleLineData(controlLines[i]);

        if (!controlLines[i].isActive && controlLines[i].timeoutUntil < sec) {
            if ((getVoltageDropPerc() < allowedVoltageDeviationPercent) && ((sec - lastActivationTimestamp) > activationInterval)) {
                activateConrolLine(controlLines[i]);
            }
        }
    }
}

unsigned long lastPrintTimestamp;
unsigned long prevLastPrintTimestamp;
void printInfo () {
    if (sec - lastPrintTimestamp > 2) {
        prevLastPrintTimestamp = lastPrintTimestamp;
        lastPrintTimestamp = sec;

        Serial.print(" ");
        Serial.print(voltage.value);

        Serial.print(" ");
        Serial.print(getVoltageDropPerc());

        Serial.print(" ");
        Serial.print(voltage.lastValueMeasuresCount);

        for (byte i = 0; i < controlLinesAmount; i++) {

            Serial.print(" ");
            Serial.print(controlLines[i].isActive);
        }

        for (byte i = 0; i < controlLinesAmount; i++) {

            Serial.print(" ");
            Serial.print(controlLines[i].current.value);
        }
        Serial.println(" ");
    }
}
void loop(void) {
    sec = millis() / 1000;
    monitorVoltage();
    monitorControlLines();

    printInfo();
    delay(random(4));
}
