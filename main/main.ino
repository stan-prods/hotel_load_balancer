const int smallBreak = 60;
const int bigBreak = 900;

struct ControlLine {
    int inputPin;
    int outputPin;
    bool isActive;
    int currentValue;
    unsigned int timeoutUntil;
    int attempts;
    int lastMinute[60];
    int lastHour[60];
};

ControlLine controlLines[6] {
    {A1, D8, false, 0, 0, 0},
    {A2, D7, false, 0, 0, 0},
    {A3, D6, false, 0, 0, 0},
    {A4, D5, false, 0, 0, 0},
    {A5, D4, false, 0, 0, 0},
    {A6, D3, false, 0, 0, 0}
};

void setup(void) {
  Serial.begin(9600);
  for (byte i = 0; i < 6; i++) {
    pinMode(controlLines[i].inputPin, INPUT);
    pinMode(controlLines[i].outputPin, OUTPUT);
  }
}


ControlLine shutdownLine () {
}

int getVoltage() {
    return 0;
}

void populateSingleLineData () {
}

void monitorControlLines () {
}

void loop(void) {
}
