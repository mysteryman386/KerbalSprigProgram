/*
    Kerbal Sprig Program V3
    screenHandler.h

    This header file contains code for the screen handler, all encapsulated in a neat class

    This Version Created 28/04/2025
    By Wilmer Zhang

*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
/////////////////PINS SPECIFIC TO THE SPRIG, DO NOT TOUCH UNLESS YOU ARE ADAPTING THIS TO ANOTHER DEVICE
constexpr int TFT_CS = 20;
constexpr int TFT_RST = 26;
constexpr int TFT_DC = 22;
constexpr int TFT_BACKLIGHT = 17;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
/////////////////////////////////////////////////////////////////////////////////////////////////////////

class screenHandler {  //Screenhandler class. These functions handle the higher level aspects of manipulating the screen.
private:
  bool backlightState = false;
  bool drawState = false;
  float verticalVelocity;
  float surfaceAltitude;
  float vesselApoapsis;
  float vesselPeriapsis;
  unsigned long previousMillis = 0;
public:
  void updateFloatInfo(int selection, float newData) {  //This function updates float based info, and plugs it into the private variables. selection = 0 manipulates vertical velocity, selection = 1 manipulates surface altitude, 2 maipulates apoapsis, 3 manipulates periapsis.
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
  float getFloatInfo(int selection) {  //This retrieves info from the private variables. same selection choice as the updateFloatInfo functions.
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
    return -1;
  }

  void startScreen() {  //This function handles the startup function of the tft screen.
    pinMode(TFT_BACKLIGHT, OUTPUT);
    tft.initR(INITR_BLACKTAB);
    digitalWrite(TFT_BACKLIGHT, HIGH);
    backlightState = true;
    tft.fillScreen(ST77XX_BLACK);
    tft.setRotation(3);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextWrap(true);
    tft.println("Kerbal Sprig Program activated\n");
    tft.println("Handshaking.");
  }
  void toggleBacklight() {  //When called, this function either disables or enables the backlight, depending on current backlight status.
    if (backlightState == false) {
      digitalWrite(TFT_BACKLIGHT, HIGH);
      backlightState = true;
    } else {
      digitalWrite(TFT_BACKLIGHT, LOW);
      backlightState = false;
    }
  }
  void showStats(String message, bool secondControls) {  //This function wipes the screen, and shows statistics on the screen. It also shows custom messages that may be handy to the end user, using the Message variable.
    rp2040.idleOtherCore();
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 0);
    tft.println(message);
    tft.print(verticalVelocity);
    tft.println("m/s vert velocity");
    tft.print(surfaceAltitude);
    tft.println("m above surface");
    tft.print(vesselApoapsis);
    tft.println("m Apoapsis");
    if (vesselPeriapsis <= 0) {
      tft.print("0");
    } else {
      tft.print(vesselPeriapsis);
    }
    tft.println("m Periapsis");
    tft.print("Second Controls ");
    if (secondControls == true) {
      tft.println("Enabled");
    } else {
      tft.println("Disabled");
    }
    rp2040.resumeOtherCore();
  }
  void syncScreen(long syncInterval, bool secondControls) {  //This function calls the showStats() function every x milliseconds. This is determined by the syncInterval() variable.
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= syncInterval) {
      showStats(" ", secondControls);
      previousMillis = currentMillis;
    }
  }

  void errorScreen(String message) {  //Neat little function for a big red screen, for errors. Perfect for the autoAbort function
    tft.fillScreen(ST77XX_RED);
    tft.setCursor(0, 0);
    tft.println(message);
  }
};
