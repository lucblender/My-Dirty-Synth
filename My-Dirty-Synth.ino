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
float mainFreqUp;
float mainFreqDown;
float midiFreq = -1;
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
float filterMidiFreq = -1;
float filterMidiFreqOLD = -1;
float dyncAttack;
float dyncAttackOLD = -1;
float dyncDecay;
float dyncDecayOLD = -1;
float dyncRelease;
float dyncReleaseOLD = -1;
float mainVolume;
float mainVolumeOLD = -1;

float effectFactorWaveFolderSetpoint = 1.0f;
float effectFactorBitCrusher = 1.0f;
float effectFactorWaveFolder = 1.0f;
float effectFactorBitCrusherSetpoint = 1.0f;

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

#define TRIGGER_DIFF 0.01

////////////////////////////////////////////////////////////////
///////////////////// MODULES //////////////////////////////////


static Oscillator osc;
static Oscillator oscOctaveOne;
static Oscillator oscOctaveTwo;
Wavefolder wavefolder;
Decimator bitcrusher;
Svf funkyFilter;
MoogLadder lowPassGate;
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
  myMidi.setHandlePitchBend(handlePitchBend);
  myMidi.setHandleControlChange(handleControlChange);
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
  lowPassGate.SetFreq(sample_rate);

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
  
  adsr.SetSustainLevel(.3);


  // DAISY SETUP
  DAISY.begin(ProcessAudio);
}


///////////////////// END SYNTH SETUP //////////////////////////
////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
///////////////////// PROCESSING AUDIO SAMPLES (LOOP) //////////


void ProcessAudio(float **in, float **out, size_t size) {

  float adsrGain;
  float afterAdsr;
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
      case 2:
        if (bitcrushFactor > 0.02)
          withEffects = bitcrusher.Process(((wavefolderFactor < 0.01) ? addedOscillators : wavefolder.Process(addedOscillators)));
        else
          withEffects = ((wavefolderFactor < 0.01) ? addedOscillators : wavefolder.Process(addedOscillators));
        break;
      
    }


    switch (filterType)
    {
      case 0:
        withFilter = withEffects;
        break;
      case 1:
        funkyFilter.Process(withEffects);
        withFilter = funkyFilter.High();
        break;
      case 2:
        funkyFilter.Process(withEffects);
        withFilter = funkyFilter.Low();
        break;
      case 3:
        funkyFilter.Process(withEffects);
        withFilter = funkyFilter.Band();
        break;
    }


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
          lowPassGate.SetFreq((sample_rate / 2) * (adsrGain * adsrGain));      
          afterAdsr = lowPassGate.Process(withFilter);
          break;
        //adsr + low pass gate
        case 2:
          adsrGain = adsr.Process(gate);
          lowPassGate.SetFreq((sample_rate / 2) * (adsrGain * adsrGain));
          float lpgOutput = lowPassGate.Process(withFilter);
          afterAdsr = lpgOutput * ((adsrGain * 3 / 4) + 0.25);
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
  int freqNote = simpleAnalogReadAndMap(AN_MAIN_FREQ, -57, 70);
  mainFreq = semitone_to_hertz(freqNote);
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

    if (mainFreq < 10 && midiFreq != -1) //if frequency is at its lowest and midi fequency has been changed, don't update frequency*
    {
      //do nothing
    }
    else
    {
      setGlobalFrequency(mainFreq);
    }
    mainFreqOLD = mainFreq;
  }
  if (abs(octave1Factor - octave1FactorOLD) > TRIGGER_DIFF)
  {
    oscOctaveOne.SetAmp(octave1Factor * OSCILLATORS_FACTOR);

    octave1FactorOLD = octave1Factor;
  }
  if (abs(octave2Factor - octave2FactorOLD) > TRIGGER_DIFF)
  {
    oscOctaveTwo.SetAmp(octave2Factor * OSCILLATORS_FACTOR);

    octave2FactorOLD = octave2Factor;
  }
  if ((abs(wavefolderFactor - wavefolderFactorOLD) > TRIGGER_DIFF) || (effectFactorWaveFolderSetpoint != effectFactorWaveFolder))
  {
    wavefolder.SetGain(5.0 * (wavefolderFactor * effectFactorWaveFolder) + 1.0);
    wavefolder.SetOffset(1.3 * (wavefolderFactor * effectFactorWaveFolder));

    wavefolderFactorOLD = wavefolderFactor;
  }
  if ((abs(bitcrushFactor - bitcrushFactorOLD) > TRIGGER_DIFF) || (effectFactorBitCrusher != effectFactorBitCrusherSetpoint))
  {
    int convertedBitFactor = (bitcrushFactor) * 9.5 + 0;
    convertedBitFactor = convertedBitFactor;

    bitcrusher.SetBitsToCrush(convertedBitFactor);//convertedBitFactor);
    float downsampleFactor = (bitcrushFactor * effectFactorBitCrusher);//+ 0.5f;
    bitcrusher.SetDownsampleFactor(downsampleFactor);

    //bitcrusher.SetDownsampleFactor((sample_rate) * (1.1 - bitcrushFactor));
    bitcrushFactorOLD = bitcrushFactor;
  }
  if (abs(filterRes - filterResOLD) > TRIGGER_DIFF)
  {
    float res = 0.3 + (filterRes * 0.69);
    funkyFilter.SetRes(res);

    filterResOLD = filterRes;
  }
  if (abs(filterDrive - filterDriveOLD) > TRIGGER_DIFF)
  {
    float drive = filterDrive * 0.7;
    funkyFilter.SetDrive(drive);

    filterDriveOLD = filterDrive;
  }

  if (filterFreq != filterFreqOLD)
  {
    if (filterMidiFreq == -1)
    {
      // if midi mod freq never changed
      float cutoffFreq = (filterFreq) * (sample_rate / 4);
      funkyFilter.SetFreq(cutoffFreq);
    }
    else
    {
      // if midi mod freq never changed, need bigger change of filterFreq to update
      if (abs(filterFreq - filterFreqOLD) > 0.008)
      {
        filterMidiFreq = -1;
        float cutoffFreq = (filterFreq) * (sample_rate / 4);
        funkyFilter.SetFreq(cutoffFreq);
      }
    }
  }

  filterFreqOLD = filterFreq;

  if (abs(filterMidiFreq - filterMidiFreqOLD) > TRIGGER_DIFF)
  {
    float cutoffFreq = (filterMidiFreq) * (sample_rate / 4);
    funkyFilter.SetFreq(cutoffFreq);

    filterMidiFreqOLD = filterMidiFreq;
  }

  if (abs(dyncAttack - dyncAttackOLD) > TRIGGER_DIFF)
  {
    adsr.SetAttackTime(dyncAttack);

    dyncAttackOLD = dyncAttack;
  }
  if (abs(dyncDecay - dyncDecayOLD) > TRIGGER_DIFF)
  {
    adsr.SetDecayTime(dyncDecay);

    dyncDecayOLD = dyncDecay;
  }
  if (abs(dyncRelease - dyncReleaseOLD) > TRIGGER_DIFF)
  {
    adsr.SetReleaseTime(dyncRelease);

    dyncReleaseOLD = dyncRelease;
  }
  if (mainVolume != mainVolumeOLD)
  {
    mainVolumeOLD = mainVolume;
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

    }

    oscType0OLD = oscType0;
    oscType1OLD = oscType1;
  }

  if (crushFold0 != crushFold0OLD || crushFold1 != crushFold1OLD)
  {
    crushFoldType = crushFold0 + crushFold1 * 2;
    crushFold0OLD = crushFold0;
    crushFold1OLD = crushFold1;

    switch (crushFoldType)
    {
      case 1://Fold only
        effectFactorWaveFolderSetpoint = 1.0f;
        break;
      case 3://F up D down
        effectFactorWaveFolderSetpoint = 1.0f;
        effectFactorBitCrusherSetpoint = 0.5f;
        break;
      case 2://F down D up
        effectFactorWaveFolderSetpoint = 0.2f;
        effectFactorBitCrusherSetpoint = 1.0f;
        break;
    }

  }
  if (filterType0 != filterType0OLD || filterType1 != filterType1OLD)
  {
    filterType = filterType0 + filterType1 * 2;

    filterType0OLD = filterType0;
    filterType1OLD = filterType1;
  }
  if (vcaMode != vcaModeOLD)
  {

    vcaModeOLD = vcaMode;
  }
  if (gateType0 != gateType0OLD || gateType1 != gateType1OLD)
  {
    gateType = gateType0 + gateType1 * 2;

    gateType0OLD = gateType0;
    gateType1OLD = gateType1;
  }

  if (effectFactorWaveFolderSetpoint != effectFactorWaveFolder)
  {
    if (effectFactorWaveFolderSetpoint > effectFactorWaveFolder)
    {
      effectFactorWaveFolder += 0.001;
    }
    else
    {
      effectFactorWaveFolder -= 0.001;
    }
    if(abs(effectFactorWaveFolderSetpoint - effectFactorWaveFolder)<0.001)
    {
        effectFactorWaveFolder = effectFactorWaveFolderSetpoint;
    }
  }
  
  if (effectFactorBitCrusher != effectFactorBitCrusherSetpoint)
  {
    if (effectFactorBitCrusherSetpoint > effectFactorBitCrusher)
    {
      effectFactorBitCrusher += 0.001;
    }
    else
    {
      effectFactorBitCrusher -= 0.001;
    }
    if(abs(effectFactorBitCrusherSetpoint - effectFactorBitCrusher)<0.001)
    {
        effectFactorBitCrusher = effectFactorBitCrusherSetpoint;
    }
  }
}

void handleNoteOn(byte inChannel, byte inNote, byte inVelocity) {
  Serial.println("handleNoteOn");
  midiFreq = semitone_to_hertz(inNote - 57);

  mainFreqUp = semitone_to_hertz(inNote - 57 + 1);
  mainFreqDown = semitone_to_hertz(inNote - 57 - 1);

  setGlobalFrequency(midiFreq);
  gate = true;
  digitalWrite(DI_O_PULSE, HIGH);
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity)
{
  Serial.println("handleNoteOff");
  gate = false;
  digitalWrite(DI_O_PULSE, LOW);
}

void handlePitchBend(unsigned char inChannel, int data)
{
  //data goest from -8192 to +8192
  if (data == 0)
  {
    setGlobalFrequency(midiFreq);
  }
  else if (data > 0)
  {
    float bendedFreq = (((mainFreqUp - midiFreq) / 8192.0f) * data);
    setGlobalFrequency(midiFreq + bendedFreq);
  }
  else if (data < 0)
  {
    float bendedFreq = (((midiFreq - mainFreqDown) / 8192.0f) * -data);
    setGlobalFrequency(midiFreq - bendedFreq);

  }

}

void handleControlChange(unsigned char inChannel, unsigned char data1, unsigned char data2)
{
  if (data1 == 1)
  {
    //data2 goes from 0 to 127
    filterMidiFreq = data2 / 127.0f;
  }
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
