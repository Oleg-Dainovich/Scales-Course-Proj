#include <HX711_ADC.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

//pins:
#define BUTT A5             // button pin
const int HX711_dout = 6;   // DT      mcu > HX711 dout pin
const int HX711_sck = 5;    // SCK     mcu > HX711 sck pin

float calibrationValue;     // calibration value
float newCalibrationValue;

bool isStarted = false;     // start scales work
bool isError = false;       // for error handler

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
  Serial.println("");
  Serial.println("Starting...");

  calibrationValue = 23.18;

  LoadCell.begin();
  unsigned long stabilizingtime = 2000;   // tare preciscion can be improved by adding a few seconds of stabilizing time
  boolean _tare = true;                   
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration factor
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
  Serial.print("Calibration value: ");
  Serial.println(LoadCell.getCalFactor());
  
  if (LoadCell.getSPS() < 7) {
    Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
  }
  else if (LoadCell.getSPS() > 100) {
    Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
  }
  // check_calibrate();
}

void loop() {
  // Serial.println(digitalRead(HX711_dout));
  // Serial.println(digitalRead(HX711_sck));
  // delay(1000);
  button = !digitalRead(BUTT);
  char num_buffer[10];

  // if (!digitalRead(HX711_dout)) {
  //   u8g2.clearBuffer();
  //   u8g2.setFont(u8g2_font_ncenB18_tr); 
  //   u8g2.drawStr(0,32, "Error");
  //   u8g2.sendBuffer();
  // }
  //else {
    static boolean newDataReady = 0;
    const int serialPrintInterval = 1000; // delay in calculating weight

    // check for new data/start next conversion:
    if (LoadCell.update()) newDataReady = true;

    // get value from scales:
    if (newDataReady) {
      if (millis() > t + serialPrintInterval) {
        //Serial.println("Go");
        if (!isStarted) {

          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_ncenB18_tr); 
          u8g2.drawStr(0,32, "Wait...");
          u8g2.sendBuffer();

          check_calibrate();
          isStarted = true;

          if (newCalibrationValue - calibrationValue >= 10 || newCalibrationValue - calibrationValue <= -10) {
            
            Serial.println("Error. Check sensors or put scales on flat area.");

            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_ncenB18_tr); 
            u8g2.drawStr(0,32, "Error");
            u8g2.sendBuffer();

            delay(4000);
            
            isStarted = false;
            isError = true;
          }
          else {
            LoadCell.setCalFactor(calibrationValue);
            isError = false;
          }
        }
        else {
          float i = LoadCell.getData();

          //if (i >= 50 || i <= -50) {
            // output in com port
            Serial.print("Load_cell output val: ");
            Serial.println(i);
            
            // output on display
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_ncenB18_tr); 
            dtostrf(i, 8, 2, num_buffer);
            u8g2.drawStr(0, 32, num_buffer);
            u8g2.sendBuffer(); 
            memset(num_buffer, 0, sizeof(num_buffer));
          //}
          // else {
          //   Serial.print("Load_cell output val: ");
          //   Serial.println("0.00");

          //   u8g2.clearBuffer();
          //   u8g2.setFont(u8g2_font_ncenB18_tr); 
          //   u8g2.drawStr(0,32, "0.00");
          //   u8g2.sendBuffer();
          // }
          newDataReady = 0;
          t = millis();
        }
      }
    }
  //}

  // tare function if button pressed
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

void check_calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");

  delay(2000);

  LoadCell.update();
  LoadCell.tare();

  Serial.print("Tared mass: ");
  Serial.println(LoadCell.getData());
  Serial.print("Tare status: ");
  Serial.println(LoadCell.getTareStatus());

  LoadCell.update();

  delay(1000);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB18_tr); 
  u8g2.drawStr(0,32, "Start");
  u8g2.sendBuffer();

  Serial.println("Start to check calibration.");

  delay(3000);

  bool isGoodMass = false;

  float known_mass = 0;

  while(!isGoodMass) {
    if(millis() > t + 1000) {
      LoadCell.update();
      known_mass = LoadCell.getData();

      if (known_mass >= 100) {
        isGoodMass = true;
      }

      // Serial.print("New mass is: ");
      // Serial.println(known_mass);

      t = millis();
    }
  }

  // Serial.print("New mass is: ");
  // Serial.println(known_mass);

  LoadCell.refreshDataSet();
  newCalibrationValue = LoadCell.getNewCalibration(220); //get the new calibration value

  Serial.print("New calibration value: ");
  Serial.println(newCalibrationValue);

  //Serial.println("End calibration");
}
