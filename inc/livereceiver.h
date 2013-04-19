/*
 * File:   livereceiver.h
 * Author: savop
 *
 * Created on 4. Juni 2009, 13:28
 */

#ifndef _LIVERECEIVER_H
#define	_LIVERECEIVER_H

#include "../common.h"
#include "filehandle.h"
#include <vdr/thread.h>
#include <vdr/receiver.h>
#include <vdr/ringbuffer.h>

#define RECEIVER_WAIT_ON_NODATA         50 // 50 ms
#define RECEIVER_WAIT_ON_NODATA_TIMEOUT 1000 * 2 // 2s
#define RECEIVER_MIN_BUFFER_FILLAGE     20 // 20%

/**
 * A receiver for live TV
 *
 * This is a receiver object which is attached to a VDR tv card device.
 * It is receiving transport stream packages and generates a single MPEG2
 * transport stream which can be distributed through the network.
 *
 */
class cLiveReceiver : public cReceiver, public cThread, public cFileHandle {
public:
    /**
     * Creates a new receiver instance
     *
     * This will create a new instance of a live receiver for the specified
     * channel at the specified priority level.
     *
     * A negativ priority means that the receiver may being detached from a
     * device.
     *
     * The receiver must be free'd with delete after it is not used any longer.
     *
     * @return returns a new liveReceiver instance
     */
    static cLiveReceiver* newInstance(
        cChannel *Channel,      ///< the channel which shall be tuned
        int Priority            ///< the priority level
    );
    virtual ~cLiveReceiver(void);
    /*! @copydoc cFileHandle::open(UpnpOpenFileMode) */
    virtual void open(UpnpOpenFileMode mode);
    /*! @copydoc cFileHandle::read(char*,size_t) */
    virtual int read(char* buf, size_t buflen);
    /*! @copydoc cFileHandle::write(char*,size_t) */
    virtual int write(char* buf, size_t buflen);
    /*! @copydoc cFileHandle::seek(off_t,int) */
    virtual int seek(off_t offset, int whence);
    /*! @copydoc cFileHandle::close() */
    virtual void close();
protected:
    /**
     * Receives data from VDR
     *
     * This is the interface for receiving packet data from the VDR. It buffers
     * the incoming transport stream packets in a linear ringbuffer and returns
     * immediatelly.
     */
    virtual void Receive(
        uchar *Data,        ///< The data received from VDR
        int Length          ///< The length of the data packet, usually 188 bytes
    );
    /**
     * Activates the receiver
     *
     * This activates the receiver which initializes internal data structures to
     * be prepared for receiving data from the VDR.
     *
     * If the parameter is \bc true, the receiver will be activated. If it is
     * \bc false, the receiver will be deactivated and stops its threads.
     */
    virtual void Activate(
        bool On             ///< Activates the receiver thread
    );
    /**
     * The receiver thread action
     *
     * This actually is the receiver thread, which runs consequitivelly and
     * buffers any received video data from the interal incoming buffer to the
     * internal outgoing buffer.
     *
     * While doing so, it tries to syncronize with the stream and creates new
     * MPEG2-TS PATs and PMTs for a single MPEG2-TS stream
     */
    virtual void Action(void);
private:
    cLiveReceiver(cChannel *Channel, cDevice *Device);
    cDevice  *mDevice;
    cChannel *mChannel;
    cRingBufferLinear *mLiveBuffer;
    cRingBufferLinear *mOutputBuffer;
    cFrameDetector *mFrameDetector;
    cPatPmtGenerator mPatPmtGenerator;
    int mVType;
};

#endif	/* _LIVERECEIVER_H */

