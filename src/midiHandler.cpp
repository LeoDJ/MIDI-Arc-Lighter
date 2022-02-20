#include "midiHandler.h"
#include <math.h>
#include "usbd_midi_if.h"
#include "toneOutput.h"

typedef struct {
    int8_t note;    // MIDI note number, -1 for no note
    int8_t channel; // MIDI channel, currently not used
    bool newNote;   // set to true if this note was just received
    float freq;     // calculated frequency, buffered for arpeggio
} curNote_t;

static curNote_t curNotePlaying[NUM_CONCURRENT_NOTES];

uint16_t pitchBend = 8192;
uint32_t lastArpSwitch = 0;
uint8_t arpCounter = 0;
uint8_t prevNumNotesPlaying = 0;

// function delcarations
void midiHandlerNoteOff(uint8_t ch, uint8_t note, uint8_t vel);

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

void printCurNotes() {
    printf("(%8ld) ", HAL_GetTick());
    for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
        int8_t n = curNotePlaying[i].note;
        if (n > -1) 
            printf("%3d ", curNotePlaying[i].note);
        else
            printf("  . ");
    }
    printf("  %d", arpCounter);
    printf("   %s", getCurNoteNumPlaying() > 2 ? "ARP" : (getCurNoteNumPlaying() == 0 ? "Off" : "Normal"));
    printf("\n");
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
    // By definition, a note-on message with vv=0 is equivalent to the message: "note-off vv=40"
    if (vel == 0) {
        midiHandlerNoteOff(ch, note, 0x40);
        return;
    }

    int idx = getFreeCurNoteIdx();
    printf("(%8ld) [MIDI] On:  %3d, Slot: %2d, Midi Ch: %2d", HAL_GetTick(), note, idx, ch);
    if (idx != -1) {
        curNotePlaying[idx].note = note;
        curNotePlaying[idx].channel = ch; // currently unused
        curNotePlaying[idx].newNote = true;
        curNotePlaying[idx].freq = calculateFrequency(note);

        // printCurNotes();
        midiHandlerUpdateNotes(); // trigger update
    }
    else {
        printf(" XXX");
    }
    printf("\n");
}

void midiHandlerNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
    int idx = getCurNoteIdx(note);
    printf("(%8ld) [MIDI] Off: %3d, Slot: %2d, Midi Ch: %2d\n", HAL_GetTick(), note, idx, ch);
    if (idx != -1) {
        curNotePlaying[idx].note = -1;
        // printCurNotes();
        midiHandlerUpdateNotes(); // trigger update
    }
}

void midiHandlerPitchBend(uint8_t ch, uint16_t bendVal) {
    // printf("(%8ld) [MIDI] PitchBend: %5d\n", HAL_GetTick(), bendVal);
    pitchBend = bendVal;    // update global variable
    // pitch bend all currently plaing notes
    // uint8_t numNotesPlaying = getCurNoteNumPlaying();
    for (int i = 0; i < NUM_CONCURRENT_NOTES; i++) {
        curNote_t *note = &curNotePlaying[i];
        if (note->note != -1) {
            note->freq = calculateFrequency(note->note); // recalculate note frequency
            midiHandlerUpdateNotes(); // trigger update
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
                    midiHandlerUpdateNotes(); // trigger update
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

void midiHandlerUpdateNotes() {
    uint8_t numNotesPlaying = getCurNoteNumPlaying();
    if (numNotesPlaying == 0) { // disable all notes
        for (int i = 0; i < NUM_CHANNELS; i++) {
            toneOutputWrite(i, 0);
        }
    }
    else if (numNotesPlaying > 0 && numNotesPlaying <= NUM_CHANNELS) { // play the notes directly on the channels
        for (int i = 0; i < NUM_CHANNELS; i++) {
            curNote_t *note = getCurNote(i);
            if (note != NULL) {
                // workaround for note sliding down in channel and when sliding back up it doesn't get played anymore
                // probably won't work for NUM_CHANNELS > 2 and also might not fix some slight arpeggio glitching, but it works for now
                // otherwise I'd need to handle new note events and currently playing notes completely differently, that's a TODO for another time
                if (numNotesPlaying == NUM_CHANNELS && prevNumNotesPlaying == 1 && i == 1) {
                    note->newNote = true;
                }
                // printf("\n%d %d\n", i, note->note);
                toneOutputWrite(i, note->freq, note->newNote);
                note->newNote = false;
            }
            else {
                // printf("\n%d NULL\n", i);
                toneOutputWrite(i, 0); // deactivate other channels, TODO: deal with notes sliding between channels
            }
        }
        arpCounter = 0; // reset ARP counter
    }
    else { // do arpeggiator
        if (HAL_GetTick() - lastArpSwitch >= ARPEGGIO_MS) {
            lastArpSwitch = HAL_GetTick();
            uint8_t numNotesPlaying = getCurNoteNumPlaying();
            if (numNotesPlaying > 2) {
                uint8_t arpeggiatorSteps = (numNotesPlaying + NUM_CHANNELS - 1) / NUM_CHANNELS; // calculate number of arp steps
                
                // reset ARP counter at the beginning, so it also gets reset if it was left too high
                if (arpCounter >= arpeggiatorSteps) {
                    arpCounter = 0;
                }

                // do arpeggio
                uint8_t ch = 0;
                for (int i = arpCounter * NUM_CHANNELS; i < (arpCounter + 1) * NUM_CHANNELS; i++) {
                    if (i < numNotesPlaying) {
                        curNote_t *note = getCurNote(i);
                        if (note != NULL) {
                            // printf("(%8ld) [ARP] numNotes: %d, arpSteps: %d, arpCounter: %d, i: %d, ch: %d, note: %d\n", HAL_GetTick(), numNotesPlaying, arpeggiatorSteps, arpCounter, i, ch, note->note);
                            toneOutputWrite(ch, note->freq, false);
                            ch++;
                        }
                    }
                }

                arpCounter++;
            }
        }
    }
    prevNumNotesPlaying = numNotesPlaying;
}

// Arpeggiator loop
// TODO: handle pitchbend without delay
void midiHandlerArpLoop() {
    if (HAL_GetTick() - lastArpSwitch >= ARPEGGIO_MS && getCurNoteNumPlaying() > NUM_CHANNELS) {
        midiHandlerUpdateNotes();
    }
}