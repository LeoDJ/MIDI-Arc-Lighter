#include "midiHandler.h"
#include <math.h>
#include "usbd_midi_if.h"
#include "toneOutput.h"

typedef struct {
    int8_t note;    // MIDI note number, -1 for no note
    int8_t channel; // MIDI channel, currently not used
    float freq;     // calculated frequency, buffered for arpeggio
} curNote_t;

static curNote_t curNotePlaying[NUM_CONCURRENT_NOTES];
static curNote_t *curArpNotes[NUM_CHANNELS];

uint16_t pitchBend = 8192;

static int getCurNoteIdx(uint8_t note) {
    for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
        if (curNotePlaying[i].note == note) {
            return i;
        }
    }
    return -1;
}

static int getFreeCurNoteIdx() {
    for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
        if (curNotePlaying[i].note == -1) {
            return i;
        }
    }
    return -1;
}

// returns number of currently playing notes
static int getCurNoteNumPlaying() {
    uint8_t numNotesPlaying = 0;
    for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
        if (curNotePlaying[i].note != -1) {
            numNotesPlaying++;
        }
    }
    return numNotesPlaying;
}

curNote_t *getCurNote(uint8_t pos) {
    uint8_t iteratorActiveNotes = 0;
    for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
        if (curNotePlaying[i].note != -1) {
            if (iteratorActiveNotes == pos) {
                return &curNotePlaying[i];
            }
            iteratorActiveNotes++; 
        }
    }
    return NULL;
}

// float implementation is uses ~14kB Flash, powf is 7.5k of that. TODO: optimize?
static float calculateFrequency(uint8_t noteNum) {
    // formula from https://dsp.stackexchange.com/a/1650/57059
    // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    float val = 440.0f * powf(2, ((noteNum - 69) / 12.0f) + ((pitchBend - 8192.0f) / (4096 * 12)));
    // HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    return val;
}

void midiHandlerNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
    int idx = getFreeCurNoteIdx();
    printf("(%8ld) [MIDI] On:  %3d, Slot: %d", HAL_GetTick(), note, idx);
    if (idx != -1) {
        curNotePlaying[idx].note = note;
        curNotePlaying[idx].channel = ch; // currently unused
        curNotePlaying[idx].freq = calculateFrequency(note);

        // only enable note when arpeggio isn't engaged, else do nothing and let the arpeggiator handle it
        if (getCurNoteNumPlaying() <= NUM_CHANNELS) {
            toneOutputWrite(idx, curNotePlaying[idx].freq);
        }
    }
    else {
        printf(" XXX");
    }
    printf("\n");
}

void midiHandlerNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
    printf("(%8ld) [MIDI] Off: %3d\n", HAL_GetTick(), note);
    int idx = getCurNoteIdx(note);
    if (idx != -1) {
        curNotePlaying[idx].note = -1;
        // only disable note when arpeggio isn't engaged, else do nothing and let the other note play
        if (getCurNoteNumPlaying() <= NUM_CHANNELS) {
            toneOutputWrite(idx, 0);
        }
    }
}

void midiHandlerPitchBend(uint8_t ch, uint16_t bendVal) {
    // printf("(%8ld) [MIDI] PitchBend: %5d\n", HAL_GetTick(), bendVal);
    pitchBend = bendVal;    // update global variable
    // pitch bend all currently plaing notes
    uint8_t numNotesPlaying = getCurNoteNumPlaying();
    for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
        curNote_t *note = &curNotePlaying[i];
        if (note->note != -1) {
            note->freq = calculateFrequency(note->note); // recalculate note frequency
            // only update note when arpeggio isn't engaged, else do nothing and let the arpeggiator handle it
            if (numNotesPlaying <= 2) {
                toneOutputWrite(i, note->freq, false);
            }
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
                for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
                    curNotePlaying[i].note = -1;
                    toneOutputWrite(i, 0);
                }
            break;
        }
    }
}

void midiHandlerInit() {
    // intialize currently playing notes as off
    for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
        curNotePlaying[i].note = -1;
    }

    midiSetCbNoteOn(midiHandlerNoteOn);
    midiSetCbNoteOff(midiHandlerNoteOff);
    midiSetCbCtrlChange(midiHandlerCtrlChange);
    midiSetCbPitchBend(midiHandlerPitchBend);
}

uint32_t lastArpSwitch = 0;
uint8_t arpCounter = 0;
uint8_t prevNumNotesPlaying = 0;


// Arpeggiator loop
// TODO: handle pitchbend without delay
void midiHandlerArpLoop() {
    if (HAL_GetTick() - lastArpSwitch >= ARPEGGIO_MS) {
        lastArpSwitch = HAL_GetTick();
        uint8_t numNotesPlaying = getCurNoteNumPlaying();
        if (numNotesPlaying > 2) {
            uint8_t arpeggiatorSteps = (numNotesPlaying + NUM_CHANNELS - 1) / NUM_CHANNELS; // calculate number of arp steps

            // do arpeggio
            uint8_t ch = 0;
            for (int i = arpCounter * NUM_CHANNELS; i < (arpCounter + 1) * NUM_CHANNELS; i++) {
                if (i < numNotesPlaying) {
                    curNote_t *note = getCurNote(i);
                    if (note != NULL) {
                        printf("(%8ld) [ARP] numNotes: %d, arpSteps: %d, arpCounter: %d, i: %d, ch: %d, note: %d\n", HAL_GetTick(), numNotesPlaying, arpeggiatorSteps, arpCounter, i, ch, note->note);
                        toneOutputWrite(ch, note->freq, false);
                        ch++;
                    }
                }
            }

            arpCounter++;
            if (arpCounter >= arpeggiatorSteps) {
                arpCounter = 0;
            }
        }

        // if arp gets disabled, restore the only two playing notes
        if (numNotesPlaying == NUM_CHANNELS && prevNumNotesPlaying > NUM_CHANNELS) {
            uint8_t ch = 0;
            for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
                if (curNotePlaying[i].note != -1) {
                    toneOutputWrite(ch, curNotePlaying[i].freq, false);
                    ch++;
                }
            }
        }

        prevNumNotesPlaying = numNotesPlaying;
    }
}