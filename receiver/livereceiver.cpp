/* 
 * File:   livereceiver.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 4. Juni 2009, 13:28
 * Last modification: March 18, 2013
 */

#include <vdr/thread.h>
#include <vdr/remux.h>
#include <vdr/device.h>
#include <vdr/channels.h>
#include <vdr/ringbuffer.h>
#include "livereceiver.h"

cLiveReceiver* cLiveReceiver::newInstance(cChannel* Channel, int Priority){
    cDevice *Device = cDevice::GetDevice(Channel, Priority, true);

    if (!Device){
        ERROR("No matching device found to serve this channel!");
        return NULL;
    }
    cLiveReceiver *Receiver = new cLiveReceiver(Channel, Device);
    if (Receiver){
        MESSAGE(VERBOSE_SDK, "Receiver for channel \"%s\" created successfully.", Channel->Name());
        return Receiver;
    }
    else {
        ERROR("Failed to create receiver!");
        return NULL;
    }
}

cLiveReceiver::cLiveReceiver(cChannel *Channel, cDevice *Device) : cReceiver(Channel), 
	              mDevice(Device), mChannel(Channel){
//: cReceiver(Channel->GetChannelID(), 0, Channel->Vpid(), Channel->Apids(), Channel->Dpids(), Channel->Spids()), mDevice(Device), mChannel(Channel){
	SetPids(Channel);
    this->mLiveBuffer = NULL;
    this->mOutputBuffer = NULL;
    this->mFrameDetector = NULL;
	this->mVType = Channel->Vtype();
}

cLiveReceiver::~cLiveReceiver(void){
    if(this->IsAttached())
        this->Detach();
}

void cLiveReceiver::open(UpnpOpenFileMode){
    this->mLiveBuffer = new cRingBufferLinear(RECEIVER_LIVEBUFFER_SIZE, RECEIVER_RINGBUFFER_MARGIN, true, "Live TV buffer");
    this->mOutputBuffer = new cRingBufferLinear(RECEIVER_OUTPUTBUFFER_SIZE, RECEIVER_RINGBUFFER_MARGIN, true, "Streaming buffer");

    this->mLiveBuffer->SetTimeouts(0, 100);
    this->mOutputBuffer->SetTimeouts(0, 500);
    
	if (!ISRADIO(mChannel)){
		this->mFrameDetector = new cFrameDetector(this->mChannel->Vpid(), this->mChannel->Vtype());
	}
	else if (this->mChannel->Apids()){
		MESSAGE(VERBOSE_LIVE_TV, " LiveReceiver opens with stream type 4; Vpid %i; Apid[0] %i", this->mChannel->Vpid(), this->mChannel->Apids()[0]);
		this->mFrameDetector = new cFrameDetector(this->mChannel->Apids()[0], 4);
	}
	else {
		ERROR("Internal error with live receiver open");
	}
    this->mPatPmtGenerator.SetChannel(this->mChannel);
    
    this->mDevice->SwitchChannel(this->mChannel, false);
    this->mDevice->AttachReceiver(this);
}

void cLiveReceiver::Activate(bool On){
    if (On){
        this->Start();
        MESSAGE(VERBOSE_LIVE_TV, "Live receiver started.");
    }
    else {
        if(this->Running()){
            this->Cancel(2);
        }
        MESSAGE(VERBOSE_LIVE_TV, "Live receiver stopped");
    }
}

void cLiveReceiver::Receive(uchar* Data, int Length){
    if (this->Running()){
        int bytesWrote = this->mLiveBuffer->Put(Data, Length);
        if (bytesWrote != Length && this->Running()){
            this->mLiveBuffer->ReportOverflow(Length - bytesWrote);
        }
    }
}

void cLiveReceiver::Action(void){
    MESSAGE(VERBOSE_LIVE_TV, "Started buffering...");
	bool isRadio = ISRADIO (this->mChannel);
	const int debugRepeat = 32;
	long debugCtr = 0;
	int accBytesWrote = 0;
    while(this->Running()){
        int bytesRead;
		if ((debugCtr % debugRepeat) == 0){
			MESSAGE(VERBOSE_BUFFERS, "Buffer is filled with %d bytes", this->mLiveBuffer->Available());
		}
        uchar* bytes = this->mLiveBuffer->Get(bytesRead);
//		MESSAGE(VERBOSE_BUFFERS, "Number of bytes read: %i", bytesRead);
        if (bytes){
            int count = this->mFrameDetector->Analyze(bytes, bytesRead);
            if (count){
				double framesPerSec = this->mFrameDetector->FramesPerSecond();
				if ((this->mFrameDetector->Synced() && (debugCtr % debugRepeat) == 0) || (!this->mFrameDetector->Synced() && (debugCtr % 200) == 0)) {
					 MESSAGE(VERBOSE_BUFFERS, "%d bytes analyzed; %2.2f FPS", count, framesPerSec);
				}
                if (!this->Running() && this->mFrameDetector->IndependentFrame())
                    break;
                if (this->mFrameDetector->Synced() || (!this->mFrameDetector->Synced() && ((int)this->mLiveBuffer->Available() > 120000 || 
					   (isRadio && (int)this->mLiveBuffer->Available() > 10000)))){
                    if(this->mFrameDetector->IndependentFrame()){
                        this->mOutputBuffer->Put(this->mPatPmtGenerator.GetPat(), TS_SIZE);
                        int i = 0;
                        while(uchar* pmt = this->mPatPmtGenerator.GetPmt(i)){
                            this->mOutputBuffer->Put(pmt, TS_SIZE);
                        }
                    }
                    int bytesWrote = this->mOutputBuffer->Put(bytes, count);
                    if (bytesWrote != count){
                        this->mLiveBuffer->ReportOverflow(count - bytesWrote);
                    }
					if ((debugCtr % 4) == 0){
						MESSAGE(VERBOSE_BUFFERS, "Wrote %d to output buffer", accBytesWrote);
						accBytesWrote = 0;
					}
					else {
						accBytesWrote += bytesWrote;
					}
                    if(bytesWrote){
                        this->mLiveBuffer->Del(bytesWrote);
                    }
                    else {
                        cCondWait::SleepMs(100);
                    }
                }
                else {
					if ((debugCtr % 200) == 0){
	                   WARNING("Cannot sync to stream, VType: %i, debug counter: %ld", this->mVType, debugCtr);
					}
                }
            }
        }
		debugCtr++;
    }
    MESSAGE(VERBOSE_LIVE_TV, "Receiver was detached from device");
}

int cLiveReceiver::read(char* buf, size_t buflen){
    int bytesRead = 0;
    if(!this->IsAttached())
        bytesRead = -1;
    else {
        int WaitTimeout = RECEIVER_WAIT_ON_NODATA_TIMEOUT;
        // Wait until the buffer size is at least half the requested buffer length
		int min_buffer_fillage = (this->mVType == 0) ? 6 : (RECEIVER_MIN_BUFFER_FILLAGE * 10); // as percentage*10 of buflen
//		int min_buffer_fillage = (this->mVType == 0) ? 11 : (RECEIVER_MIN_BUFFER_FILLAGE * 10); // as percentage*10 of buflen
        double MinBufSize = buflen * min_buffer_fillage/1000;
        int Available = 0;
        while ((double)(Available = this->mOutputBuffer->Available()) < MinBufSize){
 //           WARNING("Too few data, waiting...");
            WARNING("Waiting... Only %d bytes available, need %ld more bytes.", Available, (long)(MinBufSize-Available));
            cCondWait::SleepMs(RECEIVER_WAIT_ON_NODATA);
            if (!this->IsAttached()){
                MESSAGE(VERBOSE_LIVE_TV, "Lost device...");
                return 0;
            }
            WaitTimeout-=RECEIVER_WAIT_ON_NODATA;
            if (WaitTimeout <= 0){
                double seconds = (RECEIVER_WAIT_ON_NODATA_TIMEOUT/1000);
                ERROR("No data received for %4.2f seconds, aborting.", seconds);
                this->Activate(false);
                return 0;
            }
        }

        uchar* buffer = this->mOutputBuffer->Get(bytesRead);
        if (buffer){
            if (buflen > (size_t)bytesRead){
                memcpy(buf,(char*)buffer,bytesRead);
                this->mOutputBuffer->Del(bytesRead);
            }
            else {
                memcpy(buf,(char*)buffer,buflen);
                this->mOutputBuffer->Del(buflen);
            }
        }

    }
    MESSAGE(VERBOSE_BUFFERS, "Read %d bytes from live feed", bytesRead);
    return bytesRead;
}

int cLiveReceiver::seek(off_t, int){
    ERROR("Seeking not supported on broadcasts");
    return 0;
}

int cLiveReceiver::write(char*, size_t){
    ERROR("Writing not allowed on broadcasts");
    return 0;
}

void cLiveReceiver::close(){
    MESSAGE(VERBOSE_SDK, "Closing live receiver");
    this->Detach();
    delete this->mOutputBuffer; this->mOutputBuffer = NULL;
    delete this->mLiveBuffer; this->mLiveBuffer = NULL;
    this->mFrameDetector = NULL;
    MESSAGE(VERBOSE_LIVE_TV, "Live receiver closed.");
}