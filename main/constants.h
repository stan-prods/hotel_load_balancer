#define frequencyMeasurePin 2
#define genControlPin 3

#define attemptMinutes 10
#define activationInterval 8
#define lineShutdownInterval 0 //time before disabling next cl
#define genRestoreInterval 10
#define measureSecFraction 4

#define bottomMeasurementVoltage 188

#define voltageBoundaries {260, 210, 200, 190}
#define frequencyBoundaries {70, 47, 44, 38}
#define isTopBoundarieAllowed false
#define controlLinesAmount 6

ControlLine controlLines[controlLinesAmount] {
    {true, A1, 9, 5, 5},
    {false, A2, 8, 60, 900},
    {false, A3, 7, 60, 900},
    {false, A4, 6, 60, 900},
    {false, A5, 5, 60, 900},
    {false, A6, 4, 60, 120}
};

