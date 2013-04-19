/*
 * File:   webserver.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 30. Mai 2009, 18:13
 * Last modification: April 9, 2013
 */

#ifndef _UPNPWEBSERVER_H
#define	_UPNPWEBSERVER_H

#include "../common.h"
#include <upnp/upnp.h>
#include "resources.h"
#include <pthread.h>
/**
 * The struct handles the combination of container ID and the last access time.
 */
struct tRecordTimerPlay {
private:
  int containerId;
  time_t lastAccess;
public:
  /**
   * Construct this
   * @param contId  the container ID
   * @param access  the system time of the last access
   */
  tRecordTimerPlay (int contId, time_t access) {containerId = contId; lastAccess = access; }
  /**
   * Get the container ID
   * @return the container ID
   */
  int getContainerId(void) const { return containerId; }
  /**
   * Get the last access system time in seconds.
   * @return the last access system time
   */
  time_t getLastAccess(void) const { return lastAccess; }
  /**
   * Set the last access system time in units of seconds.
   * @param access the last access system time
   */
  void setLastAccess(time_t access) { lastAccess = access; }
};

//static pthread_mutex_t mutexrt1;

/**
 * The internal webserver
 *
 * This is the internal webserver. It distributes all the contents of the
 * UPnP-Server.
 *
 */
class cUPnPWebServer {
    friend class cUPnPServer;
private:
    static cUPnPWebServer *mInstance;
    static UpnpVirtualDirCallbacks mVirtualDirCallbacks;
	static cVector<cUPnPClassObject*> epgVector;         ///< used to prevent an EPG folder play
	static cVector<tRecordTimerPlay*> inhibitVector;     ///< used to prevent the last EPG item to invoke a record timer with an EPG folder play

    /**
     * Check if with the resource a record timer has to be triggered.
     * @params Resource the pointer to the cUPnPResource instance to be dealt
     * @return true if successful, false if not
     */
    static bool handleRecordTimer(cUPnPResource* Resource);
    /**
     * Prepare a record timer. Check for automatic folder play.
     * @params obj the epg item UPnP object for which the disc record is planned
     * @return true if successfull, false if not
     */
    static bool scheduleEpgTimer(cUPnPClassObject* obj);

	pthread_mutex_t mutexrt1;
    const char* mRootdir;
    cUPnPWebServer(const char* root = "/");
protected:
public:
    /**
     * Initializes the webserver
     *
     * It enables the webserver which comes with the <em>Intel SDK</em> and creates
     * virtual directories for shares media.
     *
     * @return returns
     * - \bc true, if initializing was successful
     * - \bc false, otherwise
     */
    bool init();
    /**
     * Uninitializes the webserver
     *
     * This stops the webserver.
     *
     * @return returns
     * - \bc true, if initializing was successful
     * - \bc false, otherwise
     */
    bool uninit();
	/**
	 * The function used for creating a thread to create a new record timer
	 * @params epgItem a pointer to the epg item used to create the new timer
	 */
    static void *newTimerFunction(void *epgItem);
    /**
     * Create a new VDR record timer using an UPnP EPG item and a UPnP broadcast channel container.
	 * A VDR recording task is scheduled with the start and stop time from the EPG item and the
	 * channel ID from the broadcast channel object.
     * @param epgItem the <code>cUPnPClassEpgItem</code> instance to be used for creating a record timer
     * @param channelObj the <code>cUPnPClassContainer</code> associated to the used UPnP EPG item
     * @return true if successfull, false if not
     */
    static bool createNewTimer(cUPnPClassEpgItem* epgItem, cUPnPClassContainer* channelObj);
    /**
     * Returns the instance of the webserver
     *
     * Returns the instance of the webserver. This will create a single
     * instance of none is existing on the very first call. A subsequent call
     * will return the same instance.
     *
     * @return the instance of webserver
     */
    static cUPnPWebServer* getInstance(
        const char* rootdir = "/" /**< the root directory of the webserver */
    );
    virtual ~cUPnPWebServer();

    /****************************************************
     *
     *  The callback functions for the webserver
     *
     ****************************************************/

    /**
     * Retrieve file information
     *
     * Returns file related information for an virtual directory file.
     *
     * @return 0 on success, -1 otherwise
     * @param filename The filename of which the information is gathered
     * @param info     The File_Info structure with the data
     */
    static int getInfo(const char* filename, struct File_Info* info);
    /**
     * Opens a virtual directory file
     *
     * Opens a file in a virtual directory with the specified mode.
     *
     * Possible modes are:
     * - \b UPNP_READ,    Opens the file for reading
     * - \b UPNP_WRITE,    Opens the file for writing
     *
     * It returns a file handle to the opened file, NULL otherwise
     *
     * @return FileHandle to the opened file, NULL otherwise
     * @param filename The file to open
     * @param mode UPNP_WRITE for writing, UPNP_READ for reading.
     */
    static UpnpWebFileHandle open(const char* filename, UpnpOpenFileMode mode);
    /**
     * Reads from the opened file
     *
     * Reads <code>buflen</code> bytes from the file and stores the content
     * to the buffer.
     *
     * Returns 0 no more bytes read (EOF)
     *         >0 bytes read from file
     *
     * @return number of bytes read, 0 on EOF
     * @param fh the file handle of the opened file
     * @param buf the buffer to write the bytes to
     * @param buflen the maximum count of bytes to read
     *
     */
    static int read(UpnpWebFileHandle fh, char* buf, size_t buflen);
    /**
     * Writes to the opened file
     *
     * Writes <code>buflen</code> bytes from the buffer and stores the content
     * in the file.
     *
     * Returns >0 bytes wrote to file, maybe less the buflen in case of write
     * errors
     *
     * @return number of bytes read, 0 on EOF
     * @param fh the file handle of the opened file
     * @param buf the buffer to read the bytes from
     * @param buflen the maximum count of bytes to write
     *
     */
    static int write(UpnpWebFileHandle fh, char* buf, size_t buflen);
    /**
     * Seek in the file
     *
     * Seeks in the opened file and sets the file pointer to the specified offset.
     *
     * Returns 0 on success, non-zero value otherwise
     *
     * @return 0 on success, non-zero value otherwise
     * @param fh the file handle of the opened file
     * @param offset a negative oder positive value which moves the pointer
     *        forward or backward
     * @param origin SEEK_CUR, SEEK_END or SEEK_SET
     *
     */
    static int seek(UpnpWebFileHandle fh, off_t offset, int origin);
    /**
     * Closes the file
     *
     * closes the opened file
     *
     * Returns 0 on success, non-zero value otherwise
     *
     * @return 0 on success, non-zero value otherwise
     * @param fh the file handle of the opened file
     *
     */
    static int close(UpnpWebFileHandle fh);
};

#endif	/* _UPNPWEBSERVER_H */

