#include "midiFile_trackParser.h"
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

MidiFile_TrackParser::MidiFile_TrackParser(FIL *midiFile, uint32_t startPos, uint32_t len, uint8_t *readBuf, size_t readBufLen) {
    _midiFile = midiFile;
    _startFilePos = startPos;
    _length = len;
    _curFilePos = _startFilePos;
    _readBuf = readBuf;
    _readBufLen = readBufLen;
    readFileIntoBuffer();   // initialize read buffer contents
}

void MidiFile_TrackParser::readFileIntoBuffer() {
    uint32_t remainingFileLength = _length - (_curFilePos - _startFilePos);
    uint8_t bytesToRead = (remainingFileLength < _readBufLen) ? remainingFileLength : _readBufLen;
    // end of buffer detection is done at getNextEvent()

    f_lseek_retry(_midiFile, _curFilePos);  // restore file position

    UINT readBytes;
    f_read_retry(_midiFile, _readBuf, bytesToRead, &readBytes);

    _curFilePos = _midiFile->fptr;          // save file position
}

// read single byte from file
uint8_t MidiFile_TrackParser::readByte() {
    if (_readBufPos >= _readBufLen) {
        readFileIntoBuffer();
    }
    return _readBuf[_readBufPos++];
}

// read variable-length quantity
uint32_t MidiFile_TrackParser::readVarLen() {
    uint32_t returnVal = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t val = readByte();
        returnVal <<= 7;
        returnVal |= val & 0x7F;

        // upper bit not set indicates end of var len quantity
        if (!(val & 0x80)) {
            return returnVal;
        }
    }
    return returnVal | 0x80000000;  // set upper bit to indicate parsing error
}

midiTrackEvent_t MidiFile_TrackParser::getNextEvent() {
    if (_curFilePos - (_readBufLen - _readBufPos) >= _startFilePos + _length) {
        midiTrackEvent_t evt = {0};
        evt.statusByte = 0; // indicate end of file by zero status byte
        return evt;
    }

    // read delta time
    uint32_t deltaT = readVarLen();
    midiTrackEvent_t evt = parseEvent();
    evt.deltaT = deltaT;

    return evt;
}

midiTrackEvent_t MidiFile_TrackParser::parseEvent() {
    midiTrackEvent_t evt = {0};
    uint8_t nextByte = readByte();

    bool runningStatus = !(nextByte & 0x80);    // upper bit is 0 for running status message
    evt.statusByte = runningStatus ? _prevStatusByte : nextByte;
    _prevStatusByte = evt.statusByte;

    uint8_t param1 = 0;
    // don't read param1 for SysEx events because the read is handled by the readVarLen function
    // assumes SysEx events never use running status
    if (evt.statusByte != 0xF0 && evt.statusByte != 0xF7) {
        param1 = runningStatus ? nextByte : readByte();    
    }

    switch (evt.statusByte) {
        case 0x80 ... 0xBF:     // 2 parameter events
        case 0xE0 ... 0xEF: 
            evt.param2 = readByte();
            // fall through
        case 0xC0 ... 0xDF: {   // 1 parameter events
            evt.param1 = param1;
            break;
        }
        case 0xFF:              // Meta event
            evt.metaType = param1;
            // fall through
        case 0xF0:              // SysEx event
        case 0xF7: {
            evt.length = readVarLen();
            evt.buf = (uint8_t*)malloc(evt.length);
            // printf("[MIDIF] Free Heap: %d\n", estimateFreeHeap(16));
            if (evt.buf == NULL) {  // not enough memory
                // seek file to next event
                f_lseek_retry(_midiFile, _midiFile->fptr + evt.length);
                // signal read error with zero length
                evt.length = 0;
            }
            else {
                UINT readLen;
                f_read_retry(_midiFile, evt.buf, evt.length, &readLen);
            }
            break;
        }
    }
    return evt;
}