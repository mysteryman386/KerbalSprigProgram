/*
    Kerbal Sprig Program Xtreme
    KerbalSprigProgram.ino

    This program is an alternate firmware for the Sprig that interfaces with Kerbal Space Program via the Kerbal Simpit mod.

    This Version Modified 27/06/2025
    By Wilmer Zhang

*/

#include <Arduino.h>
#include <string.h>
#include "FS.h"
#include "KerbalSimpit.h"
#include "buttonHandler.h"
#include "PayloadStructs.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <RPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LEAmDNS.h>

//SPRIG PIN DEFINITIONS, DO NOT TOUCH UNLESS YOU ARE AN EXPERT
constexpr int R_LED = 4;
constexpr int L_LED = 28;
constexpr int KEY_W = 5;
constexpr int KEY_A = 6;
constexpr int KEY_S = 7;
constexpr int KEY_D = 8;
constexpr int KEY_I = 12;
constexpr int KEY_J = 13;
constexpr int KEY_K = 14;
constexpr int KEY_L = 15;
//USER DEFINED SETTINGS
constexpr int armAbortAbove = 2000;       //When the craft is above this altitude in Meters, the auto abort system is armed
constexpr int triggerAbortBelow = 1750;   //When the craft is below this altitude, and satisfying speed requirements, the auto abort system is triggered
constexpr int triggerAbortSpeed = 75;     //When the craft is faster than this vertical speed (m/s), and when below TAB altitude, the auto abort system is triggered
constexpr int screenSyncInterval = 2500;  //How much time elapses (ms) before the screen updates with new data

int keyState[] = { 0, 0, 0, 0, 0, 0, 0, 0 };  //WASD, IJKL
cameraRotationMessage camMsg;
throttleMessage thrustMsg;
rotationMessage rotMsg;
bool secondControls = false;
bool autoAbort = false;
int lastYaw = 0;
int lastPitch = 0;
KerbalSimpit mySimpit(Serial);
const char *ssid = "PLACEHOLDER";
const char *password = "PLACEHOLDER";
AsyncWebServer server(80);
const String authKey = rp2040.getChipID();

bool speedConditionSatisfied;
float surfaceVelocity;
float verticalAltitude;

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

void setup() {  //initial setup, this piece of code runs when the sprig starts.
  Serial.begin(115200);
  pinMode(R_LED, OUTPUT);
  pinMode(L_LED, OUTPUT);
  for (int i = 4; i < 8; i++) {  //Sets the debounce state of the right hand buttons to false
    buttons[i].setDebounceState(false);
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Authenticaton key: ");
  Serial.println(authKey);
  Serial.println("Connecting to wifi");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(L_LED, HIGH);
    Serial.print(".");
    delay(100);
    digitalWrite(L_LED, LOW);
    delay(100);
  }
  Serial.println("Unique code: ");
  Serial.println(authKey);
  Serial.println("Wifi IP Address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(L_LED, LOW);
  MDNS.begin("KerbalSprigProgram");
  MDNS.addService("http", "tcp", 80);
  while (!mySimpit.init()) {
    digitalWrite(R_LED, HIGH);
    delay(100);
    digitalWrite(R_LED, LOW);
    delay(100);
  }
  digitalWrite(L_LED, HIGH);
  mySimpit.printToKSP("Kerbal Sprig Program has connected.", PRINT_TO_SCREEN);
  mySimpit.inboundHandler(messageHandler);  //These lines allow the sprig to interact with KerbalSimpit, allowing two way communication of information.
  mySimpit.registerChannel(ALTITUDE_MESSAGE);
  mySimpit.registerChannel(SCENE_CHANGE_MESSAGE);
  mySimpit.registerChannel(VELOCITY_MESSAGE);
  mySimpit.registerChannel(APSIDES_MESSAGE);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String("responded"));
  });

  server.on("/abort", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("auth")) {
      request->send(401, "text/plain", "Unauthorized: Missing auth");
      return;
    }
    String authParam = request->getParam("auth")->value();
    if (authParam != authKey) {
      request->send(403, "text/plain", "Unauthorized: Invalid Identifier");
      return;
    }
    abortAction();
    request->send(200, "text/plain", String("responded"));
  });

  server.on("/sas", HTTP_GET, [](AsyncWebServerRequest *request) {
    actionBind(SAS_ACTION, "SAS toggled!");
    request->send(200, "text/plain", String("responded"));
  });

  server.on("/lights", HTTP_GET, [](AsyncWebServerRequest *request) {
    actionBind(LIGHT_ACTION, "Lights toggled!");
    request->send(200, "text/plain", String("responded"));
  });

  server.on("/surfaceVelocity", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(surfaceVelocity));
  });

  server.on("/verticalAltitude", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(verticalAltitude));
  });
  server.begin();
}



void loop() {
  MDNS.update();
  handleKeyPresses();
  mySimpit.update();
}



void abortAction() {  //This function deactivates SAS, and activates the abort action group.
  mySimpit.deactivateAction(SAS_ACTION);
  mySimpit.activateAction(ABORT_ACTION);
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
}

void camMovement(int pitchFactor, int yawFactor) {  //This function rotates the camera in a pitch motion (+ve pitchFactor moves the camera up, -ve pitchFactor moves the camera down) and yaw motion (+ve yawFactor moves the camera right, -ve yawFactor moves the camera left)
  camMsg.setPitch(pitchFactor);
  camMsg.setYaw(yawFactor);
  mySimpit.send(CAMERA_ROTATION_MESSAGE, camMsg);
  camMsg.setPitch(0);
  camMsg.setYaw(0);
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

// Function pointer type for button actions
typedef void (*ButtonAction)();
void (*primaryActions[])() = {  //Actions binded to the primary action section
  []() {                        //W
    actionBind(STAGE_ACTION, "Staged!");
  },
  []() {  //A
    actionBind(SAS_ACTION, "SAS toggled!");
  },
  abortAction,  //S
  []() {        //D
    actionBind(LIGHT_ACTION, "Lights toggled!");
  },
  []() {
    camMovement(10, 0);  //I
  },
  []() {
    camMovement(0, 10);  //J
  },
  []() {
    camMovement(-10, 0);  //K
  },
  []() {
    camMovement(0, -10);  //L
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

void messageHandler(byte messageType, byte msg[], byte msgSize) {  //This function is called with mySimpit.update(), updating information.

  switch (messageType) {
    case APSIDES_MESSAGE:  // small function that grabs orbital parameters, and passes them on to the screenhandler
      if (msgSize == sizeof(apsidesMessage)) {
        apsidesMessage myApsides;
        myApsides = parseMessage<apsidesMessage>(msg);
      }
      break;
    case SCENE_CHANGE_MESSAGE:
      if (msgSize == sizeof(byte)) {
        byte scene = msg[0];
        switch (scene) {  //This sets the autoabort flag to false, so abortion sequence does not happen when you revert flight or something.
          case 0:
          case 1:
          case 2:
          case 3:
            autoAbort = false;
            break;
        }
      }
      break;
    case VELOCITY_MESSAGE:  //This case grabs vertical velocity and stores it in screenhandler.h
      if (msgSize == sizeof(velocityMessage)) {
        velocityMessage myVelocity = parseMessage<velocityMessage>(msg);
        if (myVelocity.surface <= -triggerAbortSpeed) {
          speedConditionSatisfied = true;
        } else {
          speedConditionSatisfied = false;
        }
        surfaceVelocity = myVelocity.surface;
      }
      break;
    case ALTITUDE_MESSAGE:  //Bigger case that grabs altitude data and handles the autoabort logic. It checks if certain criteria has been set, and activates abortion sequence if it does.
      if (msgSize == sizeof(altitudeMessage)) {
        altitudeMessage myAltitude;
        myAltitude = parseMessage<altitudeMessage>(msg);
        verticalAltitude = myAltitude.surface;
        if (myAltitude.surface >= armAbortAbove) {
          digitalWrite(LED_BUILTIN, HIGH);
          autoAbort = true;
        } else {
          digitalWrite(LED_BUILTIN, LOW);
          if (autoAbort == true && myAltitude.surface <= triggerAbortBelow && speedConditionSatisfied == true) {
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
  for (int i = 0; i < 8; i++) {  //Rolls through buttons, and checks if any are pressed. If so, excute their specific functions
    if (buttons[i].isPressed()) {
      if (secondControls && secondaryActions[i] != nullptr) {
        secondaryActions[i]();
      } else if (primaryActions[i] != nullptr) {
        primaryActions[i]();
      }
    }
  }
}



//?auth=[unique identifier]E6632C85937C2730
