
You can delete this folder.
It is neither required for compiling nor for using the program.

The files in this folder are only for debugging.
To use them you must

1.) copy one of the *.bin files into the directory My Documents\ElmueSoft-LogicAnalyzer (do not change the filename!)
2.) #define LOAD_CAPTURE_FROM_FILE  1  in Port.h
3.) compile and run the Debug version
4.) click the "Capture" button
5.) chose the protocol that corresponds to the content of the *.bin file
6.) click the "Analyze" button


The captured signals in this folder are:
1.) Async: A sequence of characters "ABC...XYZ" sent via RS232
2.) PS/2:  Hitting the CTRL key (scan code 0x14) and releasing the CTRL key (scan code 0xF0 + 0x14)
3.) PS/2:  Hitting the Capslock key (scan code 0x58) and releasing the Capslock key (scan code 0xF0 + 0x58)
    After hitting the Capslock key the Host (PC) sets the state of the keyboard LEDs (0xED and 0x06 turns on the Capslock + Numlock LED's)
    Each command from the Host is acknowledged with a 0xFA byte from the keyboard

