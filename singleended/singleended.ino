#include <Adafruit_ADS1X15.h>

//Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */
Adafruit_ADS1015 ads1;
void setup(void)
{
  Serial.begin(9600);
  //Serial.println("Hello!");

  //Serial.println("Getting single-ended readings from AIN0..3");
  //Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");

  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  //ads.setGain(GAIN_FOUR);
  ads1.setGain(GAIN_SIXTEEN);
  if (!ads.begin(0x48)) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
  if (!ads1.begin(0x49)) {
    Serial.println("Failed to initialize ADS1.");
    while (1);
  }
}

unsigned long timestamp, count;
void loop(void)
{
  int16_t adc0, adc1, adc2, adc3, adc4, adc5, adc6, adc7;
  float volts0, volts1, volts2, volts3;
  unsigned long avg0, avg4;  
  unsigned long currentTimestamp = millis() / 1000;

  count++;
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  adc2 = ads.readADC_SingleEnded(2);
  adc3 = ads.readADC_SingleEnded(3);
  adc4 = ads1.readADC_SingleEnded(0);
  adc5 = ads1.readADC_SingleEnded(1);
  adc6 = ads1.readADC_SingleEnded(2);
  adc7 = ads1.readADC_SingleEnded(3);

  avg0 += adc0;
  avg4 += adc4;

  //volts0 = ads.computeVolts(adc0);
  //volts1 = ads.computeVolts(adc1);
  //volts2 = ads.computeVolts(adc2);
  //volts3 = ads.computeVolts(adc3);

  if (timestamp < currentTimestamp) {
    timestamp = currentTimestamp;

    Serial.println(count);
    Serial.println(avg0 / count);
    Serial.println(avg4 / count);
    count = 0;    
    avg0 = 0;
    avg4 = 0;

  }

  //delay(10);
}
