byte midiChannel = 1;  //(EEPROM)
byte midiOutCh = 1;
int resolutionFrig = 1;
static const byte OUT_CH = 1;   // choose your synth receive channel (1–16)
bool cardStatus = false;
bool updateParams = true;  //(EEPROM)
bool recallPatchFlag = false; 

unsigned long lastDisplayTriggerTime = 0;
bool waitingToUpdate = false;
const unsigned long displayTimeout = 2000;  // e.g. 5 seconds

bool manualSyncInProgress = false;
bool suppressParamAnnounce = false;
bool bootInitInProgress = true;


// OTHE STUFF
// -------------------- CONFIG --------------------

bool showMuxRead = true;
bool manualMode = false;


String patchNameU = INITPATCHNAME;
String patchNameL = INITPATCHNAME;
String patchName = INITPATCHNAME;
int upperpatchtag = 0;
int lowerpatchtag = 1;
byte splitPoint = 0;
byte oldsplitPoint = 0;
byte newsplitPoint = 0;
byte splitTrans = 0;
byte oldsplitTrans = 0;
int lowerTranspose = 0;

int noteMsg;
int noteVel;
int lastPlayedNote = -1;  // Track the last note played
int lastPlayedVoice = 0;  // Track the voice of the last note played
int lastUsedVoice = 0;    // Global variable to store the last used voice

const char* const waveNames[15] = {
  "SAWSHAPER", "SAW", "SQUARE", "SINE", "TRIANGLE",
  "MORPHSAW", "PULSE", "NOISE", "3 SAWS", "2 SAWS",
  "BLEPSQUARE", "BLEPSAW", "HSYNC_SAW", "HSYNC_SQR", "WAVESHAPER"
};

const byte waveCC[15] = {
  1, 8, 17, 27, 37, 47, 57, 67, 74, 82, 90, 99, 108, 117, 126
};

const char* const filterwaveNames[6] = {
  "TRIANGLE", "SAW UP", "SAW DOWN", "SQUARE", "RANDOM",
  "SINE"
};

const byte filterwaveCC[6] = {
  0, 22, 43, 64, 90, 115
};

const char* const MWwaveNames[13] = {
  "NIL", "VIBRATO", "TREMELO", "F. CUTOFF", "F. LFO DEP",
  "F. LFO SPD", "VIBR SPEED", "TREM SPEED", "PWM DEPTH", "PWM SPEED", "PHASER SPD",
  "OSC1 FREQ ", "OSC2 FREQ"
};

const byte MWwaveCC[15] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
};


// SYNTH PARAMETERS
// -------------------- SYNTH PARAM CONFIG --------------------

int Detune = 0;
int Osc1CourseFreq = 0;
int ARP_Tempo = 0;
int OSC_wave = 0;
int Portamento = 0;
int Filter_EG_Depth = 0;
int Osc2CourseFreq = 0;
int KeyTrack_Depth = 0;
int Filter_Decay = 0;
int Filter_Sustain = 0;
int Filter_Release = 0;
int Filter_Attack = 0;
int Filter_Resonance = 0;
int Amp_Velocity_Depth = 0;
int Filter_Cutoff_Freq = 0;
int Volume = 0;
int Auto_Pitch_Depth = 0;
int Amp_Attack = 0;
int Amp_Decay = 0;
int Auto_Pitch_Sustain = 0;
int Filter_Velocity_Depth = 0;
int Amp_Sustain = 0;
int KeyTrack_Exponent = 0;
int Amp_Release = 0;
int AT_VCO_Depth = 0;
int Delay_Expander_Stereo = 0;
int Vibrato_Rate = 0;
int Auto_Pitch_Attack = 0;
int Auto_Pitch_Decay = 0;
int Phaser_Wet_Mix = 0;
int Delay_Time = 0;
int Delay_Feedback = 0;
int Phaser_Feedback = 0;
int Delay_Wet_Mix = 0;
int Phaser_Rate = 0;
int Duty_Cycle_Waveshape = 0;
int PWM_Rate = 0;
int PitchBend_Depth = 0;
int Filter_LFO_Depth = 0;
int LFO_Velocity_Depth = 0;
int Amp_LFO_Depth = 0;
int PWM_Depth_LFO = 0;
int Filter_LFO_Wave = 0;
int Filter_LFO_Rate = 0;
int Amp_LFO_Rate = 0;

int Arpeggiator_Switch = 0;
int ARP_Hold = 0;
int ARP_Octave_Plus = 0;
int ARP_Ext_Sync = 0;
int ARP_Mode = 0;
int ARP_Note_Length = 0;
int Filter_Type = 0;
int KeyTrack_Switch = 0;
int Legato = 0;
int DelayFX = 0;
int Phaser_Switch = 0;
int Wheel_Mod_1_Select = 0;
int Wheel_Mod_2_Select = 0;
int Wheel_Mod_3_Select = 0;
int Vibr_Amp_LFO_Wave = 0;
int Portamento_SW = 0;

int lowerSplitVoicePointer = 0;
int upperSplitVoicePointer = 0;
int oldPortamento = 0;
byte pedalMode = 0;                 // 0 = Arp toggle, 1 = Portamento toggle
boolean pedalHeld = false;
boolean sustainedNotes[128] = { false };   // notes the pedal is holding

int value = 0;


bool encCW = true;  //This is to set the encoder to increment when turned CW - Settings Option
bool announce = true;
bool expoResponse = false;

int returnvalue = 0;
