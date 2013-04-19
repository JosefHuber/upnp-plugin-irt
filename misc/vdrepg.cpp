/*
 * vdrepg.c: vdr_epg file handling; fill the EPG database tables 'BCEvents' and 'EPGChannels'.
 *
 * Author: J.Huber, IRT GmbH
 * Created on April 19, 2012
 * Last modification: April 9, 2013
 */
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include "object.h"
#include "mediator.h"
#include "resources.h"
#include "vdrepg.h"
#include "../upnp.h"

// --- cVdrEpgInfo --------------------------------------------------------

cVdrEpgInfo::cVdrEpgInfo(cMediaDatabase* metaData) {
	this->mMetaData = metaData;
	this->mDatabase = cSQLiteDatabase::getInstance();
	this->mBceSelStmt = NULL;
	this->mEChCntStmt = NULL;
	this->mEChSelStmt = NULL;
	this->mAmountChannels = 0;
	fileName = cUPnPConfig::get()->mEpgFile;
}

cVdrEpgInfo::~cVdrEpgInfo(){
}

bool cVdrEpgInfo::Read(FILE *f) {
	this->mAmountChannels = 0;
	cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
	cUPnPObjectID* EpgTvID = new cUPnPObjectID(EPG_TV_ID);
	cUPnPClassContainer* epgTvContainer = (cUPnPClassContainer*) factory->getObject(*EpgTvID);
	if (epgTvContainer == NULL){
		ERROR("The UPnP EPG TV container object was not found whith VDR EPG info read");
		return false;
	}
	MESSAGE(VERBOSE_PARSERS, "The EPG TV container object was found");
	cUPnPClassContainer* epgRadioContainer = NULL;
	cUPnPObjectID* EpgRadioID = new cUPnPObjectID(EPG_RADIO_ID);
	epgRadioContainer = (cUPnPClassContainer*) factory->getObject(*EpgRadioID);
	if (cUPnPConfig::get()->mRadioShow && epgRadioContainer == NULL){
		ERROR("The UPnP EPG RADIO container object was not found whith VDR EPG info read");
		return false;
	}

	cUPnPResource* timerResource = NULL;
	char* resourceName;
	off_t resourceSize;
	if (!findRecordTimerResource(&timerResource, &resourceName, &resourceSize)){
		MESSAGE(VERBOSE_PARSERS, "No movie object with a resource that can be used as record timer trigger was found");
 	}
    
	cUPnPClassEpgContainer* channelContainer;
	 int channelSerialNumber = 0;
	 int eventNumber = 0;
	 int ctr = 0;
	 cSplit* splitter = new cSplit();
     cReadLine ReadLine;
	 char *eventString = NULL;
	 char *eventTitle = NULL;
	 char *eventShortTitle = NULL;
	 char *eventSynopsis = NULL;
	 char *eventGenres = NULL;
     char *s;
     int line = 0;	
	 int amountChannels = cUPnPConfig::get()->mFirstChannelsAmount;
	 bool channelAction = amountChannels <= 0;
	 bool finished = false;
	 if (amountChannels <= 0 && cUPnPConfig::get()->mWithoutCA){
		 amountChannels = MAX_NUMBER_EPG_CHANNELS;
	 }
	 MESSAGE(VERBOSE_PARSERS, "VDR_EPG: read the lines of the vdr file %s", fileName);
     while ((s = ReadLine.Read(f)) != NULL && (amountChannels <= 0 || !finished)) {
         ++line;
         char * t = skipspace(s + 1);
         switch (*s) {
			 case 'c': break;
             case 'C': 
				 if (t != NULL){
					 if (ctr > 0 && channelAction){
						 MESSAGE(VERBOSE_PARSERS, "VDR_EPG: %i EPG events have been stored for the channel", ctr);
					 }
					 if (channelAction && amountChannels > 0 && this->mAmountChannels >= amountChannels){
						 finished = true;
					 }
					 channelAction = false;
					 channelSerialNumber++;
//					 MESSAGE(VERBOSE_PARSERS, "VDR_EPG: Got key 'C' in line %d; channel number: %d; event number: %d; %s", line, channelSerialNumber, eventNumber, t);
					 if (!finished && amountChannels > 0 && checkChannel(t)){
						 if (fillChannelTable(splitter, t, epgTvContainer, epgRadioContainer, &channelContainer)){
							channelAction = true;
							this->mAmountChannels++;
						 }
					 }
					 else if (amountChannels <= 0 &&
						 fillChannelTable(splitter, t, epgTvContainer, epgRadioContainer, &channelContainer)) {
						 channelAction = true;
					 }
					 ctr = 0;
				 }
                 break;
			 case 'D':
				 if (t != NULL){
				     eventSynopsis = strdup(t);
				 }
				break;			
			 case 'e':
				 if (ctr++ < EPG_EVENTS_PER_CHANNEL_MAX && channelAction &&
					 fillEventTable(splitter, eventString, eventTitle, eventShortTitle, eventSynopsis, eventGenres, channelContainer, 
					                &timerResource, resourceName, &resourceSize)){
				 }
				 eventNumber++;
				 eventString = NULL;
				 eventTitle = NULL;
				 eventShortTitle = NULL;
				 eventSynopsis = NULL;
				 break;
             case 'E': 
				 if (t != NULL){
					 eventString = strdup(t);
				 }
                 break;
			 case 'G':
				 if (t != NULL){
					 eventGenres = strdup(t);
				 }
				 break;
			 case 'R':  // parental rating ignored
				 break;
			 case 'S':
				 if (t != NULL){
				     eventShortTitle = strdup(t);
				 }
				 break;
			 case 'T':
				 if (t != NULL){
				     eventTitle = strdup(t);
				 }
				 break;		
			 case 'V': break;		
			 case 'X': break;
             case '#': break; // comments are ignored
             default: 
				MESSAGE(VERBOSE_PARSERS, "VDR_EPG data: In line %d the key '%c' was not dealt", line, *s);
                 break;
          }
     }
	MESSAGE(VERBOSE_PARSERS, "VDR_EPG: number of vdr lines read: %d, number of channels: %i of %i, number of broadcast events: %d", 
		            line, this->mAmountChannels, amountChannels, eventNumber);
	if (!finished && amountChannels > 0){
		MESSAGE(VERBOSE_PARSERS, "VDR_EPG: NOT FINISHED state");
	}
	if (amountChannels <= 0){
		this->mAmountChannels = channelSerialNumber;
	}
    return true;
}

bool cVdrEpgInfo::Read() {
  bool Result = false;
  if (fileName) {
     FILE *f = fopen(fileName, "r");
     if (f) {
        if (Read(f)){
           Result = true;
		}
        else{
           esyslog("ERROR: VDR_EPG data problem in file %s", fileName);
		}
        fclose(f);
     }
     else if (errno != ENOENT){
	    MESSAGE(VERBOSE_PARSERS, "VDR_EPG: unknown error %s", fileName);
	 }
  }
  return Result;
}

 bool cVdrEpgInfo::compare (int serialNo, int channelObjId, const char* channelId){
  bool Result = false;
  if (fileName) {
     FILE *f = fopen(fileName, "r");
     if (f) {
        if (compare(f, serialNo, channelObjId, channelId)){
           Result = true;
		}
        else{
           esyslog("ERROR: VDR_EPG data problem in file %s", fileName);
		}
        fclose(f);
     }
     else if (errno != ENOENT){
	    MESSAGE(VERBOSE_PARSERS, "VDR_EPG: unknown error %s", fileName);
	 }
  }
  return Result;
 }

 bool cVdrEpgInfo::compare (FILE* file, int serialNo, int channelObjId, const char* channelId){
	 if (serialNo == 0 && this->mAmountChannels > 0){
		 serialNo = this->mAmountChannels;
	 }
     cReadLine ReadLine;
	 cUPnPClassEpgContainer* channelContainer = NULL;
	 cSplit* splitter = new cSplit();
	 cUPnPResource* timerResource = NULL;
	 char* resourceName;
	 off_t resourceSize = 0;
	 cUPnPObjects* epgEvents = NULL;
	 int channelSerialNumber = 0;
	 char *eventString = NULL;
	 char *eventTitle = NULL;
	 char *eventShortTitle = NULL;
	 char *eventSynopsis = NULL;
	 char *eventGenres = NULL;
     char *s;
     int line = 0;
	 int ctr = -1;
	 bool compareLoop = true;
	 bool doCompare = false;
	 bool deletionChecked = false;
	 int amountChannels = cUPnPConfig::get()->mFirstChannelsAmount;
	 if (amountChannels <= 0 && cUPnPConfig::get()->mWithoutCA){
		 amountChannels = MAX_NUMBER_EPG_CHANNELS;
	 }
	 std::vector<int> eventVector;
//	 MESSAGE(VERBOSE_PARSERS, "VDR_EPG: compare the lines of the vdr file %s, check channel serial number %i", fileName, serialNo);
     while ((s = ReadLine.Read(file)) != NULL && compareLoop) {
         ++line;
         char * t = skipspace(s + 1);
         switch (*s) {
			 case 'c': break;
             case 'C': 
				 if (t != NULL){
					 channelSerialNumber++;
					 if ((amountChannels <= 0 && channelSerialNumber == serialNo) || (amountChannels > 0 && !doCompare)){
//						 MESSAGE(VERBOSE_PARSERS, "VDR_EPG compare: Got channel '%s', serial channel number: %i", t, channelSerialNumber);
						 if (startswith((const char*)t, channelId)){
							 cUPnPObjectID ObjectID = channelObjId;
							 cUPnPClassObject* Object = cUPnPObjectFactory::getInstance()->getObject(ObjectID);
							 if (Object){
								 doCompare = true;
								 channelContainer = (cUPnPClassEpgContainer*) Object->getContainer();
								 epgEvents = channelContainer->getObjectList();
								 if (epgEvents == NULL){
									 ERROR("EPG item comparison: Got no object list with ID %i", channelObjId);
									 compareLoop = false;
								 }
								 else {
									 MESSAGE(VERBOSE_PARSERS, "VDR_EPG compare '%s': The channel ID matches; # epg events: %i; serial number: %i", 
										channelContainer->getChannelName(), epgEvents->Count(), channelSerialNumber);
								 }
							 }
							 else {
								 ERROR("EPG item comparison: Got no object with ID %i", channelObjId);
								 compareLoop = false;
							 }
						 }
						 else  if (amountChannels <= 0 && channelSerialNumber == serialNo){
							 ERROR("The channel serial numbers do not match with broadcast events update for channel ID: %s", channelId);
							 compareLoop = false;
						 }
					 }
					 else if (doCompare && !deletionChecked){
//						MESSAGE(VERBOSE_PARSERS, "VDR_EPG compare: the EPG text file has %i events for the channel", (int)eventVector.size());
						checkEventDeletion (epgEvents, &eventVector);
						deletionChecked = true;
						compareLoop = false;
					 }
					 ctr = 0;
				 }
                 break;
			 case 'D':
				 if (t != NULL && doCompare){
				     eventSynopsis = strdup(t);
				 }
				break;			
			 case 'e':
				 if (ctr++ < EPG_EVENTS_PER_CHANNEL_MAX && doCompare){
					int eventObjId = -1;
					int eventId = 0;
					eCompareAction epgAction = compareEpgEvents(epgEvents, splitter, eventString, eventTitle, eventShortTitle, eventSynopsis, &eventObjId, &eventId);
					switch (epgAction){
					case actionInsert:
						if (resourceSize == 0 && timerResource == NULL){
							if (!findRecordTimerResource(&timerResource, &resourceName, &resourceSize)){
								MESSAGE(VERBOSE_PARSERS, "No movie as a resource that can be used as record timer trigger was found");
 							}
						}
						fillEventTable(splitter, eventString, eventTitle, eventShortTitle, eventSynopsis, eventGenres, channelContainer,
							           &timerResource, resourceName, &resourceSize);
						break;
					case actionUpdate:
						updateEventTable(splitter, eventString, eventTitle, eventShortTitle, eventSynopsis, channelContainer, eventObjId);
						break;
					default:
						break;
					}
					if (eventId > 0){
						eventVector.push_back(eventId);
					}
				 }
				 eventString = NULL;
				 eventTitle = NULL;
				 eventShortTitle = NULL;
				 eventSynopsis = NULL;
				 break;
             case 'E': 
				 if (t != NULL && doCompare){
					 eventString = strdup(t);
				 }
                 break;
			 case 'G': 
				 if (t != NULL && doCompare){
				     eventGenres = strdup(t);
				 }
				 break;
			 case 'R':  // parental rating
				 break;
			 case 'S':
				 if (t != NULL && doCompare){
				     eventShortTitle = strdup(t);
				 }
				 break;
			 case 'T':
				 if (t != NULL && doCompare){
				     eventTitle = strdup(t);
				 }
				 break;		
			 case 'V': break;		
			 case 'X': break;
             case '#': break; // comments are ignored
             default: 
					MESSAGE(VERBOSE_PARSERS, "VDR_EPG data compare: In line %d the key '%c' was not dealt", line, *s);
                 break;
          }
     }

	 if (!deletionChecked){
//		MESSAGE(VERBOSE_PARSERS, "VDR_EPG compare: the EPG text file has %i events for the channel", (int)eventVector.size());
		checkEventDeletion (epgEvents, &eventVector);
	}
	MESSAGE(VERBOSE_PARSERS, "VDR_EPG compare: number of lines read: %d", line);
	 return true;
 }

eCompareAction cVdrEpgInfo::compareEpgEvents(cUPnPObjects* epgEvents, cSplit* splitter, char* eventDescr, char* eventTitle,
                                             const char* eventShortTitle, const char* eventSynopsis, int* eventObjId, int* eventId){
	int duration = 0;
	int tableId = 0;
	char* version = NULL;
	long startT = 0;
	bool found = false;
	*eventObjId = -1;
	eCompareAction eventAction = actionNone;
	bool splitSuccess = splitEventDescription(splitter, eventDescr, eventId, &startT, &duration, &tableId, &version);
    if (splitSuccess){
//		MESSAGE(VERBOSE_OBJECTS, "VDR_EPG event with starttime %ld, duration %i, event ID %i", startT, duration, eventId);
		cUPnPClassEpgItem* epgItem = (cUPnPClassEpgItem*) epgEvents->First();
		while (!found && epgItem){
			if (epgItem->getEventId() == *eventId){
				found = true;
				bool doUpdate = false;
				if (epgItem->getDuration() != duration || atol(epgItem->getStartTime()) != startT || strcmp(epgItem->getTitle(), eventTitle) != 0){
					if (epgItem->getDuration() != duration){
					    MESSAGE(VERBOSE_OBJECTS, "compareEpgEvents: '%s' duration mismatch, old: %i, new: %i", eventTitle, epgItem->getDuration(), duration);
						doUpdate = true;
					}
					else if (atol(epgItem->getStartTime()) != startT){
						MESSAGE(VERBOSE_OBJECTS, "compareEpgEvents: '%s' start time mismatch, old: %s, new: %ld", eventTitle, epgItem->getStartTime(), startT);
						doUpdate = true;
					}
					else if (strcmp(epgItem->getTitle(), eventTitle) != 0){
						MESSAGE(VERBOSE_OBJECTS, "compareEpgEvents: title mismatch, old: '%s', new: '%s'", epgItem->getTitle(), eventTitle);
						doUpdate = true;
					}
					else if ((epgItem->getShortTitle() != NULL && eventShortTitle == NULL) || (epgItem->getShortTitle() == NULL && eventShortTitle != NULL) ||
						     (epgItem->getShortTitle() != NULL && eventShortTitle != NULL && strcmp(epgItem->getShortTitle(), eventShortTitle) != 0)){
						if (eventShortTitle != NULL){
							MESSAGE(VERBOSE_OBJECTS, "compareEpgEvents: short title mismatch, got new: '%s'", eventShortTitle);
						}
						else {
							MESSAGE(VERBOSE_OBJECTS, "compareEpgEvents: short title mismatch, got an empty one");
						}
						doUpdate = true;
					}
					else if ((epgItem->getSynopsis() != NULL && eventSynopsis == NULL) || (epgItem->getSynopsis() == NULL && eventSynopsis != NULL) ||
						     (epgItem->getSynopsis() != NULL && eventSynopsis != NULL && strcmp(epgItem->getSynopsis(), eventSynopsis) != 0)){
						if (eventSynopsis != NULL){
							MESSAGE(VERBOSE_OBJECTS, "compareEpgEvents: synopsis mismatch, got new: '%s'", eventSynopsis);
						}
						else {
							MESSAGE(VERBOSE_OBJECTS, "compareEpgEvents: synopsis mismatch, got an empty one");
						}
						doUpdate = true;
					}
					if (doUpdate){
						eventAction = actionUpdate;
						*eventObjId = (int) epgItem->getID();
					}
				}
			}
			if (!found){
				epgItem = (cUPnPClassEpgItem*) epgEvents->Next(epgItem);
			}
		}

		if (!found && splitSuccess){
			time_t actTime;
			time (&actTime);
			if (startT > (long) actTime && startT > 0 && (int)(startT-(long)actTime) < cUPnPConfig::get()->mEpgPreviewDays * 24 * 3600){
				eventAction = actionInsert;
				MESSAGE(VERBOSE_OBJECTS, "The not registered epg item '%s' has to be added", eventTitle); 
			}
		}
	}
	else {
		ERROR("compareEpgEvents: Inernal error when splitting the event description: %s", eventDescr);
	}
	return eventAction;
}

bool cVdrEpgInfo::checkEventDeletion (cUPnPObjects* epgEvents, std::vector<int>* eventVector){
	if (epgEvents == NULL ){
		MESSAGE(VERBOSE_PARSERS, "checkEventDeletion: got an epg events NULL object");
		return 0;
	}
	if (eventVector == NULL){
		WARNING("checkEventDeletion: got an event vector NULL object");
		return 0;
	}
	int epgEventsCount = epgEvents->Count();
	int vdrEvSize = (int) eventVector->size();
	MESSAGE(VERBOSE_PARSERS, "VDR_EPG compare: the EPG text file has %i events for the channel; existing EPG events: %i", vdrEvSize, epgEventsCount);
	if (epgEventsCount <= 0){
		return 0; // no items to delete
	}

	int ctr = 0;	
	std::vector<int> delVector;
	for (cUPnPClassEpgItem* Child = (cUPnPClassEpgItem*) epgEvents->First(); Child && ctr++ < epgEventsCount; ){
		bool found = false;
		int eventId = Child->getEventId();
		for (int i = 0; i < vdrEvSize; i++){
			if (eventId == eventVector->at(i)){
				found = true;
				i = vdrEvSize; // stop loop
			}			
		}
		if (!found){
			MESSAGE(VERBOSE_OBJECTS, "The epg event with ID %i has to be deleted", eventId);
			delVector.push_back((int) Child->getID());
		}

		if (ctr < epgEventsCount){
			Child = (cUPnPClassEpgItem*) epgEvents->Next(Child);
		}
	}
	return deleteEpgEvents (&delVector);
}

bool cVdrEpgInfo::purgeObsoleteEpgObjects(){
	if (this->mBceSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s FROM %s WHERE %s<@TM", SQLITE_COL_OBJECTID, 
					  SQLITE_COL_BCEV_STARTTIME, SQLITE_COL_BCEV_DURATION, SQLITE_TABLE_BCEVENTS, SQLITE_COL_BCEV_STARTTIME);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mBceSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a broadcast event select: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}

	std::vector<int> objVector;
	time_t actTime;
	time (&actTime);
	char timeString[25];
	sprintf(timeString, "%ld", (long) actTime);
	MESSAGE(VERBOSE_OBJECTS, "VDR_EPG: purge obsolete epg objects, Time: %s", timeString);
	bool actionSuccess = sqlite3_bind_text(this->mBceSelStmt, 1, timeString, -1, SQLITE_TRANSIENT) == SQLITE_OK;
	int ctr = 0;
	const int maxCount = 299999;
	while (actionSuccess && ctr++ < maxCount){
		int objId = -1;
		long startT = 0;
		int dur = -1;
		int stepRes = sqlite3_step(this->mBceSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				objId = sqlite3_column_int(this->mBceSelStmt, 0);
				startT = (long) sqlite3_column_int(this->mBceSelStmt, 1);
				dur = sqlite3_column_int(this->mBceSelStmt, 2);
				if (objId > 0 && startT > 0 && dur > 0){
					if (startT + (long) dur < (long) actTime){
						objVector.push_back(objId);
					}
				}
				break;
			case SQLITE_DONE:
				ctr = maxCount; // stop loop
				break;
		}
	}
	sqlite3_clear_bindings(this->mBceSelStmt);
	sqlite3_reset(this->mBceSelStmt);

	MESSAGE(VERBOSE_OBJECTS, "VDR_EPG: number of obsoleted items %d", (int)objVector.size());
	bool dbError = !actionSuccess || !deleteEpgEvents(&objVector);
	return !dbError;
}

bool cVdrEpgInfo::updateEpgItems(int virtualChannelNr){
	MESSAGE(VERBOSE_METADATA, "Already stored EPG channels: %d", this->mAmountChannels);
	if (this->mAmountChannels == 0){
		Read();
		if (this->mAmountChannels == 0){
			return false;
		}
	}
	return updateVirtualChannel((virtualChannelNr % this->mAmountChannels) + 1);
}

 bool cVdrEpgInfo::updateVirtualChannel(int virtualChannelNr){
	if (this->mEChCntStmt == NULL){
		cString resStatement = cString::sprintf("SELECT Count(*) FROM %s", SQLITE_TABLE_EPGCHANNELS);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mEChCntStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for EPGChannels row count selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	int ctr = 0;
	int noEpgChannels = 0;
	const int maxLoopCount = 5;
	while (ctr++ < maxLoopCount){
		int stepRes = sqlite3_step(this->mEChCntStmt);
		switch (stepRes){
			case SQLITE_ROW:
				noEpgChannels = sqlite3_column_int(this->mEChCntStmt, 0);
				break;
			case SQLITE_DONE:
				ctr = maxLoopCount; // stop the loop
				break;
			default:
				MESSAGE(VERBOSE_METADATA, "updateVirtualChannel: Not dealt step result: %i", stepRes);
				break;
		}
	}
	sqlite3_reset(this->mEChCntStmt);

	if (virtualChannelNr > noEpgChannels){
		WARNING("Internal error: epg channel row number too high");
		return false;
	}
	pthread_mutex_lock(&(this->mDatabase->mutex_epgChannel));
    if (this->mEChSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s FROM %s", SQLITE_COL_OBJECTID, SQLITE_COL_CHANNELID, SQLITE_TABLE_EPGCHANNELS);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mEChSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for an EPG channel row selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	ctr = 0;
	int objId = -1;
	int chObjId = -1;
	const char* channelId = NULL;
	bool found = false;
	while (ctr++ < MAX_ROW_COUNT){
		int stepRes = sqlite3_step(this->mEChSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				objId = sqlite3_column_int(this->mEChSelStmt, 0);
				channelId = strdup0((const char*) sqlite3_column_text(this->mEChSelStmt, 1));
				if (ctr == virtualChannelNr){
					found = true;
					chObjId = objId;
					ctr = MAX_ROW_COUNT;  // stop loop
					MESSAGE(VERBOSE_METADATA, "Got the channel object ID: %i, channel ID: %s", objId, channelId);
				}
				break;
			case SQLITE_DONE:
				ctr = MAX_ROW_COUNT; // stop loop
				break;
			default:
				ERROR("updateVirtualChannel: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_reset(this->mEChSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_epgChannel));
	if (!found){
		ERROR("Internal error: with the virtual channel number %i no channel could be identified", virtualChannelNr);
		return false;
	}
	return compare (virtualChannelNr, chObjId, channelId);
 }

bool cVdrEpgInfo::fillChannelTable(cSplit* splitter, char* channelDescr, cUPnPClassContainer* epgTvContainer, cUPnPClassContainer* epgRadioContainer,
	                               cUPnPClassEpgContainer** channelContainer2){
	const int channelNameMax = 120;  
	char channelBuf [channelNameMax];
	std::vector<std::string>* tokens = splitter->split(channelDescr, " ");
	int ctr = 0;
	int accSize = 0;
	channelBuf[0] = 0;

	while (ctr < (int) tokens->size() && ctr < 50){
		if (ctr == 1){
			accSize = tokens->at(ctr).size();
			if (accSize < channelNameMax){
			    strcpy (channelBuf, tokens->at(ctr).c_str());
			}
		}
		else if (ctr > 1){
			accSize += tokens->at(ctr).size() + 1; 
			if (accSize < channelNameMax){
				strcat (channelBuf, " ");
				strcat (channelBuf, tokens->at(ctr).c_str());
			}
		}
		ctr++;
	}
	if (ctr > 0){
		cUPnPConfig* config = cUPnPConfig::get();
	    bool isRadio = false;
	    cString channelId = strdup0(tokens->at(0).c_str());
		tokens = splitter->split(tokens->at(0).c_str(), "-");
		if ((int) tokens->size() > 3){
			int serviceID = atoi(tokens->at(3).c_str());
			cChannel* Channel = NULL;
			for (int Index = 0; (Channel = Channels.Get(Index)); Index = Channels.GetNextNormal(Index)){
				if (serviceID == Channel->Sid()){
					if (ISRADIO(Channel)){
						isRadio = true;					   
					}
					break;
				}
			}
		}

	   if (config->mRadioShow || (!config->mRadioShow && !isRadio)){
		   cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
		   cUPnPClassEpgContainer* channelContainer = (cUPnPClassEpgContainer*)factory->createObject(UPNP_CLASS_EPGCONTAINER, channelBuf);
		   if (channelContainer){
			   channelContainer->setChannelId(channelId);
			   channelContainer->setChannelName(channelBuf);
			   channelContainer->setRadioChannel(isRadio);
			   if (isRadio && epgRadioContainer){
				   epgRadioContainer->addObject(channelContainer);
			   }
			   else {
				   epgTvContainer->addObject(channelContainer);
			   }
			   //cClass EpgClass = { UPNP_CLASS_EPGITEM, true };
			   //channelContainer->addSearchClass(EpgClass);
//			   channelContainer->setSearchable(true);
			   channelContainer->setSearchable(false);
			   if(factory->saveObject(channelContainer)) {
				   ERROR("The epg channel %s could not be stored.", channelBuf);
				   return false;
			   }
			   *channelContainer2 = channelContainer;
			   MESSAGE(VERBOSE_OBJECTS, "The EPG channel '%s' was saved in the database, channel container ID %s", 
				       channelBuf, *channelContainer->getID());
			   return true;
		   }
		   else {
			   	ERROR("Could not create an epg container object");
				*channelContainer2 = NULL;
		   }
	   }
	   else {
		   *channelContainer2 = NULL;
	   }
	}
	return false;
}

bool cVdrEpgInfo::fillEventTable(cSplit* splitter, char* eventDescr, char* eventTitle, const char* eventShortTitle,
	                             const char* eventSynopsis, const char* eventGenres, cUPnPClassEpgContainer *channelContainer, 
								 cUPnPResource** timerResource, char* resourceName, off_t *resourceSize){
	if (channelContainer == NULL){
		return true;      // this return applies if a radio event has not to be inserted
	}
	int eventId = 0;
	int duration = 0;
	int tableId = 0;
	char* version = NULL;
	long startT = 0;
	bool splitSuccess = splitEventDescription(splitter, eventDescr, &eventId, &startT, &duration, &tableId, &version);

	time_t actTime;
	time (&actTime);
	cUPnPConfig* config = cUPnPConfig::get();

	if (splitSuccess && (int)(startT-(long)actTime) < config->mEpgPreviewDays * 24 * 3600 && startT + (long) duration > (long) actTime){   
	   cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
//	   MESSAGE(VERBOSE_OBJECTS, "Create the broadcast event '%s', the EPG channel container has the ID %s", (eventTitle) ? eventTitle : "NO TITLE", *channelContainer->getID());
	   cUPnPClassEpgItem* bcObject = (cUPnPClassEpgItem*)factory->createObject(UPNP_CLASS_EPGITEM, (eventTitle) ? strdup(eventTitle) : "NO TITLE");
	   if (bcObject){
		   bcObject->setDuration(duration);
		   bcObject->setEventId (eventId);
		   bcObject->setTableId (tableId);
		   bcObject->setVersion(version);
		   char buffer [30];
		   sprintf(buffer, "%ld", startT);
		   bcObject->setStartTime(buffer);
		   bcObject->setSynopsis(eventSynopsis);
		   bcObject->setShortTitle(eventShortTitle);
		   bcObject->setGenres(eventGenres);
		   if (*resourceSize > 0){
			   cUPnPResources::getInstance()->createConfirmationResource(bcObject, (const char*) resourceName, *resourceSize, true);
		   }
		   else if (*timerResource){
			   cUPnPResources::getInstance()->createEpgResourceByCopy(bcObject, *timerResource);		   		   
			   WARNING("Missing the folder %s or its transport stream file. A dummy record timer resource was added.", EPG_RECORD_CONFIRMATION_FOLDER);
		   }
		   channelContainer->addObject(bcObject);
		   if (factory->saveObject(bcObject)) {
			   ERROR("The broadcast event %s could not be saved.", (eventTitle) ? eventTitle : "NO TITLE");
			   return false;
		   }
	   }
	   else {
		   ERROR("Could not create an epg item object");
		   return false;
	   }
	}
	return true;
}
bool cVdrEpgInfo::updateEventTable(cSplit* splitter, char* eventDescr, char* eventTitle, const char* eventShortTitle,
	                             const char* eventSynopsis, cUPnPClassEpgContainer *channelContainer, int epgId){
	if (channelContainer == NULL){
		return true;      // this return applies if a radio event has not to be inserted
	}
	if (epgId < 0){
		ERROR("The method splitEventDescription got a wrong epg ID");
		return false;
	}
	int eventId = 0;
	int duration = 0;
	int tableId = 0;
	char* version = NULL;
	long startT = 0;
	bool splitSuccess = splitEventDescription(splitter, eventDescr, &eventId, &startT, &duration, &tableId, &version);
	if (!splitSuccess){
		ERROR("The method splitEventDescription failed");
		return false;
	}

	cUPnPObjectID ID = epgId;
	cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
//	   MESSAGE(VERBOSE_OBJECTS, "Create the broadcast event '%s', the EPG channel container has the ID %s", (eventTitle) ? eventTitle : "NO TITLE", *channelContainer->getID());
	cUPnPClassEpgItem* bcObject = (cUPnPClassEpgItem*) factory->getObject(ID);
	if (bcObject){
		bcObject->setDuration(duration);
		bcObject->setEventId (eventId);
		bcObject->setTableId (tableId);
		bcObject->setVersion(version);
		char buffer [30];
		sprintf(buffer, "%ld", startT);
		bcObject->setStartTime(buffer);
		bcObject->setSynopsis(eventSynopsis);
		bcObject->setShortTitle(eventShortTitle);
		bcObject->setTitle(eventTitle);
		factory->saveObject(bcObject);
	}
	else {
		ERROR("Got no epg item object with ID %i", epgId);
		return false;
	}
	return true;
}

bool cVdrEpgInfo::splitEventDescription(cSplit* splitter, char* eventDescr, int* eventId, long* startT, int* duration, int* tableId, char** version){
	int ctr = 0;
	std::vector<std::string>* tokens = splitter->split(eventDescr, " ");
	while (ctr < (int) tokens->size() && ctr < 10){
		if (ctr == 0){
			*eventId = atoi(tokens->at(ctr).c_str());  
		}
		else if (ctr == 1){
			*startT = atol(tokens->at(ctr).c_str());
		}
		else if (ctr == 2){
			*duration = atoi(tokens->at(ctr).c_str()); 
		}
		else if (ctr == 3){
			sscanf(tokens->at(ctr).c_str(), "%x", &(*tableId)); 
		}
		else if (ctr == 4){
			*version = strdup(tokens->at(ctr).c_str()); 
		}
		ctr++;
	}
	if (ctr > 3) {
		return true;
	}
	else {
		 MESSAGE(VERBOSE_OBJECTS, "Got %i tokens for the event %s", (int) tokens->size(), eventDescr);
	}
	return false;
}

bool cVdrEpgInfo::deleteEpgEvents(std::vector<int>* objVector){
	bool ret = false;
	if ((int)objVector->size() > 0){
		cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
		cMediatorInterface* mediator = factory->findMediatorByClass(UPNP_CLASS_EPGITEM);
		if (mediator){
			for (int i = 0; i < (int)objVector->size(); i++){
				cUPnPObjectID* objId = new cUPnPObjectID(objVector->at(i));
				cUPnPClassObject* obj = factory->getObject(*objId);
				if (obj){
//					MESSAGE(VERBOSE_OBJECTS, "Delete the epg object with ID %i",objVector->at(i));
					mediator->deleteObject(obj);
				}
				else {
					WARNING("The epg event to be deleted was not found. ID %i", objVector->at(i));
				}
			}
		}
		objVector->clear();
	}
	return !ret;
}

bool cVdrEpgInfo::findRecordTimerResource(cUPnPResource** timerResource, char** resourceName, off_t *resourceSize){
	*timerResource = NULL;
	cString resFile = cString::sprintf("%s/%s/%s", cPluginUpnp::getConfigDirectory(), EPG_RECORD_CONFIRMATION_FOLDER, EPG_RECORD_CONFIRMATION_FILE);
	MESSAGE(VERBOSE_OBJECTS, "Recording confirmation file: %s", *resFile);
	struct stat sts;
	*resourceSize = 0;
	if ((stat (*resFile, &sts)) == -1 && errno == ENOENT){
		MESSAGE(VERBOSE_RECORDS,"No EPG recording confirmation file available");
	}
	else {
		cString resFolder = cString::sprintf("%s/%s", cPluginUpnp::getConfigDirectory(), EPG_RECORD_CONFIRMATION_FOLDER);
		*resourceName = (char*) strdup(resFolder);
//		MESSAGE(VERBOSE_OBJECTS, "Recording confirmation file name: %s Size: %ld", *resourceName, (long)sts.st_size);
		*resourceSize = sts.st_size;
	}
	if (*resourceSize > 0){
		return true;
	}
	
	bool success = false;
	cUPnPClassObject* recordContainer = (cUPnPClassObject*)(this->mMetaData->getObjectByID(V_RECORDS_ID));
    if(recordContainer){
	    cList<cUPnPClassObject>* List = recordContainer->getContainer()->getObjectList();
		int ctr = 0;
        for(cUPnPClassObject* Child = (cUPnPClassObject*)List->First(); (Child) && (ctr++ < 50); Child = (cUPnPClassObject*)List->Next(Child)){
			if (Child){
			   MESSAGE(VERBOSE_OBJECTS, "findRecordTimerResource: Got a movie record with ID %s", *Child->getID());
			   cList <cUPnPResource>* resList = Child->getResources();
			   if(resList){
				   MESSAGE(VERBOSE_OBJECTS, "findRecordTimerResource: The UPnP record has resoure(s)");
				   int ctr2 = 0;
				   for(cUPnPResource* res = (cUPnPResource*)resList->First(); (res) && (ctr2++ < 50); res = (cUPnPResource*)resList->Next(res)){
					   MESSAGE(VERBOSE_OBJECTS, "findRecordTimerResource: Got the resource with ID %d, record Timer %d", res->getID(), res->getRecordTimer());
					   *timerResource = res;
						success = true;
						if (res->getResolution() && strcmp(res->getResolution(), "720x576") == 0){
							MESSAGE(VERBOSE_OBJECTS, "Used a standard resolution resource");
							return true;
						}
				   }
			   }
			   else {
				   MESSAGE(VERBOSE_OBJECTS, "findRecordTimerResource: The UPnP record has NO resoure(s)");
			   }
			}
        }
	   return success;
	}
	return success;
}

bool cVdrEpgInfo::checkChannel(char* channelId){
	cUPnPConfig* config = cUPnPConfig::get();
	int amountChannels = config->mFirstChannelsAmount;
	bool withoutCA = config->mWithoutCA;
	amountChannels = (amountChannels <= 0) ? MAX_NUMBER_EPG_CHANNELS : amountChannels;
    cChannel* Channel = NULL;
	int ctr = 0;
    for (int Index = 0; (Channel = Channels.Get(Index)) && ctr < amountChannels; Index = Channels.GetNextNormal(Index)){ // Iterating the channels
		if (!Channel->GroupSep() && Channel->Number() > 0 && (!withoutCA || (withoutCA && Channel->Ca() == 0))){        
			char* chID = strdup0((const char*) Channel->GetChannelID().ToString());
			if (chID && startswith (channelId, chID)){
				return true;
			}
			if (config->mRadioShow){
				ctr++;		
			}
			else if (!ISRADIO(Channel)){
				ctr++;
			}
		}
    }
    return false;
}