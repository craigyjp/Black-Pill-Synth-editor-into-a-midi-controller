#ifndef XVA1USERINTERFACE_BUTTON_H
#define XVA1USERINTERFACE_BUTTON_H

#include "Adafruit_MCP23017.h"

class Button {
public:
    typedef void (*onActionFunction)(Button *button, bool released);
    typedef void (*onHoldFunction)(Button *button);   // NEW: fired on long press

    int id = 0;

    Button(Adafruit_MCP23017 *mcp, uint8_t buttonPin, int id, onActionFunction actionFunc);

    virtual void begin();
    void feedInput(uint16_t gpioAB);
    void process(int pinState);

    // NEW: register a hold handler; holdMs = how long to count as a hold
    void setHold(onHoldFunction holdFunc, unsigned long holdMs = 800);

    Adafruit_MCP23017 *getMcp() const;

protected:
    Adafruit_MCP23017 *mcp = nullptr;
    uint8_t buttonPin = 0;
    onActionFunction actionFunc = nullptr;
    onHoldFunction holdFunc = nullptr;       // NEW
    unsigned long holdThreshold = 800;       // NEW

    int currentState;
    int lastButtonState;

    unsigned long lastDebounceTime = 0;
    unsigned long debounceDelay = 10;
    unsigned long pressedTime = 0;           // NEW: when this press began
    bool holdFired = false;                  // NEW: hold already actioned this press
};

#endif //XVA1USERINTERFACE_BUTTON_H