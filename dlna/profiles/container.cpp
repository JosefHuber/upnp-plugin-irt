/* 
 * File:   container.cpp
 * Author: savop
 *
 * Created on 8. Januar 2010, 11:45
 */

#include "profiles/container.h"
#include "../../common.h"
#include <vdr/remux.h>

#define DLNA_MPEG_TS_SIZE_ISO       TS_SIZE
#define DLNA_MPEG_TS_SIZE           192

enum VideoFormats {
    FORMAT_MPEG1,
    FORMAT_MPEG2,
    FORMAT_MP4,
#ifdef WITH_WINDOWS_MEDIA
    FORMAT_WMF,
#endif
};

static const struct VideoFormatMapping {
    const char* name;
    VideoFormats format;
} VideoFormatMap[] = {
    { "mpeg", FORMAT_MPEG2 },
    { "mpegts", FORMAT_MPEG2 },
#ifdef WITH_WINDOWS_MEDIA
    { "asf", FORMAT_WMF }
#endif
};

VideoContainerProfile cContainerDetector::detect(AVFormatContext* Ctx){
    if(Ctx && Ctx->iformat){
        for(int i=0; VideoFormatMap[i].name; i++){
            if(!strcasecmp(VideoFormatMap[i].name,Ctx->iformat->name))
                switch(VideoFormatMap[i].format){
                    case FORMAT_MPEG1:
                        return cContainerDetector::detectMPEG1Container(Ctx);
                    case FORMAT_MPEG2:
                        return cContainerDetector::detectMPEG2Container(Ctx);
                    case FORMAT_MP4:
                        return cContainerDetector::detectMP4Container(Ctx);
#ifdef WITH_WINDOWS_MEDIA
                    case FORMAT_WMF:
                        return cContainerDetector::detectWMFContainer(Ctx);
#endif
                    default:
                        break;
                }
        }
        ERROR("AVDetector: Unsupported input format \"%s\"", Ctx->iformat->name);
    }
    else {
        ERROR("AVDetector: Input format not found");
    }
    return DLNA_VCP_UNKNOWN;
}

VideoContainerProfile cContainerDetector::detectMPEG2Container(AVFormatContext* Ctx){

    uint8_t buf[5*1024];
    int len;
    int64_t pos;
    int PaketSize = 0;
    VideoContainerProfile VCP = DLNA_VCP_UNKNOWN;

    /* read the first 1024 bytes to get packet size */
    pos = url_ftell(Ctx->pb);
    len = get_buffer(Ctx->pb, buf, sizeof(buf));
    if (len != sizeof(buf)) PaketSize = 0;

    for(int i = 0; i<DLNA_MPEG_TS_SIZE; i++){
        if(buf[i]==0x47 && buf[i+DLNA_MPEG_TS_SIZE_ISO]==0x47){
            MESSAGE(VERBOSE_METADATA, "AVDetector: MPEG TS ISO Video container found");
            PaketSize = DLNA_MPEG_TS_SIZE_ISO;
            VCP = DLNA_VCP_MPEG2_TS_ISO;
            break;
        }
        else if(buf[i+4]==0x47 && buf[i+DLNA_MPEG_TS_SIZE]==0x47){
            PaketSize = DLNA_MPEG_TS_SIZE;
            if(buf[i]==0x00 && buf[i+1]==0x00 && buf[i+1]==0x00 && buf[i+1]==0x00){
                MESSAGE(VERBOSE_METADATA, "AVDetector: MPEG TS DLNA with zero value time stamp found");
                VCP = DLNA_VCP_MPEG2_TS_T;
                break;
            }
            else {
                MESSAGE(VERBOSE_METADATA, "AVDetector: MPEG TS DLNA with non-zero value time stamp found");
                VCP = DLNA_VCP_MPEG2_TS;
                break;
            }
        }
        else {
            VCP = DLNA_VCP_UNKNOWN;
        }
    }

    // TODO: MPEG-PS-Header

    return VCP;
}

VideoContainerProfile cContainerDetector::detectMPEG1Container(AVFormatContext* ){
    return DLNA_VCP_UNKNOWN;
}

VideoContainerProfile cContainerDetector::detectMP4Container(AVFormatContext* ){
	MESSAGE(VERBOSE_METADATA, "AVDetector: detectMP4Container returns DLNA_VCP_UNKNOWN");
    return DLNA_VCP_UNKNOWN;
}

#ifdef WITH_WINDOWS_MEDIA
VideoContainerProfile cContainerDetector::detectWMFContainer(AVFormatContext* ){
    return DLNA_VCP_UNKNOWN;
}
#endif