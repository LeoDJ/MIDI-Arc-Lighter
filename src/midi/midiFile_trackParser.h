#include <stdint.h>
#include "fatfs.h"

typedef struct {
    uint32_t deltaT;
    uint8_t statusByte;
    union {
        struct {
            uint8_t param1;
            uint8_t param2;
        };
        struct {
            uint8_t metaType;   // optional
            uint8_t *buf;
            uint32_t length;
        };
    };
} midiTrackEvent_t;

class MidiFile_TrackParser {
    public:
    MidiFile_TrackParser(FIL *midiFile, uint32_t startPos, uint32_t len);
    midiTrackEvent_t getNextEvent();

    private:
    midiTrackEvent_t parseEvent();
    uint32_t readVarLen();
    uint8_t readByte();

    FIL *_midiFile;
    uint32_t _startFilePos;     // offset in file
    uint32_t _length;           // length of track
    uint32_t _curFilePos;       // current position in file
    uint8_t _prevStatusByte;    // save previous status byte for running status functionality
};