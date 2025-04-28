#include <Arduino.h>
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library for ST7735
#include <SPI.h>
#include <string.h>
#include "KerbalSimpit.h"
#include "buttonHandler.h"
#include "PayloadStructs.h"
#include "screenHandler.h"

//SPRIG PIN DEFINITIONS, DO NOT TOUCH UNLESS YOU ARE AN EXPERT
#define R_LED 4
#define L_LED 28
#define KEY_W 5
#define KEY_A 6
#define KEY_S 7
#define KEY_D 8
#define KEY_I 12
#define KEY_J 13
#define KEY_K 14
#define KEY_L 15
//USER DEFINED SETTINGS
#define armAbortAbove 2000       //When the craft is above this altitude in Meters, the auto abort system is armed
#define triggerAbortBelow 1750   //When the craft is below this altitude, and satisfying speed requirements, the auto abort system is triggered
#define triggerAbortSpeed 75     //When the craft is faster than this vertical speed (m/s), and when below TAB altitude, the auto abort system is triggered
#define screenSyncInterval 1000  //How much time elapses (ms) before the screen updates with new data

int keyState[] = { 0, 0, 0, 0, 0, 0, 0, 0 };  //WASD, IJKL
cameraRotationMessage camMsg;
throttleMessage thrustMsg;
rotationMessage rotMsg;
bool secondControls = false;
bool autoAbort = false;
int lastYaw = 0;
int lastPitch = 0;
KerbalSimpit mySimpit(Serial);
screenHandler sprigScreen;


void abortAction() {  //This function deactivates SAS, and activates the abort action group.
  mySimpit.deactivateAction(SAS_ACTION);
  mySimpit.activateAction(ABORT_ACTION);
  tft.fillScreen(ST77XX_RED);
  tft.setCursor(0, 0);
  tft.println("Aborting Vessel:");
  autoAbort = false;
  for (int i = 0; i <= 5; i++) {  // loop that flashes the right LED on and off for 1 second
    digitalWrite(R_LED, HIGH);
    delay(100);
    digitalWrite(R_LED, LOW);
    delay(100);
  }
}

void actionBind(uint8_t ActionGroup, String message) {  //This function replaces lightAction(), sasAction(), and stageAction() from V1, putting them all into one function that rules them all. ActionGroup is for the actiongroup needed to be set, and message is for the screen popup
  mySimpit.toggleAction(ActionGroup);
  sprigScreen.showStats(message, secondControls);
}

void camYaw(int yawFactor) {  //This function rotates the camera in a yaw motion (+ve yawFactor moves the camera right, -ve yawFactor moves the camera left)
  camMsg.setYaw(yawFactor);
  mySimpit.send(CAMERA_ROTATION_MESSAGE, camMsg);
  camMsg.setYaw(0);
}

void camPitch(int pitchFactor) {  //This function rotates the camera in a pitch motion (+ve pitchFactor moves the camera up, -ve pitchFactor moves the camera down)
  camMsg.setPitch(pitchFactor);
  mySimpit.send(CAMERA_ROTATION_MESSAGE, camMsg);
  camMsg.setPitch(0);
}

void thrustAction(int control) {  //This function controls thrust control (0-32767, 0 being no thrust, 32767 being full thrust)
  thrustMsg.throttle = control;
  mySimpit.send(THROTTLE_MESSAGE, thrustMsg);
  thrustMsg.throttle = 0;
}

void controlYawPitch(int yawFactor, int pitchFactor) {  //This function controls the yaw and pitch of the craft.
  if (yawFactor != lastYaw || pitchFactor != lastPitch) {
    rotMsg.setYaw(yawFactor);
    rotMsg.setPitch(pitchFactor);
    mySimpit.send(ROTATION_MESSAGE, rotMsg);
    lastYaw = yawFactor;
    lastPitch = pitchFactor;
  }
  delay(75);
}


// Create ButtonHandler objects for each button
ButtonHandler buttons[] = {
  ButtonHandler(KEY_W),
  ButtonHandler(KEY_A),
  ButtonHandler(KEY_S),
  ButtonHandler(KEY_D),
  ButtonHandler(KEY_I),
  ButtonHandler(KEY_J),
  ButtonHandler(KEY_K),
  ButtonHandler(KEY_L)
};

// Function pointer type for button actions
typedef void (*ButtonAction)();
void (*primaryActions[])() = {  //Actions binded to the primary action section
  []() {//W
    actionBind(STAGE_ACTION, "Staged!");
  },
  []() {//A
    actionBind(SAS_ACTION, "SAS toggled!");
  },
  abortAction,//S
  []() {//D
    actionBind(LIGHT_ACTION, "Lights toggled!");
  },
  []() {           
    camPitch(10);  //I
  },
  []() {
    camYaw(10);  //J
  },
  []() {
    camPitch(-10);  //K
  },
  []() {
    camYaw(-10);  //L
  }
};
void (*secondaryActions[])() = {
  //Actions binded to the secondary action section (ie when the back button is pressed)
  []() {
    thrustAction(32767);  //Increases the thrust to full thrust
  },                      // W
  nullptr,                // A (Unused)
  []() {
    thrustAction(0);  //Cuts of thrust
  },                  // S
  []() {
    controlYawPitch(0, 0);  //Cancels out all yaw actions
  },                        // D
  []() {
    controlYawPitch(0, 32767);
  },  // I
  []() {
    controlYawPitch(-32767, 0);
  },  // J
  []() {
    controlYawPitch(0, -32767);
  },  // K
  []() {
    controlYawPitch(32767, 0);
  }  // L
};

void setup() {  //initial setup, this piece of code runs when the sprig starts.
  Serial.begin(115200);
  pinMode(R_LED, OUTPUT);
  pinMode(L_LED, OUTPUT);
  for (int i = 4; i < 8; i++) {  //Sets the debounce state of the right hand buttons to false
    buttons[i].setDebounceState(false);
  }
  sprigScreen.startScreen();  //Sprig screen starts
  tft.println("Kerbal Sprig Program activated\n");
  tft.println("Handshaking.");
  while (!mySimpit.init()) {
    digitalWrite(R_LED, HIGH);
    tft.print(".");
    delay(100);
    digitalWrite(R_LED, LOW);
  }
  digitalWrite(L_LED, HIGH);
  mySimpit.printToKSP("Kerbal Sprig Program has connected.", PRINT_TO_SCREEN);
  mySimpit.inboundHandler(messageHandler);  //These lines allow the sprig to interact with KerbalSimpit, allowing two way communication of information.
  mySimpit.registerChannel(ALTITUDE_MESSAGE);
  mySimpit.registerChannel(SCENE_CHANGE_MESSAGE);
  mySimpit.registerChannel(VELOCITY_MESSAGE);
  mySimpit.registerChannel(APSIDES_MESSAGE);
}

void loop() {
  handleKeyPresses();
  mySimpit.update();
  sprigScreen.syncScreen(screenSyncInterval, secondControls);
}

void messageHandler(byte messageType, byte msg[], byte msgSize) {  //This function is called with mySimpit.update(), updating information.
  switch (messageType) {
    case APSIDES_MESSAGE:
      if (msgSize == sizeof(apsidesMessage)) {
        apsidesMessage myApsides;
        myApsides = parseMessage<apsidesMessage>(msg);
        sprigScreen.updateFloatInfo(2, myApsides.apoapsis);
        sprigScreen.updateFloatInfo(3, myApsides.periapsis);
      }
    case SCENE_CHANGE_MESSAGE:
      if (msgSize == sizeof(byte)) {
        byte scene = msg[0];  // Extract scene info
        switch (scene) {      //This sets the autoabort flag to false, so abortion sequence does not happen when you revert flight or something.
          case 0:
          case 1:
          case 2:
          case 3:
            autoAbort = false;
            break;
        }
      }
      break;
    case VELOCITY_MESSAGE:  //This function grabs vertical velocity and stores it in screenhandler.h
      if (msgSize == sizeof(velocityMessage)) {
        velocityMessage myVelocity = parseMessage<velocityMessage>(msg);
        if (sprigScreen.getFloatInfo(0) != myVelocity.vertical) {
          sprigScreen.updateFloatInfo(0, myVelocity.vertical);
        }
      }
      break;
    case ALTITUDE_MESSAGE:  //Bigger function that handles the autoabort logic. It checks if certain criteria has been set, and activates abortion sequence if it does.
      if (msgSize == sizeof(altitudeMessage)) {
        altitudeMessage myAltitude;
        myAltitude = parseMessage<altitudeMessage>(msg);
        sprigScreen.updateFloatInfo(1, myAltitude.surface);
        if (sprigScreen.getFloatInfo(1) >= armAbortAbove) {
          digitalWrite(LED_BUILTIN, HIGH);
          autoAbort = true;
        } else {
          digitalWrite(LED_BUILTIN, LOW);
          if (autoAbort == true && sprigScreen.getFloatInfo(1) <= triggerAbortBelow && sprigScreen.getFloatInfo(0) <= -triggerAbortSpeed) {
            abortAction();
          }
        }
      }
      break;
  }
}

void handleKeyPresses() {  //This function handles key presses, and calls the appropriate functions.
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
    if (buttons[i].isPressed()) {
      if (secondControls && secondaryActions[i] != nullptr) {
        secondaryActions[i]();
      } else if (primaryActions[i] != nullptr) {
        primaryActions[i]();
      }
    }
  }
}
