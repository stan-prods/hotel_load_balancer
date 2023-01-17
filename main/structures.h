struct Boundaries {
    int topAllowedValue;
    byte lineBanValue;
    byte linesShutdownValue;
    byte bottomAllowedValue;
} noBoundaries;

struct Measure {
    int value;
    int prevValue;

    unsigned long valueTimestamp;

    bool isControlLinesAllowed;
    Boundaries boundaries = noBoundaries;
};

struct AmplitudeMeasure : Measure {
    int minAverageValue;
    int maxAverageValue;
    int sinCenter;
    int measuresCount;
    int prevValueMeasuresCount;
} voltage;

struct FrequencyMeasure : Measure {
    int reading;
    int prevReading;

    int interruptsCount;
} frequency;


struct ControlLine {
    bool isPrioLine;
    byte inputPin;
    byte outputPin;
    int smallBreakSecs;
    int bigBreakSecs;
    AmplitudeMeasure current;
    bool isActive;
    unsigned long timeoutUntil;
    byte attempts;
};
