/*
 * File:   profiles_mpeg2.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 7. Dezember 2009, 13:35
 * Last modification: April 4, 2013
 */

#ifndef _PROFILES_MPEG2_H
#define	_PROFILES_MPEG2_H

#include "profile_data.h"

extern DLNAProfile DLNA_PROFILE_MPEG_PS_NTSC;           ///< MPEG 2 PS NTSC
extern DLNAProfile DLNA_PROFILE_MPEG_PS_NTSC_XAC3;      ///< MPEG 2 PS NTSC AC3
extern DLNAProfile DLNA_PROFILE_MPEG_PS_PAL;            ///< MPEG 2 PS PAL
extern DLNAProfile DLNA_PROFILE_MPEG_PS_PAL_XAC3;       ///< MPEG 2 PS PAL AC3

extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA;          ///< MPEG 2 TS North America
extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_T;        ///< MPEG 2 TS North America with time stamp
extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_ISO;      ///< MPEG 2 TS North America without time stamp
extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA;          ///< MPEG 2 TS North America HD
extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_T;        ///< MPEG 2 TS North America with time stamp HD
extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_ISO;      ///< MPEG 2 TS North America without time stamp HD
extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_XAC3;     ///< MPEG 2 TS North America AC3
extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_XAC3_T;   ///< MPEG 2 TS North America AC3 with time stamp
extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_XAC3_ISO; ///< MPEG 2 TS North America AC3 without time stamp
extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_XAC3;     ///< MPEG 2 TS North America AC3 HD
extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_XAC3_T;   ///< MPEG 2 TS North America AC3 with time stamp HD
extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_XAC3_ISO; ///< MPEG 2 TS North America AC3 without time stamp HD

extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_EU;          ///< MPEG 2 TS Europe
extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_EU_T;        ///< MPEG 2 TS Europe with time stamp
extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_EU_ISO;      ///< MPEG 2 TS Europe without time stamp

//extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO;          ///< MPEG 2 TS Korea
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_T;        ///< MPEG 2 TS Korea with time stamp
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_ISO;      ///< MPEG 2 TS Korea without time stamp
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO;          ///< MPEG 2 TS Korea HD
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_T;        ///< MPEG 2 TS Korea with time stamp HD
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_ISO;      ///< MPEG 2 TS Korea without time stamp HD
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_XAC3;     ///< MPEG 2 TS Korea AC3
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_XAC3_T;   ///< MPEG 2 TS Korea AC3 with time stamp
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_XAC3_ISO; ///< MPEG 2 TS Korea AC3 without time stamp
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_XAC3;     ///< MPEG 2 TS Korea AC3 HD
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_XAC3_T;   ///< MPEG 2 TS Korea AC3 with time stamp HD
//extern DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_XAC3_ISO; ///< MPEG 2 TS Korea AC3 without time stamp HD

extern DLNAProfile DLNA_PROFILE_MPEG_TS_MP_LL_AAC;      ///< Low Level with AAC Audio
extern DLNAProfile DLNA_PROFILE_MPEG_TS_MP_LL_AAC_T;    ///< Low level with AAC Audio with time stamp
extern DLNAProfile DLNA_PROFILE_MPEG_TS_MP_LL_AAC_ISO;  ///< Low level with AAC Audio without time stamp

//extern DLNAProfile DLNA_PROFILE_MPEG_ES_PAL;            ///< PAL ES over RTP
//extern DLNAProfile DLNA_PROFILE_MPEG_ES_NTSC;           ///< NTSC ES over RTP
//extern DLNAProfile DLNA_PROFILE_MPEG_ES_PAL_XAC3;       ///< PAL AC3 ES over RTP
//extern DLNAProfile DLNA_PROFILE_MPEG_ES_NTSC_XAC3;      ///< NTSC AC3 ES over RTP


/**
 * The DLNA profiler class for the MPEG2 Codec
 */
class cMPEG2Profiler : public cDLNAProfiler, public cVideoProfiler, public cAudioProfiler {
public:
    /**
     * Probe the container profile.
     * @param FormatCtx the AVFormatContext given
     * @return the VideoContainerProfile or DLNA_VCP_UNKNOWN if not successful
     */
    virtual VideoContainerProfile probeContainerProfile(AVFormatContext* FormatCtx);
    /**
     * Probe the video profile.
     * @param FormatCtx the AVFormatContext given
     * @return the VideoPortionProfile or DLNA_VPP_UNKNOWN if not successful
     */
    virtual VideoPortionProfile probeVideoProfile(AVFormatContext* FormatCtx);
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

extern cMPEG2Profiler MPEG2Profiler;

#endif	/* _PROFILES_MPEG2_H */

