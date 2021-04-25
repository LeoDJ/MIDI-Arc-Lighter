#include "midiHandler.h"
#include <math.h>
#include "usbd_midi_if.h"
#include "toneOutput.h"

// -1 for no note playing
static int8_t curNotePlaying[NUM_CHANNELS];

uint16_t pitchBend = 8192;

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

// float implementation is uses ~14kB Flash. TODO: optimize?
static float calculateFrequency(uint8_t noteNum) {
    // formula from https://dsp.stackexchange.com/a/1650/57059
    // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    float val = 440.0f * powf(2, ((noteNum - 69) / 12.0f) + ((pitchBend - 8192.0f) / (4096 * 12)));
    // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    return val;
}

void midiHandlerNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
    printf("(%8ld) [MIDI] On:  %3d", HAL_GetTick(), note);
    int toneCh = getFreeChannelSlot();
    if (toneCh != -1) {
        curNotePlaying[toneCh] = note;
        // toneOutputNote(toneCh, note);
        toneOutputWrite(toneCh, calculateFrequency(note));
    }
    else {
        printf(" XXX");
    }
    printf("\n");
}

void midiHandlerNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
    printf("(%8ld) [MIDI] Off: %3d\n", HAL_GetTick(), note);
    int toneCh = getChannelByNote(note);
    if (toneCh != -1) {
        curNotePlaying[toneCh] = -1;
        toneOutputWrite(toneCh, 0);
    }
}

void midiHandlerPitchBend(uint8_t ch, uint16_t bendVal) {
    // printf("(%8ld) [MIDI] PitchBend: %5d\n", HAL_GetTick(), bendVal);
    pitchBend = bendVal;    // update global variable
    // pitch bend all currently plaing notes
    // printf("Dumb\n");
    for (int i = 0; i < NUM_CHANNELS; i++) {
        int8_t note = curNotePlaying[i];
        if (note != -1) {
            toneOutputWrite(i, calculateFrequency(note), false);
        }
    }
}

void midiHandlerCtrlChange(uint8_t ch, uint8_t num, uint8_t value) {
    // printf("(%8d) [MIDI] Ctrl: ch: %2d, %02X %02X\n", HAL_GetTick(), midi.channel, midi.byte1, midi.byte2);
    if (ch == 0) { // only do parsing for ch1 for now as MidiEditor sends the same commands to all 16 channels
        switch (num) {
            case MidiController::ControllerReset:
                pitchBend = 8192;   // reset pitchbend value
                // fall through
            case MidiController::AllSoundsOff:
            case MidiController::AllNotesOff:
                // turn off all channels
                for (int i = 0; i < NUM_CHANNELS; i++) {
                    curNotePlaying[i] = -1;
                    toneOutputWrite(i, 0);
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
    midiSetCbPitchBend(midiHandlerPitchBend);
}