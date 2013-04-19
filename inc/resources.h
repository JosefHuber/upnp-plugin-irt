/*
 * File:   resources.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 30. September 2009, 15:17
 * Last modification: April 9, 2013
 */

#ifndef _RESOURCES_H
#define	_RESOURCES_H

#include "object.h"
#include <vdr/channels.h>
#include <vdr/recording.h>
#include <pthread.h>

class cUPnPResourceMediator;
class cMediaDatabase;

/**
 * UPnP Resource
 *
 * This contains all details about a resource
 */
class cUPnPResource : public cListObject {
    friend class cUPnPResourceMediator;
    friend class cUPnPResources;
private:
    unsigned int mResourceID;
    int     mResourceType;
    cString mResource;
    cString mDuration;
    cString mResolution;
    cString mProtocolInfo;
    cString mContentType;
    cString mImportURI;
    off64_t mSize;
    unsigned int mBitrate;
    unsigned int mSampleFrequency;
    unsigned int mBitsPerSample;
    unsigned int mNrAudioChannels;
    unsigned int mColorDepth;
    int mRecordTimer;
    int mObjectId;
	bool mCacheAdded;
    cUPnPResource();

public:
    /**
     * Get resource ID
     *
     * Gets the resource ID
     *
     * @return the resource ID
     */
    unsigned int getID() const { return this->mResourceID; }
    /**
     * Get the resources
     *
     * Returns the resource. This is in most cases the file name or resource locator
     * where to find the resource.
     *
     * @return the resource string
     */
    const char* getResource() const { return this->mResource; }
    /**
     * Get the duration
     *
     * Returns a date time string with the duration of the resource.
     *
     * @return the duration of the resource
     */
    const char* getResDuration() const { return *this->mDuration; }
    /**
     * Get the resolution
     *
     * Returns the resolution string with the pattern width x height in pixels.
     *
     * @return the resolution of the resource
     */
    const char* getResolution() const { return this->mResolution; }
    /**
     * Get the protocol info
     *
     * This returns the protocol info field of a resource.
     *
     * @return the protocol info string
     */
    const char* getProtocolInfo() const { return this->mProtocolInfo; }
    /**
     * Get the content type
     *
     * Returns the mime type of the content of the resource.
     *
     * @return the content type of the resource
     */
    const char* getContentType() const { return this->mContentType; }
    /**
     * Get the import URI
     *
     * This returns the import URI where the resource was located before importing
     * it.
     *
     * @return the import URI
     */
    const char* getImportURI() const { return this->mImportURI; }
    /**
     * Get the resource type
     *
     * This returns the resource type of the resource.
     *
     * @return the resource type
     */
    int         getResourceType() const { return this->mResourceType; }
    /**
     * Get the file size
     *
     * Returns the file size in bytes of the resource or 0 if its unknown or a
     * stream.
     *
     * @return the file size
     */
    off64_t     getFileSize() const { return this->mSize; };
    /**
     * Get the last modification
     *
     * This returns the timestamp of the last modification to the file. If it
     * is a stream, then its the current time.
     *
     * @return the timestamp with the last modification of the resource
     */
    time_t      getLastModification() const;
    /**
     * Get the bitrate
     *
     * This returns the bitrate of the resource in bits per second.
     *
     * @return the bitrate of the resource
     */
    unsigned int getBitrate() const { return this->mBitrate; }
    /**
     * Get the sample frequency
     *
     * Returns the sample frequency in samples per second.
     *
     * @return the sample frequency of the resource
     */
    unsigned int getSampleFrequency() const { return this->mSampleFrequency; }
    /**
     * Get the bits per sample
     *
     * Returns the number of bits per sample.
     *
     * @return the bits per sample of the resource
     */
    unsigned int getBitsPerSample() const { return this->mBitsPerSample; }
    /**
     * Get number of audio channels
     *
     * Returns the number of audio channels of the audio stream in a video.
     *
     * @return the number of audio channels
     */
    unsigned int getNrAudioChannels() const { return this->mNrAudioChannels; }
    /**
     * Get the color depth
     *
     * Returns the color depth of the resource in pits per pixel.
     *
     * @return the color depth of the resource
     */
    unsigned int getColorDepth() const { return this->mColorDepth; }
    /**
     * Get the record timer flag for the resource.
     * @return 0 means a usual resoure. No record is planned when playing it;
     *         1 (DO_TRIGGER_TIMER) means the resource is used to trigger a record timer;
     *         8 (PURGE_RECORD_TIMER) means the resource is used to purge a scheduled timer
     */
    int getRecordTimer() const { return this->mRecordTimer; }
    /**
     * Get the ID of the associated UPnP object.
     * @return the ID of the associated UPnP object
     */
    int getObjectId() const {return this->mObjectId; }
    /**
     * Set the record timer flag for the resource.
     * @param recTimer 0 means a usual resoure. No record is planned when playing it.
     *                 1 means the resource is used to trigger a record timer.
     *                 8 means the resource is used to purge a scheduled timer
     */
    void setRecordTimer (int recTimer);
    /**
     * Set the file size in bytes.
     * @param size the file size in bytes
     */
    void setFileSize(off64_t size) {this->mSize = size; }
	/**
	 * Set the duration of the resource.
	 * @param duration the duration string
     */
    void setResDuration(char* duration);
    /**
     * Set the resolution of the resource.
     * @param resolution the resolution
     */
    void setResolution(char* resolution);
    /**
     * Set the number of audio channels for the resource.
     * @param nrAudioChann the number of audio channels
     */
    void setNrAudioChannels(unsigned int nrAudioChann);
    /**
     * Set the object ID associated to the resource.
     * @param objId the object ID associated to the resource
     */
    void setObjectId(int objId);
    /**
     * Set the bitrate.
     * @param bitRate the bitrate of the resource in bits per second
     */
    void setBitrate(unsigned int bitRate);
};

class cUPnPClassObject;
class cUPnPClassItem;
class cUPnPClassVideoItem;
class cUPnPClassVideoBroadcast;

/**
 * The resource manager
 *
 * He manages the resources in an internal cache as well as in the database. He may create new resources
 * for broadcast channels, recordings, custom files, EPG or a record timer items.
 */
class cUPnPResources {
private:
    cHash<cUPnPResource>*        mResources;
    static cUPnPResources*       mInstance;
    cUPnPResourceMediator*       mMediator;
    cSQLiteDatabase*             mDatabase;
    cUPnPResources();
	/**
	 * Add a resource to the cache
	 * @param resource the pointer to the resource to be added
	 */
	void addCache(cUPnPResource* resource);
public:
    /**
     * Fill object with its resources
     *
     * The method will load all the resources, which are associated
     * to the given object from the database.
     *
     * @param Object the object, which shall be filled
     * @return returns
     * - \bc 0, if loading was successful
     * - \bc <0, otherwise
     */
    int getResourcesOfObject(cUPnPClassObject* Object);
    /**
     * Loads all resources from database
     *
     * The method loads all resources from the database into the internal cache.
     *
     * @return returns
     * - \bc 0, if loading was successful
     * - \bc <0, otherwise
     */
    int loadResources();
    /*! @copydoc cUPnPResourceMediator::getResource */
    cUPnPResource* getResource(unsigned int ResourceID);
    virtual ~cUPnPResources();
    /**
     * Get the instance of the resource manager.
     *
     * This returns the instance of the resource manager.
     *
     * @return the instance of the manager
     */
    static cUPnPResources* getInstance();
    /**
     * Create resource from channel
     *
     * The method creates a new resource for the given channel. It determines the
     * kind of video or audio stream assigned to the channel and further details if available.
     * The resource is stored in the database after creation.
     *
     * @param Object the <code>cUPnPClassObject</code> instance which holds the resource
     * @param Channel the VDR TV or radio channel
     * @return returns
     * - \bc 0, if loading was successful
     * - \bc <0, otherwise
     */
    int createFromChannel(cUPnPClassObject* Object, cChannel* Channel);
    /**
     * Create resource from recording
     *
     * The method creates a new resource from a given recording. It determines the
     * kind of video stream assigned to the recording and further details if available.
     * The resource is stored in the database after creation.
     *
     * @param Object the <code>cUPnPClassObject</code> instance which is a video or audio item which holds the resource
     * @param Recording the VDR TV or audio recording
     * @return returns
     * - \bc 0, if loading was successful
     * - \bc <0, otherwise
     */
    int createFromRecording(cUPnPClassObject* Object, cRecording* Recording);
    /**
     * Create resource from file
     *
     * The method creates a new resource from the given file. It determines all available
     * information about the resource by analyzing the content. It stores
     * the resource in the database after creating it.
     *
     * @param Object the <code>cUPnPClassItem</code> instance which holds the resource
     * @param File the file name
     * @return returns
     * - \bc 0, if loading was successful
     * - \bc <0, otherwise
     */
    int createFromFile(cUPnPClassItem* Object, cString File);
    /**
     * Create a resource for an EPG item by copying an existing resource.
     * @param epgObject the <code>cUPnPClassObject</code> instance which is assigned to the copied resource
     * @param timerResource a <code>cUPnPResource</code> instance to be copied
     * @return 0 if successful, -1 otherwise
     */
    int createEpgResourceByCopy(cUPnPClassObject* epgObject, cUPnPResource* timerResource);
    /**
     * Create a resource for an EPG item or a record timer item. The short video assigned to the resource is used
	 * to confirm the scheduled recording task or the deletion of the scheduled task using an UPnP control point
	 * and a digital media player.
     * @param object the <code>cUPnPClassObject</code> instance to which the resource has to be assigned
     * @param fileName the fileName of the resource record
     * @param fileSize the file size of the resource record
	 * @param isEpgItem is set if the <code>cUPnPClassObject</code> is an EPG item,
	 *                  not set if the <code>cUPnPClassObject</code> is an instance of a record timer.
     * @return 0 if successful, -1 otherwise
     */
    int createConfirmationResource(cUPnPClassObject* object, const char* fileName, off_t fileSize, bool isEpgItem);
    /**
     * Delete the cached resources of an object.
     * @param delObject a pointer to the <code>cUPnPClassObject</code> instance which has the resource(s) to delete
     * @return 0 if OK, <0 in an error case
     */
    virtual int deleteCachedResources(cUPnPClassObject* delObject);
};

/**
 * The resource mediator
 *
 * This is another mediator which communicates with the database. It manages the
 * resources in the database.
 */
class cUPnPResourceMediator {
    friend class cUPnPResources;
    friend class cVdrEpgInfo;
private:
    cSQLiteDatabase* mDatabase;
    sqlite3_stmt* mPKSelStmt;
    sqlite3_stmt* mResInsStmt;
    sqlite3_stmt* mResUpdStmt;
    sqlite3_stmt* mResSelStmt;
    sqlite3_stmt* mPKUpdStmt;
    cUPnPResourceMediator();
    unsigned int getNextResourceID();
public:
    virtual ~cUPnPResourceMediator();
    /**
     * Get a resource by ID
     *
     * This returns a <code>cUPnPResource</code> instance by its resource ID.
     *
     * @param ResourceID the resource ID of the demanded resource
     * @return the requested resource
     */
    cUPnPResource* getResource(unsigned int ResourceID);
    /**
     * Save the resource
     *
     * This updates the information in the database with those in the <code>cUPnPResource</code>
     * object.
     * @param Object the associated <code>cUPnPClassObject</code> instance for the resource. The object ID is obtained from this parameter.
     * @param Resource the <code>cUPnPResource</code> instance which shall be saved
     * @return returns
     * - \bc 0, if saving was successful
     * - \bc <0, if an error occured
     */
    int saveResource(cUPnPClassObject* Object, cUPnPResource* Resource);
    /**
     * Create new resource
     *
     * This creates a new <code>cUPnPResource</code> instance and stores the skeleton in the database. The
     * new created resource will only contain the required information.
     * @param Object the <code>cUPnPClassObject</code> instance which will hold the resource
     * @param ResourceType the type of the resource
     * @param ResourceFile the file or URL, where the resource can be located
     * @param ContentType the mime type of the content
     * @param ProtocolInfo the protocol information of the resource
     * @param isEpgResource is set the resource is used as an epg record timer confirmation video, otherwise the resource is a usual one
     * @return the new created <code>cUPnPResource</code> instance
     */
    cUPnPResource* newResource(cUPnPClassObject* Object, int ResourceType, cString ResourceFile,
	                           cString ContentType, cString ProtocolInfo, bool isEpgResource);
};

#endif	/* _RESOURCES_H */

