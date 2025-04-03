#include <Arduino.h>
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
#include <SPI.h>
#include <string.h>
#include "KerbalSimpit.h"



//SPRIG PIN DEFINITIONS
#define R_LED 4
#define L_LED 28
#define TFT_CS 20
#define TFT_RST 26 
#define TFT_DC 22
#define TFT_BACKLIGHT 17
#define pBCLK 10
#define pWS 11
#define pDOUT 9
#define KEY_W 5
#define KEY_A 6
#define KEY_S 7
#define KEY_D 8
#define KEY_I 12
#define KEY_J 13
#define KEY_K 14
#define KEY_L 15

const int keys[] = { KEY_W, KEY_A, KEY_S, KEY_D, KEY_I, KEY_J, KEY_K, KEY_L };
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
int keyState[] = { 0, 0, 0, 0, 0, 0, 0, 0 };  //WASD, IJKL
int screenState = 0;
int selectedItem = 0;
cameraRotationMessage camMsg;
throttleMessage thrustMsg;
rotationMessage rotMsg;
int lastYaw = 0;
int lastPitch = 0;

bool secondControls = false;

KerbalSimpit mySimpit(Serial);




void setup() {
  Serial.begin(115200);
  pinMode(TFT_BACKLIGHT, OUTPUT);  // Backlight
  pinMode(R_LED, OUTPUT);          //Right LED
  pinMode(L_LED, OUTPUT);          //Left LED
  pinMode(27, INPUT);
  digitalWrite(TFT_BACKLIGHT, LOW);


  for (int key : keys) pinMode(key, INPUT_PULLUP);
  String vesselName;

  randomSeed(analogRead(27));
  tft.initR(INITR_BLACKTAB);  // Init ST7735S chip, black tab
  digitalWrite(TFT_BACKLIGHT, HIGH);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(3);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextWrap(true);
  tft.println("Kerbal Sprig Program activated\n");
  tft.println("Handshaking.");
  while (!mySimpit.init()) {
    digitalWrite(R_LED, HIGH);
    tft.print(".");
    delay(100);
    digitalWrite(R_LED, LOW);
  }
  digitalWrite(L_LED, HIGH);
  // Display a message in KSP to indicate handshaking is complete.
  mySimpit.printToKSP("Kerbal Sprig Program has connected.", PRINT_TO_SCREEN);
}


void loop() {
  assignStates();
  handleKeyPresses();
}

void assignStates() {
  keyState[0] = digitalRead(5);
  keyState[1] = digitalRead(6);
  keyState[2] = digitalRead(7);
  keyState[3] = digitalRead(8);
  keyState[4] = digitalRead(12);
  keyState[5] = digitalRead(13);
  keyState[6] = digitalRead(14);
  keyState[7] = digitalRead(15);
}

void stageAction() {
  mySimpit.activateAction(STAGE_ACTION);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.println("Currently controlling:");
  while (keyState[0] == LOW) {
    assignStates();
  }
}
void abortAction() {
  mySimpit.activateAction(ABORT_ACTION);
  tft.fillScreen(ST77XX_RED);
  tft.setCursor(0, 0);
  tft.println("Aborting Vessel:");
  digitalWrite(R_LED, HIGH);
  delay(100);
  digitalWrite(R_LED, LOW);
  delay(100);
  digitalWrite(R_LED, HIGH);
  delay(100);
  digitalWrite(R_LED, LOW);
  delay(100);
  digitalWrite(R_LED, HIGH);
  delay(100);
  digitalWrite(R_LED, LOW);
  delay(100);
  digitalWrite(R_LED, HIGH);
  delay(100);
  digitalWrite(R_LED, LOW);
  delay(100);
  digitalWrite(R_LED, HIGH);
  delay(100);
  digitalWrite(R_LED, LOW);
  delay(100);
}

void lightAction() {
  mySimpit.toggleAction(LIGHT_ACTION);
  tft.fillScreen(ST77XX_WHITE);
  tft.setCursor(0, 0);
  tft.println("Light bind activated:");
  while (keyState[3] == LOW) {
    assignStates();
  }
}

void sasAction() {
  mySimpit.toggleAction(SAS_ACTION);
  tft.fillScreen(ST77XX_WHITE);
  tft.setCursor(0, 0);
  tft.println("SAS activated:");
  while (keyState[1] == LOW) {
    assignStates();
  }
}

void camYaw(int yawFactor) {
  camMsg.setYaw(yawFactor);  // Rotate camera to the right
  mySimpit.send(CAMERA_ROTATION_MESSAGE, camMsg);
  camMsg.setYaw(0);  // Stop pitch movement
                     /*while (keyState[5] == LOW) {
      assignStates();
    }*/
}


void camPitch(int pitchFactor) {
  camMsg.setPitch(pitchFactor);  // Rotate camera to the right
  mySimpit.send(CAMERA_ROTATION_MESSAGE, camMsg);
  camMsg.setPitch(0);  // Stop pitch movement
                       /*while (keyState[4] == LOW) {
      assignStates();
    }*/
}

void thrustAction(int control) {

  thrustMsg.throttle = control;
  mySimpit.send(THROTTLE_MESSAGE, thrustMsg);
  thrustMsg.throttle = 0;
}

void controlYawPitch(int yawFactor, int pitchFactor) {
  if (yawFactor != lastYaw || pitchFactor != lastPitch) {
    rotMsg.setYaw(yawFactor);
    rotMsg.setPitch(pitchFactor);
    mySimpit.send(ROTATION_MESSAGE, rotMsg);
    lastYaw = yawFactor;
    lastPitch = pitchFactor;
  }
  delay(75);  // Slightly larger delay to reduce input spam
}

void handleKeyPresses() {  //This could be handled differently on a page to page basis...
  if (BOOTSEL) {
    while (BOOTSEL) {
      delay(1);
    }


    if (secondControls == false) {
      mySimpit.printToKSP("Second set of Controls activated.", PRINT_TO_SCREEN);
      secondControls = true;

    } else {
      mySimpit.printToKSP("Second set of Controls deactivated.", PRINT_TO_SCREEN);
      secondControls = false;
    }
  }
  for (int i = 0; i < 8; i++) {
    if (keyState[i] == LOW) {
      switch (i) {
        case 3:
          lightAction();
          break;
        case 1:
          sasAction();
          break;
        case 0:
          secondControls ? thrustAction(32767) : stageAction();
          break;
        case 2:
          secondControls ? thrustAction(0) : abortAction();
          break;
        case 4:
          secondControls ? controlYawPitch(0, 32767) : camPitch(10);
          break;
        case 5:
          secondControls ? controlYawPitch(-32767, 0) : camYaw(10);
          break;
        case 6:
          secondControls ? controlYawPitch(0, -32767) : camPitch(-10);
          break;
        case 7:
          secondControls ? controlYawPitch(32767, 0) : camYaw(-10);
          break;
      }
    }
  }

  if (secondControls/* && keyState[0 - 7] == HIGH*/) {
    controlYawPitch(0, 0);
  }
}
