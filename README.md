# My-Dirty-Synth

Content:
 - Main arduino code of the synth : [My-Dirty-Synth.ino](../master/My-Dirty-Synth.ino)
 - Front panel PCB in kicad 6.0 : hardware - [hardware - dirty synth folder](../master/hardware%20-%20dirty%20synth)

# Architecture

## Block diagram
Here is a block diagram of the synthesizer:
![Block diagram](pictures/block-diagram.png)

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


# Pictures

## Front panel design
![Front panel desing](pictures/FrontPanelDesign.png)