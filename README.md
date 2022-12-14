# My-Dirty-Synth

Content:
 - Main Arduino code of the synth : [My-Dirty-Synth.ino](../master/My-Dirty-Synth.ino)
 - Front panel PCB in kicad 6.0 : hardware - [hardware - dirty synth folder](../master/hardware%20-%20dirty%20synth)

# Infos
This git contains all the files to realize your own "My-Dirty-Synth" made with the [Simple Synth kit](https://www.synthux.academy/simple) made by synthux academy. The synth is based on a [Daisy seed](https://www.electro-smith.com/daisy/daisy) by electro-smith and uses their Arduino library [DaisyDuino](https://github.com/electro-smith/DaisyDuino).

Each component of the synth (potentiometer, switch, jack) comes from the Simple Synth kit except what is linked to MIDI. For this, a custom PCB acting like a front panel has been made.

# Architecture

## Block diagram
Here is a block diagram of the synthesizer:
![Block diagram](pictures/block-diagram.png)

The design starts with an oscillator whose shape can be chosen between sinus, triangle, sawtooth, and square. In addition to the main frequency, it is possible to have the 1st and 2nd octaves mixed with the main oscillator.

The signal is then sent to the effects:
- a wavefolder 
- a decimator

Both effects have a potentiometer to drive their strength.

A 3 positions switch helps to use those two effects combined:
- position 0: wavefolder only to maximum range
- position 1: wavefolder to maximum range and decimator reduced range
- position 2: wavefolder reduced range and decimator maximum range

When passing from one mode to the other, the range of value of the effect will slowly swipe to fit the selected range. This swipe last for ~2 secs.

The next module is the filter. You can change the filter resonance, cutoff frequency, and drive. With two switch you can change from
- no filter
- high pass filter
- low pass filter
- bandpass filter.

Finally, you can find the signal can pass through a VCA or lowpass gate with attack decay and release adjustable.
You can either activate the VCA/LPG with the VCA Mode switch and then select what kind of system you want your signal to pass through:
- VCA
- LPG
- VCA+LPG

## MIDI implementation

The MIDI listen to all channel. It reacts to change note On, note Off, pitch bend, and control change. 
The control change affects the filter cutoff frequency.

- Note On/Off will change the pitch. Be sure to have the oscillator main frequency potentiometer to its minimum to have no interference with MIDI.
- Control change will change the cutoff frequency of the filter. Be sure to have the cutoff frequency potentiometer to its minimum to have no interference with MIDI.


## Gpios

| Bloc       | Description           | Components       | Gpios |
|------------|-----------------------|------------------|-------|
| Oscillator | Main frequency        | Pot              | A11   |
| Oscillator | Octave 1              | Pot              | A10   |
| Oscillator | Octave 2              | Pot              | A9    |
| Effects    | Wavefolder factor     | Pot              | A8    |
| Effects    | Decimator factor      | Pot              | A7    |
| Filter     | Res frequency         | Pot              | A6    |
| Filter     | Filter drive          | Pot              | A5    |
| Filter     | Filter cutoff freq    | Pot              | A4    |
| VCA/LPG    | Attack                | Pot              | A3    |
| VCA/LPG    | Release               | Pot              | A2    |
| VCA/LPG    | Decay                 | Pot              | A1    |
| General    | Main volume           | Pot              | A0    |
| Oscillator | Oscillator type       | Switch 2 pos     | 6     |
| Oscillator | Oscillator type       | Switch 2 pos     | 7     |
| Effects    | Effect mode selection | Switch 3 pos 1.1 | 8     |
| Effects    | Effect mode selection | Switch 3 pow 1.2 | 9     |
| Filter     | Filter type           | Switch 2 pos     | 10    |
| Filter     | Filter type           | Switch 2 pos     | 11    |
| VCA/LPG    | VCA Mode              | Switch 2 pos     | 12    |
| General    | Gate out signal       | Jack             | 5     |
| VCA/LPG    | VCA LPG selection     | Switch 3 pos 2.1 | 4     |
| VCA/LPG    | VCA LPG selection     | Switch 3 pos 2.2 | 3     |
| MIDI       | MIDI input            | MIDI connector   | 1     |
| Output     | AudioOut[0]			 | Mono Jack        | AudioOut1    |
| Output     | AudioOut[1]	 	     | -  			    | AudioOut2    |


# Pictures

## Front panel design
![Front panel desing](pictures/FrontPanelDesign.png)

## Wiring

![My-Dirty-Synth wiring](pictures/My-Dirty-Synth-Wiring.jpeg)
