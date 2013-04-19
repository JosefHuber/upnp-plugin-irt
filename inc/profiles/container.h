/*
 * File:   container.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 8. Januar 2010, 10:45
 * Last modification: April 4, 2013
 */

#ifndef _CONTAINER_H
#define	_CONTAINER_H

#include "profile_data.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
/**
 * The class contains methods to detect video container profiles.
 */
class cContainerDetector {
public:
   /**
    * Detect the video container profile.
    * @param Ctx the AVFormatContext given
    * @return the VideoContainerProfile or DLNA_VCP_UNKNOWN if not successful
    */
    static VideoContainerProfile detect(AVFormatContext* Ctx);
private:
    /**
     * MPEG1
     */
    static VideoContainerProfile detectMPEG1Container(AVFormatContext* Ctx);
    /**
     * MPEG2-PS
     * MPEG2-TS
     * MPEG2-TS-DLNA
     * MPEG2-TS-DLNA-T
     */
    static VideoContainerProfile detectMPEG2Container(AVFormatContext* Ctx);
    /**
     * 3GPP
     * MP4
     */
    static VideoContainerProfile detectMP4Container(AVFormatContext* Ctx);
#ifdef WITH_WINDOWS_MEDIA
    /**
     * ASF
     */
    static VideoContainerProfile detectWMFContainer(AVFormatContext* Ctx);
#endif
};

#endif	/* _CONTAINER_H */

