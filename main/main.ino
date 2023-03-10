#include "structures.h"
#include "constants.h"

byte status;

unsigned long msec;
unsigned long sec;

unsigned long lastLineShutdownTimestamp;
unsigned long lastLineActivationTimestamp;
unsigned long lastGenShutdownTimestamp;

ControlLine activateConrolLine (ControlLine &cl) {
    if (cl.timeoutUntil < sec) {
        cl.isActive = true;
        digitalWrite(cl.outputPin, HIGH);
        lastLineActivationTimestamp = sec;
    }

    return cl;
}

ControlLine deactivateControlLine (ControlLine &cl) {
    cl.isActive = false;
    digitalWrite(cl.outputPin, LOW);

    return cl;
}

void deactivateAll () {
    for (byte i = 0; i < controlLinesAmount; i++) {
        deactivateControlLine(controlLines[i]);
    }
    status = 4;
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

void frequencyInterrupt () {
    frequency.interruptsCount++;
}

void setup (void) {
    Serial.begin(9600);

    pinMode(genControlPin, OUTPUT);
    digitalWrite(genControlPin, LOW);

    pinMode(frequencyMeasurePin, INPUT);
    attachInterrupt(digitalPinToInterrupt(frequencyMeasurePin), frequencyInterrupt, RISING);

    voltage.boundaries = voltageBoundaries;
    frequency.boundaries = frequencyBoundaries;

    for (byte i = 0; i < controlLinesAmount; i++) {
        pinMode(controlLines[i].inputPin, INPUT);
        pinMode(controlLines[i].outputPin, OUTPUT);
        digitalWrite(controlLines[i].outputPin, LOW);
    }
}


void shutdownLine (bool includePrio) {
    int biggest = -1;
    for (byte i = 0; i < controlLinesAmount; i++) {
        if (!includePrio && controlLines[i].isPrioLine) {
            continue;
        }

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

    if (!includePrio && biggest == -1) {
        shutdownLine(true);
    } else if (biggest > -1) {
        banControlLine(controlLines[biggest]);
        lastLineShutdownTimestamp = sec;
        status = 5;
    }


}


void shutdownGenInput () {
    digitalWrite(genControlPin, HIGH);
    lastGenShutdownTimestamp = sec;
    status = 2;
}

int measureFrequency (FrequencyMeasure &f) {
    if (f.valueTimestamp < (msec / (1000 / measureSecFraction))) {
        f.valueTimestamp = (msec / (1000 / measureSecFraction));

        f.prevReading = f.reading;
        f.reading = f.interruptsCount;
        f.interruptsCount = 0;

        f.prevValue = f.value;

        if (f.reading == 0 || f.prevReading == 0) {
            f.value = 0;
        } else {
            f.value = (f.reading + f.prevReading) * (measureSecFraction / 2);
        }
    }

    return f.value;
}

int measureAmplitude (AmplitudeMeasure &m, int value) {
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

void updateVoltage () {
    int value = measureAmplitude(voltage, analogRead(A0));
}

void chooseAction (Measure &m) {
    if (m.value == 0 && m.prevValue == 0)  {
        m.isControlLinesAllowed = true;
        status = 0;
    } else {
        if (m.value <= m.boundaries.bottomAllowedValue && m.prevValue <= m.boundaries.bottomAllowedValue) {
            shutdownGenInput();
            m.isControlLinesAllowed = false;
        } else if (isTopBoundarieAllowed && m.value >= m.boundaries.topAllowedValue && m.prevValue >= m.boundaries.topAllowedValue) {
            deactivateAll();
            m.isControlLinesAllowed = false;
        } else if (m.value <= m.boundaries.linesShutdownValue && m.prevValue <= m.boundaries.linesShutdownValue) {
            deactivateAll();
            m.isControlLinesAllowed = false;
        } else if (m.value <= m.boundaries.lineBanValue && (sec - lastLineShutdownTimestamp > lineShutdownInterval)) {
            shutdownLine(false);
            m.isControlLinesAllowed = false;
        } else {
            m.isControlLinesAllowed = true;
            status = 1;
        }
    }
}

void monitorVoltage () {
    updateVoltage();

    if (voltage.value <= bottomMeasurementVoltage && voltage.prevValue <= bottomMeasurementVoltage)  {
        voltage.value = 0;
    }

    chooseAction(voltage);
}

void populateSingleLineData (ControlLine &cl) {
    int value = analogRead(cl.inputPin);

    measureAmplitude(cl.current, value);
}

void monitorControlLines () {
    for (byte i = 0; i < controlLinesAmount; i++) {
        populateSingleLineData(controlLines[i]);

        if (!controlLines[i].isActive && controlLines[i].timeoutUntil < sec) {
            if (voltage.isControlLinesAllowed && frequency.isControlLinesAllowed && ((sec - lastLineActivationTimestamp) > activationInterval)) {
                activateConrolLine(controlLines[i]);
            }
        }
    }
}

void monitorFrequency () {
    measureFrequency(frequency);

    chooseAction(frequency);
}

void monitorGenInput () {
    if ((sec - lastGenShutdownTimestamp) > genRestoreInterval) {
        digitalWrite(genControlPin, LOW);
    }
}

unsigned long lastPrintTimestamp;
unsigned long prevLastPrintTimestamp;
void printInfo () {
    if (sec - lastPrintTimestamp > 0) {
        prevLastPrintTimestamp = lastPrintTimestamp;
        lastPrintTimestamp = sec;

        Serial.print("| ");
        Serial.print(frequency.value);
        Serial.print("Hz");

        Serial.print(" => ");
        Serial.print(frequency.isControlLinesAllowed ? " GO  " : "NO GO");

        Serial.print(" | ");
        Serial.print(voltage.value);
        Serial.print("v");

        Serial.print(" => ");
        Serial.print(voltage.isControlLinesAllowed ? " GO  " : "NO GO");


        Serial.print(" | ");
        Serial.print(voltage.prevValueMeasuresCount);
        Serial.print(" mes/s");

        if (voltage.prevValueMeasuresCount - 100 < 0) {
            Serial.print(" ");
        }

        Serial.print(" | ");
        Serial.print(" Input: ");
        Serial.print(digitalRead(genControlPin) ? "Off" : "On ");

        Serial.print(" | ");
        Serial.print(" Lines: ");

        for (byte i = 0; i < controlLinesAmount; i++) {

            Serial.print(" ");
            Serial.print(controlLines[i].isActive ? "On " : "Off");
        }

        Serial.print(" | ");
        Serial.print(" Current per line: ");

        for (byte i = 0; i < controlLinesAmount; i++) {

            Serial.print(" ");
            Serial.print(controlLines[i].current.value);
        }

        Serial.print(" aa");

        Serial.print(" | ");
        Serial.print(" Last status: ");
        switch (status) {
            case 0:
                Serial.print("No sensor data, all lines allowed");
                break;
            case 1:
                Serial.print("all lines allowed");
                break;
            case 2:
                Serial.print("Bottom allowed value reached, gen input deactivated");
                break;
            case 3:
                Serial.print("Top allowed value reached, all lines deactivated");
                break;
            case 4:
                Serial.print("Deactivate all lines");
                break;
            case 5:
                Serial.print("Deactivating one line");
                break;
        }

        Serial.println(" ");
    }
}

void loop(void) {
    msec = millis();
    sec = msec / 1000;

    monitorFrequency();
    monitorVoltage();
    monitorGenInput();

    monitorControlLines();

    printInfo();
    delay(random(4));
}
