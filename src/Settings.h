#include "SettingsService.h"

void settingsMIDICh();
void settingsMIDIOutCh();
void settingsEncoderDir();
void settingsUpdateParams();
void settingsEnvCurve();

int currentIndexMIDICh();
int currentIndexMIDIOutCh();
int currentIndexEncoderDir();
int currentIndexUpdateParams();
int currentIndexEnvCurve();

void settingsMIDICh(int index, const char *value) {
  if (strcmp(value, "ALL") == 0) {
    midiChannel = MIDI_CHANNEL_OMNI;
  } else {
    midiChannel = atoi(value);
  }
  storeMidiChannel(midiChannel);
}

void settingsMIDIOutCh(int index, const char *value) {
  if (strcmp(value, "Off") == 0) {
    midiOutCh = 0;
  } else {
    midiOutCh = atoi(value);
  }
  storeMidiOutCh(midiOutCh);
}

void settingsUpdateParams(int index, const char *value) {
  if (strcmp(value, "Send MIDI") == 0) {
    updateParams = true;
  } else {
    updateParams = false;
  }
  storeUpdateParams(updateParams ? 1 : 0);
}

void settingsEncoderDir(int index, const char *value) {
  if (strcmp(value, "Type 1") == 0) {
    encCW = true;
  } else {
    encCW = false;
  }
  storeEncoderDir(encCW ? 1 : 0);
}

void settingsPedalMode(int index, const char *value) {
  if (strcmp(value, "Arp") == 0)             pedalMode = 1;
  else if (strcmp(value, "Portamento") == 0) pedalMode = 2;
  else if (strcmp(value, "Hold") == 0)       pedalMode = 3;
  else                                       pedalMode = 0;   // "Off"
  storePedalMode(pedalMode);
}

void settingsEnvCurve(int index, const char *value) {
  expoResponse = (strcmp(value, "Expo") == 0);
  storeEnvCurve(expoResponse ? 1 : 0);
}

int currentIndexMIDICh() {
  return getMIDIChannel();
}

int currentIndexMIDIOutCh() {
  return getMIDIOutCh();
}

int currentIndexEncoderDir() {
  return getEncoderDir() ? 0 : 1;
}

int currentIndexUpdateParams() {
  return getUpdateParams() ? 1 : 0;
}

int currentIndexEnvCurve() {
  return getEnvCurve() ? 1 : 0;
}

int currentIndexPedalMode() {
  return getPedalMode();
}

// add settings to the circular buffer
void setUpSettings() {
  settings::append(settings::SettingsOption{ "MIDI Ch.", { "All", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0" }, settingsMIDICh, currentIndexMIDICh });
  settings::append(settings::SettingsOption{ "MIDI Out Ch.", { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0" }, settingsMIDIOutCh, currentIndexMIDIOutCh });
  settings::append(settings::SettingsOption{ "Encoder", { "Type 1", "Type 2", "\0" }, settingsEncoderDir, currentIndexEncoderDir });
  settings::append(settings::SettingsOption{ "Send MIDI", { "Off", "Send MIDI", "\0" }, settingsUpdateParams, currentIndexUpdateParams });
  settings::append(settings::SettingsOption{ "Env Curve", { "Linear", "Expo", "\0" }, settingsEnvCurve, currentIndexEnvCurve });
  settings::append(settings::SettingsOption{ "Pedal Function", { "Off", "Arp", "Portamento", "Hold" "\0" }, settingsPedalMode, currentIndexPedalMode });
}