#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MidiCC.h"
#include "Constants.h"
#include "Parameters.h"
#include "PatchMgr.h"
#include "Button.h"
#include "HWControls.h"
#include "EepromMgr.h"
#include "Settings.h"

#define PARAMETER 0      //The main page for displaying the current patch and control (parameter) changes
#define RECALL 1         //Patches list
#define SAVE 2           //Save patch page
#define REINITIALISE 3   // Reinitialise message
#define PATCH 4          // Show current patch bypassing PARAMETER
#define PATCHNAMING 5    // Patch naming page
#define DELETE 6         //Delete patch page
#define DELETEMSG 7      //Delete patch message page
#define SETTINGS 8       //Settings page
#define SETTINGSVALUE 9  //Settings page

unsigned int state = PARAMETER;

#include "ST7735Display.h"



//MIDI 5 Pin DIN
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);  // main MIDI in and out
MIDI_CREATE_INSTANCE(HardwareSerial, Serial7, MIDI7);  // main MIDI in and out

int patchNo = 1;
int voiceToReturn = -1;                 //Initialise
unsigned long earliestTime = millis();  //For voice allocation - initialise to now
unsigned long buttonDebounce = 0;

void pollAllMCPs();

void initButtons();

void initOLEDDisplays();

void setup() {

  suppressParamAnnounce = true;
  bootInitInProgress = true;

  SPI.begin();
  Wire.begin();            // Join the I2C bus as Master
  Wire.setClock(100000);   // Set I2C speed to 100 kHz

  Wire1.begin();           // Join the I2C bus as Master
  Wire1.setClock(400000);  // Set I2C speed to 400 kHz

  mcp1.begin(0, Wire1);
  delay(10);
  mcp2.begin(1, Wire1);
  delay(10);
  mcp3.begin(2, Wire1);
  delay(10);


  initButtons();

  setupMCPOutputs();
  setupDisplay();
  initOLEDDisplays();

  // put something different on each screen
  for (uint8_t ch = 0; ch < NUM_OLED; ch++) {
    char buf[8];
    snprintf(buf, sizeof(buf), "CH %u", ch);
    writeOLED(ch, screenTitle[ch], "");
  }

  setUpSettings();
  setupHardware();
  primeMuxBaseline();


  cardStatus = SD.begin(EXTERNAL_SDCARD);
  if (cardStatus) {
    Serial.println("SD card is connected");
    //Get patch numbers and names from SD card
    loadPatches();
    if (patches.size() == 0) {
      //save an initialised patch to SD card
      savePatch("1", INITPATCH);
      loadPatches();
    }
  } else {
    Serial.println("SD card is not connected or unusable");
    manualMode = true;
    showPatchPage("No SD", "conn'd / usable");
    startParameterDisplay();
  }

  //Read MIDI Channel from EEPROM
  midiChannel = getMIDIChannel();

  //Read MIDI Out Channel from EEPROM
  midiOutCh = getMIDIOutCh();

  //Read Param updates
  updateParams = getUpdateParams();

  Serial.println("MIDI Ch:" + String(midiChannel) + " (0 is Omni On)");

  //USB Client MIDI
  usbMIDI.setHandleControlChange(myControlChange);
  usbMIDI.setHandleProgramChange(myProgramChange);
  usbMIDI.setHandleAfterTouchChannel(myAfterTouch);
  usbMIDI.setHandlePitchChange(DinHandlePitchBend);
  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandleNoteOff(myNoteOff);
  Serial.println("USB Client MIDI Listening");

  //MIDI 5 Pin DIN
  MIDI.begin();
  MIDI.setHandleControlChange(myControlChange);
  MIDI.setHandleProgramChange(myProgramChange);
  MIDI.setHandleAfterTouchChannel(myAfterTouch);
  MIDI.setHandlePitchBend(DinHandlePitchBend);
  MIDI.setHandleNoteOn(myNoteOn);
  MIDI.setHandleNoteOff(myNoteOff);
  MIDI.turnThruOn(midi::Thru::Mode::Off);
  Serial.println("MIDI In DIN Listening");

    //MIDI 5 Pin DIN
  MIDI7.begin();
  MIDI7.setHandleControlChange(myControlChange);
  MIDI7.setHandleAfterTouchChannel(myAfterTouch);
  MIDI7.setHandlePitchBend(DinHandlePitchBend);
  MIDI7.setHandleNoteOn(myNoteOn);
  MIDI7.setHandleNoteOff(myNoteOff);
  MIDI7.turnThruOn(midi::Thru::Mode::Off);
  Serial.println("MIDI7 In DIN Listening");

  //Read Encoder Direction from EEPROM
  encCW = getEncoderDir();

  //setupDisplay();
  delay(100);

  patchNo = 1;
  recallPatch(patchNo);  //Load first patch
  bootInitInProgress = false;
  suppressParamAnnounce = false;
  startParameterDisplay();
}

void tcaSelect(uint8_t ch) {
  if (ch > 7) return;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << ch);
  Wire.endTransmission();
}

void tcaDisable() {
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(0x00);
  Wire.endTransmission();
}

void initOLEDDisplays() {
  for (uint8_t ch = 0; ch < NUM_OLED; ch++) {
    tcaSelect(ch);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.print("Display on channel ");
      Serial.print(ch);
      Serial.println(" failed to init");
      continue;
    }
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.clearDisplay();
    display.display();
  }
  tcaDisable();
}

void writeOLED(uint8_t ch, const char* title, const char* value) {
  tcaSelect(ch);
  display.clearDisplay();

  display.setTextSize(1);          // small title row (8px)
  display.setCursor(0, 0);
  display.print(title);

  display.setTextSize(2);          // large value (16px)
  display.setCursor(0, 14);
  display.print(value);

  display.display();
}

// MUX disable on boot

void primeMuxBaseline() {
  for (int ch = 0; ch < MUXCHANNELS; ch++) {
    digitalWriteFast(MUX_0, ch & B0001);
    digitalWriteFast(MUX_1, ch & B0010);
    digitalWriteFast(MUX_2, ch & B0100);
    delayMicroseconds(2);

    mux1ValuesPrev[ch] = adc->adc0->analogRead(MUX1_S);
    mux2ValuesPrev[ch] = adc->adc0->analogRead(MUX2_S);
    mux3ValuesPrev[ch] = adc->adc1->analogRead(MUX3_S);
    mux4ValuesPrev[ch] = adc->adc0->analogRead(MUX4_S);
    mux5ValuesPrev[ch] = adc->adc0->analogRead(MUX5_S);
    mux6ValuesPrev[ch] = adc->adc1->analogRead(MUX6_S);
  }
  muxInput = 0;
}

void startParameterDisplay() {
  updateScreen();

  lastDisplayTriggerTime = millis();
  waitingToUpdate = true;
}


// Handling encoders and buttons

void pollAllMCPs() {

  for (int j = 0; j < numMCPs; j++) {
    uint16_t gpioAB = allMCPs[j]->readGPIOAB();

    for (auto &button : allButtons) {
      if (button->getMcp() == allMCPs[j]) {
        button->feedInput(gpioAB);
      }
    }
  }
}

void initButtons() {
  for (auto &button : allButtons) {
    button->begin();
  }
}

void mainButtonChanged(Button *btn, bool released) {

  switch (btn->id) {

    case ARP_ON_SW:
      if (!released) {
        Arpeggiator_Switch = !Arpeggiator_Switch;
        myControlChange(midiChannel, CCArpeggiator_Switch, Arpeggiator_Switch);
      }
      break;

    case ARP_HOLD_SW:
      if (!released) {
        ARP_Hold = !ARP_Hold;
        myControlChange(midiChannel, CCARP_Hold, Arpeggiator_Switch);
      }
      break;

    case ARP_OCTAVE_SW:
      if (!released) {
        ARP_Octave_Plus = !ARP_Octave_Plus;
        myControlChange(midiChannel, CCARP_Octave_Plus, ARP_Octave_Plus);
      }
      break;

    case ARP_EXT_SW:
      if (!released) {
        ARP_Ext_Sync = !ARP_Ext_Sync;
        myControlChange(midiChannel, CCARP_Ext_Sync, ARP_Ext_Sync);
      }
      break;

    case ARP_BOUNCE_SW:
      if (!released) {
        ARP_Mode = ARP_Mode + 1;
        if (ARP_Mode > 2) {
          ARP_Mode = 0;
        }
        myControlChange(midiChannel, CCARP_Mode, ARP_Mode);
      }
      break;

    case ARP_NOTE_LENGTH_SW:
      if (!released) {
        ARP_Note_Length = ARP_Note_Length + 1;
        if (ARP_Note_Length > 5) {
          ARP_Note_Length = 0;
        }
        myControlChange(midiChannel, CCARP_Note_Length, ARP_Note_Length);
      }
      break;

    case VCF_TYPE_SW:
      if (!released) {
        Filter_Type = Filter_Type + 1;
        if (Filter_Type > 2) {
          Filter_Type = 0;
        }
        myControlChange(midiChannel, CCFilter_Type, Filter_Type);
      }
      break;

    case KEYTRACK_SW:
      if (!released) {
        KeyTrack_Switch = !KeyTrack_Switch;
        myControlChange(midiChannel, CCKeyTrack_Switch, KeyTrack_Switch);
      }
      break;

    case LEGATO_SW:
      if (!released) {
        Legato = !Legato;
        myControlChange(midiChannel, CCLegato, Legato);
      }
      break;

    case DELAY_SW:
      if (!released) {
        DelayFX = !DelayFX;
        myControlChange(midiChannel, CCDelayFX, DelayFX);
      }
      break;

    case PHASER_SW:
      if (!released) {
        Phaser_Switch = !Phaser_Switch;
        myControlChange(midiChannel, CCPhaser_Switch, Phaser_Switch);
      }
      break;

    case MODULATION_WAVE_SW:
      if (!released) {
        Filter_LFO_Wave = Filter_LFO_Wave + 1;
        if (Filter_LFO_Wave > 5) {
          Filter_LFO_Wave = 0;
        }
        myControlChange(midiChannel, CCFilter_LFO_Wave_SW, Filter_LFO_Wave);
      }
      break;

    case MW1_SEL_SW:
      if (!released) {
        Wheel_Mod_1_Select = Wheel_Mod_1_Select + 1;
        if (Wheel_Mod_1_Select > 12) {
          Wheel_Mod_1_Select = 0;
        }
        myControlChange(midiChannel, CCWheel_Mod_1_Select, Wheel_Mod_1_Select);
      }
      break;

    case MW2_SEL_SW:
      if (!released) {
        Wheel_Mod_2_Select = Wheel_Mod_2_Select + 1;
        if (Wheel_Mod_2_Select > 12) {
          Wheel_Mod_2_Select = 0;
        }
        myControlChange(midiChannel, CCWheel_Mod_2_Select, Wheel_Mod_2_Select);
      }
      break;

    case MW3_SEL_SW:
      if (!released) {
        Wheel_Mod_3_Select = Wheel_Mod_3_Select + 1;
        if (Wheel_Mod_3_Select > 12) {
          Wheel_Mod_3_Select = 0;
        }
        myControlChange(midiChannel, CCWheel_Mod_3_Select, Wheel_Mod_3_Select);
      }
      break; 

    case VIB_WAVE_SEL_SW:
      if (!released) {
        Vibr_Amp_LFO_Wave = Vibr_Amp_LFO_Wave + 1;
        if (Vibr_Amp_LFO_Wave > 5) {
          Vibr_Amp_LFO_Wave = 0;
        }
        myControlChange(midiChannel, CCVibr_Amp_LFO_Wave, Vibr_Amp_LFO_Wave);
      }
      break;
  }
}

int mod(int a, int b) {
  int r = a % b;
  return r < 0 ? r + b : r;
}


void myNoteOn(byte channel, byte note, byte velocity) {
  MIDI.sendNoteOn(note, velocity, midiOutCh);
}

void myNoteOff(byte channel, byte note, byte velocity) {
  MIDI.sendNoteOff(note, velocity, midiOutCh);
}

void DinHandlePitchBend(byte channel, int pitch) {
  MIDI.sendPitchBend(pitch, midiOutCh);
}

void allNotesOff() {
  midiCCOut(CCallnotesoff, 127);
}

void updateAuto_Pitch_Decay(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Auto Pitch Decay", Auto_Pitch_Decay);
    startParameterDisplay();
  }

  midiCCOut(CCAuto_Pitch_Decay, Auto_Pitch_Decay);
}


void updateFilter_Cutoff_Freq(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter Cutoff", Filter_Cutoff_Freq);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_Cutoff_Freq, Filter_Cutoff_Freq);
}

void updateDuty_Cycle_Waveshape(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("OSC PW", Duty_Cycle_Waveshape);
    startParameterDisplay();
  }

  midiCCOut(CCDuty_Cycle_Waveshape, Duty_Cycle_Waveshape);
}

void updateDelay_Time(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Delay Time", Delay_Time);
    startParameterDisplay();
  }

  midiCCOut(CCDelay_Time, Delay_Time);
}

void updateVolume(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Volume", Volume);
    startParameterDisplay();
  }

  midiCCOut(CCVolume, Volume);
}

void updatePhaser_Wet_Mix(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Phaser Wet Mix", Phaser_Wet_Mix);
    startParameterDisplay();
  }

  midiCCOut(CCPhaser_Wet_Mix, Phaser_Wet_Mix);
}

void updateDelay_Wet_Mix(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Delay Wet Mix", Delay_Wet_Mix);
    startParameterDisplay();
  }

  midiCCOut(CCDelay_Wet_Mix, Delay_Wet_Mix);
}

void updateDelay_Feedback(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Delay Feedback", Delay_Feedback);
    startParameterDisplay();
  }

  midiCCOut(CCDelay_Feedback, Delay_Feedback);
}

void updatePhaser_Feedback(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Phaser Feedback", Phaser_Feedback);
    startParameterDisplay();
  }

  midiCCOut(CCPhaser_Feedback, Phaser_Feedback);
}

void updatePWM_Rate(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("PWM Rate", PWM_Rate);
    startParameterDisplay();
  }

  midiCCOut(CCPWM_Rate, PWM_Rate);
}

void updatePhaser_Rate(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Phaser Rate", Phaser_Rate);
    startParameterDisplay();
  }

  midiCCOut(CCPhaser_Rate, Phaser_Rate);
}

void updateDetune(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Osc Detune", String(Detune));
    startParameterDisplay();
  }
  midiCCOut(CCDetune, Detune);
}

String signedCentred(int v) {          // v is 0..127, 63 = dead centre
  int d = constrain(v - 63, -63, 63);  // 63 -> 0
  return (d > 0) ? "+" + String(d) : String(d);
}

void updateOsc1CourseFreq(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("OSC1 Frequency", signedCentred(Osc1CourseFreq));
    startParameterDisplay();
  }
  midiCCOut(CCOsc1CourseFreq, Osc1CourseFreq);
}

void updateFilter_LFO_Depth(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("VCF LFO Depth", Filter_LFO_Depth);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_LFO_Depth, Filter_LFO_Depth);
}

void updateLFO_Velocity_Depth(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("VCF LFO Vel Depth", LFO_Velocity_Depth);
    startParameterDisplay();
  }

  midiCCOut(CCLFO_Velocity_Depth, LFO_Velocity_Depth);
}

void updateAmp_LFO_Depth(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Amp LFO Depth", Amp_LFO_Depth);
    startParameterDisplay();
  }

  midiCCOut(CCAmp_LFO_Depth, Amp_LFO_Depth);
}

void updateAT_VCO_Depth(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("AT VCO VIB Depth", AT_VCO_Depth);
    startParameterDisplay();
  }

  // to do, control the AT based on this depth
}

void updateAT_VCF_Depth(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("AT VCF TM Depth", AT_VCF_Depth);
    startParameterDisplay();
  }

  // to do, control the AT based on this depth
}

void updatePWM_Depth_LFO(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("PWM LFO Depth", PWM_Depth_LFO);
    startParameterDisplay();
  }

  midiCCOut(CCPWM_Depth_LFO, PWM_Depth_LFO);
}

void updateFilter_LFO_Rate(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter LFO Rate", Filter_LFO_Rate);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_LFO_Rate, Filter_LFO_Rate);
}

void updateAmp_LFO_Rate(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Amp LFO Rate", Amp_LFO_Rate);
    startParameterDisplay();
  }

  midiCCOut(CCAmp_LFO_Rate, Amp_LFO_Rate);
}

void updateKeyTrack_Depth(boolean announce) {


  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Key Track", KeyTrack_Depth);
    startParameterDisplay();
  }

  midiCCOut(CCKeyTrack_Depth, KeyTrack_Depth);
}

void updateARP_Tempo(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("ARP Tempo", ARP_Tempo);
    startParameterDisplay();
  }

  midiCCOut(CCARP_Tempo, ARP_Tempo);
}

void updateOSC_wave(boolean announce) {
  static byte lastOSC_wave = 255;  // 255 = impossible, so first call always fires

  if (OSC_wave > 14) return;             // guard: never index the arrays out of bounds
  if (OSC_wave == lastOSC_wave) return;  // unchanged since last time -> do nothing
  lastOSC_wave = OSC_wave;               // remember it for next time

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Oscillator Wave ", waveNames[OSC_wave]);
    startParameterDisplay();
  }

  midiCCOut(CCOSC_wave, waveCC[OSC_wave]);
  writeOLED(0, screenTitle[0], waveNames[OSC_wave]);
}

void updateFilter_LFO_Wave(boolean announce) {
  static byte lastFilter_LFO_Wave = 255;  // 255 = impossible, so first call always fires

  if (Filter_LFO_Wave > 5) return;                     // guard: never index the arrays out of bounds
  if (Filter_LFO_Wave == lastFilter_LFO_Wave) return;  // unchanged since last time -> do nothing
  lastFilter_LFO_Wave = Filter_LFO_Wave;               // remember it for next time

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter LFO Wave ", filterwaveNames[Filter_LFO_Wave]);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_LFO_Wave, filterwaveCC[Filter_LFO_Wave]);
  writeOLED(6, screenTitle[6], filterwaveNames[Filter_LFO_Wave]);
}

void updateDelay_Expander_Stereo(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Stereo Expand", Delay_Expander_Stereo);
    startParameterDisplay();
  }

  midiCCOut(CCDelay_Expander_Stereo, Delay_Expander_Stereo);
}

void updateVibrato_Rate(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Vibrato Rate", Vibrato_Rate);
    startParameterDisplay();
  }

  midiCCOut(CCVibrato_Rate, Vibrato_Rate);
}


void updateAuto_Pitch_Attack(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Auto Pitch Attack", Auto_Pitch_Attack);
    startParameterDisplay();
  }

  midiCCOut(CCAuto_Pitch_Attack, Auto_Pitch_Attack);
}

void updatePortamento(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Portamento", Portamento);
    startParameterDisplay();
  }

  midiCCOut(CCPortamento, Portamento);
}

void updateFilter_Decay(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter Decay", Filter_Decay);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_Decay, Filter_Decay);
}

void updateFilter_Sustain(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter Sustain", Filter_Sustain);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_Sustain, Filter_Sustain);
}

void updateFilter_Release(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter Release", Filter_Release);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_Release, Filter_Release);
}

void updateFilter_Attack(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter Attack", Filter_Attack);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_Attack, Filter_Attack);
}

void updateFilter_Resonance(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter Resonance", Filter_Resonance);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_Resonance, Filter_Resonance);
}

void updateAmp_Velocity_Depth(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Amp Velocity Depth", Amp_Velocity_Depth);
    startParameterDisplay();
  }

  midiCCOut(CCAmp_Velocity_Depth, Amp_Velocity_Depth);
}

String autoSignedCentred(int v) {        // v is 0..127, 64 = dead centre
  int d = 0;
  if (v == 64) {
    d = 0;
  }
  if (v > 64) {
    d = map(v, 65, 127, 1, 48);
  }
  if (v < 64) {
    d = map(v, 0, 63, -48, -1);
  }
  return (d > 0) ? "+" + String(d) : String(d);
}

void updateAuto_Pitch_Depth(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Auto Pitch Depth", autoSignedCentred(Auto_Pitch_Depth));
    startParameterDisplay();
  }

  midiCCOut(CCAuto_Pitch_Depth, Auto_Pitch_Depth);
}

void updateAmp_Attack(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Amp Attack", Amp_Attack);
    startParameterDisplay();
  }

  midiCCOut(CCAmp_Attack, Amp_Attack);
}

void updateAmp_Decay(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Amp Decay", Amp_Decay);
    startParameterDisplay();
  }

  midiCCOut(CCAmp_Decay, Amp_Decay);
}

void updateAuto_Pitch_Sustain(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Auto Pitch Sustain", Auto_Pitch_Sustain);
    startParameterDisplay();
  }

  midiCCOut(CCAuto_Pitch_Sustain, Auto_Pitch_Sustain);
}

void updateFilter_Velocity_Depth(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter Velocity Depth", Filter_Velocity_Depth);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_Velocity_Depth, Filter_Velocity_Depth);
}

void updateAmp_Sustain(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Amp Sustain", Amp_Sustain);
    startParameterDisplay();
  }

  midiCCOut(CCAmp_Sustain, Amp_Sustain);
}

void updateKeyTrack_Exponent(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("KeyTrack Exponent", KeyTrack_Exponent);
    startParameterDisplay();
  }

  midiCCOut(CCKeyTrack_Exponent, KeyTrack_Exponent);
}

void updateAmp_Release(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Amp Release", Amp_Release);
    startParameterDisplay();
  }

  midiCCOut(CCAmp_Release, Amp_Release);
}

void updateFilter_EG_Depth(boolean announce) {
  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Filter EG Depth", Filter_EG_Depth);
    startParameterDisplay();
  }

  midiCCOut(CCFilter_EG_Depth, Filter_EG_Depth);
}

void updateOsc2CourseFreq(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("OSC2 Frequency", signedCentred(Osc2CourseFreq));
    startParameterDisplay();
  }

  midiCCOut(CCOsc2CourseFreq, Osc2CourseFreq);
}

// // ////////////////////////////////////////////////////////////////


void updateArpeggiator_Switch(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    if (!Arpeggiator_Switch) {
      showCurrentParameterPage("Arpeggiator", "Off");
    } else {
      showCurrentParameterPage("Arpeggiator", "On");
    }
    startParameterDisplay();
  }
  switch (Arpeggiator_Switch) {
    case 0:
      midiCCOut(CCArpeggiator_Switch, 0);
      mcp1.digitalWrite(ARP_ON_LED, LOW);
      break;

    case 1:
      midiCCOut(CCArpeggiator_Switch, 127);
      mcp1.digitalWrite(ARP_ON_LED, HIGH);
      break;
  }
}

void updateARP_Hold(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    if (!ARP_Hold) {
      showCurrentParameterPage("Arp Hold", "Off");
    } else {
      showCurrentParameterPage("Arp Hold", "On");
    }
    startParameterDisplay();
  }
  switch (ARP_Hold) {
    case 0:
      midiCCOut(CCARP_Hold, 0);
      mcp1.digitalWrite(ARP_HOLD_LED, LOW);
      break;

    case 1:
      midiCCOut(CCARP_Hold, 127);
      mcp1.digitalWrite(ARP_HOLD_LED, HIGH);
      break;
  }
}

void updateARP_Octave_Plus(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    if (!ARP_Octave_Plus) {
      showCurrentParameterPage("Arp Octave Plus", "Off");
    } else {
      showCurrentParameterPage("Arp Octave Plus", "On");
    }
    startParameterDisplay();
  }
  switch (ARP_Octave_Plus) {
    case 0:
      midiCCOut(CCARP_Octave_Plus, 0);
      mcp1.digitalWrite(ARP_OCTAVE_LED, LOW);
      break;

    case 1:
      midiCCOut(CCARP_Octave_Plus, 127);
      mcp1.digitalWrite(ARP_OCTAVE_LED, HIGH);
      break;
  }
}

void updateARP_Ext_Sync(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    if (!ARP_Ext_Sync) {
      showCurrentParameterPage("Arp Ext Sync", "Off");
    } else {
      showCurrentParameterPage("Arp Ext Sync", "On");
    }
    startParameterDisplay();
  }
  switch (ARP_Ext_Sync) {
    case 0:
      midiCCOut(CCARP_Ext_Sync, 0);
      mcp1.digitalWrite(ARP_EXT_LED, LOW);
      break;

    case 1:
      midiCCOut(CCARP_Ext_Sync, 127);
      mcp1.digitalWrite(ARP_EXT_LED, HIGH);
      break;
  }
}

void updateARP_Mode(boolean announce) {

    switch (ARP_Mode) {
      case 0:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Arp Mode", "UP");
          startParameterDisplay();
        }
        midiCCOut(CCARP_Mode, 0);
        mcp1.digitalWrite(ARP_BOUNCE_LED_RED, HIGH);
        mcp1.digitalWrite(ARP_BOUNCE_LED_GREEN, LOW);
        break;

      case 1:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Arp Mode", "DOWN");
          startParameterDisplay();
        }
        midiCCOut(CCARP_Mode, 1);
        mcp1.digitalWrite(ARP_BOUNCE_LED_RED, LOW);
        mcp1.digitalWrite(ARP_BOUNCE_LED_GREEN, HIGH);
        break;

      case 2:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Arp Mode", "BOUNCE");
          startParameterDisplay();
        }
        midiCCOut(CCARP_Mode, 2);
        mcp1.digitalWrite(ARP_BOUNCE_LED_RED, HIGH);
        mcp1.digitalWrite(ARP_BOUNCE_LED_GREEN, HIGH);
        break;
    }
}

void updateARP_Note_Length(boolean announce) {

    switch (ARP_Note_Length) {
      case 0:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Arp Note Length", "WHOLE");
          startParameterDisplay();
        }
        midiCCOut(CCARP_Note_Length, 12);
        writeOLED(1, screenTitle[1], "WHOLE");
        break;

      case 1:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Arp Note Length", "HALF");
          startParameterDisplay();
        }
        midiCCOut(CCARP_Note_Length, 32);
        writeOLED(1, screenTitle[1], "HALF");
        break;

      case 2:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Arp Note Length", "QUARTER");
          startParameterDisplay();
        }
        midiCCOut(CCARP_Note_Length, 52);
        writeOLED(1, screenTitle[1], "QUARTER");
        break;

      case 3:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Arp Note Length", "EIGTH");
          startParameterDisplay();
        }
        midiCCOut(CCARP_Note_Length, 72);
        writeOLED(1, screenTitle[1], "EIGTH");
        break;

      case 4:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Arp Note Length", "SIXTEENTH");
          startParameterDisplay();
        }
        midiCCOut(CCARP_Note_Length, 92);
        writeOLED(1, screenTitle[1], "SIXTEENTH");
        break;

      case 5:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Arp Note Length", "32nd");
          startParameterDisplay();
        }
        midiCCOut(CCARP_Note_Length, 112);
        writeOLED(1, screenTitle[1], "32nd");
        break;
    }
}

void updateFilter_Type(boolean announce) {

    switch (Filter_Type) {
      case 0:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Filter Mode", "Off");
          startParameterDisplay();
        }
        midiCCOut(CCFilter_Type, 0);
        mcp2.digitalWrite(VCF_TYPE_LED_RED, LOW);
        mcp2.digitalWrite(VCF_TYPE_LED_GREEN, LOW);
        break;

      case 1:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Filter Mode", "LoPass");
          startParameterDisplay();
        }
        midiCCOut(CCFilter_Type, 40);
        mcp2.digitalWrite(VCF_TYPE_LED_RED, HIGH);
        mcp2.digitalWrite(VCF_TYPE_LED_GREEN, LOW);
        break;

      case 2:
        if (announce && !suppressParamAnnounce) {
          showCurrentParameterPage("Filter Mode", "HiPass");
          startParameterDisplay();
        }
        midiCCOut(CCFilter_Type, 100);
        mcp2.digitalWrite(VCF_TYPE_LED_RED, LOW);
        mcp2.digitalWrite(VCF_TYPE_LED_GREEN, HIGH);
        break;
    }
}

void updateKeyTrack_Switch(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    if (!KeyTrack_Switch) {
      showCurrentParameterPage("Filter Keytrack", "Off");
    } else {
      showCurrentParameterPage("Filter Keytrack", "On");
    }
    startParameterDisplay();
  }
  switch (KeyTrack_Switch) {
    case 0:
      midiCCOut(CCKeyTrack_Switch, 0);
      mcp2.digitalWrite(KEYTRACK_LED, LOW);
      break;

    case 1:
      midiCCOut(CCKeyTrack_Switch, 127);
      mcp2.digitalWrite(KEYTRACK_LED, HIGH);
      break;
  }
}

void updateLegato(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    if (!Legato) {
      showCurrentParameterPage("Amp Legato", "Off");
    } else {
      showCurrentParameterPage("Amp Legato", "On");
    }
    startParameterDisplay();
  }
  switch (Legato) {
    case 0:
      midiCCOut(CCLegato, 0);
      mcp2.digitalWrite(LEGATO_LED, LOW);
      break;

    case 1:
      midiCCOut(CCLegato, 127);
      mcp2.digitalWrite(LEGATO_LED, HIGH);
      break;
  }
}

void updateDelayFX(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    if (!DelayFX) {
      showCurrentParameterPage("Delay FX", "Off");
    } else {
      showCurrentParameterPage("Delay FX", "On");
    }
    startParameterDisplay();
  }
  switch (DelayFX) {
    case 0:
      midiCCOut(CCDelayFX, 0);
      mcp3.digitalWrite(DELAY_LED, LOW);
      break;

    case 1:
      midiCCOut(CCDelayFX, 127);
      mcp3.digitalWrite(DELAY_LED, HIGH);
      break;
  }
}

void updatePhaser_Switch(boolean announce) {

  if (announce && !suppressParamAnnounce) {
    if (!Phaser_Switch) {
      showCurrentParameterPage("Phaser FX", "Off");
    } else {
      showCurrentParameterPage("Phaser FX", "On");
    }
    startParameterDisplay();
  }
  switch (Phaser_Switch) {
    case 0:
      midiCCOut(CCPhaser_Switch, 0);
      mcp3.digitalWrite(PHASER_LED, LOW);
      break;

    case 1:
      midiCCOut(CCPhaser_Switch, 127);
      mcp3.digitalWrite(PHASER_LED, HIGH);
      break;
  }
}

void updateWheel_Mod_1_Select(boolean announce) {
  static byte lastWheel_Mod_1_Select = 255;  // 255 = impossible, so first call always fires

  if (Wheel_Mod_1_Select > 12) return;                     // guard: never index the arrays out of bounds
  if (Wheel_Mod_1_Select == lastWheel_Mod_1_Select) return;  // unchanged since last time -> do nothing
  lastWheel_Mod_1_Select = Wheel_Mod_1_Select;               // remember it for next time

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Modwheel 1 Select ", MWwaveNames[Wheel_Mod_1_Select]);
    startParameterDisplay();
  }

  midiCCOut(CCWheel_Mod_1_Select, MWwaveCC[Wheel_Mod_1_Select]);
  writeOLED(3, screenTitle[3], MWwaveNames[Wheel_Mod_1_Select]);
}

void updateWheel_Mod_2_Select(boolean announce) {
  static byte lastWheel_Mod_2_Select = 255;  // 255 = impossible, so first call always fires

  if (Wheel_Mod_2_Select > 12) return;                     // guard: never index the arrays out of bounds
  if (Wheel_Mod_2_Select == lastWheel_Mod_2_Select) return;  // unchanged since last time -> do nothing
  lastWheel_Mod_2_Select = Wheel_Mod_2_Select;               // remember it for next time

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Modwheel 2 Select ", MWwaveNames[Wheel_Mod_2_Select]);
    startParameterDisplay();
  }

  midiCCOut(CCWheel_Mod_2_Select, MWwaveCC[Wheel_Mod_2_Select]);
  writeOLED(4, screenTitle[4], MWwaveNames[Wheel_Mod_2_Select]);
}

void updateWheel_Mod_3_Select(boolean announce) {
  static byte lastWheel_Mod_3_Select = 255;  // 255 = impossible, so first call always fires

  if (Wheel_Mod_3_Select > 12) return;                     // guard: never index the arrays out of bounds
  if (Wheel_Mod_3_Select == lastWheel_Mod_3_Select) return;  // unchanged since last time -> do nothing
  lastWheel_Mod_3_Select = Wheel_Mod_3_Select;               // remember it for next time

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Modwheel 3 Select ", MWwaveNames[Wheel_Mod_3_Select]);
    startParameterDisplay();
  }

  midiCCOut(CCWheel_Mod_3_Select, MWwaveCC[Wheel_Mod_3_Select]);
  writeOLED(5, screenTitle[5], MWwaveNames[Wheel_Mod_3_Select]);
}

void updateVibr_Amp_LFO_Wave(boolean announce) {
  static byte lastVibr_Amp_LFO_Wave = 255;  // 255 = impossible, so first call always fires

  if (Vibr_Amp_LFO_Wave > 5) return;                     // guard: never index the arrays out of bounds
  if (Vibr_Amp_LFO_Wave == lastVibr_Amp_LFO_Wave) return;  // unchanged since last time -> do nothing
  lastVibr_Amp_LFO_Wave = Vibr_Amp_LFO_Wave;               // remember it for next time

  if (announce && !suppressParamAnnounce) {
    showCurrentParameterPage("Vibrato LFO Wave ", filterwaveNames[Vibr_Amp_LFO_Wave]);
    startParameterDisplay();
  }

  midiCCOut(CCVibr_Amp_LFO_Wave, filterwaveCC[Vibr_Amp_LFO_Wave]);
  writeOLED(2, screenTitle[2], filterwaveNames[Vibr_Amp_LFO_Wave]);
}

void updatePatchname() {
  showPatchPage(String(patchNo), patchName);
}

void myControlChange(byte channel, byte control, byte value) {

  switch (control) {

    case CCsustain:
      MIDI.sendControlChange(control, value, midiOutCh);
      break;

    case CCmodwheel:
      MIDI.sendControlChange(control, value, midiOutCh);
      break;

    case CCFilter_Attack:
      Filter_Attack = value;
      updateFilter_Attack(1);
      break;

    case CCFilter_Resonance:
      Filter_Resonance = value;
      updateFilter_Resonance(1);
      break;

    case CCAmp_Velocity_Depth:
      Amp_Velocity_Depth = value;
      updateAmp_Velocity_Depth(1);
      break;

    case CCAuto_Pitch_Decay:
      Auto_Pitch_Decay = value;
      updateAuto_Pitch_Decay(1);
      break;

    case CCDelay_Expander_Stereo:
      Delay_Expander_Stereo = value;  // for display
      updateDelay_Expander_Stereo(1);
      break;

    case CCVibrato_Rate:
      Vibrato_Rate = value;  // for display
      updateVibrato_Rate(1);
      break;

    case CCAuto_Pitch_Attack:
      Auto_Pitch_Attack = value;  // for display
      updateAuto_Pitch_Attack(1);
      break;

    case CCFilter_Cutoff_Freq:
      Filter_Cutoff_Freq = value;
      updateFilter_Cutoff_Freq(1);
      break;

    case CCDuty_Cycle_Waveshape:
      Duty_Cycle_Waveshape = value;
      updateDuty_Cycle_Waveshape(1);
      break;

    case CCDelay_Time:
      Delay_Time = value;
      updateDelay_Time(1);
      break;

    case CCDelay_Wet_Mix:
      Delay_Wet_Mix = value;
      updateDelay_Wet_Mix(1);
      break;

    case CCVolume:
      Volume = value;
      updateVolume(1);
      break;

    case CCPhaser_Wet_Mix:
      Phaser_Wet_Mix = value;
      updatePhaser_Wet_Mix(1);
      break;

    case CCDelay_Feedback:
      Delay_Feedback = value;
      updateDelay_Feedback(1);
      break;

    case CCPhaser_Feedback:
      Phaser_Feedback = value;  // for display
      updatePhaser_Feedback(1);
      break;

    case CCPWM_Rate:
      PWM_Rate = value;  // for display
      updatePWM_Rate(1);
      break;

    case CCPhaser_Rate:
      Phaser_Rate = value;  // for display
      updatePhaser_Rate(1);
      break;

    case CCAT_Filter_Depth:
      AT_VCF_Depth = value;
      updateAT_VCF_Depth(1);
      break;

    case CCPWM_Depth_LFO:
      PWM_Depth_LFO = value;
      updatePWM_Depth_LFO(1);
      break;

    case CCFilter_LFO_Rate:
      Filter_LFO_Rate = value;
      updateFilter_LFO_Rate(1);
      break;

    case CCAmp_LFO_Rate:
      Amp_LFO_Rate = value;
      updateAmp_LFO_Rate(1);
      break;

    case CCDetune:
      Detune = value;
      updateDetune(1);
      break;

    case CCOsc1CourseFreq:
      Osc1CourseFreq = value;  // for display
      updateOsc1CourseFreq(1);
      break;

    case CCFilter_LFO_Depth:
      Filter_LFO_Depth = value;  // for display
      updateFilter_LFO_Depth(1);
      break;

    case CCLFO_Velocity_Depth:
      LFO_Velocity_Depth = value;  // for display
      updateLFO_Velocity_Depth(1);
      break;

    case CCAmp_LFO_Depth:
      Amp_LFO_Depth = value;  // for display
      updateAmp_LFO_Depth(1);
      break;

    case CCAT_VCO_Depth:
      AT_VCO_Depth = value;
      updateAT_VCO_Depth(1);
      break;

    case CCKeyTrack_Depth:
      KeyTrack_Depth = value;
      updateKeyTrack_Depth(1);
      break;

    case CCARP_Tempo:
      ARP_Tempo = value;  // for display
      updateARP_Tempo(1);
      break;

    case CCOSC_wave:
      OSC_wave = map(value, 0, 127, 0, 14);  // for display
      updateOSC_wave(1);
      break;

    case CCFilter_LFO_Wave:
      Filter_LFO_Wave = map(value, 0, 127, 0, 5);  // for display
      updateFilter_LFO_Wave(1);
      break;

    case CCFilter_LFO_Wave_SW:
      Filter_LFO_Wave = value;  // for display
      updateFilter_LFO_Wave(1);
      break;

    case CCAuto_Pitch_Depth:
      Auto_Pitch_Depth = value;
      updateAuto_Pitch_Depth(1);
      break;

    case CCAmp_Attack:
      Amp_Attack = value;
      updateAmp_Attack(1);
      break;

    case CCAmp_Decay:
      Amp_Decay = value;
      updateAmp_Decay(1);
      break;

    case CCAuto_Pitch_Sustain:
      Auto_Pitch_Sustain = value;
      updateAuto_Pitch_Sustain(1);
      break;

    case CCFilter_Velocity_Depth:
      Filter_Velocity_Depth = value;
      updateFilter_Velocity_Depth(1);
      break;

    case CCAmp_Sustain:
      Amp_Sustain = value;
      updateAmp_Sustain(1);
      break;

    case CCKeyTrack_Exponent:
      KeyTrack_Exponent = value;
      updateKeyTrack_Exponent(1);
      break;

    case CCAmp_Release:
      Amp_Release = value;
      updateAmp_Release(true);
      break;

    case CCFilter_EG_Depth:
      Filter_EG_Depth = value;
      updateFilter_EG_Depth(1);
      break;

    case CCOsc2CourseFreq:

      Osc2CourseFreq = value;
      updateOsc2CourseFreq(1);
      break;

    case CCFilter_Decay:
      Filter_Decay = value;
      updateFilter_Decay(1);
      break;

    case CCFilter_Sustain:
      Filter_Sustain = value;
      updateFilter_Sustain(1);
      break;

    case CCFilter_Release:
      Filter_Release = value;
      updateFilter_Release(1);
      break;

    case CCPortamento:
      Portamento = value;
      updatePortamento(1);
      break;

  // Buttons ////////////////////////////////////////////////


    case CCArpeggiator_Switch:
      updateArpeggiator_Switch(1);
      break;

    case CCARP_Hold:
      updateARP_Hold(1);
      break;

    case CCARP_Octave_Plus:
      updateARP_Octave_Plus(1);
      break;

    case CCARP_Ext_Sync:
      updateARP_Ext_Sync(1);
      break;

    case CCARP_Mode:
      updateARP_Mode(1);
      break;

    case CCARP_Note_Length:
      updateARP_Note_Length(1);
      break;

    case CCFilter_Type:
      updateFilter_Type(1);
      break;

    case CCKeyTrack_Switch:
      updateKeyTrack_Switch(1);
      break;

    case CCLegato:
      updateLegato(1);
      break;

    case CCDelayFX:
      updateDelayFX(1);
      break;

    case CCPhaser_Switch:
      updatePhaser_Switch(1);
      break;

    case CCWheel_Mod_1_Select:
      updateWheel_Mod_1_Select(1);
      break;

    case CCWheel_Mod_2_Select:
      updateWheel_Mod_2_Select(1);
      break;

    case CCWheel_Mod_3_Select:
      updateWheel_Mod_3_Select(1);
      break;

    case CCVibr_Amp_LFO_Wave:
      updateVibr_Amp_LFO_Wave(1);
      break;


    case CCallnotesoff:
      allNotesOff();
      break;
  }
}


void myProgramChange(byte channel, byte program) {
  state = PATCH;
  patchNo = program + 1;
  recallPatch(patchNo);
  //Serial.print("MIDI Pgm Change:");
  //Serial.println(patchNo);
  state = PARAMETER;
}


void myAfterTouch(byte channel, byte value) {

  // uint8_t afterTouchU = (value * upperData[P_ATDepth] + 5) / 10;
  // uint8_t afterTouchL = (value * lowerData[P_ATDepth] + 5) / 10;

  // switch (upperData[P_AfterTouchDest]) {
  //   case 1:
  //     if (!wholemode) {
  //       midiCCOutUpper(CCmodwheel, afterTouchU);
  //     }
  //     break;
  //   case 2:
  //     if (!wholemode) {
  //       midiCCOutUpper(CCvcfLfoDepth, afterTouchU);
  //     }
  //     break;
  // }
  // switch (lowerData[P_AfterTouchDest]) {
  //   case 1:
  //     midiCCOutLower(CCmodwheel, afterTouchL);
  //     if (wholemode) {
  //       midiCCOutUpper(CCmodwheel, afterTouchL);
  //     }
  //     break;
  //   case 2:
  //     midiCCOutLower(CCvcfLfoDepth, afterTouchL);
  //     if (wholemode) {
  //       midiCCOutUpper(CCvcfLfoDepth, afterTouchL);
  //     }
  //     break;
  // }
}

void recallPatch(int patchNo) {
  allNotesOff();

  if (!updateParams) {
    MIDI.sendProgramChange(patchNo - 1, midiOutCh);
  }

  delay(50);  // Let synth catch up
  recallPatchFlag = true;

  // Format filename without zero-padding
  char filename[16];
  snprintf(filename, sizeof(filename), "/%d", patchNo);  // e.g., "/1", "/2"

  //Serial.print("Loading patch file: ");
  //Serial.println(filename);

  File patchFile = SD.open(filename);
  if (!patchFile) {
    //Serial.print("Patch file not found: ");
    //Serial.println(filename);
  } else {
    String data[NO_OF_PARAMS];
    recallPatchData(patchFile, data);
    setCurrentPatchData(data);
    patchFile.close();
    //Serial.println("Patch data loaded successfully.");
  }

  recallPatchFlag = false;
}

void setCurrentPatchData(String data[]) {
  patchName = data[0];
  Detune = data[1].toInt();
  Osc1CourseFreq = data[2].toInt();
  ARP_Tempo = data[3].toInt();
  OSC_wave = data[4].toInt();
  Portamento = data[5].toInt();
  Filter_EG_Depth = data[6].toInt();
  Osc2CourseFreq = data[7].toInt();
  KeyTrack_Depth = data[8].toInt();
  Filter_Decay = data[9].toInt();
  Filter_Sustain = data[10].toInt();
  Filter_Release = data[11].toInt();
  Filter_Attack = data[12].toInt();
  Filter_Resonance = data[13].toInt();
  Amp_Velocity_Depth = data[14].toInt();
  Filter_Cutoff_Freq = data[15].toInt();
  Volume = data[16].toInt();
  Auto_Pitch_Depth = data[17].toInt();
  Amp_Attack = data[18].toInt();
  Amp_Decay = data[19].toInt();
  Auto_Pitch_Sustain = data[20].toInt();
  Filter_Velocity_Depth = data[21].toInt();
  Amp_Sustain = data[22].toInt();
  KeyTrack_Exponent = data[23].toInt();
  Amp_Release = data[24].toInt();
  AT_VCO_Depth = data[25].toInt();
  Delay_Expander_Stereo = data[26].toInt();
  Vibrato_Rate = data[27].toInt();
  Auto_Pitch_Attack = data[28].toInt();
  Auto_Pitch_Decay = data[29].toInt();
  Phaser_Wet_Mix = data[30].toInt();
  Delay_Time = data[31].toInt();
  Delay_Feedback = data[32].toInt();
  Phaser_Feedback = data[33].toInt();
  Delay_Wet_Mix = data[34].toInt();
  Phaser_Rate = data[35].toInt();
  Duty_Cycle_Waveshape = data[36].toInt();
  PWM_Rate = data[37].toInt();
  AT_VCF_Depth = data[38].toInt();
  Filter_LFO_Depth = data[39].toInt();
  LFO_Velocity_Depth = data[40].toInt();
  Amp_LFO_Depth = data[41].toInt();
  PWM_Depth_LFO = data[42].toInt();
  Filter_LFO_Wave = data[43].toInt();
  Filter_LFO_Rate = data[44].toInt();
  Amp_LFO_Rate = data[45].toInt();
  Arpeggiator_Switch = data[46].toInt();
  ARP_Hold =  data[47].toInt();
  ARP_Octave_Plus =  data[48].toInt();
  ARP_Ext_Sync = data[49].toInt();
  ARP_Mode =  data[50].toInt();
  ARP_Note_Length =  data[51].toInt();
  Filter_Type =  data[52].toInt();
  KeyTrack_Switch =  data[53].toInt();
  Legato = data[54].toInt(); 
  DelayFX = data[55].toInt(); 
  Phaser_Switch = data[56].toInt(); 
  Wheel_Mod_1_Select = data[57].toInt(); 
  Wheel_Mod_2_Select = data[58].toInt(); 
  Wheel_Mod_3_Select = data[59].toInt(); 
  Vibr_Amp_LFO_Wave = data[60].toInt(); 

  updatePatchname();

  lowerParamsToDisplay();
  setAllButtons();
}

void lowerParamsToDisplay() {

  updateDetune(0);
  updateAuto_Pitch_Decay(0);
  updateFilter_Attack(0);
  updateFilter_Cutoff_Freq(0);
  updateVolume(0);
  updatePhaser_Wet_Mix(0);
  updateDelay_Feedback(0);
  updateDelay_Time(0);
  updateDuty_Cycle_Waveshape(0);
  updateDelay_Wet_Mix(0);
  updateAuto_Pitch_Depth(0);
  updateAmp_Attack(0);
  updateAmp_Decay(0);
  updateAuto_Pitch_Sustain(0);
  updateFilter_Velocity_Depth(0);
  updateAmp_Sustain(0);
  updateKeyTrack_Exponent(0);
  updateAmp_Release(0);
  updateAT_VCO_Depth(0);
  updateAT_VCF_Depth(0);
  updatePWM_Depth_LFO(0);
  updateFilter_LFO_Rate(0);
  updateAmp_LFO_Rate(0);
  updateDelay_Expander_Stereo(0);
  updateVibrato_Rate(0);
  updateAuto_Pitch_Attack(0);
  updateFilter_EG_Depth(0);
  updateOsc2CourseFreq(0);
  updateAmp_Velocity_Depth(0);
  updateFilter_Sustain(0);
  updateFilter_Decay(0);
  updateFilter_Release(0);
  updatePortamento(0);
  updateARP_Tempo(0);
  updateOSC_wave(0);
  updateFilter_LFO_Wave(0);
  updatePhaser_Feedback(0);
  updatePWM_Rate(0);
  updatePhaser_Rate(0);
  updateOsc1CourseFreq(0);
  updateFilter_LFO_Depth(0);
  updateLFO_Velocity_Depth(0);
  updateAmp_LFO_Depth(0);
  updateKeyTrack_Depth(0);
  updateFilter_Resonance(0);
}

void setAllButtons() {
  updateArpeggiator_Switch(0);
  updateARP_Hold(0);
  updateARP_Octave_Plus(0);
  updateARP_Ext_Sync(0);
  updateARP_Mode(0);
  updateARP_Note_Length(0);
  updateFilter_Type(0);
  updateKeyTrack_Switch(0);
  updateLegato(0);
  updateDelayFX(0);
  updatePhaser_Switch(0);
  updateWheel_Mod_1_Select(0);
  updateWheel_Mod_2_Select(0);
  updateWheel_Mod_3_Select(0);
  updateVibr_Amp_LFO_Wave(0);
}

String getCurrentPatchData() {
  return patchName + "," + String(Detune) + "," + String(Osc1CourseFreq) + "," + String(ARP_Tempo) + "," + String(OSC_wave) + "," + String(Portamento) + "," + String(Filter_EG_Depth)
         + "," + String(Osc2CourseFreq) + "," + String(KeyTrack_Depth) + "," + String(Filter_Decay) + "," + String(Filter_Sustain) + "," + String(Filter_Release) + "," + String(Filter_Attack)
         + "," + String(Filter_Resonance) + "," + String(Amp_Velocity_Depth) + "," + String(Filter_Cutoff_Freq) + "," + String(Volume) + "," + String(Auto_Pitch_Depth) + "," + String(Amp_Attack)
         + "," + String(Amp_Decay) + "," + String(Auto_Pitch_Sustain) + "," + String(Filter_Velocity_Depth) + "," + String(Amp_Sustain) + "," + String(KeyTrack_Exponent) + "," + String(Amp_Release)
         + "," + String(AT_VCO_Depth) + "," + String(Delay_Expander_Stereo) + "," + String(Vibrato_Rate) + "," + String(Auto_Pitch_Attack) + "," + String(Auto_Pitch_Decay) + "," + String(Phaser_Wet_Mix)
         + "," + String(Delay_Time) + "," + String(Delay_Feedback) + "," + String(Phaser_Feedback) + "," + String(Delay_Wet_Mix) + "," + String(Phaser_Rate) + "," + String(Duty_Cycle_Waveshape)
         + "," + String(PWM_Rate) + "," + String(AT_VCF_Depth) + "," + String(Filter_LFO_Depth) + "," + String(LFO_Velocity_Depth) + "," + String(Amp_LFO_Depth) + "," + String(PWM_Depth_LFO)
         + "," + String(Filter_LFO_Wave) + "," + String(Filter_LFO_Rate) + "," + String(Amp_LFO_Rate) + "," + String(Arpeggiator_Switch) + "," + String(ARP_Hold) + "," + String(ARP_Octave_Plus)
         + "," + String(ARP_Ext_Sync) + "," + String(ARP_Mode) + "," + String(ARP_Note_Length) + "," + String(Filter_Type) + "," + String(KeyTrack_Switch) + "," + String(Legato)
         + "," + String(DelayFX) + "," + String(Phaser_Switch)+ "," + String(Wheel_Mod_1_Select) + "," + String(Wheel_Mod_2_Select) + "," + String(Wheel_Mod_3_Select) + "," + String(Vibr_Amp_LFO_Wave);
}

void midiCCOut(byte cc, byte value) {
  if (updateParams) {
    MIDI.sendControlChange(cc, value, midiOutCh);  //MIDI DIN main out
    delay(1);
  }
}

void showSettingsPage() {
  showSettingsPage(settings::current_setting(), settings::current_setting_value(), state);
}


void updatereinitialiseToPanel() {
  if (manualMode) {
    showMuxRead = false;
    reinitialiseToPanel();
  }
}

bool anyMuxNeedsReread() {
  for (int i = 0; i < MUXCHANNELS; i++) {
    if (mux1ValuesPrev[i] == RE_READ) return true;
    if (mux2ValuesPrev[i] == RE_READ) return true;
    if (mux3ValuesPrev[i] == RE_READ) return true;
    if (mux4ValuesPrev[i] == RE_READ) return true;
    if (mux5ValuesPrev[i] == RE_READ) return true;
    if (mux6ValuesPrev[i] == RE_READ) return true;
  }
  return false;
}

void reinitialiseToPanel() {

  manualSyncInProgress = true;   // NEW
  suppressParamAnnounce = true;  // ON for the entire re-read pass


  muxInput = 0;

  for (int i = 0; i < MUXCHANNELS; i++) {
    mux1ValuesPrev[i] = RE_READ;
    mux2ValuesPrev[i] = RE_READ;
    mux3ValuesPrev[i] = RE_READ;
    mux4ValuesPrev[i] = RE_READ;
    mux5ValuesPrev[i] = RE_READ;
    mux6ValuesPrev[i] = RE_READ;
  }

  patchName = INITPATCHNAME;
  showPatchPage("Initial", "Panel Settings");
  lowerParamsToDisplay();
  setAllButtons();
}

// ---------- Main input scan ----------
void checkSwitches() {

  saveButton.update();
  if (saveButton.held()) {
    switch (state) {
      case PARAMETER:
      case PATCH:
        state = DELETE;
        break;
    }
    updateScreen();
  } else if (saveButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        if (patches.size() < PATCHES_LIMIT) {
          resetPatchesOrdering();  //Reset order of patches from first patch
          patches.push({ patches.size() + 1, INITPATCHNAME });
          state = SAVE;
        }
        updateScreen();
        break;
      case SAVE:
        //Save as new patch with INITIALPATCH name or overwrite existing keeping name - bypassing patch renaming
        patchName = patches.last().patchName;
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patches.last().patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        updateScreen();
        break;
      case PATCHNAMING:
        if (renamedPatch.length() > 0) patchName = renamedPatch;  //Prevent empty strings
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        updateScreen();
        break;
    }
  }

  settingsButton.update();
  if (settingsButton.held()) {
    //If recall held, set current patch to match current hardware state
    //Reinitialise all hardware values to force them to be re-read if different
    state = REINITIALISE;
    reinitialiseToPanel();
    updateScreen();
  } else if (settingsButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = SETTINGS;
        showSettingsPage();
        updateScreen();
        break;
      case SETTINGS:
        showSettingsPage();
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        updateScreen();
        break;
    }
  }

  backButton.update();
  if (backButton.held()) {
    //If Back button held, Panic - all notes off
  } else if (backButton.numClicks() == 1) {
    switch (state) {
      case RECALL:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case SAVE:
        renamedPatch = "";
        state = PARAMETER;
        loadPatches();  //Remove patch that was to be saved
        setPatchesOrdering(patchNo);
        updateScreen();
        break;
      case PATCHNAMING:
        charIndex = 0;
        renamedPatch = "";
        state = SAVE;
        updateScreen();
        break;
      case DELETE:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case SETTINGS:
        state = PARAMETER;
        updateScreen();
        break;
      case SETTINGSVALUE:
        state = SETTINGS;
        showSettingsPage();
        updateScreen();
        break;
    }
  }

  //Encoder switch
  recallButton.update();
  if (recallButton.held()) {
    //If Recall button held, return to current patch setting
    //which clears any changes made
    state = PATCH;
    //Recall the current patch
    patchNo = patches.first().patchNo;
    recallPatch(patchNo);
    state = PARAMETER;
    updateScreen();
  } else if (recallButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = RECALL;  //show patch list
        updateScreen();
        break;
      case RECALL:
        state = PATCH;
        //Recall the current patch
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case SAVE:
        showRenamingPage(patches.last().patchName);
        patchName = patches.last().patchName;
        state = PATCHNAMING;
        updateScreen();
        break;
      case PATCHNAMING:
        if (renamedPatch.length() < 12)  //actually 12 chars
        {
          renamedPatch.concat(String(currentCharacter));
          charIndex = 0;
          currentCharacter = CHARACTERS[charIndex];
          showRenamingPage(renamedPatch);
        }
        updateScreen();
        break;
      case DELETE:
        //Don't delete final patch
        if (patches.size() > 1) {
          state = DELETEMSG;
          patchNo = patches.first().patchNo;     //PatchNo to delete from SD card
          patches.shift();                       //Remove patch from circular buffer
          deletePatch(String(patchNo).c_str());  //Delete from SD card
          loadPatches();                         //Repopulate circular buffer to start from lowest Patch No
          renumberPatchesOnSD();
          loadPatches();                      //Repopulate circular buffer again after delete
          patchNo = patches.first().patchNo;  //Go back to 1
          recallPatch(patchNo);               //Load first patch
        }
        state = PARAMETER;
        updateScreen();
        break;
      case SETTINGS:
        state = SETTINGSVALUE;
        showSettingsPage();
        updateScreen();
        break;
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        updateScreen();
        break;
    }
  }
}


// ---------- Encoder rotation: ONLY naming/settings/perf naming ----------
void checkEncoder() {
  //Encoder works with relative inc and dec values
  //Detent encoder goes up in 4 steps, hence +/-3

  long encRead = encoder.read();
  if ((encCW && encRead > encPrevious + 3) || (!encCW && encRead < encPrevious - 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.push(patches.shift());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case RECALL:
        patches.push(patches.shift());
        updateScreen();
        break;
      case SAVE:
        patches.push(patches.shift());
        updateScreen();
        break;
      case PATCHNAMING:
        if (charIndex == TOTALCHARS) charIndex = 0;  //Wrap around
        currentCharacter = CHARACTERS[charIndex++];
        showRenamingPage(renamedPatch + currentCharacter);
        updateScreen();
        break;
      case DELETE:
        patches.push(patches.shift());
        updateScreen();
        break;
      case SETTINGS:
        settings::increment_setting();
        showSettingsPage();
        updateScreen();
        break;
      case SETTINGSVALUE:
        settings::increment_setting_value();
        showSettingsPage();
        updateScreen();
        break;
    }
    encPrevious = encRead;
  } else if ((encCW && encRead < encPrevious - 3) || (!encCW && encRead > encPrevious + 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.unshift(patches.pop());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case RECALL:
        patches.unshift(patches.pop());
        updateScreen();
        break;
      case SAVE:
        patches.unshift(patches.pop());
        updateScreen();
        break;
      case PATCHNAMING:
        if (charIndex == -1)
          charIndex = TOTALCHARS - 1;
        currentCharacter = CHARACTERS[charIndex--];
        showRenamingPage(renamedPatch + currentCharacter);
        updateScreen();
        break;
      case DELETE:
        patches.unshift(patches.pop());
        updateScreen();
        break;
      case SETTINGS:
        settings::decrement_setting();
        showSettingsPage();
        updateScreen();
        break;
      case SETTINGSVALUE:
        settings::decrement_setting_value();
        showSettingsPage();
        updateScreen();
        break;
    }
    encPrevious = encRead;
  }
}

String getPatchName(int patchNo) {
  for (int i = 0; i < patches.size(); i++) {
    if (patches[i].patchNo == patchNo) return patches[i].patchName;
  }
  return "-";
}

inline bool isRereadSentinel(int v) {
  return (v == RE_READ);
}

void checkMux() {

  if (bootInitInProgress) {
    muxInput++;
    if (muxInput >= MUXCHANNELS) muxInput = 0;
    return;
  }

  digitalWriteFast(MUX_0, muxInput & B0001);
  digitalWriteFast(MUX_1, muxInput & B0010);
  digitalWriteFast(MUX_2, muxInput & B0100);
  delayMicroseconds(2);

  mux1Read = adc->adc0->analogRead(MUX1_S);
  mux2Read = adc->adc0->analogRead(MUX2_S);
  mux3Read = adc->adc1->analogRead(MUX3_S);
  mux4Read = adc->adc0->analogRead(MUX4_S);
  mux5Read = adc->adc0->analogRead(MUX5_S);
  mux6Read = adc->adc1->analogRead(MUX6_S);

  bool reread1 = isRereadSentinel(mux1ValuesPrev[muxInput]);

  if (reread1 || mux1Read > (mux1ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux1Read < (mux1ValuesPrev[muxInput] - QUANTISE_FACTOR)) {

    mux1ValuesPrev[muxInput] = mux1Read;
    mux1Read = (mux1Read >> resolutionFrig);

    // During RE_READ pass: do not announce UI
    bool prevSuppress = suppressParamAnnounce;
    if (reread1) suppressParamAnnounce = true;

    switch (muxInput) {
      case MUX1_Detune:
        myControlChange(midiChannel, CCDetune, mux1Read);
        break;
      case MUX1_Osc1CourseFreq:
        myControlChange(midiChannel, CCOsc1CourseFreq, mux1Read);
        break;
      case MUX1_ARP_Tempo:
        myControlChange(midiChannel, CCARP_Tempo, mux1Read);
        break;
      case MUX1_OSC_wave:
        myControlChange(midiChannel, CCOSC_wave, mux1Read);
        break;



      case MUX1_Filter_EG_Depth:
        myControlChange(midiChannel, CCFilter_EG_Depth, mux1Read);
        break;
      case MUX1_Osc2CourseFreq:
        myControlChange(midiChannel, CCOsc2CourseFreq, mux1Read);
        break;
      case MUX1_KeyTrack_Depth:
        myControlChange(midiChannel, CCKeyTrack_Depth, mux1Read);
        break;
    }
    suppressParamAnnounce = prevSuppress;
  }

  bool reread2 = isRereadSentinel(mux2ValuesPrev[muxInput]);

  if (reread2 || mux2Read > (mux2ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux2Read < (mux2ValuesPrev[muxInput] - QUANTISE_FACTOR)) {

    mux2ValuesPrev[muxInput] = mux2Read;
    mux2Read = (mux2Read >> resolutionFrig);

    // During RE_READ pass: do not announce UI
    bool prevSuppress = suppressParamAnnounce;
    if (reread2) suppressParamAnnounce = true;

    switch (muxInput) {
      case MUX2_Filter_Decay:
        myControlChange(midiChannel, CCFilter_Decay, mux2Read);
        break;
      case MUX2_Filter_Sustain:
        myControlChange(midiChannel, CCFilter_Sustain, mux2Read);
        break;
      case MUX2_Filter_Release:
        myControlChange(midiChannel, CCFilter_Release, mux2Read);
        break;
      case MUX2_Filter_Attack:
        myControlChange(midiChannel, CCFilter_Attack, mux2Read);
        break;
      case MUX2_Filter_Resonance:
        myControlChange(midiChannel, CCFilter_Resonance, mux2Read);
        break;
      case MUX2_Amp_Velocity_Depth:
        myControlChange(midiChannel, CCAmp_Velocity_Depth, mux2Read);
        break;
      case MUX2_Filter_Cutoff_Freq:
        myControlChange(midiChannel, CCFilter_Cutoff_Freq, mux2Read);
        break;
      case MUX2_Volume:
        myControlChange(midiChannel, CCVolume, mux2Read);
        break;
    }
    suppressParamAnnounce = prevSuppress;
  }

  bool reread3 = isRereadSentinel(mux3ValuesPrev[muxInput]);

  if (reread3 || mux3Read > (mux3ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux3Read < (mux3ValuesPrev[muxInput] - QUANTISE_FACTOR)) {

    mux3ValuesPrev[muxInput] = mux3Read;
    mux3Read = (mux3Read >> resolutionFrig);

    // During RE_READ pass: do not announce UI
    bool prevSuppress = suppressParamAnnounce;
    if (reread3) suppressParamAnnounce = true;

    switch (muxInput) {
      case MUX3_Auto_Pitch_Depth:
        myControlChange(midiChannel, CCAuto_Pitch_Depth, mux3Read);
        break;
      case MUX3_Amp_Attack:
        myControlChange(midiChannel, CCAmp_Attack, mux3Read);
        break;
      case MUX3_Amp_Decay:
        myControlChange(midiChannel, CCAmp_Decay, mux3Read);
        break;
      case MUX3_Auto_Pitch_Sustain:
        myControlChange(midiChannel, CCAuto_Pitch_Sustain, mux3Read);
        break;
      case MUX3_Filter_Velocity_Depth:
        myControlChange(midiChannel, CCFilter_Velocity_Depth, mux3Read);
        break;
      case MUX3_Amp_Sustain:
        myControlChange(midiChannel, CCAmp_Sustain, mux3Read);
        break;
      case MUX3_KeyTrack_Exponent:
        myControlChange(midiChannel, CCKeyTrack_Exponent, mux3Read);
        break;
      case MUX3_Amp_Release:
        myControlChange(midiChannel, CCAmp_Release, mux3Read);
        break;
    }
    suppressParamAnnounce = prevSuppress;
  }

  bool reread4 = isRereadSentinel(mux4ValuesPrev[muxInput]);

  if (reread4 || mux4Read > (mux4ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux4Read < (mux4ValuesPrev[muxInput] - QUANTISE_FACTOR)) {

    mux4ValuesPrev[muxInput] = mux4Read;
    mux4Read = (mux4Read >> resolutionFrig);

    // During RE_READ pass: do not announce UI
    bool prevSuppress = suppressParamAnnounce;
    if (reread4) suppressParamAnnounce = true;

    switch (muxInput) {
      case MUX4_AT_VCO_Depth:
        myControlChange(midiChannel, CCAT_VCO_Depth, mux4Read);
        break;
      case MUX4_Delay_Expander_Stereo:
        myControlChange(midiChannel, CCDelay_Expander_Stereo, mux4Read);
        break;
      case MUX4_Portamento:
        myControlChange(midiChannel, CCPortamento, mux4Read);
        break;
      case MUX4_Vibrato_Rate:
        myControlChange(midiChannel, CCVibrato_Rate, mux4Read);
        break;
      case MUX4_Auto_Pitch_Attack:
        myControlChange(midiChannel, CCAuto_Pitch_Attack, mux4Read);
        break;



      case MUX4_Auto_Pitch_Decay:
        myControlChange(midiChannel, CCAuto_Pitch_Decay, mux4Read);
        break;
    }
    suppressParamAnnounce = prevSuppress;
  }

  bool reread5 = isRereadSentinel(mux5ValuesPrev[muxInput]);

  if (reread5 || mux5Read > (mux5ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux5Read < (mux5ValuesPrev[muxInput] - QUANTISE_FACTOR)) {

    mux5ValuesPrev[muxInput] = mux5Read;
    mux5Read = (mux5Read >> resolutionFrig);

    // During RE_READ pass: do not announce UI
    bool prevSuppress = suppressParamAnnounce;
    if (reread5) suppressParamAnnounce = true;

    switch (muxInput) {
      case MUX5_Phaser_Wet_Mix:
        myControlChange(midiChannel, CCPhaser_Wet_Mix, mux5Read);
        break;
      case MUX5_Delay_Time:
        myControlChange(midiChannel, CCDelay_Time, mux5Read);
        break;
      case MUX5_Delay_Feedback:
        myControlChange(midiChannel, CCDelay_Feedback, mux5Read);
        break;
      case MUX5_Phaser_Feedback:
        myControlChange(midiChannel, CCPhaser_Feedback, mux5Read);
        break;
      case MUX5_Delay_Wet_Mix:
        myControlChange(midiChannel, CCDelay_Wet_Mix, mux5Read);
        break;
      case MUX5_Duty_Cycle_Waveshape:
        myControlChange(midiChannel, CCDuty_Cycle_Waveshape, mux5Read);
        break;
      case MUX5_Phaser_Rate:
        myControlChange(midiChannel, CCPhaser_Rate, mux5Read);
        break;
      case MUX5_PWM_Rate:
        myControlChange(midiChannel, CCPWM_Rate, mux5Read);
        break;
    }
    suppressParamAnnounce = prevSuppress;
  }

  bool reread6 = isRereadSentinel(mux6ValuesPrev[muxInput]);

  if (reread6 || mux6Read > (mux6ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux6Read < (mux6ValuesPrev[muxInput] - QUANTISE_FACTOR)) {

    mux6ValuesPrev[muxInput] = mux6Read;
    mux6Read = (mux6Read >> resolutionFrig);

    // During RE_READ pass: do not announce UI
    bool prevSuppress = suppressParamAnnounce;
    if (reread6) suppressParamAnnounce = true;

    switch (muxInput) {
      case MUX6_Amp_LFO_Rate:
        myControlChange(midiChannel, CCAmp_LFO_Rate, mux6Read);
        break;
      case MUX6_Filter_LFO_Depth:
        myControlChange(midiChannel, CCFilter_LFO_Depth, mux6Read);
        break;
      case MUX6_LFO_Velocity_Depth:
        myControlChange(midiChannel, CCLFO_Velocity_Depth, mux6Read);
        break;
      case MUX6_Amp_LFO_Depth:
        myControlChange(midiChannel, CCAmp_LFO_Depth, mux6Read);
        break;
      case MUX6_PWM_Depth_LFO:
        myControlChange(midiChannel, CCPWM_Depth_LFO, mux6Read);
        break;
      case MUX6_Filter_LFO_Wave:
        myControlChange(midiChannel, CCFilter_LFO_Wave, mux6Read);
        break;
      case MUX6_Filter_LFO_Rate:
        myControlChange(midiChannel, CCFilter_LFO_Rate, mux6Read);
        break;
      case MUX6_AT_Filter_Depth:
        myControlChange(midiChannel, CCAT_Filter_Depth, mux6Read);
        break;
    }
    suppressParamAnnounce = prevSuppress;
  }

  muxInput++;
  if (muxInput >= MUXCHANNELS) {
    muxInput = 0;
  }

  if (manualSyncInProgress && !anyMuxNeedsReread()) {
    manualSyncInProgress = false;
    suppressParamAnnounce = false;

    // Optional: one clean UI update at end
    showPatchPage("--", "Manual");
    startParameterDisplay();
  }
}

void loop() {


  checkMux();
  checkSwitches();
  pollAllMCPs();
  checkEncoder();
  MIDI7.read(1);
  MIDI.read(midiChannel);
  usbMIDI.read(midiChannel);

  if (waitingToUpdate && (millis() - lastDisplayTriggerTime >= displayTimeout)) {
    updateScreen();  // retrigger
    waitingToUpdate = false;
  }
}
