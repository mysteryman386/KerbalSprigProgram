#include <Arduino.h>
class ButtonHandler {
private:
  int pin;
  bool lastState;
  unsigned long lastDebounceTime;
  const unsigned long debounceDelay = 50;  // Debounce time in ms
  bool debounceState = true;

public:
  ButtonHandler(int buttonPin) {
    pin = buttonPin;
    pinMode(pin, INPUT_PULLUP);
    lastState = digitalRead(pin);
    lastDebounceTime = 0;
  }

  void setDebounceState(bool debounceFlag) {
    debounceState = debounceFlag;
  }


  bool isPressed() {
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
