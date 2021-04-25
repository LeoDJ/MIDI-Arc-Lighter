#pragma once

#include "usbd_midi.h"
#include "usbd_desc.h"


extern USBD_MIDI_ItfTypeDef USBD_MIDI_Interface_fops_FS;

void midiSendNoteOn(uint8_t ch, uint8_t note, uint8_t vel);
void midiSendNoteOff(uint8_t ch, uint8_t note);
void midiSendCtrlChange(uint8_t ch, uint8_t num, uint8_t value);

// callback register functions
void midiSetCbNoteOff(void (*cb)(uint8_t ch, uint8_t note, uint8_t vel));
void midiSetCbNoteOn(void (*cb)(uint8_t ch, uint8_t note, uint8_t vel));
void midiSetCbCtrlChange(void (*cb)(uint8_t ch, uint8_t num, uint8_t value));
void midiSetCbPitchBend(void (*cb)(uint8_t ch, uint16_t value));


// see midi10.pdf, chapter 4
struct MidiCIN {
    enum MidiCodeIndexNumber {
        Misc,
        CableEvt,
        SystemCommon2,
        SystemCommon3,
        SysExStart,
        SystemCommon1   = 5,
        SysExEnd1       = 5,
        SysExEnd2,
        SysExEnd3,
        NoteOff,
        NoteOn,
        PolyKeyPress,
        ControlChange,
        ProgramChange,
        ChannelPressure,
        PitchBend,
        SingleByte
    };
};

struct MidiStatus {
    enum MidiStatusCodes {
        NoteOff = 0x8,
        NoteOn,
        PolyTouch, // Polyphonic Aftertouch
        CtrlChange,
        ProgChange,
        MonoTouch, // Monophonic Channel Aftertouch
        PitchBend,
        SystemExclusive
    };
};

struct MidiController {
    enum MidiControllers {
        AllSoundsOff    = 0x78,
        ControllerReset = 0x79,
        AllNotesOff     = 0x7B,
    };
};

typedef union {
    struct {
        uint8_t channel : 4;    // byte 0 - low nibble
        uint8_t status : 4;     // byte 0 - high nibble, highest bit always 1
        uint8_t byte1 : 7;      // byte 1 - 7 usable bits
        uint8_t : 1;            // padding, should be 0 for data byte
        uint8_t byte2 : 7;      // byte 2 - 7 usable bits
        uint8_t : 1;            // padding, should be 0 for data byte
    };
    uint8_t raw8[3];
} midiMessage_t;

typedef union {
    struct {
        uint8_t cableNumber : 4;
        uint8_t codeIndexNumber : 4;
        midiMessage_t midiMsg;
    };
    uint32_t raw32;
    uint8_t raw8[4];
} usbMidiEvent_t;