/*
 * File:   fileplayer.h
 * Author: J.Huber, IRT GmbH
 *
 * Created on Jule 3, 2012
 */

#ifndef _FILEPLAYER_H
#define	_FILEPLAYER_H

#include "../common.h"
#include "filehandle.h"
#include <vdr/recording.h>

/**
 * The file player
 *
 * This class provides the ability to play VDR records consisting of one transport stream.
 *
 */
class cFilePlayer : public cFileHandle {
public:
    /**
     * Get a new instance of a file player
     *
     * This returns a new instance of a file player which plays the
     * specified VDR file.
     *
     * @param fileName the file to play
     * @return the new instance of the file player
     */
    static cFilePlayer *newInstance(const char* fileName);
    virtual ~cFilePlayer();
    virtual void open(UpnpOpenFileMode mode);
    virtual int read(char* buf, size_t buflen);
    virtual int write(char* buf, size_t buflen);
    virtual int seek(off_t offset, int origin);
    virtual void close();
private:
    void scanLastOffsets();
    cFilePlayer(const char* fileName);
    off_t*      mLastOffsets;
    int         mLastFileNumber;
    cFileName  *mRecordingFile;
    cUnbufferedFile *mCurrentFile;
};

#endif	/* _FILEPLAYER_H */

