/*
 * File:   profile_data.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 5. Januar 2010, 12:27
 * Last modification: April 4, 2013
 */

#ifndef _PROFILE_DATA_H
#define	_PROFILE_DATA_H

#ifdef __cplusplus
#include <vdr/tools.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#endif

#define MAX_BITRATES        32  ///< maximum amount of different CBR bitrates
#define MAX_CHANNEL_LAYOUTS 20  ///< maximum amount of different channel layout modes
#define MAX_SAMPLE_RATES    16  ///< maximum amount of different sampling rates

#define Kbps(x)             x*1000          ///< Kbit per second
#define Mbps(x)             x*1000*1000     ///< Mbit per second
#define KHz(x)              x*1000
#define MHz(x)              x*1000*1000

#define CHANNEL_LAYOUT_10   CH_LAYOUT_MONO                      ///< 1/0    1
#define CHANNEL_LAYOUT_10_1 (CHANNEL_LAYOUT_10|CH_LOW_FREQUENCY)///< 1/0    1.1 (incl. LFE)
#define CHANNEL_LAYOUT_20   CH_LAYOUT_STEREO                    ///< 2/0    2
#define CHANNEL_LAYOUT_20_1 (CHANNEL_LAYOUT_20|CH_LOW_FREQUENCY)///< 2/0    2.1 (incl. LFE)
#define CHANNEL_LAYOUT_21   (CH_LAYOUT_STEREO|CH_BACK_CENTER)   ///< 2/1    3
#define CHANNEL_LAYOUT_21_1 (CHANNEL_LAYOUT_21|CH_LOW_FREQUENCY)///< 2/1    3.1 (incl. LFE)
#define CHANNEL_LAYOUT_22   CH_LAYOUT_QUAD                      ///< 2/2    4
#define CHANNEL_LAYOUT_22_1 (CHANNEL_LAYOUT_22|CH_LOW_FREQUENCY)///< 2/2    4.1 (incl. LFE)
#define CHANNEL_LAYOUT_30   CH_LAYOUT_SURROUND                  ///< 3/0    3
#define CHANNEL_LAYOUT_30_1 (CHANNEL_LAYOUT_30|CH_LOW_FREQUENCY)///< 3/0    3.1 (incl. LFE)
#define CHANNEL_LAYOUT_31   (CH_LAYOUT_SURROUND|CH_BACK_CENTER) ///< 3/1    4
#define CHANNEL_LAYOUT_31_1 (CHANNEL_LAYOUT_31|CH_LOW_FREQUENCY)///< 3/1    4.1 (incl. LFE)
#define CHANNEL_LAYOUT_32   CH_LAYOUT_5POINT0                   ///< 3/2    5
#define CHANNEL_LAYOUT_5_1  CH_LAYOUT_5POINT1                   ///< 5.1    5.1 (incl. LFE)
#define CHANNEL_LAYOUT_7_1  CH_LAYOUT_7POINT1                   ///< 7.1    7.1 (incl. LFE)

/**
 * The combination of DLNA profile ID and the corresponding mime type
 *
 * This complies with the DLNA media format guidelines. Though this is very
 * similar to the profile structure of libdlna, it comes without the additional
 * label field as it seams to be not needed.
 */
struct DLNAProfile {
    const char* ID;     ///< the DLNA profile ID
    const char* mime;   ///< the mime type of the resource
};

/**
 * The DLNA profile for a icon image
 *
 * This complies with the DLNA media format guidelines. It contains a valid
 * mime type, the resolution of the image and the corresponding bit depth
 */
struct DLNAIconProfile {
    const char* mime;       ///< the mime type of the image
    unsigned short width;   ///< image width in pixel
    unsigned short height;  ///< image height in pixel
    unsigned char bitDepth; ///< bit depth in bits per pixel
};

enum VideoContainerProfile {
    DLNA_VCP_UNKNOWN=-1,
    DLNA_VCP_MPEG1=0,
    DLNA_VCP_MPEG2_PS,
    DLNA_VCP_MPEG2_TS,
    DLNA_VCP_MPEG2_TS_T,
    DLNA_VCP_MPEG2_TS_ISO,
    DLNA_VCP_MP4,
    DLNA_VCP_3GPP,
    DLNA_VCP_ASF,
};

enum VideoPortionProfile {
    DLNA_VPP_UNKNOWN=-1,
    DLNA_VPP_MPEG1=0,
    DLNA_VPP_MPEG2_PAL_SD,
    DLNA_VPP_MPEG2_PAL_HD,
    DLNA_VPP_MPEG2_NTSC_SD,
    DLNA_VPP_MPEG2_NTSC_HD,
    DLNA_VPP_MPEG2_SP,
    DLNA_VPP_MPEG2_MP_LL,
    DLNA_VPP_MPEG4_P2_SP_L0B,
    DLNA_VPP_MPEG4_P2_SP_L2,
    DLNA_VPP_MPEG4_P2_SP_L3,
    DLNA_VPP_MPEG4_P2_SP_L3_VGA,
    DLNA_VPP_MPEG4_P2_ASP_L5,
    DLNA_VPP_MPEG4_P2_ASP_L5_SO,
    DLNA_VPP_MPEG4_P2_ASP_L4_SO,
    DLNA_VPP_MPEG4_P2_H263_P0_L10,
    DLNA_VPP_MPEG4_P2_H263_P3_L10,
    DLNA_VPP_MPEG4_P2_CO,
    DLNA_VPP_MPEG4_P10_MP_SD,
    DLNA_VPP_MPEG4_P10_MP_HD,
    DLNA_VPP_MPEG4_P10_BL_L3_SD,
    DLNA_VPP_MPEG4_P10_BL_L3L_SD,
    DLNA_VPP_MPEG4_P10_BL_CIF30,
    DLNA_VPP_MPEG4_P10_BL_L2_CIF30,
    DLNA_VPP_MPEG4_P10_BL_CIF15,
    DLNA_VPP_MPEG4_P10_BL_L12_CIF15,
    DLNA_VPP_MPEG4_P10_BL_L1B_QCIF
};

enum AudioPortionProfile {
    DLNA_APP_UNKNOWN=-1,
    DLNA_APP_LPCM=0,
    DLNA_APP_MPEG1_L1,
    DLNA_APP_MPEG1_L2,
    DLNA_APP_MPEG1_L3,
    DLNA_APP_MPEG1_L3X,
    DLNA_APP_MPEG2_L2,          //TODO: Distinguish MPEG1 oder MPEG2 audio with FFMPEG
    DLNA_APP_AAC,
    DLNA_APP_AAC_MULT5,
    DLNA_APP_AAC_LTP,
    DLNA_APP_AAC_LTP_MULT5,
    DLNA_APP_AAC_LTP_MULT7,
    DLNA_APP_HEAAC,
    DLNA_APP_HEAAC_L2,
    DLNA_APP_HEAAC_MULT5,
    DLNA_APP_ATRAC3plus,
    DLNA_APP_AC3,
    DLNA_APP_XAC3,
    DLNA_APP_G726,
    DLNA_APP_AMR,
    DLNA_APP_AMR_WBplus,
    DLNA_APP_BL_QCIF15,
    DLNA_APP_BSAC,
    DLNA_APP_BSAC_MULT5
};
/**
 * A combination of DLNAProfile, VideoContainerProfile, VideoPortionProfile and AudioPortionProfile
 */
struct DLNAVideoMapping {
    DLNAProfile*            Profile;                    ///< the DLNA profile
    VideoContainerProfile   VideoContainer;             ///< the video container profile
    VideoPortionProfile     VideoProfile;               ///< the video portion profile
    AudioPortionProfile     AudioProfile;               ///< the audio portion profile
};
/**
 * A combination of VBR and valid bitrates.
 */
struct AcceptedBitrates {
    /**
     * <b>true</b> if VBR, <b>false</b> otherwise
     */
    bool                    VBR;
    /**
     * list of valid bitrates.
     *
     * if VBR is true, the array must contain exactly two items.
     * The first item is the minimum and the second item is the maximum bitrate
     *
     * The bitrate unit is bps. So, if you have 15bps, it is 15000000bps
     */
    int                     bitrates[MAX_BITRATES];
};
/**
 * A combination of size, frames per second and multiplier
 */
struct AcceptedResolution {
    /**
     * Screen width
     */
    int                     width;
    /**
     * Screen height
     */
    int                     height;
    /**
     * Frames per second
     *
     * this value may contain a higher value which is divided by the multiplier
     * given in <b>multiplier</b>
     */
    int                     fps;
    /**
     * Multiplier for calculating FPS
     *
     * The multiplier typically contains values like 1 or 1001 to calculate
     * the exact frame rate of 29,97 FPS in most NTSC systems
     */
    int                     multiplier;
};

/**
 * A combination that describes the accepted audio channels.
 */
struct AcceptedAudioChannels {
	/**
	 * the maximum number of audio channels
	 */
    int                     max_channels;
    /**
     * the individual channel layout
     */
    int64_t                 layouts[MAX_CHANNEL_LAYOUTS];
    /**
     * the flag is set if low frequency effects are supported
     */
    bool                    supportsLFE;
};

/**
 * The struct defines the valid sampling rates
 */
struct AcceptedSamplingRates {
	/**
	 * an array with accepted sampling rates
	 */
    int                     rates[MAX_SAMPLE_RATES];
};
/**
 * With the class the DLNA profile can be probed.
 */
class cDLNAProfiler {
public:
    /**
     * Probe the DLNA profile.
     * @param FormatCtx the AVFormatContext given
     * @param VideoContProf if known, the actual VideoContainerProfile, otherwise DLNA_VCP_UNKNOWN
     * @return the DLNAProfile or NULL if not successful
     */
    virtual DLNAProfile* probeDLNAProfile(AVFormatContext* FormatCtx, VideoContainerProfile* VideoContProf) = 0;
};
/**
 * With the class the audio profile can be probed.
 */
class cAudioProfiler {
public:
  /**
   * Probe the audio profile.
   * @param FormatCtx the AVFormatContext given
   * @return the AudioPortionProfile or DLNA_APP_UNKNOWN if not successful
   */
    virtual AudioPortionProfile probeAudioProfile(AVFormatContext* FormatCtx) = 0;
};
/**
 * With the class the video profile can be probed.
 */
class cVideoProfiler {
public:
    /**
     * Probe the video profile.
     * @param FormatCtx the AVFormatContext given
     * @return the VideoPortionProfile or DLNA_VPP_UNKNOWN if not successful
     */
    virtual VideoPortionProfile probeVideoProfile(AVFormatContext* FormatCtx) = 0;
    /**
     * Probe the container profile.
     * @param FormatCtx the AVFormatContext given
     * @return the VideoContainerProfile or DLNA_VCP_UNKNOWN if not successful
     */
    virtual VideoContainerProfile probeContainerProfile(AVFormatContext* FormatCtx) = 0;
};

#endif	/* _PROFILE_DATA_H */

