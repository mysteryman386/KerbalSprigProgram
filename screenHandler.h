#include <Arduino.h>
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
/////////////////PINS SPECIFIC TO THE SPRIG, DO NOT TOUCH UNLESS YOU ARE ADAPTING THIS TO ANOTHER DEVICE
#define TFT_CS 20
#define TFT_RST 26
#define TFT_DC 22
#define TFT_BACKLIGHT 17
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); 
/////////////////

class screenHandler {
private:
  bool backlightState = false;
  bool drawState = false;
  float verticalVelocity;
  float surfaceAltitude;
  float vesselApoapsis;
  float vesselPeriapsis;
public:
  void updateFloatInfo(int selection, float newData) {//This function updates float based info, and plugs it into the private variable.
    switch (selection) {
      case 0:
        verticalVelocity = newData;
        break;
      case 1:
        surfaceAltitude = newData;
        break;
      case 2:
        vesselApoapsis = newData;
        break;
      case 3:
        vesselPeriapsis = newData;
        break;
    }
  }
  float getFloatInfo(int selection) {//This retrieves info from the private variables. Only use this for display purposes, as this may cause some issues.
    switch (selection) {
      case 0:
        return verticalVelocity;
        break;
      case 1:
        return surfaceAltitude;
        break;
      case 2:
        return vesselApoapsis;
        break;
      case 3:
        return vesselPeriapsis;
        break;
    }
    return NULL;
  }

  void startScreen() { //This starts the screen
    pinMode(TFT_BACKLIGHT, OUTPUT);  // Backlight
    tft.initR(INITR_BLACKTAB);
    digitalWrite(TFT_BACKLIGHT, HIGH);
    backlightState = true;
    tft.fillScreen(ST77XX_BLACK);
    tft.setRotation(3);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextWrap(true);
  }
  void toggleBacklight() {
    if (backlightState == false) {
      digitalWrite(TFT_BACKLIGHT, HIGH);
      backlightState = true;
    } else {
      digitalWrite(TFT_BACKLIGHT, LOW);
      backlightState = false;
    }
  }
  void showStats(String message) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.println(message);
    tft.print(verticalVelocity);
    tft.println("m/s vert velocity");
    tft.print(surfaceAltitude);
    tft.println("m above surface");
    tft.print(vesselApoapsis);
    tft.println("m Apoapsis");
    if (vesselPeriapsis <= 0){
      tft.print("0");
    }
    else{
    tft.print(vesselPeriapsis);
    }
    tft.println("m Periapsis");
  }
};
