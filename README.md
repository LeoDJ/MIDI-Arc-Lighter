# MIDI-Arc-Lighter

A cheap chinese dual arc lighter, modified with a custom PCB and the power of an STM32F072 to play music with the arcs.

Follow the [Twitter Build Thread](https://twitter.com/_LeoDJ/status/1379586092094087168) for the newest developments and videos of the lighter in action.

The project has developed into a playground for implementing various USB features and modifying the default STM USB libraries way beyond what they were meant for*.

The features include:
- USB composite device
  - USB MIDI for direct control of the 
  - USB MSC (mass storage) to appear as a drive on a PC for dragging MIDI files onto the SPI flash.
  - USB CDC (serial port) for debugging and log messages
- 5-pin DIN MIDI input via adapter**
- Standalone MIDI playback
- SPI flash for storing MIDIs 
- Single button user interface
- Battery charging and monitoring

\* If I find the time and motivation I'll write an in-depth tutorial about the modifications.  
\*\* Adapter from 5-pin DIN plug to micro USB, optocoupler in the DIN plug, single-ended MIDI signal over the micro USB ID pin. Details will be shared at a later date.

### => [PCB Repository](https://github.com/Techbeard/Arc-Lighter-PCB)