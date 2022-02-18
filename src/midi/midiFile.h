
#include "fatfs.h"
#include "util.h"

#define MAX_MIDI_TRACKS 8

typedef struct {
    char type[4];
    be_uint32_t length;
    uint8_t data[];
} midiChunk_t;

typedef struct {
    be_uint16_t format;
    be_uint16_t tracks;
    be_uint16_t division;
} midiHeader_t;

typedef struct {
    uint32_t startFilePos;  // offset in file
    uint32_t length;        // length of track
    uint32_t curFilePos;    // current position in file
} midiTrack_t;

class MidiFile {
    public:
    int openFile(const char* path);
    int closeFile();

    private:
    int parseHeader();
    midiChunk_t readChunkHeader();
    int parseNextEvent();
    uint32_t readVarLen();

    FIL _midiFile;
    uint16_t _numTracks;
    uint32_t _usPerTick;
    uint32_t _usPerQuarter = 500000; // default 120 BPM
    midiTrack_t tracks[MAX_MIDI_TRACKS];
};

#define MIDI_CHUNK_TYPE_HEADER  "MThd"
#define MIDI_CHUNK_TYPE_TRACK   "MTrk"

