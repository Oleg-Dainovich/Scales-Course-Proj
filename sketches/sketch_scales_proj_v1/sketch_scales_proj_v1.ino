#include <HX711_ADC.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

//pins:
#define BUTT A5             // button pin
const int HX711_dout = 6;   // DT      mcu > HX711 dout pin
const int HX711_sck = 7;    // SCK     mcu > HX711 sck pin

bool button_flag = 0;
bool button;
unsigned long last_press; 

// HX711 constructor
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Display lib init
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0); 

unsigned long t = 0;

void setup() {
  pinMode(BUTT, INPUT_PULLUP);
  u8g2.begin();
  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");

  float calibrationValue; // calibration value
  calibrationValue = 23.18;

  LoadCell.begin();
  //LoadCell.setReverseOutput();
  unsigned long stabilizingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration factor (float)
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
  Serial.print("Calibration value: ");
  Serial.println(LoadCell.getCalFactor());
  Serial.print("HX711 measured conversion time ms: ");
  Serial.println(LoadCell.getConversionTime());
  Serial.print("HX711 measured sampling rate HZ: ");
  Serial.println(LoadCell.getSPS());
  Serial.print("HX711 measured settlingtime ms: ");
  Serial.println(LoadCell.getSettlingTime());
  Serial.println("Note that the settling time may increase significantly if you use delay() in your sketch!");
  if (LoadCell.getSPS() < 7) {
    Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
  }
  else if (LoadCell.getSPS() > 100) {
    Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
  }
}

void loop() {
  button = !digitalRead(BUTT);
  char num_buffer[10];
  
  static boolean newDataReady = 0;
  const int serialPrintInterval = 500; // delay in calculating weight

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(i);
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB18_tr); 
      dtostrf(i, 8, 2, num_buffer);
      u8g2.drawStr(0,32,num_buffer);
      u8g2.sendBuffer();
      memset(num_buffer, 0, sizeof(num_buffer));
      newDataReady = 0;
      t = millis();
    }
  }

  if (button == 1 && button_flag == 0 && millis() - last_press > 100) {
    button_flag = 1;
    //Serial.println("Pressed");
    last_press = millis();
  }

  if (button == 0 && button_flag == 1) {
    button_flag = 0;
    LoadCell.tareNoDelay();
    //Serial.println("Released");
  }

  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare done");

      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB18_tr); 
      u8g2.drawStr(0,32, "Tare done");
      u8g2.sendBuffer();
      delay(1000); 
  }

}
