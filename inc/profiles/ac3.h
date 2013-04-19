/*
 * File:   profiles_ac3.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 7. Dezember 2009, 13:04
 * Last modification: April 4, 2013
 */

#ifndef _PROFILES_AC3_H
#define	_PROFILES_AC3_H

#include "profile_data.h"

extern DLNAProfile DLNA_PROFILE_AC3;                    ///< DLNA AC3
/**
 * The class contains methods for probing profiles.
 */
class cAC3Profiler : public cDLNAProfiler, public cAudioProfiler {
public:
   /**
    * Probe the DLNA profile.
    * @param FormatCtx the AVFormatContext given
    * @param VideoContProf if known, the actual VideoContainerProfile, otherwise DLNA_VCP_UNKNOWN
    * @return the DLNAProfile or NULL if not successful
    */
    virtual DLNAProfile* probeDLNAProfile(AVFormatContext* FormatCtx, VideoContainerProfile* VideoContProf);
  /**
   * Probe the audio profile.
   * @param FormatCtx the AVFormatContext given
   * @return the AudioPortionProfile or DLNA_APP_UNKNOWN if not successful
   */
    virtual AudioPortionProfile probeAudioProfile(AVFormatContext* FormatCtx);
};

extern cAC3Profiler AC3Profiler;

#endif	/* _PROFILES_AC3_H */

