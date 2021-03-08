# Arduino WAV player

## General info
In this project contents of wav file are being read from SD card into buffer. Interrupt from timer 1 is triggered with frequency of 48000 Hz which is required sample rate for WAV file. In interrupt routine 16-bit sample is taken from buffer and put on port A and C of arduino Mega where 16-bit R-2R ladder is located. Output of R-2R ladder is connected to audio jack. Song played can be changed with rotary encoder connected to port D.

## Requirements
Project requires:
* Arduino Mega 2560
* SD card reader
* Rotary encoder
* bunch of resistors
