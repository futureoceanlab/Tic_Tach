# FlowMeter

Code, scematics,  and  documentation for  the  Mesobot FlowMeter,  whose responsibility  is to power  & eat data  from the two Omega Hall-sensor flow sensors at the tail of the eDNA sampler, count pulses, and ouput the pulses over a  CAN-bus interface to the Mesobot TX2.

    src/Tic_Tach/

Firmware running the Teensy 3.5 inside the Tic-Tach, responsible for counting pulses, reporting the count via a timer-based heartbeat over the CAN-bus, and responding to requests (Reset, New-Pulse)

    src/Tic_Tach_Unscrambler/

Matlab code to recover the 20 surviving bits after an early nibble-vs-byte typo.

    schematic/

Schematic for the Tic_Tach
