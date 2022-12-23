const byte smallBreak = 6;
const byte bigBreak = 90;
const byte attemptMinutes = 3;
const byte activationInterval = 15;
const byte genRestoreSeconds = 5;
const byte voltageNorm = 230;
const byte allowedVoltageDeviationPercent = 13;

const byte measureSecFracture = 2;

unsigned int lastShotdownTimestamp;
unsigned int lastActivationTimestamp;

struct Measure {
    int value;
    int minAverageValue;
    int maxAverageValue;
    int sinCenter;
    int measuresCount;
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

    deactivateControlLine(controlLines[biggest]);
    lastShotdownTimestamp = millis();
}

void measure(Measure &m, int value) {
    unsigned int sec = millis() / (1000 / measureSecFracture);

    if (m.valueTimestamp < sec) {
        m.valueTimestamp = sec;

        if (!m.sinCenter) {
            m.sinCenter = m.maxAverageValue - m.minAverageValue;
        } else {
            m.sinCenter = (m.sinCenter + (m.maxAverageValue - m.minAverageValue)) / 2;
        }

        m.value = max(m.maxAverageValue, m.sinCenter + m.minAverageValue);

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
}

void updateVoltage() {
    int value = analogRead(A0);

    if (value > 260) {
        //TODO emergency shotdown
    }

    measure(voltage, value);
    if(voltage.value/3 < 150) {
        voltage.value = 0;
    }
    Serial.println(voltage.value/3);

}

int getVoltageDropPerc() {
    if (voltage.value / 3 > voltageNorm) {
        return 0;
    }

    return 100 - (voltage.value / 3) / voltageNorm * 100;
}

void monitorVoltage() {
    updateVoltage();

    if (getVoltageDropPerc() > allowedVoltageDeviationPercent && millis() > lastShotdownTimestamp * (genRestoreSeconds * 1000)) {
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

        if (!controlLines[i].isActive && controlLines[i].timeoutUntil < millis()) {
            if (getVoltageDropPerc() < allowedVoltageDeviationPercent / 2 && (lastActivationTimestamp - millis() / 1000) > activationInterval) {
                activateConrolLine(controlLines[i]);
            }
        }
    }
}

void loop(void) {
    monitorVoltage();
    monitorControlLines();

    delay(random(4));
}
