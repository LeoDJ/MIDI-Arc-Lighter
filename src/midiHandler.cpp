#include "midiHandler.h"
#include "usbd_midi_if.h"
#include "toneOutput.h"

#define NUM_CHANNELS    2

// -1 for no note playing
static int8_t curNotePlaying[NUM_CHANNELS];

static int getChannelByNote(uint8_t note) {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (curNotePlaying[i] == note) {
            return i;
        }
    }
    return -1;
}

static int getFreeChannelSlot() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (curNotePlaying[i] == -1) {
            return i;
        }
    }
    return -1;
}

void midiHandlerNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
    printf("(%8d) [MIDI] On:  %3d", HAL_GetTick(), note);
    int toneCh = getFreeChannelSlot();
    if (toneCh != -1) {
        curNotePlaying[toneCh] = note;
        toneOutputNote(toneCh, note);
    }
    else {
        printf(" XXX");
    }
    printf("\n");
}

void midiHandlerNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
    printf("(%8d) [MIDI] Off: %3d\n", HAL_GetTick(), note);
    int toneCh = getChannelByNote(note);
    if (toneCh != -1) {
        curNotePlaying[toneCh] = -1;
        toneOutputFreq(toneCh, 0);
    }
}

void midiHandlerCtrlChange(uint8_t ch, uint8_t num, uint8_t value) {
    // printf("(%8d) [MIDI] Ctrl: ch: %2d, %02X %02X\n", HAL_GetTick(), midi.channel, midi.byte1, midi.byte2);
    if (ch == 0) { // only do parsing for ch1 for now as MidiEditor sends the same commands to all 16 channels
        switch (num) {
            case MidiController::AllSoundsOff:
            case MidiController::ControllerReset:
            case MidiController::AllNotesOff:
                // turn off all channels
                for (int i = 0; i < NUM_CHANNELS; i++) {
                    curNotePlaying[i] = -1;
                    toneOutputFreq(i, 0);
                }
            break;
        }
    }
}

void midiHandlerInit() {
    memset(curNotePlaying, -1, NUM_CHANNELS);

    midiSetCbNoteOn(midiHandlerNoteOn);
    midiSetCbNoteOff(midiHandlerNoteOff);
    midiSetCbCtrlChange(midiHandlerCtrlChange);
}