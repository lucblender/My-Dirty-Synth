////////////////////////////////////////////////////////////////
///////////////////// VARIABLES & LIBRARIES ////////////////////

#include "DaisyDuino.h"
#include <MIDI.h>

using namespace daisysp;

HardwareSerial SerialMidi(1, 2);
MIDI_CREATE_INSTANCE(HardwareSerial, SerialMidi, myMidi);



////////////////////////////////////////////////////////////////
///////////////////// INPUT OUTPUT DEFINITION///////////////////

#define AN_MAIN_FREQ A11
#define AN_OCTAVE_1 A10
#define AN_OCTAVE_2 A9
#define AN_WAVEFOLD A8
#define AN_BITCRUSH A7
#define AN_F_RES A6
#define AN_F_DRIVE A5
#define AN_F_FREQ A4
#define AN_DYNC_ATT A3
#define AN_DYNC_REL A2
#define AN_DYNC_DEC A1
#define AN_MAIN_VOLUME A0

#define DI_I_OSC_TYPE_0 6
#define DI_I_OSC_TYPE_1 7
#define DI_I_CRUSH_FOLD_0 8
#define DI_I_CRUSH_FOLD_1 9
#define DI_I_FILTER_TYPE_0 10
#define DI_I_FILTER_TYPE_1 11
#define DI_I_VCA_MODE 12
#define DI_O_PULSE 5
#define DI_I_GATE_TYPE_0 4
#define DI_I_GATE_TYPE_1 3

#define OSCILLATORS_FACTOR 1

float mainFreq;
float mainFreqOLD = -1;
float octave1Factor;
float octave1FactorOLD = -1;
float octave2Factor;
float octave2FactorOLD = -1;
float wavefolderFactor;
float wavefolderFactorOLD = -1;
float bitcrushFactor;
float bitcrushFactorOLD = -1;
float filterRes;
float filterResOLD = -1;
float filterDrive;
float filterDriveOLD = -1;
float filterFreq;
float filterFreqOLD = -1;
float dyncAttack;
float dyncAttackOLD = -1;
float dyncDecay;
float dyncDecayOLD = -1;
float dyncRelease;
float dyncReleaseOLD = -1;
float mainVolume;
float mainVolumeOLD = -1;

uint8_t oscType0;
uint8_t oscType0OLD = 10;
uint8_t oscType1;
uint8_t oscType1OLD = 10;
uint8_t crushFold0;
uint8_t crushFold0OLD = 10;
uint8_t crushFold1;
uint8_t crushFold1OLD = 10;
uint8_t crushFoldType;
uint8_t filterType0;
uint8_t filterType0OLD = 10;
uint8_t filterType1;
uint8_t filterType;
uint8_t filterType1OLD = 10;
uint8_t vcaMode;
uint8_t vcaModeOLD = 10;
uint8_t gateType0;
uint8_t gateType0OLD = 10;
uint8_t gateType1;
uint8_t gateType1OLD = 10;
uint8_t gateType;

bool gate = false;

int titi = 0;

////////////////////////////////////////////////////////////////
///////////////////// MODULES //////////////////////////////////


static Oscillator osc;
static Oscillator oscOctaveOne;
static Oscillator oscOctaveTwo;
Wavefolder wavefolder;
Decimator bitcrusher;
Svf funkyFilter;
Svf lowPassGate;
Adsr adsr;
float frequency = 440;
float sample_rate;

float filterFrequency;


////////////////////////////////////////////////////////////////
///////////////////// START SYNTH SETUP ////////////////////////


void setup() {
  Serial.begin(15200);

  myMidi.setHandleNoteOn(handleNoteOn);
  myMidi.setHandleNoteOff(handleNoteOff);
  myMidi.begin(MIDI_CHANNEL_OMNI);

  //Setupt input outputs
  pinMode(DI_I_OSC_TYPE_0, INPUT_PULLUP);
  pinMode(DI_I_OSC_TYPE_1, INPUT_PULLUP);
  pinMode(DI_I_CRUSH_FOLD_0, INPUT_PULLUP);
  pinMode(DI_I_CRUSH_FOLD_1, INPUT_PULLUP);
  pinMode(DI_I_FILTER_TYPE_0, INPUT_PULLUP);
  pinMode(DI_I_FILTER_TYPE_1, INPUT_PULLUP);
  pinMode(DI_I_VCA_MODE, INPUT_PULLUP);
  pinMode(DI_O_PULSE, OUTPUT);
  pinMode(DI_I_GATE_TYPE_0, INPUT_PULLUP);
  pinMode(DI_I_GATE_TYPE_1, INPUT_PULLUP);

  // DAISY SETUP
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);

  wavefolder.Init();
  wavefolder.SetGain(1);
  wavefolder.SetOffset(0);

  sample_rate = DAISY.get_samplerate();
  
  bitcrusher.Init();
  bitcrusher.SetDownsampleFactor(0.4f);
  funkyFilter.Init(sample_rate);
  lowPassGate.Init(sample_rate);

  lowPassGate.SetRes(0.9);
  lowPassGate.SetDrive(0.9);

  // OSCILLATOR SETUP
  osc.Init(sample_rate);
  oscOctaveOne.Init(sample_rate);
  oscOctaveTwo.Init(sample_rate);
  setGlobalFrequency(440);
  osc.SetAmp(OSCILLATORS_FACTOR);
  oscOctaveOne.SetAmp(0);
  oscOctaveTwo.SetAmp(0);

  osc.SetWaveform(osc.WAVE_SIN);
  oscOctaveOne.SetWaveform(osc.WAVE_SIN);
  oscOctaveTwo.SetWaveform(osc.WAVE_SIN);

  adsr.Init(sample_rate);


  // DAISY SETUP
  DAISY.begin(ProcessAudio);
}


///////////////////// END SYNTH SETUP //////////////////////////
////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
///////////////////// PROCESSING AUDIO SAMPLES (LOOP) //////////


void ProcessAudio(float **in, float **out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    float sample = osc.Process();

    //if octave factor == 0, don't even compute the oscillator
    float sampleOctaveOne = (octave1Factor  < 0.01) ? 0 : oscOctaveOne.Process();
    float sampleOctaveTwo = (octave2Factor  < 0.01) ? 0 : oscOctaveTwo.Process();

    float addedOscillators = sample + sampleOctaveOne + sampleOctaveTwo;

    float withEffects;

    float withFilter;

    switch (crushFoldType)
    {
      //no bitcrusher
      case 1:
        withEffects = ((wavefolderFactor < 0.01) ? addedOscillators : wavefolder.Process(addedOscillators));
        break;
      //wavefolder then bitcrusher
      case 3:
        if (bitcrushFactor > 0.1)
          withEffects = bitcrusher.Process(((wavefolderFactor < 0.01) ? addedOscillators : wavefolder.Process(addedOscillators)));
        else
          withEffects = ((wavefolderFactor < 0.01) ? addedOscillators : wavefolder.Process(addedOscillators));
        break;
      //bitcrusher then wavefolder
      case 2:
        if (bitcrushFactor > 0.1)
          withEffects = (wavefolderFactor < 0.01) ? bitcrusher.Process(addedOscillators) : wavefolder.Process(bitcrusher.Process(addedOscillators));
        else
          withEffects = (wavefolderFactor < 0.01) ? addedOscillators : wavefolder.Process(addedOscillators);
        break;
    }


    switch (filterType)
    {
      case 0:
        withFilter = withEffects;
        break;
      case 1:
        funkyFilter.Process(withEffects);
        withFilter = funkyFilter.Low();
        break;
      case 2:
        funkyFilter.Process(withEffects);
        withFilter = funkyFilter.High();
        break;
      case 3:
        funkyFilter.Process(withEffects);
        withFilter = funkyFilter.Band();
        break;
    }

    float adsrGain;
    float afterAdsr;

    if (vcaMode == 0)
    {
      adsrGain = 1.0;
      afterAdsr = withFilter;
    }
    else
    {
      switch (gateType)
      {
        //simple adsr
        case 1:
          adsrGain = adsr.Process(gate);
          afterAdsr = withFilter * adsrGain;
          break;
        //simple low pass gate
        case 3:
          adsrGain = adsr.Process(gate);
          lowPassGate.SetFreq(adsrGain * (sample_rate / 2));
          lowPassGate.Process(withFilter);
          afterAdsr = lowPassGate.Low();
          break;
        //adsr + low pass gate
        case 2:
          adsrGain = adsr.Process(gate);
          lowPassGate.SetFreq(adsrGain * (sample_rate / 2));
          lowPassGate.Process(withFilter);
          afterAdsr = lowPassGate.Low() * ((adsrGain * 3 / 4) + 0.25);
          break;
      }

    }




    float finalOutput = afterAdsr * mainVolume;

    out[0][i] = finalOutput;//Filter.High();
    out[1][i] = finalOutput;//funkyFilter.High();
  }
}


void setGlobalFrequency(float localFrequency)
{
  frequency = localFrequency;
  osc.SetFreq(frequency);
  oscOctaveOne.SetFreq(frequency / 2);
  oscOctaveTwo.SetFreq(frequency / 4);
}

////////////////////////////////////////////////////////////////
///////////////////// START CONTROLS LOOP //////////////////////


void loop() {


  myMidi.read();
  // put your main code here, to run repeatedly:
  mainFreq = semitone_to_hertz(simpleAnalogReadAndMap(AN_MAIN_FREQ, -57, 70));
  octave1Factor = simpleAnalogRead(AN_OCTAVE_1);
  octave2Factor = simpleAnalogRead(AN_OCTAVE_2);
  wavefolderFactor = simpleAnalogRead(AN_WAVEFOLD);
  bitcrushFactor = simpleAnalogRead(AN_BITCRUSH);
  filterRes = simpleAnalogRead(AN_F_RES);
  filterDrive = simpleAnalogRead(AN_F_DRIVE);
  filterFreq = simpleAnalogRead(AN_F_FREQ);
  dyncAttack = simpleAnalogRead(AN_DYNC_ATT);
  dyncDecay = simpleAnalogRead(AN_DYNC_DEC);
  dyncRelease = simpleAnalogRead(AN_DYNC_REL);
  mainVolume = simpleAnalogRead(AN_MAIN_VOLUME);

  oscType0 = digitalRead(DI_I_OSC_TYPE_0);
  oscType1 = digitalRead(DI_I_OSC_TYPE_1);
  crushFold0 = digitalRead(DI_I_CRUSH_FOLD_0);
  crushFold1 = digitalRead(DI_I_CRUSH_FOLD_1);
  filterType0 = digitalRead(DI_I_FILTER_TYPE_0);
  filterType1 = digitalRead(DI_I_FILTER_TYPE_1);
  vcaMode = digitalRead(DI_I_VCA_MODE);
  gateType0 = digitalRead(DI_I_GATE_TYPE_0);
  gateType1 = digitalRead(DI_I_GATE_TYPE_1);


#ifdef DEBUG_PRINT_SWITCH
  Serial.print("oscType0 ");
  Serial.print(oscType0);
  Serial.print(", oscType1 ");
  Serial.print(oscType1);
  Serial.print(", crushFold0 ");
  Serial.print(crushFold0);
  Serial.print(", crushFold1 ");
  Serial.print(crushFold1);
  Serial.print(", filterType0 ");
  Serial.print(filterType0);
  Serial.print(", filterType1 ");
  Serial.print(filterType1);
  Serial.print(", vcaMode ");
  Serial.println(vcaMode);
#endif

#ifdef DEBUG_PRINT_POTENTIOMETERS
  Serial.print("mainFreq ");
  Serial.print(mainFreq);
  Serial.print(" ,octave1Factor ");
  Serial.print(octave1Factor);
  Serial.print(" ,octave2Factor ");
  Serial.print(octave2Factor);
  Serial.print(" ,wavefolderFactor ");
  Serial.print(wavefolderFactor);
  Serial.print(" ,bitcrushFactor ");
  Serial.print(bitcrushFactor);
  Serial.print(" ,filterRes ");
  Serial.print(filterRes);
  Serial.print(" ,filterDrive ");
  Serial.print(filterDrive);
  Serial.print(" ,filterFreq ");
  Serial.print(filterFreq);
  Serial.print(" ,dyncAttack ");
  Serial.print(dyncAttack);
  Serial.print(" ,dyncDecay ");
  Serial.print(dyncDecay);
  Serial.print(" ,dyncRelease ");
  Serial.print(dyncRelease);
  Serial.print(" ,AN_MAIN_VOLUME ");
  Serial.println(mainVolume);
#endif
  if (mainFreq != mainFreqOLD)
  {
    Serial.println(mainFreq);
    setGlobalFrequency(mainFreq);
  }
  if (octave1Factor != octave1FactorOLD)
  {
    oscOctaveOne.SetAmp(octave1Factor * OSCILLATORS_FACTOR);
  }
  if (octave2Factor != octave2FactorOLD)
  {
    oscOctaveTwo.SetAmp(octave2Factor * OSCILLATORS_FACTOR);
  }
  if (wavefolderFactor != wavefolderFactorOLD)
  {
    wavefolder.SetGain(5.0 * wavefolderFactor + 1.0);
    wavefolder.SetOffset(1.3 * wavefolderFactor);
  }
  if (bitcrushFactor != bitcrushFactorOLD)
  {
    int convertedBitFactor = bitcrushFactor * 7.5 + 8;
    convertedBitFactor = convertedBitFactor;

    bitcrusher.SetBitsToCrush(convertedBitFactor);//convertedBitFactor);
    bitcrusher.SetDownsampleFactor(bitcrushFactor);

    //bitcrusher.SetDownsampleFactor((sample_rate) * (1.1 - bitcrushFactor));
  }
  if (filterRes != filterResOLD)
  {
    float res = 0.3 + (filterRes * 0.69);
    funkyFilter.SetRes(res);
  }
  if (filterDrive != filterDriveOLD)
  {
    funkyFilter.SetDrive(0.3 + (filterDrive * 1.7));
  }
  if (filterFreq != filterFreqOLD)
  {
    float cutoffFreq = (filterFreq) * (sample_rate / 4);
    funkyFilter.SetFreq(cutoffFreq);
  }
  if (dyncAttack != dyncAttackOLD)
  {
    adsr.SetAttackTime(dyncAttack);
  }
  if (dyncDecay != dyncDecayOLD)
  {
    adsr.SetDecayTime(dyncDecay);
  }
  if (dyncRelease != dyncReleaseOLD)
  {
    adsr.SetReleaseTime(dyncRelease);
  }
  if (mainVolume != mainVolumeOLD)
  {
  }

  if (oscType0 != oscType0OLD || oscType1 != oscType1OLD)
  {
    int type = oscType0 + oscType1 * 2;

    switch (type)
    {
      case 0:
        osc.SetWaveform(osc.WAVE_SIN);
        oscOctaveOne.SetWaveform(osc.WAVE_SIN);
        oscOctaveTwo.SetWaveform(osc.WAVE_SIN);
        break;
      case 1:
        osc.SetWaveform(osc.WAVE_POLYBLEP_TRI);
        oscOctaveOne.SetWaveform(osc.WAVE_POLYBLEP_TRI);
        oscOctaveTwo.SetWaveform(osc.WAVE_POLYBLEP_TRI);
        break;
      case 2:
        osc.SetWaveform(osc.WAVE_POLYBLEP_SAW);
        oscOctaveOne.SetWaveform(osc.WAVE_POLYBLEP_SAW);
        oscOctaveTwo.SetWaveform(osc.WAVE_POLYBLEP_SAW);
        break;
      case 3:
        osc.SetWaveform(osc.WAVE_POLYBLEP_SQUARE);
        oscOctaveOne.SetWaveform(osc.WAVE_POLYBLEP_SQUARE);
        oscOctaveTwo.SetWaveform(osc.WAVE_POLYBLEP_SQUARE);
        break;

    };
  }

  if (crushFold0 != crushFold0OLD || crushFold1 != crushFold1OLD)
  {
    crushFoldType = crushFold0 + crushFold1 * 2;
  }
  if (filterType0 != filterType0OLD || filterType1 != filterType1OLD)
  {
    filterType = filterType0 + filterType1 * 2;
  }
  if (vcaMode != vcaModeOLD)
  {
  }
  if (gateType0 != gateType0OLD || gateType1 != gateType1OLD)
  {
    gateType = gateType0 + gateType1 * 2;
  }


  mainFreqOLD = mainFreq;
  octave1FactorOLD = octave1Factor;
  octave2FactorOLD = octave2Factor;
  wavefolderFactorOLD = wavefolderFactor;
  bitcrushFactorOLD = bitcrushFactor;
  filterResOLD = filterRes;
  filterDriveOLD = filterDrive;
  filterFreqOLD = filterFreq;
  dyncAttackOLD = dyncAttack;
  dyncDecayOLD = dyncDecay;
  dyncReleaseOLD = dyncRelease;
  mainVolumeOLD = mainVolume;
  oscType0OLD = oscType0;
  oscType1OLD = oscType1;
  crushFold0OLD = crushFold0;
  crushFold1OLD = crushFold1;
  filterType0OLD = filterType0;
  filterType1OLD = filterType1;
  vcaModeOLD = vcaMode;
  gateType0OLD = gateType0;
  gateType1OLD = gateType1;



}

void handleNoteOn(byte inChannel, byte inNote, byte inVelocity) {    
    float midiFreq = semitone_to_hertz(inNote-57);
    setGlobalFrequency(midiFreq);
    gate = true;
    digitalWrite(DI_O_PULSE,HIGH);
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity)
{
    gate = false;
    digitalWrite(DI_O_PULSE,LOW);
}

float semitone_to_hertz(int8_t note_number) {
  return 220 * pow(2, ((float)note_number - 0) / 12);
}

float simpleAnalogRead(uint32_t pin) {
  return (1023.0 - (float)analogRead(pin)) / 1023.0;
}

// Reads a simple pot and maps it to a value bewtween to integer values
float simpleAnalogReadAndMap(uint32_t pin, long min, long max) {
  return map(1023 - analogRead(pin), 0, 1023, min, max);
}