/* 
 * File:   profiles_ac3.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 7. Dezember 2009, 13:04
 * Last modification: March 22, 2013
 */

#include "profiles/ac3.h"
#include "util.h"
#include "avdetector.h"

DLNAProfile DLNA_PROFILE_AC3 = { "AC3" , "audio/vnd.dolby.dd-raw" };

/**
 * Accepted audio bitrates
 *
 * This are all accepted audio bitrates of this profile
 * The standard says 64Kbps - 640Kbps. However, 32Kbps is used as
 * lower limit to accept low bitrate streams in ATSC and DVB broadcast streams.
 */
AcceptedBitrates        DLNA_BITRATES_AC3  = { true, {Kbps(32), Kbps(448)}};
AcceptedBitrates        DLNA_BITRATES_XAC3 = { true, {Kbps(64), Kbps(640)}};

/**
 * Accepted audio channel layouts
 *
 * This are all accepted audio channel layouts including LFE.
 * Though the LFE is optional and not explicitly mentioned in the documents to be supported
 * by this profile, it is supported by this plugin as it makes no sense to do not.
 * However, this may result in difficulties on some players if they follow the standard correctly
 *
 */
AcceptedAudioChannels   DLNA_AUDIOCHANNELS_AC3 = { 5, { CHANNEL_LAYOUT_10, CHANNEL_LAYOUT_20, CHANNEL_LAYOUT_21,
                                                        CHANNEL_LAYOUT_22, CHANNEL_LAYOUT_30, CHANNEL_LAYOUT_31,
                                                        CHANNEL_LAYOUT_32, CHANNEL_LAYOUT_10_1, CHANNEL_LAYOUT_20_1,
                                                        CHANNEL_LAYOUT_21_1, CHANNEL_LAYOUT_22_1, CHANNEL_LAYOUT_30_1,
                                                        CHANNEL_LAYOUT_31_1, CHANNEL_LAYOUT_5_1
                                                   }, true };

/**
 * Accepted audio sample rates
 *
 * This are all accepted audio sample rates
 * In ATSC systems only 48kHz are supported, the other two sample rates were
 * removed from standard. However, I do not specialice here if there is no
 * real reason for.
 */
AcceptedSamplingRates   DLNA_SAMPLINGRATES_AC3  = {{ KHz(32), KHz(44.1), KHz(48) }};
AcceptedSamplingRates   DLNA_SAMPLINGRATES_XAC3 = {{ KHz(48) }};

AudioPortionProfile cAC3Profiler::probeAudioProfile(AVFormatContext* FormatCtx){
#ifndef AVCODEC53
    AVCodecContext* AudioCodec = cCodecToolKit::getFirstCodecContext(FormatCtx, CODEC_TYPE_AUDIO);
#endif
#ifdef AVCODEC53
    AVCodecContext* AudioCodec = cCodecToolKit::getFirstCodecContext(FormatCtx, AVMEDIA_TYPE_AUDIO);
#endif

    if(AudioCodec->codec_id == CODEC_ID_AC3){
        // VBR
        if(cCodecToolKit::matchesAcceptedBitrates(DLNA_BITRATES_AC3, AudioCodec) &&
           cCodecToolKit::matchesAcceptedAudioChannels(DLNA_AUDIOCHANNELS_AC3, AudioCodec) &&
           cCodecToolKit::matchesAcceptedSamplingRates(DLNA_SAMPLINGRATES_AC3, AudioCodec)){
            return DLNA_APP_AC3;
        }
        else if(cCodecToolKit::matchesAcceptedBitrates(DLNA_BITRATES_XAC3, AudioCodec) &&
           cCodecToolKit::matchesAcceptedAudioChannels(DLNA_AUDIOCHANNELS_AC3, AudioCodec) &&
           cCodecToolKit::matchesAcceptedSamplingRates(DLNA_SAMPLINGRATES_XAC3, AudioCodec)){
            return DLNA_APP_XAC3;
        }
        else {
            return DLNA_APP_UNKNOWN;
        }
    }
    else {
        return DLNA_APP_UNKNOWN;
    }
}

DLNAProfile* cAC3Profiler::probeDLNAProfile(AVFormatContext* FormatCtx, VideoContainerProfile* VideoContProf){
    AudioPortionProfile Profile = AC3Profiler.probeAudioProfile(FormatCtx);
    if(Profile==DLNA_APP_AC3 || Profile==DLNA_APP_XAC3){
        return &DLNA_PROFILE_AC3;
    }
    else {
        return NULL;
    }
}

cAC3Profiler AC3Profiler;