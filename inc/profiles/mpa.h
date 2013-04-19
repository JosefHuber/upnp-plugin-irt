/*
 * File:   profiles_mp3.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 7. Dezember 2009, 13:08
 * Last modification: April 4, 2013
 */

#ifndef _PROFILES_MP3_H
#define	_PROFILES_MP3_H

#include "profile_data.h"

extern DLNAProfile DLNA_PROFILE_MP3;                    ///< DLNA MP3
extern DLNAProfile DLNA_PROFILE_MP3X;                   ///< MP3x
/**
 * The class contains methods for profile probing.
 */
class cMPEGAudioProfiler : public cDLNAProfiler, public cAudioProfiler {
public:
   /**
    * Probe the audio profile.
    * @param FormatCtx the AVFormatContext given
    * @return the AudioPortionProfile or DLNA_APP_UNKNOWN if not successful
    */
    virtual AudioPortionProfile probeAudioProfile(AVFormatContext* FormatCtx);
    /**
     * Probe the DLNA profile.
     * @param FormatCtx the AVFormatContext given
     * @param VideoContProf if known, the actual VideoContainerProfile, otherwise DLNA_VCP_UNKNOWN
     * @return the DLNAProfile or NULL if not successful
     */
    virtual DLNAProfile* probeDLNAProfile(AVFormatContext* FormatCtx, VideoContainerProfile* VideoContProf);
};

extern cMPEGAudioProfiler MPEGAudioProfiler;

#endif	/* _PROFILES_MP3_H */

