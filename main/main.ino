const byte attemptMinutes = 10;
const byte activationInterval = 8;
const byte genRestoreSeconds = 0; //time before disabling next cl
const byte voltageNorm = 230;
const int topAllowedVoltage = 260;
const int bottomAllowedVoltage = 208;
const byte lineBanVoltage = 215;

unsigned long msec;
unsigned long sec;
const byte measureSecFraction = 4;

unsigned long lastShotdownTimestamp;
unsigned long lastActivationTimestamp;

struct Measure {
    int value;
    int prevValue;
    int minAverageValue;
    int maxAverageValue;
    int sinCenter;
    int measuresCount;
    int prevValueMeasuresCount;
    unsigned long valueTimestamp;
};

Measure voltage;

struct ControlLine {
    byte inputPin;
    byte outputPin;
    int smallBreakSecs;
    int bigBreakSecs;
    Measure current;
    bool isActive;
    unsigned long timeoutUntil;
    byte attempts;
};


const int controlLinesAmount = 6;
ControlLine controlLines[controlLinesAmount] {
    {A1, 8, 60, 60},
    {A2, 7, 60, 900},
    {A3, 6, 60, 900},
    {A4, 5, 60, 900},
    {A5, 4, 60, 900},
    {A6, 3, 60, 120}
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

void deactivateAll () {
    for (byte i = 0; i < controlLinesAmount; i++) {
        deactivateControlLine(controlLines[i]);
    }
}

ControlLine banControlLine (ControlLine &cl) {
    if (cl.timeoutUntil - sec < 60 * attemptMinutes) {
        cl.attempts++;
    } else {
        cl.attempts = 0;
    }

    cl.timeoutUntil = sec + (cl.attempts < 3 ? cl.smallBreakSecs : cl.bigBreakSecs);

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
    if (m.valueTimestamp < (msec / (1000 / measureSecFraction))) {
        m.valueTimestamp = (msec / (1000 / measureSecFraction));

        if (!m.sinCenter) {
            m.sinCenter = m.maxAverageValue - m.minAverageValue;
        } else {
            m.sinCenter = (m.sinCenter + (m.maxAverageValue - m.minAverageValue)) / 2;
        }

        m.prevValue = m.value;
        m.value = max(m.maxAverageValue, m.sinCenter + m.minAverageValue) / 3;

        m.prevValueMeasuresCount = m.measuresCount;
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

void monitorVoltage() {
    updateVoltage();

    if (voltage.value > topAllowedVoltage && voltage.prevValue > topAllowedVoltage) {
        deactivateAll();
    }

    if (voltage.value < bottomAllowedVoltage && voltage.prevValue < bottomAllowedVoltage) {
        deactivateAll();
    }

    if (voltage.value < lineBanVoltage && (sec - lastShotdownTimestamp > genRestoreSeconds)) {
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
            if (voltage.value > lineBanVoltage && ((sec - lastActivationTimestamp) > activationInterval)) {
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
        Serial.print(voltage.prevValueMeasuresCount);

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
    msec = millis();
    sec = msec / 1000;

    monitorVoltage();
    monitorControlLines();

    printInfo();
    delay(random(4));
}
