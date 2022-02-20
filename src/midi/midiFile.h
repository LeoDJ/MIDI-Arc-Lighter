
#include "fatfs.h"
#include "util.h"
#include "midiFile_trackParser.h"

#define MAX_MIDI_TRACKS 8

typedef struct {
    char type[4];
    be_uint32_t length;
    uint8_t data[];
} midiChunk_t;

#define MIDI_CHUNK_TYPE_HEADER  "MThd"
#define MIDI_CHUNK_TYPE_TRACK   "MTrk"

typedef struct {
    be_uint16_t format;
    be_uint16_t tracks;
    be_uint16_t division;
} midiHeader_t;


class MidiFile {
    public:
    int openFile(const char* path);
    int closeFile();
    void process();

    private:
    void clearTrackParsers();
    midiChunk_t readChunkHeader();
    int parseHeader();

    FIL _midiFile;
    uint16_t _numTracks;
    uint32_t _usPerTick;
    uint32_t _usPerQuarter = 500000; // default 120 BPM
    MidiFile_TrackParser *trackParser[MAX_MIDI_TRACKS] = {NULL};
};


