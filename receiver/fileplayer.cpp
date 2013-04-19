/* 
 * File:   fileplayer.cpp
 * Author: J.Huber, IRT GmbH
 * 
 * Created on Jule 3, 2012
 * Last modification: April 9, 2013
 */

#include <stdio.h>
#include <fcntl.h>
#include <vdr/recording.h>
#include <vdr/tools.h>
#include "fileplayer.h"

cFilePlayer *cFilePlayer::newInstance(const char* fileName){
    cFilePlayer *Player = new cFilePlayer(fileName);
    return Player;
}
cFilePlayer::~cFilePlayer() {
    delete this->mRecordingFile;
    delete [] this->mLastOffsets;
}

cFilePlayer::cFilePlayer(const char* fileName)  {
    MESSAGE(VERBOSE_SDK, "Created FilePlayer");

    this->mRecordingFile = new cFileName(/*this->mRecording->FileName()*/(char*)fileName, false, false, false);
    this->mLastOffsets = new off_t[VDR_MAX_FILES_PER_TSRECORDING+1];
    this->scanLastOffsets();
}

void cFilePlayer::open(UpnpOpenFileMode){
    // Open() does not work?!
    this->mCurrentFile = this->mRecordingFile->SetOffset(1);
    if(this->mCurrentFile){
        MESSAGE(VERBOSE_RECORDS, "File player opened");
    }
    else {
        ERROR("Error while opening file player file");
    }
}

void cFilePlayer::close(){
    this->mRecordingFile->Close();
}

int cFilePlayer::write(char*, size_t){
    ERROR("Writing not allowed on recordings");
    return 0;
}

int cFilePlayer::read(char* buf, size_t buflen){
    if(!this->mCurrentFile){
        ERROR("Current part of record is not open");
        return -1;
    }
    MESSAGE(VERBOSE_RECORDS, "Reading %d from record", buflen);
    int bytesread = 0;
    while((bytesread = this->mCurrentFile->Read(buf, buflen)) == 0){ // EOF, try next file
        if(!(this->mCurrentFile = this->mRecordingFile->NextFile())){
            // no more files to read... finished!
            break;
        }
    }
    return bytesread;
}

int cFilePlayer::seek(off_t offset, int origin){
    if(!this->mCurrentFile){
        ERROR("Current part of record is not open");
        return -1;
    }
    
    MESSAGE(VERBOSE_RECORDS, "Seeking...");

    off_t relativeOffset = 0;
    off_t curpos = this->mCurrentFile->Seek(0, SEEK_CUR); // this should not change anything
    int index;
    // recalculate the absolute position in the record
    switch(origin){
        case SEEK_END:
            offset = this->mLastOffsets[this->mLastFileNumber] + offset;
            break;
        case SEEK_CUR:
            offset = this->mLastOffsets[this->mRecordingFile->Number()-1] + curpos +  offset;
            break;
        case SEEK_SET:
            // Nothing to change
            break;
        default:
            ERROR("Seek operation invalid");
            return -1;
    }
    // finally, we can seek
    // TODO: binary search
    for(index = 1; this->mLastOffsets[index]; index++){
        if(this->mLastOffsets[index-1] <= offset && offset <= this->mLastOffsets[index]){
            relativeOffset = offset - this->mLastOffsets[index-1];
            break;
        }
    }
    if(!(this->mCurrentFile = this->mRecordingFile->SetOffset(index, relativeOffset))){
        // seeking failed!!! should never happen.
        this->mCurrentFile = this->mRecordingFile->SetOffset(1);
        return -1;
    }

    return 0;
}

void cFilePlayer::scanLastOffsets(){
    // rewind
    this->mCurrentFile = this->mRecordingFile->SetOffset(1);
    for(int i = 1; (this->mCurrentFile = this->mRecordingFile->NextFile()); i++){
        this->mLastOffsets[i] = this->mLastOffsets[i-1] + this->mCurrentFile->Seek(0, SEEK_END);
        this->mLastFileNumber = this->mRecordingFile->Number();
    }
}

