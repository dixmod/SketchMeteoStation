/*
 Vin (Voltage In)    ->  3.3V
 Gnd (Ground)        ->  Gnd
 SDA (Serial Data)   ->  A4 on Uno/Pro-Mini
 SCK (Serial Clock)  ->  A5 on Uno/Pro-Mini
*/

#include <BME280I2C.h>
#include <OLED_I2C.h>

#define PLOT_LEN      100
#define STORAGE_TIME  270

OLED  myOLED(SDA, SCL, 8);
BME280I2C bme;

// Temperature Oversampling Rate, Humidity Oversampling Rate, Pressure Oversampling Rate, Mode, Standby Time, Filter, SPI Enable, BME280 Address
// BME280I2C bme(0x1, 0x1, 0x1, 0x1, 0x5, 0x0, false, 0x77); // Version for SparkFun BME280
  
extern uint8_t MediumNumbers[];
extern uint8_t SmallFont[];

struct {
  byte temp = 0;
  byte hum = 0;
  byte pres = 0;
} infoArr[PLOT_LEN];

struct {
  float temp = 0;
  float hum = 0;
  float pres = 0;
  int counter = 0;
} avrg;

byte wait_cnt = 0;
bool fastMode = true;

void setup() {
  myOLED.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);

  while (!bme.begin()) {
    // if bme280 not found - blink LED
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
  }
  delay(500);
}

void loop() {
  
  bool metric = true;
  float val(NAN), temp(NAN), hum(NAN), pres(NAN), minVal(NAN), maxVal(NAN);
  String title(""), unit("");
  int leftTitle(1);
  uint8_t pressureUnit(0); // unit: B000 = Pa, B001 = hPa, B010 = Hg, B011 = atm, B100 = bar, B101 = torr, B110 = N/m^2, B111 = psi

  bme.read(pres, temp, hum, metric, pressureUnit);
//  temp -= 0.3; // correct temp
  pres /= 133.3; // convert to mmHg  

    byte minTemp = 255;
    byte minHum = 255;
    byte minPres = 255;
    byte maxTemp = 0;
    byte maxHum = 0;
    byte maxPres = 0;
     
    avrg.temp += temp;
    avrg.hum += hum;
    avrg.pres += pres;
    avrg.counter++;
  
    if (fastMode && avrg.counter >= STORAGE_TIME) {
      fastMode = false;
      for (int i = 0; i < PLOT_LEN - 1; i++) {
        infoArr[i].temp = 0;
        infoArr[i].hum = 0;
        infoArr[i].pres = 0;
      }
    }
  
    if (fastMode || avrg.counter >= STORAGE_TIME) {
      if (avrg.counter >= STORAGE_TIME) {
        temp = avrg.temp / avrg.counter;
        hum = avrg.hum / avrg.counter;
        pres = avrg.pres / avrg.counter;
        avrg.temp = 0;
        avrg.hum = 0;
        avrg.pres = 0;
        avrg.counter = 0;
      }
      for (int i = 1; i < PLOT_LEN; i++) {
        infoArr[i - 1] = infoArr[i];
      }
      infoArr[PLOT_LEN - 1].temp = round(temp) + 50;
      infoArr[PLOT_LEN - 1].pres = round(pres) - 650;
      infoArr[PLOT_LEN - 1].hum = round(hum);
    }
  
    for (int i = PLOT_LEN - 1; i >= 0 ; i--) {
      if (infoArr[i].temp == 0 && infoArr[i].hum == 0 && infoArr[i].pres == 0) break;
  
      if (infoArr[i].temp < minTemp) minTemp = infoArr[i].temp;
      if (infoArr[i].hum < minHum) minHum = infoArr[i].hum;
      if (infoArr[i].pres < minPres) minPres = infoArr[i].pres;
  
      if (infoArr[i].temp > maxTemp) maxTemp = infoArr[i].temp;
      if (infoArr[i].hum > maxHum) maxHum = infoArr[i].hum;
      if (infoArr[i].pres > maxPres) maxPres = infoArr[i].pres;
    }
    
    if (maxTemp - minTemp < 10) maxTemp = minTemp + 10;
    if (maxHum - minHum < 10) maxHum = minHum + 10;
    if (maxPres - minPres < 10) maxPres = minPres + 10;


  myOLED.setBrightness(10);
  myOLED.clrScr();

  if(wait_cnt == 0){
      title = "TEMPERATURE";
      leftTitle = 32;
      
      unit = "~C";
      val = temp; 

      minVal = minTemp-50;
      maxVal = maxTemp-50;
  }else if(wait_cnt == 1){
      title = "HUMIDITY";
      leftTitle = 40;
      
      unit = "%";
      val = hum;

      minVal = minHum;
      maxVal = maxHum;
  }else if(wait_cnt == 2){
      title = "PRESSURE";
      leftTitle = 40;
      
      unit = "MM";
      val = pres;

      minVal = minPres+650;
      maxVal = maxPres+650;
  }
  
  myOLED.setFont(SmallFont);
  myOLED.drawRoundRect(1, 1, 127, 15); // черный прямоугольник со скругленными вершинами.       
  myOLED.print(title, leftTitle, 5);  

  myOLED.drawRoundRect(1, 16, 80, 63); // черный прямоугольник со скругленными вершинами.
  myOLED.setFont(MediumNumbers);  
  myOLED.print(String(val, 1), 10, 31);

  myOLED.setFont(SmallFont);
  myOLED.print(unit, 64, 20);

  myOLED.drawRoundRect(82, 16, 127, 63); // черный прямоугольник со скругленными вершинами.
  myOLED.setFont(SmallFont);
  myOLED.print("MIN", 95, 20);
  myOLED.print(String(minVal, 1), 85, 32);

  myOLED.print("MAX", 95, 43);
  myOLED.print(String(maxVal, 1), 85, 53);  

  myOLED.update();
  
  delay(15000);

  /*
    Graph
  */
  wait_cnt++;
  if (wait_cnt > 2) {
    wait_cnt = 0;
    myOLED.clrScr();

    myOLED.setFont(SmallFont);
    myOLED.print(String(minTemp - 50), 0, 12);
    myOLED.print(String(maxTemp - 50), 0, 2);

    myOLED.print(String(minHum), 0, 34);
    myOLED.print(String(maxHum), 0, 24);

    myOLED.print(String(minPres + 650), 0, 56);
    myOLED.print(String(maxPres + 650), 0, 46);

    int x = 24;
    for (int i = 0; i < PLOT_LEN - 1; i++) {
      if (infoArr[i].temp == 0 && infoArr[i].hum == 0 && infoArr[i].pres == 0) continue;

      myOLED.drawLine(x, map(infoArr[i].temp, minTemp, maxTemp, 18, 0), x + 1, map(infoArr[i + 1].temp, minTemp, maxTemp, 18, 0));
      myOLED.drawLine(x, map(infoArr[i].hum, minHum, maxHum, 40, 22), x + 1, map(infoArr[i + 1].hum, minHum, maxHum, 40, 22));
      myOLED.drawLine(x, map(infoArr[i].pres, minPres, maxPres, 62, 44), x + 1, map(infoArr[i + 1].pres, minPres, maxPres, 62, 44));

      x++;
    }

    myOLED.update();

    delay(15000);
  }

  

}
