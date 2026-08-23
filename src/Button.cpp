#include "Button.h"

Button::Button(Adafruit_MCP23017 *mcp, uint8_t buttonPin, int id, onActionFunction actionFunc) {
    this->mcp = mcp;
    this->buttonPin = buttonPin;
    this->id = id;
    this->actionFunc = actionFunc;
    currentState = HIGH;
    lastButtonState = HIGH;
}

void Button::begin() {
    mcp->pinMode(buttonPin, INPUT);
    mcp->pullUp(buttonPin, HIGH);     // Pulled high ~100k
    currentState = mcp->digitalRead(buttonPin);
}

void Button::setHold(onHoldFunction holdFunc, unsigned long holdMs) {
    this->holdFunc = holdFunc;
    this->holdThreshold = holdMs;
}

void Button::feedInput(uint16_t gpioAB) {
    uint8_t pinState = bitRead(gpioAB, buttonPin);
    process(pinState);
}

void Button::process(int pinState) {
    if (pinState != lastButtonState) {
        lastDebounceTime = millis();
    }

    unsigned long time = millis() - lastDebounceTime;

    if (time > debounceDelay) {
        if (pinState != currentState) {
            // Debounced press or release
            currentState = pinState;
            bool released = pinState == HIGH;   // active-low: HIGH = released

            if (!released) {
                // Press just started
                pressedTime = millis();
                holdFired = false;
            } else if (holdFired) {
                // This release ends a hold we already actioned -> swallow it
                // so the normal tap action doesn't also run.
                holdFired = false;
                lastButtonState = pinState;
                return;
            }

            if (actionFunc) {
                actionFunc(this, released);
            }
        }
        else if (currentState == LOW && !holdFired && holdFunc &&
                 (millis() - pressedTime) >= holdThreshold) {
            // Still held down, past the threshold -> fire hold once
            holdFired = true;
            holdFunc(this);
        }
    }

    lastButtonState = pinState;
}

Adafruit_MCP23017 *Button::getMcp() const {
    return mcp;
}