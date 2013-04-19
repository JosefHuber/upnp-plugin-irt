/*
 * File:   avdetector.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 26. Oktober 2009, 13:02
 * Last modification: March 22, 2013
 */

#ifndef _AVDETECTOR_H
#define	_AVDETECTOR_H

#include "profiles.h"
#include <vdr/recording.h>
#include <vdr/channels.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

/**
 * The audio/video detector
 *
 * This is the audio video detector, which analizes the audio and video stream
 * of a file to gather more information about the resource. This is also
 * required for determination of a suitable DLNA profile.
 */
class cAudioVideoDetector {
private:
    void init();
    void uninit();
    int detectChannelProperties();
    int detectFileProperties();
    int detectRecordingProperties();
    /**
     * Detect video properties
     *
     * This detects video properties of a video stream
     *
     * @return returns
     * - \bc 0, if the detection was successful
     * - \bc <0, otherwise
     */
    int analyseVideo(AVFormatContext* FormatCtx);
    /**
     * Detect audio properties
     *
     * This detects audio properties of a video or audio stream
     *
     * @return returns
     * - \bc 0, if the detection was successful
     * - \bc <0, otherwise
     */
    int analyseAudio(AVFormatContext* FormatCtx);
    /**
     * Detect DLNA profile
     *
     * This detects the matching DLNA profile for the video or audio item.
     *
     * @return returns
     * - \bc 0, if the detection was successful
     * - \bc <0, otherwise
     */
    int detectDLNAProfile(AVFormatContext* FormatCtx);

    UPNP_RESOURCE_TYPES mResourceType;
    union {
        const cChannel*     Channel;        ///<  the pointer to the <code>cChannel</code> instance in case of a broadcast channel
        const cRecording*   Recording;      ///<  the pointer to the <code>cRecding</code> instance in case of a recording
        const char*         Filename;       ///<  the file name in case of a file
    }                       mResource;
    int                     mWidth;
    int                     mHeight;
    int                     mBitrate;
    int                     mBitsPerSample;
    int                     mColorDepth;
    off64_t                 mDuration;
    off64_t                 mSize;
    int                     mSampleFrequency;
    int                     mNrAudioChannels;
    DLNAProfile*            mDLNAProfile;
public:
    /**
	 * Construct an instance of the class
	 * @param Filename the file name
	 */
    cAudioVideoDetector(const char* Filename);
	/**
	 * Construct an instance of the class
	 * @param Channel the <code>cChannel</code> instance
	 */
    cAudioVideoDetector(const cChannel* Channel);
	/**
	 * Construct an instance of the class
	 * @param Recording the <code>cRecording</code> instance
	 */
    cAudioVideoDetector(const cRecording* Recording);
	/**
	 * Destruction of the class instance
	 */
    virtual ~cAudioVideoDetector();
    /**
     * Detect resource properties of the file
     *
     * This detects the resource properties of a file. If the returned value
     * is 0, no erros occured while detection and the properties are properly
     * set. Otherwise, in case of an error, the properties may have
     * unpredictable values.
     *
     * @return returns
     * - \bc 0, if the detection was successful
     * - \bc <0, otherwise
     */
    int detectProperties();
	/**
	 * Get the DLNA profile
	 * @return the DLNA profile
	 */
    DLNAProfile* getDLNAProfile() const { return this->mDLNAProfile; }
	/**
	 * Get the content type
	 * @return the content type
	 */
    const char* getContentType() const { return (this->mDLNAProfile) ? this->mDLNAProfile->mime : NULL; }
	/**
	 * Get the DLNA/UPnP ProtocolInfo value
	 * @return the DLNA/UPnP ProtocolInfo value
	 */
    const char* getProtocolInfo() const;
	/**
	 * Get the width in case of a video
	 * @return the width in pixel
	 */
    int         getWidth() const { return this->mWidth; }
	/**
	 * Get the height in case of a video
	 * @return the heigth in pixel
	 */
    int         getHeight() const { return this->mHeight; }
	/**
	 * Get the bit rate
	 * @return the bit rate
	 */
    int         getBitrate() const { return this->mBitrate; }
	/**
	 * Get the bits per sample value
	 * @return the bits per sample value
	 */
    int         getBitsPerSample() const { return this->mBitsPerSample; }
	/**
	 *  Get the sample frequency in Hz
	 *  @return the sample frequency in Hz
	 */
    int         getSampleFrequency() const { return this->mSampleFrequency; }
	/**
	 * Get the number of audio channels
	 * @return the number of audio channels
	 */
    int         getNumberOfAudioChannels() const { return this->mNrAudioChannels; }
	/**
	 * Get the color depth
	 * @return the color depth
	 */
    int         getColorDepth() const { return this->mColorDepth; }
	/**
	 * Get the file size in bytes
	 * @return the file size in bytes
	 */
    off64_t     getFileSize() const { return this->mSize; }
	/**
	 * Get the duration in seconds
	 * @return the duration in seconds
	 */
    off64_t     getDuration() const { return this->mDuration; }
};
/**
 * The class contains static methods which can be used with audio/video considerations.
 */
class cCodecToolKit {
private:
#ifndef AVCODEC53
    /**
     * Get the stream index to access AVFormatContext->stream[]
     * @param FormatCtx the pointer to the <code>AVFormatContext</code> instance
     * @param Type the actual CodecType value
     * @return the stream index or -1 if not available
     */
    static int getContextIndex (AVFormatContext* FormatCtx, CodecType Type);
#endif
#ifdef AVCODEC53
    /**
     * Get the stream index to access AVFormatContext->stream[]
     * @param FormatCtx the pointer to the <code>AVFormatContext</code> instance
     * @param Type the actual AVMediaType value
     * @return the stream index or -1 if not available
     */
    static int getContextIndex (AVFormatContext* FormatCtx, AVMediaType Type);
#endif
public:
#ifndef AVCODEC53
    /**
	 * Get the AVCodecContext of the first stream for a codec type.
	 * @param FormatCtx a <code>AVFormatContext</code> instance
	 * @param Type the actual CodecType value
	 * @return the matching <code>AVCodecContext</code> instance
	 */
    static AVCodecContext* getFirstCodecContext(AVFormatContext* FormatCtx, CodecType Type);
	/**
	 * Get the AV Stream properties of the first stream for a codec type.
	 * @param FormatCtx a <code>AVFormatContext</code> instance
	 * @param Type the codec type to check against
	 * @return the matching <code>AVStream</code> instance
	 */
    static AVStream* getFirstStream(AVFormatContext* FormatCtx, CodecType Type);
#endif
#ifdef AVCODEC53
    /**
	 * Get the AVCodecContext of the first stream for a codec type.
	 * @param FormatCtx a <code>AVFormatContext</code> instance
	 * @param Type the actual AVMediaType value
	 * @return the matching <code>AVCodecContext</code> instance
	 */
    static AVCodecContext* getFirstCodecContext(AVFormatContext* FormatCtx, AVMediaType Type);
	/**
	 * Get the AV Stream properties of the first stream for a codec type.
	 * @param FormatCtx a <code>AVFormatContext</code> instance
	 * @param Type the AVMediaType to check against
	 * @return the matching <code>AVStream</code> instance
	 */
    static AVStream* getFirstStream(AVFormatContext* FormatCtx, AVMediaType Type);
#endif
	/**
	 * Check whether a format matches one of the expected bit rates.
	 * @param Bitrates an AcceptedBitrates array
	 * @param Codec the <code>AVCodecContext</code> instance to check against
	 * @return true if the bit rate matches, false if not
	 */
    static bool matchesAcceptedBitrates(AcceptedBitrates Bitrates, AVCodecContext* Codec);
	/**
	 * Check whether a format matches one of the expected system bit rates.
	 * @param Bitrate an AcceptedBitrates array
	 * @param Format the <code>AVFormatContext</code> instance to check against
	 * @return true if the bit rate matches, false if not
	 */
    static bool matchesAcceptedSystemBitrate(AcceptedBitrates Bitrate, AVFormatContext* Format);
	/**
	 * Check whether a codec matches one of the expected audio channels.
	 * @param Channels the AcceptedAudioChannels array
	 * @param Codec the <code>AVCodecContext</code> instance to check against
	 * @return true if the channel matches, false if not
	 */
    static bool matchesAcceptedAudioChannels(AcceptedAudioChannels Channels, AVCodecContext* Codec);
	/**
	 * Check whether a codec matches one of the expected sampling rates.
	 * @param SamplingRates an sampling rates array
	 * @param Codec the <code>AVCodecContext</code> to check against
	 * @return true if the sampling rate matches, false if not
	 */
    static bool matchesAcceptedSamplingRates(AcceptedSamplingRates SamplingRates, AVCodecContext* Codec);
	/**
	 * Check whether the video has one the expected resolutions in width, height and frame rate.
	 * @param Resolutions a resolutions array
	 * @param Count the size of the resolutions array
	 * @param Stream the <code>AVStream</code> instance to compare against
	 * @return true if the resolution matches, false if not
	 */
    static bool matchesAcceptedResolutions(AcceptedResolution *Resolutions, int Count, AVStream* Stream);
};

#endif	/* _AVDETECTOR_H */

