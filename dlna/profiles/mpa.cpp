/* 
 * File:   profiles_mp3.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 7. Dezember 2009, 13:08
 * Last modification: March 22, 2013
 */

#include "profiles/mpa.h"
#include "profiles/profile_data.h"
#include "avdetector.h"

DLNAProfile DLNA_PROFILE_MP3  = { "MP3" , "audio/mpeg" };
DLNAProfile DLNA_PROFILE_MP3X = { "MP3X" , "audio/mpeg" };

AcceptedBitrates            DLNA_BITRATES_MPEG1_L1_DVB  = { true, { Kbps(32), Kbps(448) }};
AcceptedBitrates            DLNA_BITRATES_MPEG1_L2_DVB  = { true, { Kbps(32), Kbps(384) }};
AcceptedBitrates            DLNA_BITRATES_MPEG1_L3_VBR  = { true, { Kbps(32), Kbps(320) }};
AcceptedBitrates            DLNA_BITRATES_MPEG1_L3_CBR  = { false, { Kbps(32), Kbps(40),
                                                                     Kbps(48), Kbps(56),
                                                                     Kbps(64), Kbps(80),
                                                                     Kbps(96), Kbps(112),
                                                                     Kbps(128), Kbps(160),
                                                                     Kbps(192), Kbps(224),
                                                                     Kbps(256), Kbps(320)}};
AcceptedBitrates            DLNA_BITRATES_MPEG1_L3X_VBR = { true, {  Kbps(8), Kbps(320) }};
AcceptedBitrates            DLNA_BITRATES_MPEG1_L3X_CBR = { false, { Kbps(8), Kbps(16), Kbps(24),
                                                                     Kbps(32), Kbps(40),
                                                                     Kbps(48), Kbps(56),
                                                                     Kbps(64), Kbps(80),
                                                                     Kbps(96), Kbps(112),
                                                                     Kbps(128), Kbps(160),
                                                                     Kbps(192), Kbps(224),
                                                                     Kbps(256), Kbps(320)}};
AcceptedBitrates            DLNA_BITRATES_MPEG1_L2      = { true, { Kbps(32), Kbps(384) }};

AcceptedSamplingRates       DLNA_SAMPLINGRATES_MPEG1_L1_DVB = {{ KHz(16), KHz(22.05), KHz(24),
                                                                 KHz(32), KHz(44.1), KHz(48) }};
AcceptedSamplingRates       DLNA_SAMPLINGRATES_MPEG1_L2     = {{ KHz(32), KHz(44.1), KHz(48) }};
AcceptedSamplingRates       DLNA_SAMPLINGRATES_MPEG1_L2_DVB = {{ KHz(16), KHz(22.05), KHz(24),
                                                                 KHz(32), KHz(44.1), KHz(48) }};
AcceptedSamplingRates       DLNA_SAMPLINGRATES_MPEG1_L3     = {{ KHz(32), KHz(44.1), KHz(48) }};
AcceptedSamplingRates       DLNA_SAMPLINGRATES_MPEG1_L3X    = {{ KHz(16), KHz(22.05), KHz(24),
                                                                 KHz(32), KHz(44.1), KHz(48) }};

AcceptedAudioChannels       DLNA_AUDIOCHANNELS_MPEG1_L1_DVB = { 6, { CHANNEL_LAYOUT_10, CHANNEL_LAYOUT_20,
                                                                     CHANNEL_LAYOUT_21, CHANNEL_LAYOUT_22,
                                                                     CHANNEL_LAYOUT_30, CHANNEL_LAYOUT_31,
                                                                     CHANNEL_LAYOUT_32, }, false };
AcceptedAudioChannels       DLNA_AUDIOCHANNELS_MPEG1_L2_DVB = { 6, { CHANNEL_LAYOUT_10, CHANNEL_LAYOUT_20,
                                                                     CHANNEL_LAYOUT_21, CHANNEL_LAYOUT_22,
                                                                     CHANNEL_LAYOUT_30, CHANNEL_LAYOUT_31,
                                                                     CHANNEL_LAYOUT_32, }, false };

AcceptedAudioChannels       DLNA_AUDIOCHANNELS_MPEG1_L2     = { 2, { CHANNEL_LAYOUT_10, CHANNEL_LAYOUT_20 }, false };
AcceptedAudioChannels       DLNA_AUDIOCHANNELS_MPEG1_L3     = { 2, { CHANNEL_LAYOUT_10, CHANNEL_LAYOUT_20 }, false };
AcceptedAudioChannels       DLNA_AUDIOCHANNELS_MPEG1_L3X    = { 2, { CHANNEL_LAYOUT_10, CHANNEL_LAYOUT_20 }, false };

AudioPortionProfile cMPEGAudioProfiler::probeAudioProfile(AVFormatContext* FormatCtx){
#ifndef AVCODEC53
    AVCodecContext* AudioCodec = cCodecToolKit::getFirstCodecContext(FormatCtx, CODEC_TYPE_AUDIO);
#endif
#ifdef AVCODEC53
    AVCodecContext* AudioCodec = cCodecToolKit::getFirstCodecContext(FormatCtx, AVMEDIA_TYPE_AUDIO);
#endif

    if(AudioCodec->codec_id == CODEC_ID_MP1){
        if(cCodecToolKit::matchesAcceptedBitrates(DLNA_BITRATES_MPEG1_L1_DVB, AudioCodec) &&
           cCodecToolKit::matchesAcceptedSamplingRates(DLNA_SAMPLINGRATES_MPEG1_L1_DVB, AudioCodec) &&
           cCodecToolKit::matchesAcceptedAudioChannels(DLNA_AUDIOCHANNELS_MPEG1_L1_DVB, AudioCodec)){
            return DLNA_APP_MPEG1_L1;
        }
        else {
            return DLNA_APP_UNKNOWN;
        }
    }
    else if(AudioCodec->codec_id == CODEC_ID_MP2){
        if(cCodecToolKit::matchesAcceptedBitrates(DLNA_BITRATES_MPEG1_L2, AudioCodec) &&
           cCodecToolKit::matchesAcceptedSamplingRates(DLNA_SAMPLINGRATES_MPEG1_L2, AudioCodec) &&
           cCodecToolKit::matchesAcceptedAudioChannels(DLNA_AUDIOCHANNELS_MPEG1_L2, AudioCodec)){
            return DLNA_APP_MPEG1_L2;
        }
        else if(cCodecToolKit::matchesAcceptedBitrates(DLNA_BITRATES_MPEG1_L2_DVB, AudioCodec) &&
           cCodecToolKit::matchesAcceptedSamplingRates(DLNA_SAMPLINGRATES_MPEG1_L2_DVB, AudioCodec) &&
           cCodecToolKit::matchesAcceptedAudioChannels(DLNA_AUDIOCHANNELS_MPEG1_L2_DVB, AudioCodec)){
            return DLNA_APP_MPEG1_L2;
        }
        else {
            return DLNA_APP_UNKNOWN;
        }
    }
    else if(AudioCodec->codec_id == CODEC_ID_MP3){
        if((cCodecToolKit::matchesAcceptedBitrates(DLNA_BITRATES_MPEG1_L3_VBR, AudioCodec) ||
            cCodecToolKit::matchesAcceptedBitrates(DLNA_BITRATES_MPEG1_L3_CBR, AudioCodec)) &&
            cCodecToolKit::matchesAcceptedSamplingRates(DLNA_SAMPLINGRATES_MPEG1_L3, AudioCodec) &&
            cCodecToolKit::matchesAcceptedAudioChannels(DLNA_AUDIOCHANNELS_MPEG1_L3, AudioCodec)){
            return DLNA_APP_MPEG1_L3;
        }
        else if((cCodecToolKit::matchesAcceptedBitrates(DLNA_BITRATES_MPEG1_L3X_VBR, AudioCodec) ||
                 cCodecToolKit::matchesAcceptedBitrates(DLNA_BITRATES_MPEG1_L3X_CBR, AudioCodec)) &&
                 cCodecToolKit::matchesAcceptedSamplingRates(DLNA_SAMPLINGRATES_MPEG1_L3X, AudioCodec) &&
                 cCodecToolKit::matchesAcceptedAudioChannels(DLNA_AUDIOCHANNELS_MPEG1_L3X, AudioCodec)){
            return DLNA_APP_MPEG1_L3X;
        }
        else {
            return DLNA_APP_UNKNOWN;
        }
    }
    else {
        return DLNA_APP_UNKNOWN;
    }
}

DLNAProfile* cMPEGAudioProfiler::probeDLNAProfile(AVFormatContext* FormatCtx, VideoContainerProfile* VideoContProf){
    AudioPortionProfile Profile = MPEGAudioProfiler.probeAudioProfile(FormatCtx);
    if(Profile == DLNA_APP_MPEG1_L3){
        return &DLNA_PROFILE_MP3;
    }
    else if(Profile == DLNA_APP_MPEG1_L3X){
        return &DLNA_PROFILE_MP3X;
    }
    else {
        return NULL;
    }
}

cMPEGAudioProfiler MPEGAudioProfiler;