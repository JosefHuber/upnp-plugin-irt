/*
 * File:   metadata.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 28. Mai 2009, 21:14
 * Last modified: April 9, 2013
 */

#ifndef _METADATA_H
#define	_METADATA_H

#include <vdr/tools.h>
#include <vdr/channels.h>
#include <vdr/recording.h>
#include <pthread.h>
#include "../common.h"
#include "database.h"
#include "object.h"
#include "mediator.h"
#include "resources.h"
#include "config.h"
#include "vdrepg.h"

/**
 * The result set of a request
 *
 * This contains the results of a previous \e Browse or \e Search request.
 */
struct cUPnPResultSet {
    int mNumberReturned; ///< The number of returned matches
    int mTotalMatches;   ///< The total amount of matches
    const char* mResult; ///< The DIDL-Lite fragment
};


/**
 * The media database
 *
 * This class acts as a global UPnP object manager. It holds every object in a local cache.
 * Only this class is allowed to create new objects.
 * The UPnP's broadcast channel, movie, audio record, EPG and record timer representation objects
 * are generated and periodically updated by this class.
 * If EPG items option is used with the start of this plugin, all existing EPG objects are deleted
 * and new created from the VDRs EPG file at plugin start time.
 * For performance reasons SQLite database starts with 'synchronous = OFF' and 'journal_mode = MEMORY'.
 * After startup the database access mode is switched to 'synchronous = NORMAL' and 'journal_mode = TRUNCATE'
 * allowing a higher database integrity.
 * In addition ten SQLite delete triggers which monitor the database integrity, are installed with
 * delay at startup time allowing a better start performance.
 * @see cUPnPClassObject
 */
class cMediaDatabase : public cThread {
    friend class cUPnPServer;
    friend class cUPnPObjectMediator;
private:
    unsigned int             mSystemUpdateID;
    cUPnPObjectFactory*      mFactory;
    cHash<cUPnPClassObject>* mObjects;
    cSQLiteDatabase*         mDatabase;
    cUPnPObjectID            mLastInsertObjectID;
    pthread_mutex_t          mutex_fastFind;
    pthread_mutex_t          mutex_system;
	bool                     mIsInitialised;
    int                      getNextObjectID();
    void                     cacheObject(cUPnPClassObject* Object);
	/**
	 * Prepare the database tables for usage. The basic UPnP containers are created.
	 * If EPG items are used the EPG containers are emptied (and refilled later).
	 */
    int                      prepareDatabase();
    /**
     * Activate triggers to be performed on the deletion of objects or items
     * @return true if successful, false if not
     */
    bool                     activateDeleteTriggers();
	/**
	 * Purge all objects within a container
	 * @param objId the object ID of the container
	 * @return true if successful, false if not
	 */
    bool                    purgeContainerObjects(int objId);
	/**
	 * Get the children list for an object id
	 * @param objId the object id
	 * @return a pointer to a cUPnPObjects instance or NULL if not successful
	 */
	cUPnPObjects*			getChildrenList(int objId);
	/**
	 *  Delete broadcast items given by its object IDs from the database tables.
	 *  @param bcVector a pointer to the vector containing the object IDs for the items to be deleted
	 *  @param upnpClass the UPnP class that specifies the kind of broadcast item to be deleted
	 */
	void deleteBroadcastItems(std::vector<int>* bcVector, const char* upnpClass);
	/**
	 * Store the UPnP object IDs from the bcObjects list into a vector.
	 * @param bcVector a pointer to the vector into which the object IDs are transferred
	 * @param bcObjects a cUPnPObjects instance with the objects to be transferred
	 * @param isVideo   is set if the video PID is not 0; otherwise not set
	 */
	void fetchBroadcastItems(std::vector<int>* bcVector, cUPnPObjects* bcObjects, bool isVideo);
	/**
	 * Check whether the broadcast channel is among the first <amountChannels> part from channels.conf
	 * @param channelNr the channel number to check against
	 * @param isVideo   is set if the video PID is not 0; otherwise not set
	 * @param amountChannels the amount of channels to check
	 * @param withoutCA is set if only free to air channels should be selected
	 * @return true if the channel number was found in the first part of the channel list, false if not
	 */
	bool isChannelAmongFirst(int channelNr, bool isVideo, int amountChannels, bool withoutCA);
#ifndef WITHOUT_TV
    int loadChannels();
    void updateChannelEPG();
#endif
#ifndef WITHOUT_AUDIO
	void updateRadioChannelEPG();
#endif
#ifndef WITHOUT_RECORDS
  /**
   * Find a resource that can be used with record timer deletion.
   * @param resourceName  a pointer to the EPG confirmation resource file name
   * @param resourceSize  a pointer to the EPG confirmation resource file size
   * @return true if a timer resource was found, false otherwise
   */
    bool findRecordTimerDeleteResource(char** resourceName, off_t *resourceSize);
	/**
	 * Load the record timer items from the 'timer.conf' file
	 * @return true if succesful, false if not
	 */
	bool   loadRecordTimerItems();
	/**
	 * Purge obsolete UPnP record timer items
	 * @return true if succesful, false if not
	 */
	bool   purgeObsoleteRecordTimerItems();
	/**
	 * Create a record timer item from a timer record line.
	 * @param timer a pointer to a cTimer instance
	 * @return true if successful, false if not
	 */
    bool   createRecordTimerItem(cTimer* timer,  char** resourceName, off_t* resourceSize);
	/**
	 * Add all object IDs from the objects belonging directly to the container to a vector
	 * @param containerId the container ID for which the underlying objects are searched
	 * @param idVector a pointer to the result vector with the object IDs
	 * @return true if successful false if not
	 */
	bool   addContainerObjectIds(int containerId, std::vector<int>* idVector);
	/**
	 * Check whether new record timer items have to be added to the record timer containers. Update if neccessary.
	 * @return true if successful false if not
	 */
	bool   updateRecordTimerItems();
    /**
     * Synchronize the hard disc record folder with the database.
     * Movie items can be added or deleted.
     * At the end of the record the file size and duration in the 'Resources' table is updated.
     * @return 0 if successful, -1 if not
     */
    int loadRecordings();
    /**
	 * Synchronize the hard disc record folder with the database. Update the database if necessary.
	 * The size and duration of a record may change during the recording process.
	 */
    void updateRecordings();
    /**
      * Determine the length in bytes of the HDD record(s) assigned to a record title .
      * @param Recording  the cRecording instance for the selected UPnP object
      * @param Index      the cIndexFile instance
      * @param sizeArray  the array with the file size for each record
      * @param arrLength  the total length of the result array
      * @return the number of the existing record files for the recording object
      */
    int readHddRecordSize(cRecording* Recording, cIndexFile* Index, off64_t* sizeArray, const int arrLength);

    /**
     * Synchronize hard disc records with the database: If records have been removed
     * from disc the database will be updated.
     */
    void checkDeletedHDDRecordings();
    /**
     * Update the size and duration of the resource object
     * @param Resource the pointer to the cUPnPResource instance
     * @return 0 if successful, -1 otherwise
     */
    int updateResSizeDuration(cUPnPResource* Resource);
#endif
#ifndef WITHOUT_EPG
  /**
    * Purge the existing UPnP objects associated to EPG channel containers from the db tables 'Objects' and 'EPGChannels'.
    * @return true if successful, false otherwise
    */
    bool purgeAllEpgChannels();
  /**
   * Delete all UPnP objects whoose object IDs are stored within the vector delVector.
   * The vector is cleared after deletion.
   * @param objVector the vector with the object IDs to be deleted.
   */
   bool purgeSelectedChannels(std::vector<int>* delVector);
   /**
    * Delete all UPnP (container) objects given by bcObjects
	* @param bcObjects the UPnP objects to be deleted
	* @return true if the deletion was successful, false if not
	*/
   bool deleteEpgContainer(cUPnPObjects* bcObjects);
    /**
     * Delete the objects and resources associated with epg items, and epg items within the table SearchClass
     * @return true if successful, false if not
     */
    bool deleteObjectsResourcesSearchClass();
    /**
     * Select objects from the resource table and delete them from the table Items
     * @return true if successful, false if not
     */
    bool selectAndDeleteItems();
#endif
    /**
     * Initialize the database, update video channels and recordings.
     */
    bool init();
    /**
     * Increment the SystemUpdateID in the db table 'System'.
     */
    void updateSystemID();
    /**
     * Implementation of the virtual cThread Action method.
     */
    virtual void Action();
public:
    /**
     * Returns the SystemUpdateID
     *
     * This returns the \e SystemUpdateID. This changes whenever anything changed
     * within the content directory. This value will be sent through the UPnP
     * network every 2 seconds.
     *
     * @return the SystemUpdateID
     */
    unsigned int getSystemUpdateID();
    /**
     * Returns a CSV list with ContainerUpdateIDs
     *
     * This list contains an unordered list of ordered pairs of ContainerID and
     * its ContainerUpdateID. It contains only recent changes which are not yet
     * beeing evented. This means that evented updates will be removed from list.
     *
     * @return CSV list of ContainerUpdateIDs
     */
    const char*  getContainerUpdateIDs();
    /**
     * Constructor
     *
     * Create an instance of the media database.
     */
    cMediaDatabase();
    virtual ~cMediaDatabase();
    /**
     * Add a Fastfind object-string pair
     *
     * This creates a \e Fastfind entry. It is a string which can be used to
     * relocate an object ID. Usually this is a file name, the broadcast channel's external representation
     * or another ID with which the related object can be found.
     *
     * @return returns
     * - \bc -1, if the creation was successful
     * - \bc 0, otherwise
     */
    int addFastFind(
        cUPnPClassObject* Object, ///< the <code>cUPnPClassObject</code> instance, which should be registered
        const char* FastFind      ///< the string with which the object shall be
                                  ///< relocated
    );
    /**
     * Finds a object by Fastfind
     *
     * Geit an UPnP object via a \e Fastfind string. The object must be
     * previosly registered via \c cMediaDatabase::addFastFind().
     *
     * The method tries to find the object in the internal object cache. If this fails,
     * the object will be loaded from the database.
     *
     * @see cMediaDatabase::addFastFind
     * @return The object associated with FastFind
     */
    cUPnPClassObject* getObjectByFastFind(
        const char* FastFind ///< the string with which the object shall be relocated
    );
    /**
     * Find an UPnP object by its ObjectID
     *
     * This returns the object via its \e ObjectID.
     *
     * It tries to find the object in the internal object cache. If this fails,
     * the object will be loaded from the database.
     *
     * @return The <code>cUPnPClassObject</code> instance registered with FastFind
     */
    cUPnPClassObject* getObjectByID(
        cUPnPObjectID ID ///< The <code>cUPnPObjectID</code> instance for the requested object
    );
    /**
     * Performs an UPnP content directory browse
     *
     * This method performs an UPnP object browse request and returns a structure
     * containing the matching count and DIDL-Lite fragement which is sent to
     * the requesting control point in case of success.
     *
     * @return returns an integer representing one of the following:
     * - \bc UPNP_CDS_E_INVALID_SORT_CRITERIA, when the sort criteria is malformed
     * - \bc UPNP_CDS_E_CANT_PROCESS_REQUEST, when there is an internal error while
     *                                        processing the request
     * - \bc UPNP_CDS_E_NO_SUCH_OBJECT, when the requested ObjectID does not exist
     * - \bc UPNP_SOAP_E_ACTION_FAILED, when the action failed due any reasons
     * - \bc UPNP_E_SUCCESS, if the request was successful
     */
    int browse(
        OUT cUPnPResultSet** Results,   ///< the <code>cUPnPResultSet</code> representation of the request.
		                                ///< This is a triple consisting of the number of returned matches,
		                                ///< the total amount of matches and the DIDL-Lite fragment in case of success.
        IN const char* ID,              ///< the UPnP object ID integer value for the request
        IN bool BrowseMetadata,         ///< \b true to browse the metadata of the given object, \b false to browse the direct children (of the container with that ID)
        IN const char* Filter = "*",    ///< the filter applied with the metadata selection, '*' means all items are selected
        IN unsigned int Offset = 0,     ///< the starting offset within a UPnP container
        IN unsigned int Count = 0,      ///< the maximum amount of items to be returned, '0' means all items
        IN const char* SortCriteria = "" ///< sorts the results before returning them
    );
    /**
     * Performs an UPnP content directory search
     *
     * This performs a search request on the database and returns a structure
     * containing the matching count and DIDL-Lite fragement which is sent to
     * the control point.
     *
     * @note
     * The submitted ID must be a ContainerID. Searches are performed only
     * in this container.
     *
     * @return returns an integer representing one of the following:
     * - \bc UPNP_CDS_E_INVALID_SORT_CRITERIA, when the sort criteria is malformed
     * - \bc UPNP_CDS_E_CANT_PROCESS_REQUEST, when there is an internal error while
     *                                        processing the request
     * - \bc UPNP_CDS_E_NO_SUCH_OBJECT, when the requested ObjectID does not exist
     * - \bc UPNP_SOAP_E_ACTION_FAILED, when the action failed due any reasons
     * - \bc UPNP_E_SUCCESS, if the request was successful
     */
    int search(
        OUT cUPnPResultSet** Results,       ///< the result of the request
        IN const char* ID,                  ///< the ContainerID
        IN const char* Search,              ///< the search string
        IN const char* Filter = "*",        ///< the filter applied to the returned metadata, '*' means all items are selected
        IN unsigned int Offset = 0,         ///< the starting offset within a UPnP container
        IN unsigned int Count = 0,          ///< the maximum amount of items to be returned, '0' means all items
        IN const char* SortCriteria = ""    ///< sorts the results before returning them
    );
};

#endif	/* _METADATA_H */

