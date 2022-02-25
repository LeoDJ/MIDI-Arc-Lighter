#include "midiFile.h"
#include "midiHandler.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


int MidiFile::openFile(const char* path) {
    // reset relevant internal variables
    _curPlaying = false;
    _curMidiTick = 0;
    _lastTempoChangeTick = 0;
    _usPerQuarter = 500000;
    _finishedTracks = 0;
    calcTickTime();

    printf("[MIDIF] Opening file %s\n", path);
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

    return parseRes;

    // printf("[MIDIF] Header parsing successful. tracks: %d, usPerTick: %d\n", _numTracks, _usPerTick);
    // for (int i = 0; i < _numTracks; i++) {
    //     midiTrack_t t = tracks[i];
    //     printf("[MIDIF] Track %d: start: %d, length: %d, curFilePos: %d\n", i, t.startFilePos, t.length, t.curFilePos);
    // }
}

int MidiFile::closeFile() {
    FRESULT res = f_close(&_midiFile);
    clearTrackParsers();
    return res;
}

void MidiFile::play() {
    if (!_curPlaying) {
        _lastTempoChangeTime = HAL_GetTick();
        _curPlaying = true;
    }
}
void MidiFile::pause() {
    // _lastTempoChangeTime = HAL_GetTick(); // TODO: time calculation
    _curPlaying = false;
}

void MidiFile::handleEvent(midiTrackEvent_t evt) {
    switch (evt.command) {
        case 0:         // invalid event or end of track
            break;
        case 0x8: {    // Note Off
            midiHandlerNoteOff(evt.channel, evt.param1, evt.param2);
            printf("(%8ld) [MIDIF] Event: NoteOff %d, %02X, %02X\n", HAL_GetTick(), evt.channel, evt.param1, evt.param2);
            break;
        }
        case 0x9: {    // Note On
            midiHandlerNoteOn(evt.channel, evt.param1, evt.param2);
            printf("(%8ld) [MIDIF] Event: NoteOn  %d, %02X, %02X\n", HAL_GetTick(), evt.channel, evt.param1, evt.param2);
            break;
        }
        case 0xB: {    // Control Change
            midiHandlerCtrlChange(evt.channel, evt.param1, evt.param2);
            printf("(%8ld) [MIDIF] Event: CtrlChg %d, %02X, %02X\n", HAL_GetTick(), evt.channel, evt.param1, evt.param2);
            break;
        }
        case 0xE: {    // Pitch Bend
            midiHandlerPitchBend(evt.channel, evt.param2 << 7 | evt.param1);
            printf("(%8ld) [MIDIF] Event: PitchBd %d, %5d\n", HAL_GetTick(), evt.channel, evt.param2 << 7 | evt.param1);
            break;
        }
        case 0xF: {    // SysEx or Meta
            switch (evt.statusByte) {
                case 0xFF: {                // Meta Event
                    switch (evt.metaType) { // Meta Event Type
                        case 0x20: {
                            printf("(%8ld) [MIDIF] Error! Unhandled MIDI Channel Prefix Meta Event\n", HAL_GetTick());
                            break;
                        }
                        case 0x51: {        // Set Tempo
                            if (evt.length == 3) {
                                _usPerQuarter = evt.buf[0] << 16 | evt.buf[1] << 8 | evt.buf[2];
                                printf("(%8ld) [MIDIF] Event: SetTemp %6d (%02X %02X %02X)\n", HAL_GetTick(), _usPerQuarter, evt.buf[0], evt.buf[1], evt.buf[2]);
                                calcTickTime();
                            }    
                            break;
                        }
                        case 0x2F: {        // End Of Track
                            _finishedTracks++;
                            printf("(%8ld) [MIDIF] Event: EOTrack, sum: %d\n", HAL_GetTick(), _finishedTracks);
                            if (_finishedTracks >= _numTracks) {
                                _curPlaying = false;
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }


    if (evt.buf != NULL) {
        free(evt.buf);
        evt.buf = NULL;
    }
}

void MidiFile::process() {
    if (_curPlaying) {
        _curMidiTick = _lastTempoChangeTick + ((uint64_t)(HAL_GetTick() - _lastTempoChangeTime) * 1000 / _usPerTick);

        for (int i = 0; i < _numTracks; i++) {
            uint32_t nextTrackMidiTick = _trackStatus[i].curMidiTick + _trackStatus[i].evt.deltaT;
            if (_curMidiTick >= nextTrackMidiTick && _trackStatus[i].evt.statusByte != 0) {
                handleEvent(_trackStatus[i].evt);
                printf("Finished parsing Event\n");
                _trackStatus[i].evt = _trackParser[i]->getNextEvent();  // fetch upcoming event
                printf("(%8ld) [MIDIF] Track %d cache event - dt: %5ld, status: %02X, curTick: %d, nextTick: %d\n", HAL_GetTick(), i, _trackStatus[i].evt.deltaT, _trackStatus[i].evt.statusByte, _curMidiTick, nextTrackMidiTick);
                _trackStatus[i].curMidiTick = nextTrackMidiTick;
            }
        }
    }
}

void MidiFile::clearTrackParsers() {
    for (int i = 0; i < MAX_MIDI_TRACKS; i++) {
        if (_trackParser[i] != NULL) {
            if (_trackStatus[i].evt.buf != NULL) {
                free(_trackStatus[i].evt.buf);
                _trackStatus[i].evt.buf = NULL;
            }
            delete _trackParser[i];
            _trackParser[i] = NULL;
        }
    }
}

midiChunk_t MidiFile::readChunkHeader() {
    midiChunk_t chunk;
    unsigned int readBytes;

    // read header chunk information
    FRESULT res = f_read_retry(&_midiFile, &chunk, sizeof(midiChunk_t), &readBytes);
    if (res != FR_OK || readBytes != sizeof(midiChunk_t)) {
        chunk.length = 0;   // signal, that reading the chunk header failed
    }

    return chunk;
}

void MidiFile::calcTickTime() {
    // _lastTempoChangeTime = HAL_GetTick();
    _lastTempoChangeTime += ((_curMidiTick - _lastTempoChangeTick) * _usPerTick) / 1000;
    _lastTempoChangeTick = _curMidiTick;
    _usPerTick = _usPerQuarter / _ticksPerQuarterNote;
}

int MidiFile::parseHeader() {
    unsigned int readBytes;
    FRESULT res;

    // read header chunk header information
    midiChunk_t headerChunk = readChunkHeader();
    if (headerChunk.length == 0) {
        closeFile();
        return -1;
    }
    // check if header string is correct
    if (strncmp(headerChunk.type, MIDI_CHUNK_TYPE_HEADER, 4) != 0) {
        // not a MIDI file
        return -1;
    }

    // read header
    uint8_t midiHeaderBuf[(uint16_t)headerChunk.length];
    res = f_read_retry(&_midiFile, &midiHeaderBuf, headerChunk.length, &readBytes);
    if (res != FR_OK || readBytes != headerChunk.length) {
        // read failed
        closeFile();
        return -1;
    }
    midiHeader_t *midiHeader = (midiHeader_t *)midiHeaderBuf;

    if (midiHeader->format >= 2) {
        // only support formats 0 and 1
        closeFile();
        return -1;
    }

    _numTracks = midiHeader->tracks;
    if (_numTracks > MAX_MIDI_TRACKS) { // limit track number
        printf("(%8ld) [MIDIF] Warning! Too many channels (%d), limited to %d.\n", HAL_GetTick(), _numTracks, MAX_MIDI_TRACKS);
        _numTracks = MAX_MIDI_TRACKS;
    }

    if ((midiHeader->division & 0x8000) == 1) {
        // SMTPE frame timing not supported (yet)
        closeFile();
        return -1;
    }
    else { // pulses per quarter note
        _ticksPerQuarterNote = midiHeader->division;
        calcTickTime();
    }

    // find tracks
    // memset(tracks, 0, sizeof(tracks));
    clearTrackParsers();
    for (int i = 0; i < _numTracks; i++) {
        midiChunk_t trackChunkHeader = readChunkHeader();
        if (trackChunkHeader.length == 0 || strncmp(trackChunkHeader.type, MIDI_CHUNK_TYPE_TRACK, 4) != 0) {
            // error during parsing of tracks
            closeFile();
            return -1;
        }
        uint32_t startFilePos = _midiFile.fptr;    // read beginning offset of track segment
        uint32_t length = trackChunkHeader.length;
        printf("[MIDIF] Init track parser %d, start: %ld, len: %ld\n", i, startFilePos, length);
        fflush(stdout);
        _trackParser[i] = new MidiFile_TrackParser(&_midiFile, startFilePos, length);
        _trackStatus[i].evt = _trackParser[i]->getNextEvent();  // get initial event

        // calculate offset to next chunk and seek to that
        uint32_t nextChunk = startFilePos + length;  
        res = f_lseek_retry(&_midiFile, nextChunk);
        if (res != FR_OK) {
            // seeking to next chunk failed
            closeFile();
            return -1;
        }
    }

    return 0;
}