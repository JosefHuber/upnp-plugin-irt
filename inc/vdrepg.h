/*
 * File:   vdrepg.h    
 * Author: J.Huber, IRT GmbH
 * Created on April 4, 2012
 * Last modification: February 25, 2013
 */

#ifndef __VDREPG_H
#define __VDREPG_H

#include <time.h>
#include <vdr/tools.h>
#include "database.h"
#include "metadata.h"
#include "split.h"

#define DO_TRIGGER_TIMER    1
#define PURGE_RECORD_TIMER  8
#define EPG_EVENTS_PER_CHANNEL_MAX  5000
#define MAX_NUMBER_EPG_CHANNELS  999999
#define EPG_RECORD_CONFIRMATION_FOLDER "EpgProgrammingConfirmation"
#define REC_TIMER_PURGE_CONFIRMATION_FOLDER "RecTimerPurgeConfirmation"
#define EPG_RECORD_CONFIRMATION_FILE "00001.ts"

enum eCompareAction {actionNone, actionUpdate, actionInsert, actionDelete};

 /**
  * This class contains methods for reading and comparing DVB-SI EPG data extracted 
  * by the VDR from the transport stream into a file. 
  * Broadcast channels found in the EPG file are transferred to UPnP EPG containers when 
  * the upnp-plugin '-E' start option is selected.
  * TV and radio channels are separated.
  * Broadcast event related EPG items are created and put into the EPG containers.
  * Channel and event informations are stored in the SQLite tables
  * 'EPGChannels' and 'BCEvents'.
  * The number of EPG preview days and the amount of the selected channels can be determined
  * with upnp-plugin start options.
  * The UPnP EPG items are updated periodically.
  */
class cVdrEpgInfo {

private:
  int  mAmountChannels;
  char *channelName;
  char *fileName;
  cSQLiteDatabase* mDatabase;
  cMediaDatabase*  mMetaData;
  sqlite3_stmt*    mBceSelStmt;
  sqlite3_stmt*    mEChCntStmt;
  sqlite3_stmt*    mEChSelStmt;
  
  /**
   * The EPG file is parsed and UPnP EPG items and channel containers are created and stored in the
   * database.
   * @param f the pointer to the FILE instance of the EPG text file
   */
  bool Read(FILE *f);
  /**
   * Insert the channel items to the database table EPGChannels.
   * @param splitter a pointer to the cSplit instance
   * @param channelDescr a space separated string containing the channel ID and the channel name
   * @param epgTvContainer the cUPnPClassContainer instance for the UPnP EPG TV container
   * @param epgRadioContainer the cUPnPClassContainer instance for the UPnP EPG Radio container or NULL if no radio
   * @param channelContainer the cUPnPClassEpgContainer instance with the UPnP channel container, created by the method
   * @return true if successful, false otherwise
   */
  bool fillChannelTable(cSplit* splitter, char* channelDescr, cUPnPClassContainer* epgTvContainer, 
                        cUPnPClassContainer* epgRadioContainer, cUPnPClassEpgContainer** channelContainer);

   /**
   * Insert the broadcast event items to the database table BCEvents.
   * @param splitter a pointer to the cSplit instance
   * @param eventDescr a space separated string containing the event ID, start time, duration, table ID and version
   * @param eventTitle the title of the event
   * @param eventShortTitle the short title of the event
   * @param eventSynopsis the long description of the event
   * @param eventGenres the genres as space separated numbers defined in EN 300 468 
   * @param channelContainer the cUPnPClassEpgContainer instance with the UPnP EPG channel container 
   *                         into which the event has to be added
   * @param timerResource a pointer to the cUPnPResource instance of a timer resource, or null if not available
   * @param resourceName   the EPG confirmation resource file name
   * @param resourceSize  a pointer to the EPG confirmation resource file size
   * @return true if successful, false otherwise
   */
  bool fillEventTable(cSplit* splitter, char* eventDescr, char* eventTitle, const char* eventShortTitle,
                      const char* eventSynopsis, const char* eventGenres, cUPnPClassEpgContainer* channelContainer, 
					  cUPnPResource** timerResource, char* resourceName, off_t *resourceSize);
  /**
   * Update an EPG broadcast event.
   * @param splitter a pointer to the cSplit instance
   * @param eventDescr a space separated string containing the event ID, start time, duration, table ID and version
   * @param eventTitle the title of the event
   * @param eventShortTitle the short title of the event
   * @param eventSynopsis the long description of the event
   * @param channelContainer the cUPnPClassEpgContainer instance with the UPnP EPG channel container 
   *                         into which the event has to be added
   * @param epgId the UPnP object ID of the UPnP object that has to be modified
   * @return true if successful, false otherwise
   */   
  bool updateEventTable(cSplit* splitter, char* eventDescr, char* eventTitle, const char* eventShortTitle,
                        const char* eventSynopsis, cUPnPClassEpgContainer* channelContainer, int epgId);
					  
  /**
   * Delete all objects whoose object IDs are stored within the vector objVector from the tables 'Objects' and 'BCEvents'.
   * The vector is cleared after deletion.
   * @param objVector the vector with the objectIDs to be deleted.
   */
  bool deleteEpgEvents(std::vector<int>* objVector);
  /**
   * Find a timer resource from a movie record.
   * @param timerResource a pointer to the cUPnPResource instance of a timer resource, or null if not available
   * @param resourceName  a pointer to the EPG confirmation resource file name
   * @param resourceSize  a pointer to the EPG confirmation resource file size
   * @return true if a timer resource was found, false otherwise
   */
  bool findRecordTimerResource(cUPnPResource** timerResource, char** resourceName, off_t *resourceSize);
  /**
   * Update the EPG items of a channel, given by its serial number
   * @param virtualChannelNr the serial channel number of a channel to be updated
   * @return true if successful, false otherwise
   */
  bool updateVirtualChannel(int virtualChannelNr);
  /**
   * Compare the EPG data of one channel from text file with the database
   * @param serialNo the serial number of the channel within the EPG text file
   * @param channelObjId the object ID of the channel within the database
   * @param channelId    the broadcast channel ID
   * @return true if comparison was successful, false if not
   */
  bool compare (int serialNo, int channelObjId, const char* channelId);
  /**
   * Compare the EPG data of one channel of from text file with the database
   * @param file the pointer to the FILE instance of the EPG text file
   * @param serialNo the serial number of the channel within the EPG text file
   * @param channelObjId the object ID of the channel within the database
   * @param channelId    the broadcast channel ID
   * @return true if comparison was successful, false if not
   */
  bool compare (FILE* file, int serialNo, int channelObjId, const char* channelId);
  /**
   * Check wheather an EPG event has to be added to the BCEvents table
   * @param epgEvents a pointer to an cUPnPObjects instance with all EPG items belonging to the broadcast channel
   * @param splitter  a pointer to the cSplit instance
   * @param eventDescr a space separated string containing the event ID, start time, duration, table ID and version
   * @param eventTitle a pointer to the title of the event
   * @param eventShortTitle a pointer to the short title of the event
   * @param eventSynopsis a pointer to the long description of the event
   * @param eventObjId a pointer to the EPG event object id. The ID is set in the update case.
   * @param eventId    a pointer to the EPG event ID
   * @return the action to be performed with the EPG event
   */
  eCompareAction compareEpgEvents(cUPnPObjects* epgEvents, cSplit* splitter, char* eventDescr, char* eventTitle,
                        const char* eventShortTitle, const char* eventSynopsis, int* eventObjId, int* eventId);
  /**
   * Check whether events have to be deleted for a broadcast channel
   * @param epgEvents a pointer to an cUPnPObjects instance with all EPG items registered to the broadcast channel
   * @param eventVector a pointer to a vector with the actual event IDs for the channel, resulting from the epg text file
   * @return true if comparison was successful, false if not
   */   
  bool checkEventDeletion (cUPnPObjects* epgEvents,	std::vector<int>* eventVector);				
  /**
   * Split the event description string into the event ID, start time, duration, table ID and version.
   * @param splitter  a pointer to the cSplit instance
   * @param eventDescr a space separated string containing the event ID, start time, duration, table ID and version
   * @param eventId   a pointer to the event ID
   * @param startT    a pointer to the start time of the event
   * @param duration  a pointer to the duration of the event
   * @param tableId   a pointer to the table ID of the event
   * @param version   a pointer to the version of the event
   * @return true if successful, false if not
   */   
  bool splitEventDescription(cSplit* splitter, char* eventDescr, int* eventId, long* startT, int* duration, 
                             int* tableId, char** version);
  /**
   * Check whether the channel ID is within the specified first section of channel.conf
   * @param channelId the ID of the channel to be checked
   * @return true if the channel ID is within the specified first section of channel.conf, false if not
   */   
  bool checkChannel(char* channelId);
public:
  /**
   * Construct an instance of the class.
   * @param metaData a <code>cMediaDatabase</code> instance to be used
   */
  cVdrEpgInfo(cMediaDatabase* metaData);
  ~cVdrEpgInfo();
  /**
    * Read the EPG textual file produced by the VDR which contains informations about channels and broadcast events
	* and create UPnP EPG channels and items.
	* The number of EPG preview days and the amount of the selected channels can be determined
    * with upnp-plugin start options. The default value for number of preview days is 7 and all channels are selected by default.
    * @return true if successful, false otherwise
    */
  bool Read();
  /**
    * Detect and purge obsolete UPnP EPG items. 
    * @return true if successful, false otherwise
    */
  bool purgeObsoleteEpgObjects();
  /**
   * Update the UPnP EPG items attached to a channel, that is determined by the parameter 'virtualChannelNr' and
   * the total amount of the selected EPG channels using the modulo function.
   * @param virtualChannelNr a serial channel number for which the epg items should be updated
   * @return true if successful, false otherwise
   */ 
  bool updateEpgItems(int virtualChannelNr);
};

#endif //__VDREPG_H