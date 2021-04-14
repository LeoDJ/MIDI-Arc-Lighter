#include "usbd_midi_if.h"
#include "stm32f0xx_hal.h"

// Baisc MIDI RX/TX functions
static uint16_t midiRx(uint8_t *msg, uint16_t length);
static uint16_t midiTx(uint8_t *msg, uint16_t length);

inline void parseMidiPacket(uint8_t *pkt);

// callback variables
void (*cbNoteOff)(uint8_t ch, uint8_t note, uint8_t vel);
void (*cbNoteOn)(uint8_t ch, uint8_t note, uint8_t vel);
void (*cbCtlChange)(uint8_t ch, uint8_t num, uint8_t value);

USBD_MIDI_ItfTypeDef USBD_MIDI_Interface_fops_FS = {
    midiRx, 
    midiTx
};

static uint16_t midiRx(uint8_t *msg, uint16_t length) {
    // printf("[MIDI RX] ");
    // for (int i = 0; i < length; i++) {
    //     printf("%02X ", msg[i]);
    // }
    // printf("\n");

    // only handle 4 byte long usb midi packets
    if (length % 4 == 0) {
        for (uint16_t i = 0; i < length / 4; i++) {
            // directly parse midi message and call respective callbacks
            // hopefully this doesn't stall the USB interrupt too long
            parseMidiPacket(msg + i * 4);
        }
    }
    return 0;
}

static uint16_t midiTx(uint8_t *msg, uint16_t length) {
    uint32_t i = 0;
    while (i < length) {
        APP_Rx_Buffer[APP_Rx_ptr_in] = *(msg + i);
        APP_Rx_ptr_in++;
        i++;
        if (APP_Rx_ptr_in == APP_RX_DATA_SIZE) {
            APP_Rx_ptr_in = 0;
        }
    }
    return USBD_OK;
}

void parseMidiPacket(usbMidiEvent_t *pkt) {
    midiMessage_t midi = pkt->midiMsg;
    // printf("Parse MIDI packet: %02X %02X %02X %02X, status: %02X\n", pkt->raw8[0], pkt->raw8[1], pkt->raw8[2], pkt->raw8[3], midi.status);
    switch (midi.status) {
        case MidiStatus::NoteOff:
            if (cbNoteOff != NULL) {
                cbNoteOff(midi.channel, midi.byte1, midi.byte2);
            }
        break;
        case MidiStatus::NoteOn:
            if (midi.byte2 != 0) {
                if (cbNoteOn != NULL) {
                    cbNoteOn(midi.channel, midi.byte1, midi.byte2);
                }
            }
            else {  // if note velocity == 0, turn off note
                if (cbNoteOff != NULL) {
                    cbNoteOff(midi.channel, midi.byte1, midi.byte2);
                }
            }
        break;
        case MidiStatus::PitchBend:
            // TODO: implement
        break;
        case MidiStatus::CtrlChange:
            if (cbCtlChange != NULL) {
                cbCtlChange(midi.channel, midi.byte1, midi.byte2);
            }
        break;
    }
}

// expects a pointer to a 4 byte USB MIDI event message
inline void parseMidiPacket(uint8_t *pkt) {
    parseMidiPacket((usbMidiEvent_t *)pkt);
}

void midiSendNoteOn(uint8_t ch, uint8_t note, uint8_t vel) {
    usbMidiEvent_t packet;
    packet.codeIndexNumber = MidiCIN::NoteOn;
    packet.midiMsg.status = MidiStatus::NoteOn;
    packet.midiMsg.channel = ch;
    packet.midiMsg.byte1 = note;
    packet.midiMsg.byte2 = vel;
    midiTx((uint8_t *)&packet, sizeof(usbMidiEvent_t));
}

void midiSendNoteOff(uint8_t ch, uint8_t note) {
    usbMidiEvent_t packet;
    packet.codeIndexNumber = MidiCIN::NoteOff;
    packet.midiMsg.status = MidiStatus::NoteOff;
    packet.midiMsg.channel = ch;
    packet.midiMsg.byte1 = note;
    packet.midiMsg.byte2 = 0;
    midiTx((uint8_t *)&packet, sizeof(usbMidiEvent_t));
}

void midiSendCtrlChange(uint8_t ch, uint8_t num, uint8_t value) {
    usbMidiEvent_t packet;
    packet.codeIndexNumber = MidiCIN::ControlChange;
    packet.midiMsg.status = MidiStatus::CtrlChange;
    packet.midiMsg.channel = ch;
    packet.midiMsg.byte1 = num;
    packet.midiMsg.byte2 = value;
    midiTx((uint8_t *)&packet, sizeof(usbMidiEvent_t));
}

void midiSetCbNoteOff(void (*cb)(uint8_t ch, uint8_t note, uint8_t vel)) {
    cbNoteOff = cb;
}
void midiSetCbNoteOn(void (*cb)(uint8_t ch, uint8_t note, uint8_t vel)) {
    cbNoteOn = cb;
}
void midiSetCbCtrlChange(void (*cb)(uint8_t ch, uint8_t num, uint8_t value)) {
    cbCtlChange = cb;
}