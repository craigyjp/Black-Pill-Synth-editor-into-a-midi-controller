

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 32          // 128x32 panels
#define OLED_RESET    -1          // no dedicated reset line
#define TCA_ADDR      0x70
#define NUM_OLED      7           // displays on channels 0..6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// one label per screen — rename to match your panel layout
const char* screenTitle[NUM_OLED] = {
  "OSCILLATOR WAVE", "ARPEGGIATOR NOTE", "VIBRATO WAVE", "MODWHEEL 1 SELECT", "MODWHEEL 2 SELECT", "MODWHEEL 3 SELECT", "FILTER LFO WAVE"
};

// This optional setting causes Encoder to use more optimized code,
// It must be defined before Encoder.h is included.
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Bounce.h>
#include "TButton.h"
#include <ADC.h>
#include <ADC_util.h>

ADC *adc = new ADC();

#include "Rotary.h"
#include "RotaryEncOverMCP.h"

//Mux 1 Connections
#define MUX1_Detune 0
#define MUX1_Osc1CourseFreq 1
#define MUX1_ARP_Tempo 2
#define MUX1_OSC_wave 3
#define MUX1_SPARE_4 4
#define MUX1_Filter_EG_Depth 5
#define MUX1_Osc2CourseFreq 6
#define MUX1_KeyTrack_Depth 7

//Mux 2 Connections
#define MUX2_Filter_Decay 0
#define MUX2_Filter_Sustain 1
#define MUX2_Filter_Release 2
#define MUX2_Filter_Attack 3
#define MUX2_Filter_Resonance 4
#define MUX2_Amp_Velocity_Depth 5
#define MUX2_Filter_Cutoff_Freq 6
#define MUX2_Volume 7

//Mux 3 Connections
#define MUX3_Auto_Pitch_Depth 0
#define MUX3_Amp_Attack 1
#define MUX3_Amp_Decay 2
#define MUX3_Auto_Pitch_Sustain 3
#define MUX3_Filter_Velocity_Depth 4
#define MUX3_Amp_Sustain 5
#define MUX3_KeyTrack_Exponent 6
#define MUX3_Amp_Release 7

#define MUX4_AT_VCO_Depth 0
#define MUX4_Delay_Expander_Stereo 1
#define MUX4_Portamento 2
#define MUX4_Vibrato_Rate 3
#define MUX4_Auto_Pitch_Attack 4
#define MUX4_SPARE_5 5
#define MUX4_Auto_Pitch_Decay 6
#define MUX4_SPARE_7 7

#define MUX5_Phaser_Wet_Mix 0
#define MUX5_Delay_Time 1
#define MUX5_Delay_Feedback 2
#define MUX5_Phaser_Feedback 3
#define MUX5_Delay_Wet_Mix 4
#define MUX5_Duty_Cycle_Waveshape 5
#define MUX5_Phaser_Rate 6
#define MUX5_PWM_Rate 7

#define MUX6_Amp_LFO_Rate 0
#define MUX6_Filter_LFO_Depth 1
#define MUX6_LFO_Velocity_Depth 2
#define MUX6_Amp_LFO_Depth 3
#define MUX6_PWM_Depth_LFO 4
#define MUX6_Filter_LFO_Wave 5
#define MUX6_Filter_LFO_Rate 6
#define MUX6_PitchBend_Depth 7

// Buttons

// Buttons
// GP1
#define ARP_NOTE_LENGTH_SW 0
#define ARP_ON_SW 1
#define ARP_HOLD_SW 2
#define ARP_EXT_SW 3
#define ARP_OCTAVE_SW 4
#define ARP_BOUNCE_SW 5

// GP2
#define VCF_TYPE_SW 6
#define KEYTRACK_SW 7
#define LEGATO_SW 8

// GP3
#define VIB_WAVE_SEL_SW 9
#define MW1_SEL_SW 10
#define MW2_SEL_SW 11
#define MW3_SEL_SW 12
#define DELAY_SW 13
#define PHASER_SW 14
#define MODULATION_WAVE_SW 15

// Pins for MCP23017
#define GPA0 0
#define GPA1 1
#define GPA2 2
#define GPA3 3
#define GPA4 4
#define GPA5 5
#define GPA6 6
#define GPA7 7
#define GPB0 8
#define GPB1 9
#define GPB2 10
#define GPB3 11
#define GPB4 12
#define GPB5 13
#define GPB6 14
#define GPB7 15

void mainButtonChanged(Button *btn, bool released);

Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;
Adafruit_MCP23017 mcp3;

//Array of pointers of all MCPs
Adafruit_MCP23017 *allMCPs[] = { &mcp1, &mcp2, &mcp3 };

/* Array of all rotary encoders and their pins */
RotaryEncOverMCP rotaryEncoders[] = {

};

// after your rotaryEncoders[] definition
constexpr size_t NUM_MCP = sizeof(allMCPs) / sizeof(allMCPs[0]);
constexpr int numMCPs = (int)(sizeof(allMCPs) / sizeof(*allMCPs));
constexpr int numEncoders = (int)(sizeof(rotaryEncoders) / sizeof(*rotaryEncoders));

// an array of vectors to hold pointers to the encoders on each MCP
//std::vector<RotaryEncOverMCP *> encByMCP[NUM_MCP];

Button arp_note_length_Button = Button(&mcp1, 2, ARP_NOTE_LENGTH_SW, &mainButtonChanged);
Button arp_on_Button = Button(&mcp1, 8, ARP_ON_SW, &mainButtonChanged);
Button arp_hold_Button = Button(&mcp1, 9, ARP_HOLD_SW, &mainButtonChanged);
Button arp_ext_Button = Button(&mcp1, 10, ARP_EXT_SW, &mainButtonChanged);
Button arp_octave_Button = Button(&mcp1, 11, ARP_OCTAVE_SW, &mainButtonChanged);
Button arp_bounce_Button = Button(&mcp1, 12, ARP_BOUNCE_SW, &mainButtonChanged);

Button vcf_type_Button = Button(&mcp2, 8, VCF_TYPE_SW, &mainButtonChanged);
Button keytrack_Button = Button(&mcp2, 9, KEYTRACK_SW, &mainButtonChanged);
Button legato_Button = Button(&mcp2, 10, LEGATO_SW, &mainButtonChanged);


Button vib_wave_Button = Button(&mcp3, 6, VIB_WAVE_SEL_SW, &mainButtonChanged);
Button mw1_sel_Button = Button(&mcp3, 5, MW1_SEL_SW, &mainButtonChanged);
Button mw2_sel_Button = Button(&mcp3, 4, MW2_SEL_SW, &mainButtonChanged);
Button mw3_sel_Button = Button(&mcp3, 3, MW3_SEL_SW, &mainButtonChanged);
Button delay_Button = Button(&mcp3, 2, DELAY_SW, &mainButtonChanged);
Button phaser_Button = Button(&mcp3, 1, PHASER_SW, &mainButtonChanged);
Button modulation_wave_Button = Button(&mcp3, 0, MODULATION_WAVE_SW, &mainButtonChanged);


Button *mainButtons[] = {
  &arp_note_length_Button,
  &arp_on_Button,
  &arp_hold_Button,
  &arp_ext_Button,
  &arp_octave_Button,
  &arp_bounce_Button,
  &vcf_type_Button,
  &keytrack_Button,
  &legato_Button,
  &vib_wave_Button,
  &mw1_sel_Button,
  &mw2_sel_Button,
  &mw3_sel_Button,
  &delay_Button,
  &phaser_Button,
  &modulation_wave_Button,
};

Button *allButtons[] = {
  &arp_note_length_Button,
  &arp_on_Button,
  &arp_hold_Button,
  &arp_ext_Button,
  &arp_octave_Button,
  &arp_bounce_Button,
  &vcf_type_Button,
  &keytrack_Button,
  &legato_Button,
  &vib_wave_Button,
  &mw1_sel_Button,
  &mw2_sel_Button,
  &mw3_sel_Button,
  &delay_Button,
  &phaser_Button,
  &modulation_wave_Button,
};

// LEDS

//GP1
#define ARP_BOUNCE_LED_RED 0
#define ARP_BOUNCE_LED_GREEN 1
#define ARP_OCTAVE_LED 7
#define ARP_ON_LED 13
#define ARP_HOLD_LED 14
#define ARP_EXT_LED 15

//GP2
#define KEYTRACK_LED 0
#define LEGATO_LED 1
#define VCF_TYPE_LED_GREEN 7
#define VCF_TYPE_LED_RED 15

//GP3

#define DELAY_LED 7
#define PHASER_LED 15


struct LedRef {
  Adafruit_MCP23017* mcp;  // or whatever your MCP class type is
  uint8_t pin;
}; 

// System Switches etc

#define MUX1_S A0  // ADC0
#define MUX2_S A1  // ADC0
#define MUX3_S A6  // ADC1
#define MUX4_S A7  // ADC0
#define MUX5_S A8  // ADC0
#define MUX6_S A9  // ADC1

#define MUX_0 6
#define MUX_1 7
#define MUX_2 8

#define EXTERNAL_SDCARD 9

#define RECALL_SW 32
#define SAVE_SW 30
#define SETTINGS_SW 24
#define BACK_SW 26

#define ENCODER_PINA 5
#define ENCODER_PINB 4


#define DEBOUNCE 30
#define MUXCHANNELS 8
#define QUANTISE_FACTOR 1

#define DEBOUNCE 30

static int mux1ValuesPrev[MUXCHANNELS] = {};
static int mux2ValuesPrev[MUXCHANNELS] = {};
static int mux3ValuesPrev[MUXCHANNELS] = {};
static int mux4ValuesPrev[MUXCHANNELS] = {};
static int mux5ValuesPrev[MUXCHANNELS] = {};
static int mux6ValuesPrev[MUXCHANNELS] = {};

static int mux1Read = 0;
static int mux2Read = 0;
static int mux3Read = 0;
static int mux4Read = 0;
static int mux5Read = 0;
static int mux6Read = 0;

static byte muxInput = 0;

static long encPrevious = 0;

TButton saveButton{ SAVE_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton settingsButton{ SETTINGS_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton backButton{ BACK_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton recallButton{ RECALL_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };  //On encoder

Encoder encoder(ENCODER_PINB, ENCODER_PINA);  //This often needs the pins swapping depending on the encoder

void setupHardware() {

  adc->adc0->setAveraging(32);                                          // set number of averages 0, 4, 8, 16 or 32.
  adc->adc0->setResolution(8);                                          // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);  // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);           // change the sampling speed

  //MUXs on ADC1
  adc->adc1->setAveraging(32);                                          // set number of averages 0, 4, 8, 16 or 32.
  adc->adc1->setResolution(8);                                          // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);  // change the conversion speed
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);           // change the sampling speed

  //Mux address pins

  pinMode(MUX_0, OUTPUT);
  pinMode(MUX_1, OUTPUT);
  pinMode(MUX_2, OUTPUT);
  //pinMode(EXTERNAL_SDCARD, OUTPUT);

  digitalWrite(MUX_0, LOW);
  digitalWrite(MUX_1, LOW);
  digitalWrite(MUX_2, LOW);

  //Mux ADC
  pinMode(MUX1_S, INPUT_DISABLE);
  pinMode(MUX2_S, INPUT_DISABLE);
  pinMode(MUX3_S, INPUT_DISABLE);
  pinMode(MUX4_S, INPUT_DISABLE);
  pinMode(MUX5_S, INPUT_DISABLE);
  pinMode(MUX6_S, INPUT_DISABLE);

  //Switches

  pinMode(RECALL_SW, INPUT_PULLUP);  //On encoder
  pinMode(SAVE_SW, INPUT_PULLUP);
  pinMode(SETTINGS_SW, INPUT_PULLUP);
  pinMode(BACK_SW, INPUT_PULLUP);
  
}

void setupMCPOutputs() {

  mcp1.pinMode(0, OUTPUT);   // pin 1 = GPA7 of MCP2301X
  mcp1.pinMode(1, OUTPUT);   // pin 1 = GPA7 of MCP2301X
  mcp1.pinMode(7, OUTPUT);   // pin 7 = GPB1 of MCP2301X
  mcp1.pinMode(13, OUTPUT);  // pin 13 = GPB5 of MCP2301X
  mcp1.pinMode(14, OUTPUT);  // pin 14 = GPB6 of MCP2301X
  mcp1.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp2.pinMode(0, OUTPUT);   // pin 1 = GPA1 of MCP2301X
  mcp2.pinMode(1, OUTPUT);   // pin 2 = GPA3 of MCP2301X
  mcp2.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp2.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp3.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp3.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

}
