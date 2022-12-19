
void setup(void)
{
  Serial.begin(9600);

}

unsigned long timestamp, count;
unsigned long avg0, avg1; 
void loop(void)
{
  int16_t adc0, adc1, adc2, adc3, adc4, adc5, adc6, adc7;
   
  unsigned long currentTimestamp = millis() / 1000;

  count++;
  adc0 = analogRead(A0);
  adc1 = analogRead(A1);
  adc2 = analogRead(A2);
  adc3 = analogRead(A3);
  adc4 = analogRead(A4);
  adc5 = analogRead(A5);
  adc6 = analogRead(A6);
  adc7 = analogRead(A7);

  avg0 += abs(adc0 - 512);
  avg1 += abs(adc4 - 512); 

  if (timestamp < currentTimestamp) {
    timestamp = currentTimestamp;

    Serial.print(count);
    Serial.print(" ");
    Serial.print(avg1 / count);
    Serial.print(" ");
    Serial.println((avg0 / count) * 2);
    count = 0;    
    avg0 = 0;
    avg1 = 0;

  }

  //delay(10);
}
