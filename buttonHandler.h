/*
    Kerbal Sprig Program V3
    buttonHandler.h

    This header file contains code for button handling, all encapsulated in a neat class

    This Version Created 28/04/2025
    By Wilmer Zhang

*/

#include <Arduino.h>
class ButtonHandler {
private:
  int pin;
  bool lastState;
  unsigned long lastDebounceTime;
  const unsigned long debounceDelay = 50;  // Debounce time in ms
  bool debounceState = true;

public:
  ButtonHandler(int buttonPin) { //This function initializes the buttons for use, using the buttonPin variable to activate the pullup Resistors.
    pin = buttonPin;
    pinMode(pin, INPUT_PULLUP);
    lastState = digitalRead(pin);
    lastDebounceTime = 0;
  }

  void setDebounceState(bool debounceFlag) { //This function sets the debounce state of a button, allowing debounce to be enabled or disabled
    debounceState = debounceFlag;
  }


  bool isPressed() { //This function is a bool function that reads the pinstate, and check if it has been set low. If it does, it returns true, else it sets to false. Also contains debounce that prevents accidental repeat presses
    bool currentState = digitalRead(pin);
    if (debounceState == true && currentState == LOW && lastState == HIGH && (millis() - lastDebounceTime) > debounceDelay) {
      lastDebounceTime = millis();
      lastState = LOW;
      return true;
    } else if (currentState == LOW && debounceState == false) {
      return true;
    }
    lastState = currentState;
    return false;
  }
};
