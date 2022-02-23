#include <stdint.h>
#include "fatfs.h"

#pragma pack(push, 1)
typedef struct {
    uint32_t deltaT;
    union {
        struct {
            uint8_t channel : 4;
            uint8_t command : 4;
        };
        uint8_t statusByte;
    };
    union {
        struct {
            uint8_t param1;
            uint8_t param2;
        };
        struct {
            uint8_t metaType;   // optional
            uint32_t length;
            uint8_t *buf;       // has to be at end of union so it doesn't collide with param1/2 and messes with NULL checks
        };
    };
} midiTrackEvent_t;
#pragma pack(pop)

class MidiFile_TrackParser {
    public:
    MidiFile_TrackParser(FIL *midiFile, uint32_t startPos, uint32_t len);
    midiTrackEvent_t getNextEvent();    // don't forget to free evt.buf if exists!

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