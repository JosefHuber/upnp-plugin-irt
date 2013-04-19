/* 
 * File:   profiles_mpeg2.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 7. Dezember 2009, 13:35
 * Last modification: March 22, 2013
 */

#include "profiles/mpeg2.h"
#include "profiles/container.h"
#include "util.h"
#include "profiles/ac3.h"
#include "avdetector.h"

AcceptedBitrates DLNA_VIDEOBITRATES_MPEG2_TS_NA_SYSTEM = { true, {1, Mbps(19.3927)}};
AcceptedBitrates DLNA_VIDEOBITRATES_MPEG2_TS_EU        = { true, {1, Mbps(15)}};
AcceptedBitrates DLNA_VIDEOBITRATES_MPEG2_PS           = { true, {1, Mbps(9.80)}};

AcceptedResolution     DLNA_RESOLUTIONS_MPEG2_PAL[]     = { { 720, 576, 25, 1 },
                                                          { 704, 576, 25, 1 },  ///< compatibility mode for PAL
                                                          { 544, 576, 25, 1 },
                                                          { 480, 576, 25, 1 },
                                                          { 352, 576, 25, 1 },
                                                          { 352, 288, 25, 1 } };
AcceptedResolution     DLNA_RESOLUTIONS_MPEG2_NTSC_SD[] = { { 720, 480, 30, 1001},
                                                          { 704, 480, 30, 1001},
                                                          { 704, 480, 30, 1},
                                                          { 704, 480, 24, 1001},
                                                          { 704, 480, 24, 1},
                                                          { 640, 480, 30, 1001},
                                                          { 640, 480, 30, 1},
                                                          { 640, 480, 24, 1001},
                                                          { 640, 480, 24, 1},
                                                          { 544, 480, 30, 1001},
                                                          { 480, 480, 30, 1001},
                                                          { 352, 480, 30, 1001} };
AcceptedResolution     DLNA_RESOLUTIONS_MPEG2_NTSC_HD[] = { { 1920, 1080, 30, 1001},
                                                          { 1920, 1080, 30, 1},
                                                          { 1920, 1080, 24, 1001},
                                                          { 1920, 1080, 24, 1},
                                                          { 1280, 720, 30, 1001},
                                                          { 1280, 720, 30, 1},
                                                          { 1280, 720, 24, 1001},
                                                          { 1280, 720, 24, 1},
                                                          { 1440, 1080, 30, 1001},
                                                          { 1440, 1080, 30, 1},
                                                          { 1440, 1080, 24, 1001},
                                                          { 1440, 1080, 24, 1},
                                                          { 1280, 1080, 30, 1001},
                                                          { 1280, 1080, 30, 1},
                                                          { 1280, 1080, 24, 1001},
                                                          { 1280, 1080, 24, 1} };

DLNAProfile DLNA_PROFILE_MPEG_PS_NTSC               = { "MPEG_PS_NTSC", "video/mpeg" };
DLNAProfile DLNA_PROFILE_MPEG_PS_NTSC_XAC3          = { "MPEG_PS_NTSC_XAC3", "video/mpeg" };
DLNAProfile DLNA_PROFILE_MPEG_PS_PAL                = { "MPEG_PS_PAL", "video/mpeg" };
DLNAProfile DLNA_PROFILE_MPEG_PS_PAL_XAC3           = { "MPEG_PS_PAL_XAC3", "video/mpeg"};

DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA              = { "MPEG_TS_SD_NA", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_T            = { "MPEG_TS_SD_NA_T", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_ISO          = { "MPEG_TS_SD_NA_ISO", "video/mpeg"};
DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA              = { "MPEG_TS_HD_NA", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_T            = { "MPEG_TS_HD_NA_T", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_ISO          = { "MPEG_TS_HD_NA_ISO", "video/mpeg"};
DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_XAC3         = { "MPEG_TS_SD_NA_XAC3", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_XAC3_T       = { "MPEG_TS_SD_NA_XAC3_T", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_SD_NA_XAC3_ISO     = { "MPEG_TS_SD_NA_XAC3_ISO", "video/mpeg"};
DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_XAC3         = { "MPEG_TS_HD_NA_XAC3", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_XAC3_T       = { "MPEG_TS_HD_NA_XAC3_T", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_HD_NA_XAC3_ISO     = { "MPEG_TS_HD_NA_XAC3_ISO", "video/mpeg"};

DLNAProfile DLNA_PROFILE_MPEG_TS_SD_EU              = { "MPEG_TS_SD_EU", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_SD_EU_T            = { "MPEG_TS_SD_EU_T", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_SD_EU_ISO          = { "MPEG_TS_SD_EU_ISO", "video/mpeg"};

// The Korean profiles are almost identical to the north american profiles.
//DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO              = { "MPEG_TS_SD_KO", "video/vnd.dlna.mpeg-tts"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_T            = { "MPEG_TS_SD_KO_T", "video/vnd.dlna.mpeg-tts"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_ISO          = { "MPEG_TS_SD_KO_ISO", "video/mpeg"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO              = { "MPEG_TS_HD_KO", "video/vnd.dlna.mpeg-tts"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_T            = { "MPEG_TS_HD_KO_T", "video/vnd.dlna.mpeg-tts"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_ISO          = { "MPEG_TS_HD_KO_ISO", "video/mpeg"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_XAC3         = { "MPEG_TS_SD_KO_XAC3", "video/vnd.dlna.mpeg-tts"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_XAC3_T       = { "MPEG_TS_SD_KO_XAC3_T", "video/vnd.dlna.mpeg-tts"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_SD_KO_XAC3_ISO     = { "MPEG_TS_SD_KO_XAC3_ISO", "video/mpeg"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_XAC3         = { "MPEG_TS_HD_KO_XAC3", "video/vnd.dlna.mpeg-tts"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_XAC3_T       = { "MPEG_TS_HD_KO_XAC3_T", "video/vnd.dlna.mpeg-tts"};
//DLNAProfile DLNA_PROFILE_MPEG_TS_HD_KO_XAC3_ISO     = { "MPEG_TS_HD_KO_XAC3_ISO", "video/mpeg"};

DLNAProfile DLNA_PROFILE_MPEG_TS_MP_LL_AAC          = { "MPEG_TS_MP_LL_AAC", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_MP_LL_AAC_T        = { "MPEG_TS_MP_LL_AAC_T", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_MPEG_TS_MP_LL_AAC_ISO      = { "MPEG_TS_MP_LL_AAC_ISO", "video/mpeg"};

// The Elementary Stream profiles are currently not supported as they are only supported within RTP streaming
//DLNAProfile DLNA_PROFILE_MPEG_ES_PAL                = { "MPEG_ES_PAL", "video/mpeg"};
//DLNAProfile DLNA_PROFILE_MPEG_ES_NTSC               = { "MPEG_ES_NTSC", "video/mpeg"};
//DLNAProfile DLNA_PROFILE_MPEG_ES_PAL_XAC3           = { "MPEG_ES_PAL_XAC3", "video/mpeg"};
//DLNAProfile DLNA_PROFILE_MPEG_ES_NTSC_XAC3          = { "MPEG_ES_NTSC_XAC3", "video/mpeg"};

DLNAVideoMapping MPEG2_VIDEO_MAP[] = {
    { &DLNA_PROFILE_MPEG_PS_NTSC,           DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_LPCM},
    { &DLNA_PROFILE_MPEG_PS_NTSC,           DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_MPEG1_L2},
    { &DLNA_PROFILE_MPEG_PS_NTSC,           DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_MPEG2_L2},
    { &DLNA_PROFILE_MPEG_PS_NTSC,           DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_PS_NTSC_XAC3,      DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_XAC3},
    { &DLNA_PROFILE_MPEG_PS_PAL,            DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_LPCM},
    { &DLNA_PROFILE_MPEG_PS_PAL,            DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG1_L2},
    { &DLNA_PROFILE_MPEG_PS_PAL,            DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG2_L2},
    { &DLNA_PROFILE_MPEG_PS_PAL,            DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_PS_PAL_XAC3,       DLNA_VCP_MPEG2_PS,      DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_XAC3},
    { &DLNA_PROFILE_MPEG_TS_SD_NA,          DLNA_VCP_MPEG2_TS,      DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_TS_SD_NA_T,        DLNA_VCP_MPEG2_TS_T,    DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_TS_SD_NA_ISO,      DLNA_VCP_MPEG2_TS_ISO,  DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_TS_SD_NA_XAC3,     DLNA_VCP_MPEG2_TS,      DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_XAC3},
    { &DLNA_PROFILE_MPEG_TS_SD_NA_XAC3_T,   DLNA_VCP_MPEG2_TS_T,    DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_XAC3},
    { &DLNA_PROFILE_MPEG_TS_SD_NA_XAC3_ISO, DLNA_VCP_MPEG2_TS_ISO,  DLNA_VPP_MPEG2_NTSC_SD, DLNA_APP_XAC3},
    { &DLNA_PROFILE_MPEG_TS_HD_NA,          DLNA_VCP_MPEG2_TS,      DLNA_VPP_MPEG2_NTSC_HD, DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_TS_HD_NA_T,        DLNA_VCP_MPEG2_TS_T,    DLNA_VPP_MPEG2_NTSC_HD, DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_TS_HD_NA_ISO,      DLNA_VCP_MPEG2_TS_ISO,  DLNA_VPP_MPEG2_NTSC_HD, DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_TS_HD_NA_XAC3,     DLNA_VCP_MPEG2_TS,      DLNA_VPP_MPEG2_NTSC_HD, DLNA_APP_XAC3},
    { &DLNA_PROFILE_MPEG_TS_HD_NA_XAC3_T,   DLNA_VCP_MPEG2_TS_T,    DLNA_VPP_MPEG2_NTSC_HD, DLNA_APP_XAC3},
    { &DLNA_PROFILE_MPEG_TS_HD_NA_XAC3_ISO, DLNA_VCP_MPEG2_TS_ISO,  DLNA_VPP_MPEG2_NTSC_HD, DLNA_APP_XAC3},
    { &DLNA_PROFILE_MPEG_TS_SD_EU,          DLNA_VCP_MPEG2_TS,      DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG1_L1},
    { &DLNA_PROFILE_MPEG_TS_SD_EU,          DLNA_VCP_MPEG2_TS,      DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG1_L2},
    { &DLNA_PROFILE_MPEG_TS_SD_EU,          DLNA_VCP_MPEG2_TS,      DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG2_L2},
    { &DLNA_PROFILE_MPEG_TS_SD_EU,          DLNA_VCP_MPEG2_TS,      DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_TS_SD_EU_T,        DLNA_VCP_MPEG2_TS_T,    DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG1_L1},
    { &DLNA_PROFILE_MPEG_TS_SD_EU_T,        DLNA_VCP_MPEG2_TS_T,    DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG1_L2},
    { &DLNA_PROFILE_MPEG_TS_SD_EU_T,        DLNA_VCP_MPEG2_TS_T,    DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG2_L2},
    { &DLNA_PROFILE_MPEG_TS_SD_EU_T,        DLNA_VCP_MPEG2_TS_T,    DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_TS_SD_EU_ISO,      DLNA_VCP_MPEG2_TS_ISO,  DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG1_L1},
    { &DLNA_PROFILE_MPEG_TS_SD_EU_ISO,      DLNA_VCP_MPEG2_TS_ISO,  DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG1_L2},
    { &DLNA_PROFILE_MPEG_TS_SD_EU_ISO,      DLNA_VCP_MPEG2_TS_ISO,  DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_MPEG2_L2},
    { &DLNA_PROFILE_MPEG_TS_SD_EU_ISO,      DLNA_VCP_MPEG2_TS_ISO,  DLNA_VPP_MPEG2_PAL_SD,  DLNA_APP_AC3},
    { &DLNA_PROFILE_MPEG_TS_MP_LL_AAC,      DLNA_VCP_MPEG2_TS,      DLNA_VPP_MPEG2_MP_LL,   DLNA_APP_AAC},
    { &DLNA_PROFILE_MPEG_TS_MP_LL_AAC_T,    DLNA_VCP_MPEG2_TS_T,    DLNA_VPP_MPEG2_MP_LL,   DLNA_APP_AAC},
    { &DLNA_PROFILE_MPEG_TS_MP_LL_AAC_ISO,  DLNA_VCP_MPEG2_TS_ISO,  DLNA_VPP_MPEG2_MP_LL,   DLNA_APP_AAC},

};

DLNAProfile* cMPEG2Profiler::probeDLNAProfile(AVFormatContext* FormatCtx, VideoContainerProfile* VideoContProf){
	*VideoContProf = DLNA_VCP_UNKNOWN;
    VideoContainerProfile   VCP = MPEG2Profiler.probeContainerProfile(FormatCtx);
    VideoPortionProfile     VPP = MPEG2Profiler.probeVideoProfile(FormatCtx);
    AudioPortionProfile     APP = MPEG2Profiler.probeAudioProfile(FormatCtx);

//    MESSAGE(VERBOSE_METADATA, "VCP: %d, VPP: %d, APP: %d", VCP, VPP, APP);
	if (APP == DLNA_APP_MPEG2_L2){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_MPEG2_L2");
	}
	else if (APP == DLNA_APP_MPEG1_L1){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_MPEG1_L1");
	}
	else if (APP == DLNA_APP_MPEG1_L2){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_MPEG1_L2");
	}
	else if (APP == DLNA_APP_MPEG1_L3){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_MPEG1_L3");
	}
	else if (APP == DLNA_APP_LPCM){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_LPCM");
	}
	else if (APP == DLNA_APP_AC3){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_AC3");
	}
	else {
		MESSAGE(VERBOSE_METADATA, "Not dealt audio profile is %d", APP);
	}

	if (VCP == DLNA_VCP_MPEG2_TS){
		MESSAGE(VERBOSE_METADATA, "The video container profile is DLNA_VCP_MPEG2_TS");
	}
	else if (VCP == DLNA_VCP_MP4){
		MESSAGE(VERBOSE_METADATA, "The video container profile is DLNA_VCP_MP4");
	}
	else if (VCP == DLNA_VCP_MPEG2_TS_T){
		MESSAGE(VERBOSE_METADATA, "The video container profile is DLNA_VCP_MPEG2_TS_T");
	}
	else if (VCP == DLNA_VCP_MPEG2_TS_ISO){
		MESSAGE(VERBOSE_METADATA, "The video container profile is DLNA_VCP_MPEG2_TS_ISO");
	}

	*VideoContProf = VCP;
    for(int i=0; i < (int) (sizeof(MPEG2_VIDEO_MAP)/sizeof(DLNAVideoMapping)); i++){
        if(     MPEG2_VIDEO_MAP[i].VideoContainer == VCP &&
                MPEG2_VIDEO_MAP[i].VideoProfile   == VPP &&
                MPEG2_VIDEO_MAP[i].AudioProfile   == APP){			
            return MPEG2_VIDEO_MAP[i].Profile;
        }
    }

    return NULL;
}

VideoPortionProfile cMPEG2Profiler::probeVideoProfile(AVFormatContext* FormatCtx){
#ifndef AVCODEC53
    AVCodecContext* VideoCodec = cCodecToolKit::getFirstCodecContext(FormatCtx, CODEC_TYPE_VIDEO);
    AVStream* VideoStream = cCodecToolKit::getFirstStream(FormatCtx, CODEC_TYPE_VIDEO);
#endif
#ifdef AVCODEC53
    AVCodecContext* VideoCodec = cCodecToolKit::getFirstCodecContext(FormatCtx, AVMEDIA_TYPE_VIDEO);
    AVStream* VideoStream = cCodecToolKit::getFirstStream(FormatCtx, AVMEDIA_TYPE_VIDEO);
#endif

    MESSAGE(VERBOSE_METADATA, "Codec-ID:             %d", VideoCodec->codec_id);
    MESSAGE(VERBOSE_METADATA, "Codec-Name:           %s", VideoCodec->codec_name);
    MESSAGE(VERBOSE_METADATA, "Codec Bitrate:        %d", VideoCodec->bit_rate);
    MESSAGE(VERBOSE_METADATA, "Codec width:          %d", VideoCodec->coded_width);
    MESSAGE(VERBOSE_METADATA, "Codec height:         %d", VideoCodec->coded_height);
    MESSAGE(VERBOSE_METADATA, "Codec Profile:        %d", VideoCodec->profile);
    MESSAGE(VERBOSE_METADATA, "Codec Level:          %d", VideoCodec->level);
    MESSAGE(VERBOSE_METADATA, "Codec Chroma:         %d", VideoCodec->pix_fmt);
    MESSAGE(VERBOSE_METADATA, "Stream aspect ratio   %d:%d", VideoStream->sample_aspect_ratio.num, VideoStream->sample_aspect_ratio.den);
    MESSAGE(VERBOSE_METADATA, "Stream fps            %2.3f", av_q2d(VideoStream->r_frame_rate));

    if(VideoCodec->codec_id == CODEC_ID_MPEG2VIDEO){
        if(cCodecToolKit::matchesAcceptedResolutions(DLNA_RESOLUTIONS_MPEG2_PAL,
                (int) (sizeof(DLNA_RESOLUTIONS_MPEG2_PAL)/sizeof(AcceptedResolution)) , VideoStream) &&
           (cCodecToolKit::matchesAcceptedBitrates(DLNA_VIDEOBITRATES_MPEG2_TS_EU, VideoCodec) ||
            cCodecToolKit::matchesAcceptedBitrates(DLNA_VIDEOBITRATES_MPEG2_PS, VideoCodec))){
            return DLNA_VPP_MPEG2_PAL_SD;
        }
        else if(cCodecToolKit::matchesAcceptedResolutions(DLNA_RESOLUTIONS_MPEG2_NTSC_SD,
                (int) (sizeof(DLNA_RESOLUTIONS_MPEG2_NTSC_SD)/sizeof(AcceptedResolution)), VideoStream) &&
                (cCodecToolKit::matchesAcceptedSystemBitrate(DLNA_VIDEOBITRATES_MPEG2_TS_NA_SYSTEM, FormatCtx) ||
                 cCodecToolKit::matchesAcceptedBitrates(DLNA_VIDEOBITRATES_MPEG2_PS, VideoCodec))) {
            return DLNA_VPP_MPEG2_NTSC_SD;
        }
        else if(cCodecToolKit::matchesAcceptedResolutions(DLNA_RESOLUTIONS_MPEG2_NTSC_HD,
                (int) (sizeof(DLNA_RESOLUTIONS_MPEG2_NTSC_HD)/sizeof(AcceptedResolution)), VideoStream) &&
                cCodecToolKit::matchesAcceptedSystemBitrate(DLNA_VIDEOBITRATES_MPEG2_TS_NA_SYSTEM, FormatCtx)) {
            return DLNA_VPP_MPEG2_NTSC_HD;
        } 
    }
	else if(VideoCodec->codec_id == CODEC_ID_H264){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_H264");
	}
	else if (VideoCodec->codec_id == CODEC_ID_MPEG4){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_MPEG4");
	}
	else if (VideoCodec->codec_id == CODEC_ID_AAC){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_AAC");
	}
	else if (VideoCodec->codec_id == CODEC_ID_AC3){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_AC3");
	}
	else if (VideoCodec->codec_id == CODEC_ID_NONE){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_NONE");
	}
	else {
		MESSAGE(VERBOSE_METADATA, "cMPEG2Profiler: VideoCodec->codec_id has the unexpected value %i", (int)VideoCodec->codec_id);
	}

    return DLNA_VPP_UNKNOWN;
}

AudioPortionProfile cMPEG2Profiler::probeAudioProfile(AVFormatContext* FormatCtx){
    AudioPortionProfile Profile;
    if((Profile = AC3Profiler.probeAudioProfile(FormatCtx)) != DLNA_APP_UNKNOWN){
        MESSAGE(VERBOSE_METADATA, "AC3: %d", Profile);
        return Profile;
    }
    // First codec is not AC3... trying other codecs
    else if((Profile = MPEGAudioProfiler.probeAudioProfile(FormatCtx)) != DLNA_APP_UNKNOWN){
 //       MESSAGE(VERBOSE_METADATA, "MPA: %d", Profile);
        return Profile;
    }
    else {
        return DLNA_APP_UNKNOWN;
    }
}

VideoContainerProfile cMPEG2Profiler::probeContainerProfile(AVFormatContext* FormatCtx){
    return cContainerDetector::detect(FormatCtx);
}

cMPEG2Profiler MPEG2Profiler;