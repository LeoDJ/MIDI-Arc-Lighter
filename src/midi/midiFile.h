
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

typedef struct {
    uint32_t curMidiTick;
    midiTrackEvent_t evt;   // next event
} midiTrackStatus_t;


class MidiFile {
    public:
    int openFile(const char* path);
    int closeFile();
    void process();
    void play();
    void pause();

    private:
    void clearTrackParsers();
    midiChunk_t readChunkHeader();
    int parseHeader();
    void calcTickTime();
    void handleEvent(midiTrackEvent_t evt);

    FIL _midiFile;
    uint16_t _numTracks;
    bool _curPlaying = false;
    uint32_t _curMidiTick = 0;
    uint32_t _lastTempoChangeTime;      // ms
    uint32_t _lastTempoChangeTick = 0;  // MIDI ticks
    uint32_t _usPerTick;
    uint32_t _ticksPerQuarterNote;
    uint32_t _usPerQuarter = 500000;    // default 120 BPM
    uint8_t _finishedTracks = 0;
    MidiFile_TrackParser *_trackParser[MAX_MIDI_TRACKS] = {NULL};
    midiTrackStatus_t _trackStatus[MAX_MIDI_TRACKS] = {0};
};


