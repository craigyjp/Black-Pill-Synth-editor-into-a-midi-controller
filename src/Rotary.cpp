#include "Arduino.h"
#include "Rotary.h"

// ---------------------------------------------------------------------------
// State tables for both decoding modes
// ---------------------------------------------------------------------------

#define R_START 0x0

// ----- Half-step defines -----
#define RH_CCW_BEGIN      0x1
#define RH_CW_BEGIN       0x2
#define RH_START_M        0x3
#define RH_CW_BEGIN_M     0x4
#define RH_CCW_BEGIN_M    0x5

const unsigned char ttable_half[6][4] = {
  // R_START (00)
  {RH_START_M,             RH_CW_BEGIN,      RH_CCW_BEGIN,   R_START},
  // R_CCW_BEGIN
  {RH_START_M | DIR_CCW,   R_START,          RH_CCW_BEGIN,   R_START},
  // R_CW_BEGIN
  {RH_START_M | DIR_CW,    RH_CW_BEGIN,      R_START,        R_START},
  // R_START_M (11)
  {RH_START_M,             RH_CCW_BEGIN_M,   RH_CW_BEGIN_M,  R_START},
  // R_CW_BEGIN_M
  {RH_START_M,             RH_START_M,       RH_CW_BEGIN_M,  R_START | DIR_CW},
  // R_CCW_BEGIN_M
  {RH_START_M,             RH_CCW_BEGIN_M,   RH_START_M,     R_START | DIR_CCW},
};

// ----- Full-step table (emits at 00 only) -----
#define R_CW_FINAL   0x1
#define R_CW_BEGIN   0x2
#define R_CW_NEXT    0x3
#define R_CCW_BEGIN  0x4
#define R_CCW_FINAL  0x5
#define R_CCW_NEXT   0x6

const unsigned char ttable_full[7][4] = {
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},                // R_START
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},       // R_CW_FINAL
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},                // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},                // R_CW_NEXT
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},                // R_CCW_BEGIN
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},      // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},                // R_CCW_NEXT
};

// ---------------------------------------------------------------------------
// External flag from settings menu
// ---------------------------------------------------------------------------
extern bool halfStepMode;

// ---------------------------------------------------------------------------
// Rotary implementation
// ---------------------------------------------------------------------------
Rotary::Rotary(char _pin1, char _pin2) {
  pin1 = _pin1;
  pin2 = _pin2;
  state = R_START;
}

void Rotary::begin(bool pullup) {
  if (pullup) {
    pinMode(pin1, INPUT_PULLUP);
    pinMode(pin2, INPUT_PULLUP);
  } else {
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);
  }
}

unsigned char Rotary::process() {
  return process(digitalRead(pin1), digitalRead(pin2));
}

unsigned char Rotary::process(unsigned char pin1State, unsigned char pin2State) {
  unsigned char pinstate = (pin2State << 1) | pin1State;

  if (halfStepMode) {
    state = ttable_half[state & 0x0F][pinstate];
  } else {
    state = ttable_full[state & 0x0F][pinstate];
  }

  return state & 0x30;
}
