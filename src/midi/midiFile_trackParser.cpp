#include "midiFile_trackParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include "util.h"

MidiFile_TrackParser::MidiFile_TrackParser(FIL *midiFile, uint32_t startPos, uint32_t len, uint8_t *readBuf, uint16_t readBufLen) {
    _midiFile = midiFile;
    _startFilePos = startPos;
    _length = len;
    _curFilePos = _startFilePos;
    _readBuf = readBuf;
    _readBufLen = readBufLen;
    readFileIntoBuffer();   // initialize read buffer contents
}

void MidiFile_TrackParser::readFileIntoBuffer() {
    uint32_t remainingTrackLength = _length - (_curFilePos - _startFilePos);
    uint8_t bytesToRead = MIN(remainingTrackLength, _readBufLen);
    // end of buffer detection is done at getNextEvent()

    f_lseek_retry(_midiFile, _curFilePos);  // restore file position

    UINT bytesRead;
    f_read_retry(_midiFile, _readBuf, bytesToRead, &bytesRead);
    _bytesInReadBuf = bytesRead;

    _curFilePos = _midiFile->fptr;          // save file position
}

// read single byte from file
uint8_t MidiFile_TrackParser::readByte() {
    if (_readBufPos >= _readBufLen) {
        readFileIntoBuffer();
        _readBufPos = 0;
    }
    return _readBuf[_readBufPos++];
}

void MidiFile_TrackParser::skipBytes(uint32_t len) {
    if (len > _readBufLen) {
        uint32_t bytesLeftInBuf = _readBufLen - _readBufPos - 1;
        f_lseek_retry(_midiFile, _curFilePos - bytesLeftInBuf + len);
        _curFilePos = _midiFile->fptr;
        // refill buffer
        _readBufPos = 0;
        readFileIntoBuffer();
    }
    else {
        for (uint32_t i = 0; i < len; i++) {
            readByte();
        }
    }
}

void MidiFile_TrackParser::readBytes(uint8_t *dstBuf, uint32_t len) {
    if (len > _readBufLen) {
        uint32_t bytesLeftInBuf = _readBufLen - _readBufPos - 1;
        // seek back
        f_lseek_retry(_midiFile, _curFilePos - bytesLeftInBuf);
        UINT bytesRead;
        f_read_retry(_midiFile, dstBuf, len, &bytesRead);
        _curFilePos = _midiFile->fptr;

        // refill buffer
        _readBufPos = 0;
        readFileIntoBuffer();
    }
    else {
        for (uint32_t i = 0; i < len; i++) {
            dstBuf[i] = readByte();
        }
    }
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
    // printf("(%8ld) [MIDIFTP] _curFilePos: %ld, _readBufPos: %d\n", HAL_GetTick(), _curFilePos, _readBufPos);
    uint32_t trackEnd = _startFilePos + _length;
    uint32_t curReadPos = _curFilePos - (MIN(_bytesInReadBuf, _length) - _readBufPos - 1);  // get read position regarding remaining bytes in buffer
    if (curReadPos >= trackEnd) {
        midiTrackEvent_t evt = {0};
        evt.statusByte = 0; // indicate end of track by zero status byte
        // printf("(%8ld) [MIDIFTP] END OF TRACK LENGTH\n", HAL_GetTick());
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
                skipBytes(evt.length);
                // signal read error with zero length
                evt.length = 0;
            }
            else {
                readBytes(evt.buf, evt.length);
            }
            break;
        }
    }
    return evt;
}