#include "midiFile.h"
#include <string.h>
#include <stdio.h>


int MidiFile::openFile(const char* path) {
    // printf("[MIDIF] Opening file %s\n", path);
    FRESULT res = f_open(&_midiFile, path, FA_READ);
    if (res != FR_OK) {
        printf("[MIDIF] File open failed.\n");
        return res;
    }

    // parse header and save meta information
    int parseRes = parseHeader();
    if (parseRes < 0) {
        printf("[MIDIF] Header parsing failed.\n");
    }

    // printf("[MIDIF] Header parsing successful. tracks: %d, usPerTick: %d\n", _numTracks, _usPerTick);
    // for (int i = 0; i < _numTracks; i++) {
    //     midiTrack_t t = tracks[i];
    //     printf("[MIDIF] Track %d: start: %d, length: %d, curFilePos: %d\n", i, t.startFilePos, t.length, t.curFilePos);
    // }
}

int MidiFile::closeFile() {
    FRESULT res = f_close(&_midiFile);
    return res;
}

midiChunk_t MidiFile::readChunkHeader() {
    midiChunk_t chunk;
    unsigned int readBytes;

    // read header chunk information
    FRESULT res = f_read(&_midiFile, &chunk, sizeof(midiChunk_t), &readBytes);
    if (res != FR_OK || readBytes != sizeof(midiChunk_t)) {
        chunk.length = 0;   // signal, that reading the chunk header failed
    }

    return chunk;
}

int MidiFile::parseHeader() {
    unsigned int readBytes;
    FRESULT res;

    // read header chunk header information
    midiChunk_t headerChunk = readChunkHeader();
    if (headerChunk.length == 0) {
        return -1;
    }
    // check if header string is correct
    if (strncmp(headerChunk.type, MIDI_CHUNK_TYPE_HEADER, 4) != 0) {
        // not a MIDI file
        return -1;
    }

    // read header
    uint8_t midiHeaderBuf[(uint16_t)headerChunk.length];
    res = f_read(&_midiFile, &midiHeaderBuf, headerChunk.length, &readBytes);
    if (res != FR_OK || readBytes != headerChunk.length) {
        // read failed
        return -1;
    }
    midiHeader_t *midiHeader = (midiHeader_t *)midiHeaderBuf;

    if (midiHeader->format >= 2) {
        // only support formats 0 and 1
        return -1;
    }

    if (midiHeader->tracks > MAX_MIDI_TRACKS) {
        // too many MIDI tracks in file
        return -1;
    }
    _numTracks = midiHeader->tracks;

    if ((midiHeader->division & 0x8000) == 1) {
        // SMTPE frame timing not supported (yet)
        return -1;
    }
    else { // pulses per quarter note
        _usPerTick = midiHeader->division;
    }

    // find tracks
    memset(tracks, 0, sizeof(tracks));
    for (int i = 0; i < _numTracks; i++) {
        midiChunk_t trackChunkHeader = readChunkHeader();
        if (trackChunkHeader.length == 0 || strncmp(trackChunkHeader.type, MIDI_CHUNK_TYPE_TRACK, 4) != 0) {
            // error during parsing of tracks
            return -1;
        }
        tracks[i].startFilePos = _midiFile.fptr;    // save beginning offset of track segment
        tracks[i].curFilePos = tracks[i].startFilePos;
        tracks[i].length = trackChunkHeader.length;

        // calculate offset to next chunk and seek to that
        uint32_t nextChunk = tracks[i].startFilePos + tracks[i].length;  
        res = f_lseek(&_midiFile, nextChunk);
        if (res != FR_OK) {
            // seeking to next chunk failed
            return -1;
        }
    }

    return 0;
}

int MidiFile::parseNextEvent() {

}