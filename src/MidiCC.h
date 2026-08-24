// BP-Synth
// MIDI Control Changes List
// CC# Name Description (If needed)
// ===============================================================================
// 1 ModWheel The large wheel next to the PitchWheel. Default is Vibrato.
// 2-4 Not Used (further on, if a CC# is not listed, it is Not Used).
#define CCPortamento 5          // Turns on Portamento and adjusts. time.
#define CCOSC_wave 6            // Select Selects the “Voice” or timbre (sometimes multi Osc’d).
#define CCVolume 7              // Adjust Main Output Volume.
#define CCLegato 8              // Switch Turn Legato On/Off.

#define CCOsc1CourseFreq 11     // Oscillator 1 Coarse Frequency Set.
#define CCOsc2CourseFreq 12     // Oscillator 2 Coarse Frequency Set (only used with HardSync).
#define CCDelayFX 13            // Switch Switch Delay On/Off.
#define CCDelay_Time 14         // Length of Delay loop.
#define CCDelay_Feedback 15     // Strength of Repeats/Echo.
#define CCDelay_Wet_Mix 16      // Delay Mix Control.

#define CCPhaser_Switch 26      // Switch Phaser On/Off.
#define CCPhaser_Rate 27        // Adjust Speed of phasing.
#define CCPhaser_Feedback 28    // Increase Feedback/Resonance.
#define CCPhaser_Wet_Mix 29     // Phaser Mix Control.
#define CCAuto_Pitch_Depth 30   // Depth of Influence to Pitch.
#define CCAuto_Pitch_Attack 31  // Attack Rate
#define CCAuto_Pitch_Decay 32   // Decay Rate
#define CCAuto_Pitch_Sustain 33 // Final Pitch.
#define CCArpeggiator_Switch 34 // Turns ARP On/Off.
#define CCARP_Tempo 35          // Adjust ARP Tempo.
#define CCARP_Ext_Sync 36       // Turn External Sync On/Off.
#define CCARP_Note_Length 37    // Adjust Note Length.
#define CCARP_Mode 38           // 0=Up, 1=Down, 3=Bounce.
#define CCARP_Hold 39           // Turn ARP Hold On/Off.
#define CCARP_Octave_Plus 40    // Play extra Octave On/Off.
#define CCAmp_Attack 41         // Amp Attack
#define CCAmp_Decay 42
#define CCAmp_Sustain 43
#define CCAmp_Release 44
// CC# Name Description (If needed)
// ==============================================================================
#define CCFilter_Attack 45          // Filter EG settings also used for PWM and Hard Sync Pitch EG.
#define CCFilter_Decay 46
#define CCFilter_Sustain 47
#define CCFilter_Release 48
#define CCFilter_EG_Depth 49        // Filter Envelope Generator Influence
#define CCAmp_Velocity_Depth 50     // Key Velocity Influence to Volume
#define CCFilter_Velocity_Depth 51  // Key Velocity Influence to Filter
#define CCLFO_Velocity_Depth 52     // Key Velocity Influence to Filter LFO Speed

#define CCAmp_LFO_Depth 57          // LFO’s Influence to Volume
#define CCAmp_LFO_Rate 58           // Speed of Amp LFO

#define CCVibrato_Rate 60           // Speed of LFO to Pitch
#define CCKeyTrack_Switch 61        // Turn KeyTrack On/Off
#define CCKeyTrack_Depth 62         // Adjust KeyTrack Influence
#define CCKeyTrack_Exponent 63      // Influence Balance/Ratio (Low Keys:High keys)

#define CCFilter_LFO_Rate 66        // Speed of LFO to Filter
#define CCVibr_Amp_LFO_Wave 67      // Waveshape Select for Vibrato/Amp LFO
#define CCFilter_LFO_Wave 68        // Waveshape Select for Filter LFO
#define CCPWM_Depth_LFO 69          // Influence to PWM Duty Cycle
#define CCPWM_Rate 70               // Speed of PWM’s LFO
#define CCFilter_Resonance 71
#define CCFilter_LFO_Depth 72       // LFO influence to Filter
#define CCFilter_Type 73            // Select Lo or Hi Pass Filter Type
#define CCFilter_Cutoff_Freq 74

#define CCWheel_Mod_1_Select 77     // a feature for Modwheel to influence: 0=Nil, 1=Vibr, 2=Trem,
                                    // 3=FilterCO, 4=FltrLFOdepth, 5= FltrLFOrate, 6= VibrRate,
                                    // 7=TremRate, 8=PWMdepth, 9= PWMrate, 10=PhaserRate,
                                    // 11= Osc1Freq, 12= Osc2Freq
#define CCWheel_Mod_2_Select 78     // a feature for Modwheel to influence. (see above list)
#define CCWheel_Mod_3_Select 79     // a feature for Modwheel to influence. (see above list)

#define CCDelay_Expander_Stereo 85  // Expand Echoes. (Subtle Stereo on up to a Ping Pong Delay)
#define CCDetune 94                 // Detune 3 Saw and 2 Saw voices.
#define CCDuty_Cycle_Waveshape 99   // (Most Square or Waveshaper voices.)

#define CCAT_VCO_Depth 100          // AT depth to VCO (not part of synth
#define CCPitchBend_Depth 101       // PitchBend Depth
#define CCPortamento_SW 65          // Portamento On/off 


//MIDI CC control numbers
//These broadly follow standard CC assignments
#define   CCmodwheel      1 //pitch LFO amount - less from mod wheel
#define   CCsustain       64
#define   CCallnotesoff   123//Panic button




