/*
 * File:   metadata.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 28. Mai 2009, 16:50
 * Last modification: April 9, 2013
 */

#include <upnp/ixml.h>
#include <time.h>
#include <vdr/tools.h>
#include "object.h"
#include "resources.h"
#include "metadata.h"
#include "../common.h"
#include "../upnp.h"
#include "search.h"
#include <vdr/channels.h>
#include <vdr/epg.h>
#include <vdr/videodir.h>
#include <upnp/upnp.h>
#include <vdr/device.h>
#include <sys/stat.h>
#include "vdrepg.h"
#include "config.h"

 /**********************************************\
 *                                              *
 *  Media database                              *
 *                                              *
 \**********************************************/

cMediaDatabase::cMediaDatabase(){
    this->mSystemUpdateID = 0;
    this->mLastInsertObjectID = 0;
	this->mIsInitialised = false;
    this->mDatabase = cSQLiteDatabase::getInstance();
    this->mObjects = new cHash<cUPnPClassObject>;
    this->mFactory = cUPnPObjectFactory::getInstance();
    this->mFactory->registerMediator(UPNP_CLASS_ITEM, new cUPnPItemMediator(this));
    this->mFactory->registerMediator(UPNP_CLASS_EPGITEM, new cUPnPEpgItemMediator(this));
    this->mFactory->registerMediator(UPNP_CLASS_CONTAINER, new cUPnPContainerMediator(this));
    this->mFactory->registerMediator(UPNP_CLASS_EPGCONTAINER, new cUPnPEpgContainerMediator(this));
    this->mFactory->registerMediator(UPNP_CLASS_VIDEO, new cUPnPVideoItemMediator(this));
    this->mFactory->registerMediator(UPNP_CLASS_VIDEOBC, new cUPnPVideoBroadcastMediator(this));
    this->mFactory->registerMediator(UPNP_CLASS_MOVIE, new cUPnPMovieMediator(this));
    this->mFactory->registerMediator(UPNP_CLASS_AUDIO, new cUPnPAudioItemMediator(this));
    this->mFactory->registerMediator(UPNP_CLASS_AUDIOBC, new cUPnPAudioBroadcastMediator(this));
	this->mFactory->registerMediator(UPNP_CLASS_MUSICTRACK, new cUPnPAudioRecordMediator(this));
	this->mFactory->registerMediator(UPNP_CLASS_BOOKMARKITEM, new cUPnPRecordTimerItemMediator(this));
	if (pthread_mutex_init(&mutex_system, NULL) != 0){
		ERROR("pthread_mutex_init failed with system mutex");
	}
	if (pthread_mutex_init(&mutex_fastFind, NULL) != 0){
		ERROR("pthread_mutex_init failed with fast find mutex");
	}
}

cMediaDatabase::~cMediaDatabase(){
    delete this->mDatabase;
}

bool cMediaDatabase::init(){
    MESSAGE(VERBOSE_SDK,"Initializing...");
    if(this->prepareDatabase()){
        ERROR("Initializing of database failed.");
        return false;
    }
#ifndef WITHOUT_TV
    if(this->loadChannels()){
        ERROR("Loading channels failed");
        return false;
    }
#endif
	this->mIsInitialised = true;
#ifndef WITHOUT_RECORDS
    if(this->loadRecordings()){
        ERROR("Loading records failed");
        return false;
    }
#endif	
    return true;
}

void cMediaDatabase::updateSystemID(){
	pthread_mutex_lock(&mutex_system);
	int stepRes = sqlite3_step(this->mDatabase->getSystemUpdateStatement());
	pthread_mutex_unlock(&mutex_system);
	if (stepRes == SQLITE_ERROR){
		ERROR("ERROR while updateSystemID");
	}
}

const char* cMediaDatabase::getContainerUpdateIDs(){
    return "";
}

unsigned int cMediaDatabase::getSystemUpdateID(){
//	MESSAGE(VERBOSE_METADATA, "Get system update ID");
	pthread_mutex_lock(&mutex_system);
	int ctr = 0;
	while (ctr++ < 5){
		int stepRes = sqlite3_step(this->mDatabase->getSystemSelectStatement());
		switch (stepRes){
			case SQLITE_ROW:
				this->mSystemUpdateID = sqlite3_column_int(this->mDatabase->getSystemSelectStatement(), 0);
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				MESSAGE(VERBOSE_METADATA, "Got the system step value %d", stepRes);
				break;
		}
	}
	pthread_mutex_unlock(&mutex_system);
    return this->mSystemUpdateID;
}

int cMediaDatabase::getNextObjectID(){
	int ret = 0;
	pthread_mutex_lock(&(this->mDatabase->mutex_primKey));
	sqlite3_stmt* pkSelStmt = this->mDatabase->getPKSelectStatement();
	bool actionSuccess = sqlite3_bind_int (pkSelStmt, 1, PK_OBJECTS_NR) == SQLITE_OK;
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(pkSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				ret = sqlite3_column_int(pkSelStmt, 0);				
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
		}
	}
	sqlite3_clear_bindings(pkSelStmt);
	sqlite3_reset(pkSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_primKey));
	this->mLastInsertObjectID = ret;
    return ret;
}

int cMediaDatabase::addFastFind(cUPnPClassObject* Object, const char* FastFind){
    if(!Object || !FastFind){
        MESSAGE(VERBOSE_OBJECTS,"Invalid fast find parameters");
        return -1;
    }
//	MESSAGE(VERBOSE_OBJECTS,"addFastFind: Insert item fast ID: %s", FastFind);
	pthread_mutex_lock(&mutex_fastFind);
	sqlite3_stmt* ifdInsStmt = this->mDatabase->getItemFinderInsertStatement();
	bool actionSuccess = sqlite3_bind_int (ifdInsStmt, 1, (unsigned int) Object->getID()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(ifdInsStmt, 2, FastFind, -1, SQLITE_TRANSIENT) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("addFastFind: Error with sqlite3_bind for item fast ID insertion; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(ifdInsStmt) != SQLITE_DONE){
		ERROR("addFastFind: sqlite3_step failed with insertion of data; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(ifdInsStmt);
	sqlite3_reset(ifdInsStmt);
	pthread_mutex_unlock(&mutex_fastFind);
	return (actionSuccess) ? 0 : -1;
}

cUPnPClassObject* cMediaDatabase::getObjectByFastFind(const char* FastFind){
    if (!FastFind) {
		return NULL;
	}

	pthread_mutex_lock(&mutex_fastFind);
	bool actionSuccess = sqlite3_bind_text (this->mDatabase->getItemFinderSelectStatement(), 1, FastFind, -1, SQLITE_TRANSIENT) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("getObjectByFastFind: Error with sqlite3_bind for object ID selection; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	
	int ctr = 0;
	int objectId = -1;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(this->mDatabase->getItemFinderSelectStatement());
		switch (stepRes){
			case SQLITE_ROW:
				objectId = sqlite3_column_int(this->mDatabase->getItemFinderSelectStatement(), 0);
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
		}
	}
	sqlite3_clear_bindings(this->mDatabase->getItemFinderSelectStatement());
	sqlite3_reset(this->mDatabase->getItemFinderSelectStatement());
	pthread_mutex_unlock(&mutex_fastFind);
	if (objectId >= 0){
		return this->getObjectByID(objectId);
	}
    return NULL;
}

cUPnPClassObject* cMediaDatabase::getObjectByID(cUPnPObjectID ID){
    cUPnPClassObject* Object;
    if ((Object = this->mObjects->Get((unsigned int)ID))){
//        MESSAGE(VERBOSE_OBJECTS, "Found cached object with ID '%s'", *ID);
    }
    else if ((Object = this->mFactory->getObject(ID))){
//        MESSAGE(VERBOSE_OBJECTS, "Found object with ID '%s' in database", *ID);
    }
    else {
        WARNING("No object with such ID '%s'", *ID);
        return NULL;
    }
    return Object;
}

void cMediaDatabase::cacheObject(cUPnPClassObject* Object){
    if (this->mObjects->Get((unsigned int)Object->getID()) == NULL){
        this->mObjects->Add(Object, (unsigned int)Object->getID());
    }
}

/**
 *
 */
int cMediaDatabase::prepareDatabase(){
	cUPnPConfig* config = cUPnPConfig::get();
	bool isAudio = false;
#ifndef WITHOUT_RECORDS
	if(this->mDatabase->execStatement(SQLITE_CREATE_TABLE_RECORDTIMERS)){
		ERROR("Error when creating the RecordTimers table");
	} 
#endif
#ifndef WITHOUT_EPG
	if (this->mDatabase->execStatement(SQLITE_DROP_TABLE_BCEVENTS)){
		ERROR("Error when dropping the BCEvents table");
	}

	selectAndDeleteItems();
	deleteObjectsResourcesSearchClass();

	if (this->mDatabase->execStatement(SQLITE_CREATE_TABLE_BCEVENTS)){
		ERROR("Error when creating the BCEvents table");
	} 

	MESSAGE(VERBOSE_OBJECTS, "Create EPG tables if they are not existing");
	if (this->mDatabase->execStatement(SQLITE_CREATE_TABLE_EPGCHANNELS)){
		ERROR("Error when creating the EPGChannels table");
		return -1;
	}

	if (this->mDatabase->execStatement("UPDATE %s SET %s=%s WHERE %s=%s", SQLITE_TABLE_PRIMARY_KEYS, SQLITE_COL_KEY, SQLITE_FIRST_EPG_RESOURCE_ID,
										SQLITE_COL_KEYID, PK_RESOURCES_EPG)){
		ERROR("Error when updating the start epg resource ID in the primary key table");
	}
#endif
	activateDeleteTriggers();
	cUPnPClassContainer* Root = (cUPnPClassContainer*) this->getObjectByID(ROOT_ID);
    if (Root == NULL){
        MESSAGE(VERBOSE_SDK, "Creating database structure");
        Root = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _(PLUGIN_SHORT_NAME));
		if (!Root){
			ERROR("Could not create the UPnP root object");
			return -1;
		}
        Root->setID(ROOT_ID);
        if (this->mFactory->saveObject(Root)) {
			ERROR("Could not save the object root");
			return -1;
		}
	}

#ifndef WITHOUT_VIDEO
	if (this->getObjectByID(VIDEO_ID) == NULL){
		cUPnPClassContainer* Video = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("Video"));
		if (!Video){
			ERROR("Could not create the UPnP video object");
			return -1;
		}
		Video->setID(VIDEO_ID);
		Root->addObject(Video);
		cClass VideoClass = { UPNP_CLASS_VIDEO, true };
		Video->addSearchClass(VideoClass);
		Video->setSearchable(true);
		if (this->mFactory->saveObject(Video)){
			ERROR("Could not save the object video");
			return -1;
		}
#ifndef WITHOUT_TV
		if (this->getObjectByID(TV_ID) == NULL){
			cUPnPClassContainer* TV = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("TV"));
			if (!TV){
				ERROR("Could not create the UPnP TV object");
				return -1;
			}
			TV->setID(TV_ID);
			TV->setContainerType(DLNA_CONTAINER_TUNER);
			TV->setSearchable(true);
			cClass VideoBCClass = { UPNP_CLASS_VIDEOBC, true };
			TV->addSearchClass(VideoBCClass);
			Video->addObject(TV);
			if (this->mFactory->saveObject(TV)){
				ERROR("Could not save the object TV");
				return -1;
			}
		}
#endif
#ifndef WITHOUT_RECORDS
		if (this->getObjectByID(V_RECORDS_ID) == NULL){
			MESSAGE(VERBOSE_SDK, "Creating video records container");
			cUPnPClassContainer* Records = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("Records"));
			if (!Records){
				ERROR("Could not create the UPnP video records object");
				return -1;
			}
			Records->setID(V_RECORDS_ID);
			Video->addObject(Records);
			Records->addSearchClass(VideoClass);
			Records->setSearchable(true);
			if (this->mFactory->saveObject(Records)){
				ERROR("Could not save object video records");
				return -1;
			}
		}
#endif
	}
#endif
#ifndef WITHOUT_AUDIO
	isAudio = true;
	cUPnPClassContainer* Audio = (cUPnPClassContainer*) this->getObjectByID(AUDIO_ID);
	if (Audio == NULL){
		MESSAGE(VERBOSE_SDK, "Creating an audio container");
		Audio = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("Audio"));
		if (!Audio){
			ERROR("Could not create the UPnP audio object");
			return -1;
		}
		Audio->setID(AUDIO_ID);
		Root->addObject(Audio);
		cClass AudioClass = { UPNP_CLASS_AUDIO, true };
		Audio->addSearchClass(AudioClass);
		Audio->setSearchable(true);
		if (this->mFactory->saveObject(Audio)){
			ERROR("Could not save object Audio");
			return -1;
		}
	}
#ifndef WITHOUT_RECORDS
	if (isAudio && config->mRadioShow && this->getObjectByID(A_RECORDS_ID) == NULL){
		MESSAGE(VERBOSE_SDK, "Creating an audio records container");
		cUPnPClassContainer* Records = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("Records"));
		if (!Records){
			ERROR("Could not create the UPnP audio records object");
			return -1;
		}
		Records->setID(A_RECORDS_ID);
		if (!Audio){
			ERROR("The UPnP audio object is NULL");
			return -1;
		}
		Audio->addObject(Records);
		cClass mtClass = { UPNP_CLASS_MUSICTRACK, true };
		Records->addSearchClass(mtClass);
		Records->setSearchable(true);
		if (this->mFactory->saveObject(Records)) {
			ERROR("Could not save object audio Records");
			return -1;
		}
	}
#endif
#ifndef WITHOUT_RADIO	
	if (config->mRadioShow && this->getObjectByID(RADIO_ID) == NULL){
		MESSAGE(VERBOSE_SDK, "Creating the radio container");
		cUPnPClassContainer* Radio = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("Radio"));
		if (!Radio){
			ERROR("Could not create the UPnP radio object");
			return -1;
		}
		Radio->setID(RADIO_ID);
		cClass AudioBCClass = { UPNP_CLASS_AUDIOBC, true };
		Radio->addSearchClass(AudioBCClass);
		Radio->setSearchable(true);
		Audio->addObject(Radio);
		if (this->mFactory->saveObject(Radio)){
			ERROR("Could not save object Radio");
			return -1;
		}
	}
#endif	
#endif
#ifndef WITHOUT_CUSTOM_VIDEOS
	if (this->getObjectByID(CUSTOMVIDEOS_ID) == NULL){
		MESSAGE(VERBOSE_SDK, "Creating the user videos container");
		cUPnPClassContainer* CustomVideos = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("User videos"));
		if (!CustomVideos){
			ERROR("Could not create the UPnP CustomVideos object");
			return -1;
		}
		CustomVideos->setID(CUSTOMVIDEOS_ID);
		Video->addObject(CustomVideos);
		CustomVideos->addSearchClass(VideoClass);
		CustomVideos->setSearchable(true);
		if (this->mFactory->saveObject(CustomVideos)){
			ERROR("Could not save custom videos");
			return -1;
		}
	}
#endif
#ifndef WITHOUT_EPG
	cUPnPClassContainer* Epg = (cUPnPClassContainer*) this->getObjectByID(EPG_ID);
	cClass EpgClass = { UPNP_CLASS_EPGITEM, true };
	if (Epg == NULL){
		MESSAGE(VERBOSE_SDK, "Creating the basic EPG container");
		Epg = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("EPG"));
		if (!Epg){
			ERROR("Could not create the UPnP epg object");
			return -1;
		}
		Epg->setID(EPG_ID);		
		Epg->addSearchClass(EpgClass);
		Epg->setSearchable(true);
		Root->addObject(Epg);
		if (this->mFactory->saveObject(Epg)){
			ERROR("The EPG container could not be saved in the database.");
			return -1;
		}
	}
#ifndef WITHOUT_TV
	if (this->getObjectByID(EPG_TV_ID) == NULL){
		cUPnPClassContainer* EpgTv = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("TV"));
		if (!EpgTv){
			ERROR("Could not create the UPnP epg tv object");
			return -1;
		}
		EpgTv->setID(EPG_TV_ID);
		Epg->addObject(EpgTv);
		EpgTv->addSearchClass(EpgClass);
		EpgTv->setSearchable(true);
		if (this->mFactory->saveObject(EpgTv)){
			ERROR("The EPG TV container could not be saved in the database.");
			return -1;
		}
	}
#endif
#ifndef WITHOUT_RADIO
	cUPnPClassContainer* EpgRadio = (cUPnPClassContainer*) this->getObjectByID(EPG_RADIO_ID);
	if (config->mRadioShow && EpgRadio == NULL){
		EpgRadio = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("Radio"));
		if (!EpgRadio){
			ERROR("Could not create the UPnP epg radio object");
			return -1;
		}
		EpgRadio->setID(EPG_RADIO_ID);
		Epg->addObject(EpgRadio);
		EpgRadio->addSearchClass(EpgClass);
		EpgRadio->setSearchable(true);
		if (this->mFactory->saveObject(EpgRadio)){
			ERROR("The EPG RADIO container could not be saved in the database.");
			return -1;
		}
	}
#endif	
#endif
purgeAllEpgChannels();
#ifndef WITHOUT_RECORDS
	if (this->getObjectByID(REC_TIMER_ID) == NULL){
		MESSAGE(VERBOSE_SDK, "Creating the basic record timer container");
		cUPnPClassContainer* recTimer = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("Record Timer"));
		if (!recTimer){
			ERROR("Could not create the UPnP record timer object");
			return -1;
		}
		recTimer->setID(REC_TIMER_ID);
		//cClass EpgClass = { UPNP_CLASS_EPGITEM, true };
		//recTimer->addSearchClass(EpgClass);
		recTimer->setSearchable(true);
		Root->addObject(recTimer);
		if (this->mFactory->saveObject(recTimer)){
			ERROR("The record timer container could not be saved in the database.");
			return -1;
		}
#ifndef WITHOUT_TV
		if (this->getObjectByID(REC_TIMER_TV_ID) == NULL){
			MESSAGE(VERBOSE_SDK, "Creating the record timer TV container");
			cUPnPClassContainer* recTimerTv = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("Timer TV"));
			if (!recTimerTv){
				ERROR("Could not create the UPnP tv record timer object");
				return -1;
			}
			recTimerTv->setID(REC_TIMER_TV_ID);
			recTimerTv->setSearchable(true);
			recTimer->addObject(recTimerTv);
			if (this->mFactory->saveObject(recTimerTv)){
				ERROR("The record timer TV container could not be saved in the database.");
				return -1;
			}
		}
#endif
#ifndef WITHOUT_RADIO
		if (this->getObjectByID(REC_TIMER_RADIO_ID) == NULL){
			MESSAGE(VERBOSE_SDK, "Creating the record timer Radio container");
			cUPnPClassContainer* recTimerRadio = (cUPnPClassContainer*)this->mFactory->createObject(UPNP_CLASS_CONTAINER, _("Timer Radio"));
			if (!recTimerRadio){
				ERROR("Could not create the UPnP radio record timer object");
				return -1;
			}
			recTimerRadio->setID(REC_TIMER_RADIO_ID);
			recTimerRadio->setSearchable(true);
			recTimer->addObject(recTimerRadio);
			if (this->mFactory->saveObject(recTimerRadio)){
				ERROR("The record timer Radio container could not be saved in the database.");
				return -1;
			}
		}
#endif
	}
#endif
    return 0;
}

bool cMediaDatabase::activateDeleteTriggers(){
	bool tError = false;
	if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_CONTAINERS_SEARCHCLASSES)==-1) {  
		ERROR("Error when initializing the container_searchclasses control trigger on delete");
		tError = true;
	}
	if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_ITEMS_AUDIOITEMS)==-1) { 
		ERROR("Error when initializing the audio_items control trigger on delete");
		tError = true;
	}
    if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_ITEMS_IMAGEITEMS)==-1) { 
		ERROR("Error when initializing the image_items control trigger on delete");
		tError = true;
	}
	if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_ITEMS_ITEMS)==-1){
		 ERROR("Error when initializing the item reference ID control trigger on delete");
		 tError = true;
	}
	if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_ITEMS_VIDEOITEMS)==-1){  
		 ERROR("Error when initializing the item video_item control trigger on delete");
		 tError = true;
	 }
	if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_OBJECTS_ITEMFINDER)==-1){  
		ERROR("Error when initializing the object itemfinder control trigger on delete");
		tError = true;
	}
	if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_OBJECT_ITEMS)==-1){      
		ERROR("Error when initializing the object item control trigger on delete");
		tError = true;
	}
	if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_OBJECT_RESOURCES)==-1){  
		ERROR("Error when initializing the object resource control trigger on delete");
		tError = true;
	}
    if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_OBJECTS_OBJECTS)==-1){ 
		ERROR("Error when initializing the object parent control trigger on delete");
		tError = true;
	}
	if(this->mDatabase->execStatement(SQLITE_TRIGGER_D_OBJECT_CONTAINERS)==-1){  
		ERROR("Error when initializing the object container control trigger on delete");
		tError = true;
	}
	return tError;
}

#ifndef WITHOUT_EPG
bool cMediaDatabase::selectAndDeleteItems(){
	bool statementError = false;
	sqlite3_stmt *resSelStmt;	
	cString resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s>=%s", SQLITE_COL_OBJECTID, SQLITE_TABLE_RESOURCES, SQLITE_COL_RESOURCEID,
		                               SQLITE_FIRST_EPG_RESOURCE_ID);
	const char *zSql = *resStatement;
	const char **pzTail = NULL;
	if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(resSelStmt), pzTail) != SQLITE_OK ){
		ERROR("Error when compiling the SQL statement for a resource selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		statementError = true;
	}
	std::vector<int> objVector;
	int ctr = 0;
	while (ctr++ < MAX_ROW_COUNT){
		int objId = -1;
		int stepRes = sqlite3_step(resSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				objId = sqlite3_column_int(resSelStmt, 0);
				objVector.push_back(objId);
				break;
			case SQLITE_DONE:
				ctr = MAX_ROW_COUNT; // stop loop
				break;
		}
	}
	MESSAGE(VERBOSE_OBJECTS, "number of epg object items: %d", (int)objVector.size());

	if ((int)objVector.size() > 0){                                                              //   Delete EPG objects from table Items
		resStatement = cString::sprintf("DELETE FROM %s WHERE %s=:ID", SQLITE_TABLE_ITEMS, SQLITE_COL_OBJECTID);
		zSql = *resStatement;
		pzTail = NULL;
		sqlite3_stmt *itmDelStmt;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &itmDelStmt, pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for item deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
			statementError = true;
		}
		for (int i = 0; i < (int)objVector.size(); i++){
			bool actionSuccess = sqlite3_bind_int (itmDelStmt, 1, (int) objVector.at(i)) == SQLITE_OK;
			if (!actionSuccess){
				ERROR("Error when binding an object for item deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
				statementError = true;
			}
			if (actionSuccess && sqlite3_step(itmDelStmt) != SQLITE_DONE){
				ERROR("Error when deleting the item with ID %d", (int) objVector.at(i));
				statementError = true;
			}
			sqlite3_clear_bindings(itmDelStmt);
			sqlite3_reset(itmDelStmt);
			if (((i+1) % 5000) == 0){
				MESSAGE(VERBOSE_OBJECTS, "%d rows from table Items have been deleted", (i+1));
			}
		}
		sqlite3_finalize(itmDelStmt);
		MESSAGE(VERBOSE_OBJECTS, "%d rows deleted from the table Items", (int)objVector.size());
	}

	cString sqlObjects = cString::sprintf("DELETE FROM %s WHERE %s>%s", SQLITE_TABLE_RESOURCES, SQLITE_COL_RESOURCEID, SQLITE_FIRST_EPG_RESOURCE_ID);
	zSql = *sqlObjects;
	pzTail = NULL;
	sqlite3_stmt *ppStmt;
	if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &ppStmt, pzTail) != SQLITE_OK ){
		ERROR("Error when compiling the SQL statement for resources deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		statementError = true;
	}
	else if (ppStmt){
		 if (sqlite3_step(ppStmt)  != SQLITE_DONE){                                             //   Delete EPG objects from table Resources
			 ERROR("Error when deleting resources: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
			 statementError =true;
		 }
		 sqlite3_finalize(ppStmt);
		 MESSAGE(VERBOSE_OBJECTS, "The compiled deletion statement for epg resources in the table Resources was executed");
	}

	if ((int)objVector.size() > 0){                                                             //   Delete EPG objects from table Objects
		resStatement = cString::sprintf("DELETE FROM %s WHERE %s=:ID", SQLITE_TABLE_OBJECTS, SQLITE_COL_OBJECTID);
		zSql = *resStatement;
		pzTail = NULL;
		sqlite3_stmt *itmDelStmt;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &itmDelStmt, pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for object deletion in table Objects: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
			statementError = true;
		}
		for (int i = 0; i < (int)objVector.size(); i++){
			bool actionSuccess = sqlite3_bind_int (itmDelStmt, 1, (int) objVector.at(i)) == SQLITE_OK;
			if (!actionSuccess){
				ERROR("Error when binding an object for deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
				statementError = true;
			}
			if (actionSuccess && sqlite3_step(itmDelStmt) != SQLITE_DONE){
				ERROR("Error when deleting the object with ID %d", (int) objVector.at(i));
				statementError = true;
			}
			sqlite3_clear_bindings(itmDelStmt);
			sqlite3_reset(itmDelStmt);
			if (((i+1) % 5000) == 0){
				MESSAGE(VERBOSE_OBJECTS, "%d rows from table Objects have been deleted", (i+1));
			}
		}
		sqlite3_finalize(itmDelStmt);
		MESSAGE(VERBOSE_OBJECTS, "%d rows deleted from the table Objects", (int)objVector.size());
	}
	return statementError;
}

bool cMediaDatabase::deleteObjectsResourcesSearchClass(){
	bool statementError = false;
	sqlite3_stmt *ppStmt;
	cString sqlObjects = cString::sprintf("DELETE FROM %s WHERE %s='%s'", SQLITE_TABLE_SEARCHCLASS, SQLITE_COL_CLASS, UPNP_CLASS_EPGITEM);
	const char *zSql = *sqlObjects;
	const char **pzTail = NULL;
	if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &ppStmt, pzTail) != SQLITE_OK ){
		ERROR("Error when compiling the SQL statement for search class deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		statementError = true;
	}
	else if (ppStmt){
		 if (sqlite3_step(ppStmt) != SQLITE_DONE){
			 ERROR("Error when deleting search class entries of type epg-items: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
			 statementError =true;
		 }
		 sqlite3_finalize(ppStmt);
		 MESSAGE(VERBOSE_OBJECTS, "The compiled deletion statement for search classes was executed");
	}
	return statementError;
}
#endif

#ifndef WITHOUT_TV
int cMediaDatabase::loadChannels(){
    MESSAGE(VERBOSE_LIVE_TV ,"Loading channels");
	cUPnPConfig* config = cUPnPConfig::get();
	int amountChannels = config->mFirstChannelsAmount;
	bool withoutCA = config->mWithoutCA;
	int radioCount = 0;
    cUPnPClassContainer* tvContainer = (cUPnPClassContainer*)this->getObjectByID(TV_ID);
	cUPnPClassContainer* radioContainer = NULL;
	if (config->mRadioShow){
	   radioContainer = (cUPnPClassContainer*)this->getObjectByID(RADIO_ID);
	}
	cUPnPObjects* radioObjects = NULL;
	if (radioContainer){
		radioObjects = radioContainer->getObjectList();
		if (radioObjects){
			radioCount = radioObjects->Count();
		}
	}
    if (tvContainer){
		int tvCount = 0;
		cUPnPObjects* tvObjects = tvContainer->getObjectList();
		if (tvObjects){
			tvCount = tvObjects->Count();
		}
		MESSAGE(VERBOSE_LIVE_TV, "Number of already existing tv channels: %i, of radio channels: %i", tvCount, radioCount);
		if (amountChannels > 0 || withoutCA){ //check whether UPnP broadcast channels have to be deleted and delete spare items
			std::vector<int> bcVector;
			if (radioCount > 0){
				MESSAGE(VERBOSE_LIVE_TV, "Delete %i radio channels", radioCount);
				fetchBroadcastItems(&bcVector, radioObjects, false);
				deleteBroadcastItems(&bcVector, UPNP_CLASS_AUDIOBC);
			}
			if (tvCount > 0){
				MESSAGE(VERBOSE_LIVE_TV, "Delete TV channels");
				fetchBroadcastItems(&bcVector, tvObjects, true);
				deleteBroadcastItems(&bcVector, UPNP_CLASS_VIDEOBC);
			}
		}
        bool noResource = false;
        // TODO: Add to setup
        // if an error occured while loading resources, add the channel anyway
        bool addWithoutResources = false;
        cChannel* Channel = NULL;
		int ctr = 0;
		int vidCtr = 0;
		amountChannels = (amountChannels <= 0) ? MAX_NUMBER_EPG_CHANNELS : amountChannels;
        for (int Index = 0; (Channel = Channels.Get(Index)) && ctr < amountChannels; Index = Channels.GetNextNormal(Index)){
            // Iterating the channels
			if (!Channel->GroupSep() && (!withoutCA || (withoutCA && Channel->Ca() == 0))){  
				tChannelID ChannelID = Channel->GetChannelID();
				bool isRadio = ISRADIO(Channel);
				cUPnPClassObject* ChannelItem = this->getObjectByFastFind(ChannelID.ToString());
				bool inList = (ChannelItem &&
							((!isRadio && tvContainer->getObject(ChannelItem->getID())) ||
							(radioContainer && isRadio && radioContainer->getObject(ChannelItem->getID()))));		
				if (!inList){
					if (config->mRadioShow && Channel->Number() > 0 && isRadio){
						ctr++;
						if (radioContainer){
							MESSAGE(VERBOSE_LIVE_TV, "Adding radio channel '%s'; ID: %s; Conditional access: %i", Channel->Name(), *ChannelID.ToString(), Channel->Ca());
							cUPnPClassAudioBroadcast* radioChannelItem = (cUPnPClassAudioBroadcast*)this->mFactory->createObject(UPNP_CLASS_AUDIOBC, Channel->Name());
							radioChannelItem->setChannelName(Channel->Name());
							radioChannelItem->setChannelNr(Channel->Number());
							if (cUPnPResources::getInstance()->createFromChannel(radioChannelItem, Channel)){
								ERROR("Unable to get resources for the radio channel");
								noResource = true;
							}
						
							radioContainer->addObject(radioChannelItem);
							if (this->mFactory->saveObject(radioChannelItem) || this->addFastFind(radioChannelItem, ChannelID.ToString())){
								ERROR("Failure with radio item saveObject or addFastFind; delete the radio item again.");
								this->mFactory->deleteObject(radioChannelItem);
								return -1;
							}
						}
						else {
							MESSAGE(VERBOSE_LIVE_TV, "Skipping radio '%s'", Channel->Name());
						}
					}
					else if (Channel->Number() > 0 && !isRadio) {
						ctr++;
						vidCtr++;
						noResource = false;
						MESSAGE(VERBOSE_LIVE_TV, "Adding video channel '%s' ID:%s; CA: %i; Channel #: %i", Channel->Name(), *ChannelID.ToString(), Channel->Ca(), Channel->Number());
						cUPnPClassVideoBroadcast* tvChannelItem = (cUPnPClassVideoBroadcast*)this->mFactory->createObject(UPNP_CLASS_VIDEOBC, Channel->Name());
						tvChannelItem->setChannelName(Channel->Name());
						tvChannelItem->setChannelNr(Channel->Number());
						// Set primary language of the stream
						if (Channel->Alang(0)){
							tvChannelItem->setLanguage(Channel->Alang(0));
						}
						if (cUPnPResources::getInstance()->createFromChannel(tvChannelItem, Channel)){
							ERROR("Unable to get resources for this channel");
							noResource = true;
						}
						if (!noResource || addWithoutResources) {
							tvContainer->addObject(tvChannelItem);
							if (this->mFactory->saveObject(tvChannelItem) ||
								this->addFastFind(tvChannelItem, ChannelID.ToString())){
								ERROR("Failure with video item saveObject or addFastFind; delete the video item again.");
								this->mFactory->deleteObject(tvChannelItem);
								return -1;
							}
						}
						else {
							// Delete temporarily created object with no resource
							this->mFactory->deleteObject(tvChannelItem);
							MESSAGE(VERBOSE_LIVE_TV, "The channel '%s' was not added to the TV folder", tvChannelItem->getChannelName());
						}
					}
					else {
						MESSAGE(VERBOSE_LIVE_TV, "The channel '%s' with channel number %i has been skipped", Channel->Name(), Channel->Number());
					}
				}
				else {
					ctr++;
					MESSAGE(VERBOSE_LIVE_TV, "Skipping '%s', already in database", Channel->Name());
				}
			}
		}
		MESSAGE(VERBOSE_LIVE_TV, "The TV container has now %i items; video Ctr %i", tvContainer->countObjects(), vidCtr);
    }
    return 0;
}

void cMediaDatabase::fetchBroadcastItems(std::vector<int>* bcVector, cUPnPObjects* bcObjects, bool isVideo){
	int amountChannels = cUPnPConfig::get()->mFirstChannelsAmount;
	bool withoutCA = cUPnPConfig::get()->mWithoutCA;
	cUPnPClassObject* radioObj = NULL;
	int ctr = 0;
	int bcCount = bcObjects->Count();
	for (radioObj = bcObjects->First(); radioObj && ctr < bcCount; ctr++){
		int channelNr = 0;
		if (isVideo){
			channelNr = ((cUPnPClassVideoBroadcast*) radioObj)->getChannelNr();
		}
		else {
			channelNr = ((cUPnPClassAudioBroadcast*) radioObj)->getChannelNr();
		}

		if (!isChannelAmongFirst(channelNr, isVideo, amountChannels, withoutCA)){
		   bcVector->push_back((int) radioObj->getID());
		}
		if (ctr < bcCount){						
			radioObj = bcObjects->Next(radioObj);
		}
	}
}

bool cMediaDatabase::isChannelAmongFirst(int channelNr, bool isVideo, int amountChannels, bool withoutCA){   
	if (channelNr <= 0){
		if (isVideo){
		    WARNING("isChannelAmongFirst: Got a wrong video channel number %i", channelNr);
		}
		else {
			WARNING("isChannelAmongFirst: Got a wrong radio channel number %i", channelNr);
		}
		return false;  // delete the channel object with the invalid number
	}
	int ctr = 0;
	cChannel* Channel = NULL;
    for (int Index = 0; (Channel = Channels.Get(Index)) && ctr < amountChannels; Index = Channels.GetNextNormal(Index)){ 
		if (!Channel->GroupSep() && (!withoutCA || (withoutCA && Channel->Ca() == 0))){    
			if (Channel->Number() > 0 && ISRADIO(Channel)){
				ctr++;
				if (!isVideo && channelNr == Channel->Number()){
					return true;
				}
			}
			else if (Channel->Number() > 0 && !ISRADIO(Channel)){
				ctr++;
				if (isVideo && channelNr == Channel->Number()){
					return true;
				}
			}
		}
	}
	return false;
}

void cMediaDatabase::deleteBroadcastItems(std::vector<int>* bcVector, const char* upnpClass){
	cMediatorInterface* mediator = cUPnPObjectFactory::getInstance()->findMediatorByClass(upnpClass);
	if (mediator){
		for (int i = 0; i < (int) bcVector->size(); i++){
			cUPnPObjectID* objId = new cUPnPObjectID(bcVector->at(i));
			cUPnPClassObject* obj = mediator->getObject(*objId);
			if (obj){
				mediator->deleteObject(obj);
			}
			delete objId;
		}
	}
	bcVector->clear();
}

void cMediaDatabase::updateChannelEPG(){
    cUPnPClassContainer* TV = (cUPnPClassContainer*)this->getObjectByID(TV_ID);
	if (!TV){
		 WARNING("There exists no UPnP TV container");
		 return;
	}

    // Iterating channels
    MESSAGE(VERBOSE_EPG_UPDATES, "Getting schedule...");
    cSchedulesLock SchedulesLock;
    const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);

    cList<cUPnPClassObject>* List = TV->getObjectList();
	int tvItems = List->Count();
	if (tvItems <= 0){
		return;
	}
    MESSAGE(VERBOSE_EPG_UPDATES, "TV folder has %d items", tvItems);
	int checkedTVChannels = 0;
	int channelUpdates = 0;
	int channelsNoEpg = 0;
	int ctr = 0;
    for (cUPnPClassVideoBroadcast* ChannelItem = (cUPnPClassVideoBroadcast*) List->First(); ChannelItem && ctr < tvItems; ctr++){
		if (ChannelItem->getChannelNr() <= 0){
			ERROR("Invalid channel number for channel '%s'", (ChannelItem->getChannelName()) ? ChannelItem->getChannelName() : "?");
		}
        cChannel* Channel = Channels.GetByNumber(ChannelItem->getChannelNr());
        if (!Channel || ChannelItem->getChannelNr() <= 0){
			if (!Channel){
			    WARNING("Got no channel from the channel list with channel number %i", ChannelItem->getChannelNr());
			}
			else {
				WARNING("Got an invalid channel with the channel number %i", ChannelItem->getChannelNr());
			}
        }
		else if (ISRADIO(Channel)) {
			WARNING("Got an unexpected radio channel when updating video");
		}
        else {
			bool withEpgData = false;
            const cSchedule* Schedule = Schedules->GetSchedule(Channel);
            const cEvent* Event = Schedule ? Schedule->GetPresentEvent() : NULL;
            if (Event){
				withEpgData = true;
                time_t LastEPGChange = Event->StartTime();
                time_t LastObjectChange = ChannelItem->modified();
                if (LastEPGChange >= LastObjectChange){
                    if (Event){
                        ChannelItem->setTitle(Event->Title() ? Event->Title() : Channel->Name());
                        ChannelItem->setLongDescription(Event->Description());
                        ChannelItem->setDescription(Event->ShortText());
						channelUpdates++;
                    }
                    else {
	                    MESSAGE(VERBOSE_EPG_UPDATES, "Updating a channel item, no broadcast event");
                        ChannelItem->setTitle(Channel->Name());
                        ChannelItem->setLongDescription(NULL);
                        ChannelItem->setDescription(NULL);
                    }
                    this->mFactory->saveObject(ChannelItem);
                }
            }
            else {
                ChannelItem->setTitle(Channel->Name());
                ChannelItem->setLongDescription(NULL);
                ChannelItem->setDescription(NULL);
            }
			if (!withEpgData){
				channelsNoEpg++;
//				MESSAGE(VERBOSE_EPG_UPDATES, "TV channel with ID '%s' has no EPG data.", *Channel->GetChannelID().ToString());
			}
			checkedTVChannels++;
        }
		if (ctr < tvItems){
			ChannelItem = (cUPnPClassVideoBroadcast*)List->Next(ChannelItem);
		}
    }
	MESSAGE(VERBOSE_EPG_UPDATES, "%i TV channels have been checked; %i channels have been updated; %i channels without EPG", 
		checkedTVChannels, channelUpdates, channelsNoEpg);
}
#endif

#ifndef WITHOUT_AUDIO
void cMediaDatabase::updateRadioChannelEPG(){
    cUPnPClassContainer* radio = (cUPnPClassContainer*) this->getObjectByID(RADIO_ID);
	if (!radio){
		WARNING("There exists no UPnP radio container");
		return;
	}

    // Iterating channels
    MESSAGE(VERBOSE_EPG_UPDATES, "Getting schedule for radio ...");
    cSchedulesLock SchedulesLock;
    const cSchedules *Schedules = cSchedules::Schedules(SchedulesLock);

    cList<cUPnPClassObject>* List = radio->getObjectList();
	int radioItems = List->Count();
	if (radioItems <= 0){
		return;
	}

    MESSAGE(VERBOSE_EPG_UPDATES, "radio folder has %d items", radioItems);
	int checkedRadioChannels = 0;
	int withoutEpg = 0;
	int detailsPresentUpdated = 0;
	int ctr = 0;
    for (cUPnPClassAudioBroadcast* ChannelItem = (cUPnPClassAudioBroadcast*) List->First(); ChannelItem && ctr < radioItems; ctr++){
//            MESSAGE(VERBOSE_EPG_UPDATES, "Find radio channel by number %d", ChannelItem->getChannelNr());
        cChannel* Channel = Channels.GetByNumber(ChannelItem->getChannelNr());
        if (!Channel || ChannelItem->getChannelNr() <= 0){ // valid channel numbers are greater 0
			WARNING("Got a not valid channel");
        }
		else if (!ISRADIO(Channel)) {
			WARNING("Got an unexpected video channel when updating radio");
		}
        else {
            const cSchedule* Schedule = Schedules->GetSchedule(Channel);
            const cEvent* Event = Schedule ? Schedule->GetPresentEvent() : NULL;
			bool withEpgData = false;
            if (Event){
				withEpgData = true;
                time_t LastEPGChange = Event->StartTime();
                time_t LastObjectChange = ChannelItem->modified();
                if (LastEPGChange >= LastObjectChange){
                    if (Event){
	                    MESSAGE(VERBOSE_EPG_UPDATES, "Updating radio details using the broadcast present event");
                        ChannelItem->setTitle(Event->Title() ? Event->Title() : Channel->Name());
						detailsPresentUpdated++;
                    }
                    else {
	                    MESSAGE(VERBOSE_EPG_UPDATES, "Updating radio details, no broadcast event");
                        ChannelItem->setTitle(Channel->Name());
                    }
                    this->mFactory->saveObject(ChannelItem);
                }
            }
            else {
                ChannelItem->setTitle(Channel->Name());
            }
			if (!withEpgData){
//				MESSAGE(VERBOSE_EPG_UPDATES, "Radio channel with ID '%s' has no EPG data.", *Channel->GetChannelID().ToString());
				withoutEpg++;
			}
			checkedRadioChannels++;
        }
		if (ctr < radioItems){
			ChannelItem = (cUPnPClassAudioBroadcast*)List->Next(ChannelItem);
		}
    }
	if (checkedRadioChannels > 0){
		MESSAGE(VERBOSE_EPG_UPDATES, "%i radio channels have been checked; %i channels have no EPG data", checkedRadioChannels, withoutEpg);
	}
	if (detailsPresentUpdated > 0){
		MESSAGE(VERBOSE_EPG_UPDATES, "With the broadcast present event %i channel titels were updated", detailsPresentUpdated);
	}
}
#endif
#ifndef WITHOUT_RECORDS
bool cMediaDatabase::loadRecordTimerItems(){
	int timerMax = Timers.Count();
	int ctr = 0;
	if (timerMax > 0){
		off_t resourceSize = 0;
		char* resourceName = NULL;
		for (cTimer* timer = Timers.First(); ctr++ < timerMax && timer; ){
			MESSAGE(VERBOSE_RECORDS, "loadRecordTimerItems: %s; Start %ld", *timer->ToText(), (long)timer->Start());
			createRecordTimerItem(timer, &resourceName, &resourceSize);
			if (ctr < timerMax){
				timer = Timers.Next(timer);
			}
		}
	}
	return true;
}

bool cMediaDatabase::addContainerObjectIds(int containerId, std::vector<int>* idVector){
	int ctr = 0;
	cUPnPObjects* Children = getChildrenList (containerId);
	if (Children && Children->Count() > 0){
		int noChildren = Children->Count();				
		cUPnPClassObject* Child = Children->First();
		for(; ctr++ < noChildren && Child; ){
			idVector->push_back((int) Child->getID());
			if (ctr < noChildren){
				Child = Children->Next(Child);
			}
		}
	}
	return true;
}

bool cMediaDatabase::updateRecordTimerItems(){
	int timerMax = Timers.Count();
	if (timerMax <= 0){
		return true; // nothing to add
	}
	std::vector<int> objectVector;
	addContainerObjectIds(REC_TIMER_TV_ID, &objectVector);
	addContainerObjectIds(REC_TIMER_RADIO_ID, &objectVector);
	
	if ((int)objectVector.size() <= 0){
		return loadRecordTimerItems();
	}
	off_t resourceSize = 0;
	char* resourceName = NULL;
	cTimer* timer = NULL;
	int ctr = 0;
	for (timer = Timers.First(); ctr++ < timerMax && timer; ){
		bool found = false;
		for (int i = 0; i < (int)objectVector.size() && !found; i++){
			cUPnPObjectID ID = objectVector.at(i);
			cUPnPClassRecordTimerItem* rtObj = (cUPnPClassRecordTimerItem*) cUPnPObjectFactory::getInstance()->getObject(ID);					
			if (timer->Start() == rtObj->getStart() && strcmp(*timer->Channel()->GetChannelID().ToString(), rtObj->getChannelId()) == 0){
				found = true;
			}
		}
		if (!found){		
			MESSAGE(VERBOSE_RECORDS, "updateRecordTimerItems, add item: %s; Start %ld", *timer->ToText(), (long)timer->Start());
			createRecordTimerItem(timer, &resourceName, &resourceSize);
		}
		if (ctr < timerMax){
			timer = Timers.Next(timer);
		}
	}
	return true;
}

bool cMediaDatabase::purgeObsoleteRecordTimerItems(){
	std::vector<int> objectVector;
	addContainerObjectIds(REC_TIMER_TV_ID, &objectVector);
	addContainerObjectIds(REC_TIMER_RADIO_ID, &objectVector);
	if ((int)objectVector.size() <= 0){
		return true;
	}
	cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
	int ctr = 0;
	int timerMax = Timers.Count();
	cTimer* timer = NULL;
	MESSAGE(VERBOSE_METADATA, "purgeObsoleteRecordTimerItems: Number of record timers: %i", timerMax);
	if (timerMax > 0){
		for (int i = 0; i < (int)objectVector.size(); i++){
			cUPnPObjectID ID = objectVector.at(i);
			cUPnPClassRecordTimerItem* rtObj = (cUPnPClassRecordTimerItem*) factory->getObject(ID);
			bool found = false;
			if (rtObj){
				for (timer = Timers.First(); ctr++ < timerMax && !found && timer; ){
					MESSAGE(VERBOSE_METADATA, "Timer %i: %s", ctr, *timer->ToText());
			        if (timer->Start() == rtObj->getStart() && strcmp(*timer->Channel()->GetChannelID().ToString(), rtObj->getChannelId()) == 0){
						found = true;
					}
					if (ctr < timerMax){
						timer = Timers.Next(timer);
					}
				}
			}
			if (!found){
				cUPnPObjectID ID = objectVector.at(i);
				cUPnPClassObject* obj = factory->getObject(ID);
				if (obj){
					factory->deleteObject(obj);	
				}
			}
		}
	}
	else {
		for (int i = 0; i < (int)objectVector.size(); i++){
			cUPnPObjectID ID = objectVector.at(i);
			cUPnPClassObject* obj = factory->getObject(ID);
			if (obj){
				factory->deleteObject(obj);	
			}
		}		
	}
	objectVector.clear();
	return true;
}

bool cMediaDatabase::createRecordTimerItem(cTimer* timer, char** resourceName, off_t* resourceSize){
	if ((timer->Flags() && tfActive) == 0){
		return true;
	}
	bool isRadio = false;
	cChannel* Channel = NULL;
	for (int Index = 0; (Channel = Channels.Get(Index)); Index = Channels.GetNextNormal(Index)){
		tChannelID ChannelID = Channel->GetChannelID();
		if (strcmp(*timer->Channel()->GetChannelID().ToString(), *ChannelID.ToString()) == 0){
			if (ISRADIO(Channel)){
				isRadio = true;					   
			}
			break;
		}
	}
	cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
	cUPnPClassRecordTimerItem* rtObject = (cUPnPClassRecordTimerItem*)factory->createObject(UPNP_CLASS_BOOKMARKITEM, (timer->File()) ? timer->File() : "NO TITLE");
	rtObject->setFile(timer->File());
	rtObject->setStatus((int)timer->Flags());
	rtObject->setChannelId(*timer->Channel()->GetChannelID().ToString());
	rtObject->setDay(*timer->PrintDay(timer->Day(), timer->WeekDays(), true));
	rtObject->setStart(timer->Start());
	rtObject->setStop(timer->Stop());
	rtObject->setPriority(timer->Priority());
	rtObject->setLifetime(timer->Lifetime());
	rtObject->setAux(timer->Aux());
	rtObject->setRadioChannel(isRadio);
	cUPnPClassContainer* container = (cUPnPClassContainer*)this->getObjectByID((isRadio) ? REC_TIMER_RADIO_ID : REC_TIMER_TV_ID);
	if (container){
		container->addObject(rtObject);
		MESSAGE(VERBOSE_RECORDS, "record timer object added to container");
	}
	if (*resourceSize <= 0){
		findRecordTimerDeleteResource (resourceName, resourceSize);
	}
	if (*resourceSize > 0){
		cUPnPResources::getInstance()->createConfirmationResource(rtObject, (const char*) *resourceName, *resourceSize, false);
	}
	if (factory->saveObject(rtObject)) {
		ERROR("The record timer %s could not be saved.", timer->File());
		return false;
	}
	return true;
}

bool cMediaDatabase::findRecordTimerDeleteResource (char** resourceName, off_t *resourceSize){
	cString resFile = cString::sprintf("%s/%s/%s", cPluginUpnp::getConfigDirectory(), REC_TIMER_PURGE_CONFIRMATION_FOLDER, EPG_RECORD_CONFIRMATION_FILE);
	MESSAGE(VERBOSE_OBJECTS, "Recording timer delete confirmation file: %s", *resFile);
	struct stat sts;
	*resourceSize = 0;
	if ((stat (*resFile, &sts)) == -1 && errno == ENOENT){
		MESSAGE(VERBOSE_RECORDS,"No record timer delete confirmation file available");
	}
	else {
		cString resFolder = cString::sprintf("%s/%s", cPluginUpnp::getConfigDirectory(), REC_TIMER_PURGE_CONFIRMATION_FOLDER);
		*resourceName = (char*) strdup(resFolder);
		MESSAGE(VERBOSE_OBJECTS, "Record timer deletion confirmation file name: %s Size: %lu", *resourceName, (unsigned long)sts.st_size);
		*resourceSize = sts.st_size;
	}
	if (*resourceSize > 0){
		return true;
	}
	return false;
}

void cMediaDatabase::updateRecordings(){
    cUPnPClassContainer* vRecords = (cUPnPClassContainer*)this->getObjectByID(V_RECORDS_ID);
	cUPnPClassContainer* aRecords = (cUPnPClassContainer*)this->getObjectByID(A_RECORDS_ID);
    if (vRecords){
		checkDeletedHDDRecordings();    // synchronize the database with the hard disc: if records have been deleted from the HDD then update the db
        bool noResource = false;
        // TODO: Add to setup
        // if an error occured while loading resources, add the channel anyway
        cRecording* Recording = NULL;
        for (Recording = Recordings.First(); Recording; Recording = Recordings.Next(Recording)){
            // Iterating the records
            bool inList = false;
			bool isRadio = false;
            const cRecordingInfo* RecInfo = Recording->Info();
            MESSAGE(VERBOSE_RECORDS, "Determine whether the stored record %s is already listed in the database", Recording->FileName());
			cEvent* recEvent = (cEvent*)RecInfo->GetEvent();
			if (recEvent){
				cComponents* components = (cComponents*)recEvent->Components();
				if (components){
					if (components->NumComponents() < AUDIO_COMPONENTS_THRESHOLD) isRadio = true;
				}
			}
            cUPnPClassAudioRecord* audioRecItem = NULL;
			cUPnPClassMovie* MovieItem = NULL;
			// Find the UPnP contentdirectory objectID using the file name and get the cUPnPClassMovie instance for it.
			if (!isRadio){
                MovieItem = (cUPnPClassMovie*)this->getObjectByFastFind(Recording->FileName());
			}
			else {
				audioRecItem = (cUPnPClassAudioRecord*)this->getObjectByFastFind(Recording->FileName());
			}
            inList = ((MovieItem && vRecords->getObject(MovieItem->getID())) || 
				      (audioRecItem && aRecords && aRecords->getObject(audioRecItem->getID()))) ? true : false;
            if (inList){
				cUPnPClassObject* upnpObject = NULL;
				if (isRadio){
					upnpObject = (cUPnPClassObject*) audioRecItem;
				}
				else {
					upnpObject = (cUPnPClassObject*) MovieItem;
				}
				if (upnpObject){
					cList <cUPnPResource> * resources = upnpObject->getResources();
					//MESSAGE(VERBOSE_RECORDS, "Check file size and duration for the record item with ID %s, recording length(sec): %i", 
					//		 *upnpObject->getID(), Recording->LengthInSeconds());					
					if (resources == NULL || resources->Count() <= 0){
						ERROR("Found no resource for the record item with ID %s", *upnpObject->getID());
						noResource = true;
					}
					else {
						int numResources = resources->Count();
						if (numResources != 1){
							MESSAGE(VERBOSE_RECORDS, "Found number of resources for the record: %i", numResources);
						}
						const int arrLength = 40;
						off64_t sizeArray [arrLength];
						cIndexFile* Index = new cIndexFile(Recording->FileName(), false, Recording->IsPesRecording());					
						int amountRecord = readHddRecordSize(Recording, Index, &sizeArray[0], arrLength);
						int i = 0;
						bool changeSize = false;
						off64_t storedFileSize = 0;
						
						int ctr = 0;
						for (cUPnPResource* Resource = resources->First(); ctr++ < numResources && Resource && i < amountRecord && i <= arrLength; i++){
							storedFileSize += Resource->getFileSize();
							if (i+1 < arrLength && ((unsigned long)Resource->getFileSize() != (unsigned long)sizeArray[0])){
								changeSize = true;
								char* dur = duration((off64_t) (Index->Last() * AVDETECTOR_TIME_BASE / SecondsToFrames(1, Recording->FramesPerSecond())), AVDETECTOR_TIME_BASE);
								MESSAGE(VERBOSE_METADATA,"File size mismatch for resource: %ctr -> %lu <-> %lu, a db update is necessary. duration: %s", (i+1), (unsigned long)sizeArray[0],
									(unsigned long)Resource->getFileSize(), dur);
								Resource->setFileSize(sizeArray[0]);
								Resource->setResDuration(dur);
								if (updateResSizeDuration(Resource) < 0){
									ERROR("Could not update the Resource size and duration for the record file: %s", Recording->FileName());
								}
							}
							if (ctr < numResources){
								Resource = resources->Next(Resource);
							}
						}	
						Index->cIndexFile::~cIndexFile();
//						MESSAGE(VERBOSE_RECORDS, "Found a resource with the Record item, total HDD size: %lu - size in DB: %lu", (unsigned long) sizeArray[0], (unsigned long) storedFileSize);
					}
				}
			}
            else {
                MESSAGE(VERBOSE_RECORDS, "Skipping %s while updating the recordings' file sizes", Recording->FileName());
            }
        }
    }
}

int cMediaDatabase::loadRecordings(){
//    MESSAGE(VERBOSE_RECORDS, "Loading recordings; the video folder is %s; the update folder is %s", VideoDirectory, Recordings.GetUpdateFileName());
    cUPnPClassContainer* vRecords = (cUPnPClassContainer*)this->getObjectByID(V_RECORDS_ID);
	cUPnPClassContainer* aRecords = (cUPnPClassContainer*)this->getObjectByID(A_RECORDS_ID);
    if (vRecords){
		checkDeletedHDDRecordings();    // synchronize the database with the hard disc: if records have been deleted from the HDD then update the db
        bool noResource = false;
        // TODO: Add to setup
        // if an error occured while loading resources, add the channel anyway
        bool addWithoutResources = false;
        cRecording* Recording = NULL;
        for (Recording = Recordings.First(); Recording; Recording = Recordings.Next(Recording)){
            // Iterating the records
            bool inList = false;
			bool isRadio = false;
            const cRecordingInfo* RecInfo = Recording->Info();
            MESSAGE(VERBOSE_RECORDS, "Determine if the stored record '%s' is already listed in the database", Recording->FileName());
			cEvent* recEvent = (cEvent*)RecInfo->GetEvent();
			if (recEvent){
				cComponents* components = (cComponents*)recEvent->Components();
				if (components){
					if (components->NumComponents() < AUDIO_COMPONENTS_THRESHOLD) isRadio = true;
				}
			}
            cUPnPClassAudioRecord* audioRecItem = NULL;
			cUPnPClassMovie* MovieItem = NULL;
			// Find the UPnP contentdirectory objectID using the file name and get the cUPnPClassMovie instance for it.
			if (!isRadio){
                MovieItem = (cUPnPClassMovie*)this->getObjectByFastFind(Recording->FileName());
			}
			else {
				audioRecItem = (cUPnPClassAudioRecord*)this->getObjectByFastFind(Recording->FileName());
			}
            inList = ((MovieItem && vRecords->getObject(MovieItem->getID())) || 
				      (audioRecItem && aRecords && aRecords->getObject(audioRecItem->getID()))) ? true : false;
            if (!inList){
                noResource = false;				               
                MESSAGE(VERBOSE_RECORDS, "Adding movie or audio record '%s' File name:%s, ChannelName %s, Rec.Name %s", RecInfo->Title(), Recording->FileName(),
					        RecInfo->ChannelName(), Recording->Name());

				if (!isRadio){
					MovieItem = (cUPnPClassMovie*)this->mFactory->createObject(UPNP_CLASS_MOVIE, RecInfo->Title()?RecInfo->Title():Recording->Name());
					MovieItem->setDescription(RecInfo->ShortText());
					MovieItem->setLongDescription(RecInfo->Description());
					MovieItem->setStorageMedium(UPNP_STORAGE_HDD);
					MovieItem->setChannelName(RecInfo->ChannelName());
                
					if (RecInfo->Components()){
						// The first component
						tComponent *Component = RecInfo->Components()->Component(0);
						if(Component) MovieItem->setLanguage(Component->language);
					}

					if(cUPnPResources::getInstance()->createFromRecording(MovieItem, Recording)){
						ERROR("Unable to get resources for this video record");
						noResource = true;
					}
					if(!noResource || addWithoutResources) {
						vRecords->addObject(MovieItem);
						if(this->mFactory->saveObject(MovieItem) ||
						   this->addFastFind(MovieItem, Recording->FileName())){
							this->mFactory->deleteObject(MovieItem);
							ERROR("Error when saving movie item or adding movie to fast find table");
							return -1;
						}
						MESSAGE(VERBOSE_RECORDS, "Successfully added movie");
					}
					else {
						// Delete temporarily created object with no resource
						this->mFactory->deleteObject(MovieItem);
					}
				}
				else {
					audioRecItem = (cUPnPClassAudioRecord*)this->mFactory->createObject(UPNP_CLASS_MUSICTRACK, RecInfo->Title() ? RecInfo->Title():Recording->Name());
					audioRecItem->setStorageMedium(UPNP_STORAGE_HDD);
					audioRecItem->setChannelName(RecInfo->ChannelName());
					if (cUPnPResources::getInstance()->createFromRecording(audioRecItem, Recording)){
						ERROR("Unable to get resources for this audio record");
						noResource = true;
					}
					if (!noResource || addWithoutResources) {
						if (aRecords == NULL){
							WARNING("There is no audio record container to add an item");
							continue;
						}
						aRecords->addObject(audioRecItem);
						if(this->mFactory->saveObject(audioRecItem) ||
						   this->addFastFind(audioRecItem, Recording->FileName())){
							this->mFactory->deleteObject(audioRecItem);
							ERROR("Error when saving audio record item or adding audio record to fast find table");
							return -1;
						}
						MESSAGE(VERBOSE_RECORDS, "Successfully added audio record");
					}
					else {
						// Delete temporarily created object with no resource
						this->mFactory->deleteObject(audioRecItem);
					}
				}
            }
            else {
                MESSAGE(VERBOSE_RECORDS, "Skipping '%s', the record is already in the database", Recording->FileName());
            }
        }
    }
    return 0;
}

int cMediaDatabase::updateResSizeDuration(cUPnPResource* Resource){
	cString Format = "UPDATE %s SET %s=%lld,%s=%Q WHERE %s=%d";
	MESSAGE(VERBOSE_METADATA,"execute db statement for resource; duration: %s", Resource->getResDuration());
	pthread_mutex_lock(&(this->mDatabase->mutex_resource));
	if (this->mDatabase->execStatement(Format, SQLITE_TABLE_RESOURCES,                                                                                                                        
                                    SQLITE_COL_SIZE, Resource->getFileSize(),
                                    SQLITE_COL_DURATION, Resource->getResDuration(),                                                                                                                                                               
                                    SQLITE_COL_RESOURCEID, Resource->getID())){
		pthread_mutex_unlock(&(this->mDatabase->mutex_resource));
        ERROR("Error while executing statement update resource size and duration");
        return -1;
    }
	pthread_mutex_unlock(&(this->mDatabase->mutex_resource));
	return 0;
}

int cMediaDatabase::readHddRecordSize(cRecording* Recording, cIndexFile* Index, off64_t* sizeArray, const int arrLength){
	cFileName*  RecFile = new cFileName(Recording->FileName(), false, false, Recording->IsPesRecording());
	uint16_t    FileNumber = 0;
	off_t       FileOffset = 0;
	off64_t     recordSize = 0;
	int numberOfFiles = 0;
	if (Index->Get(Index->Last()-1, &FileNumber, &FileOffset)){
		for (int i = 0; i < FileNumber; i++){
			struct stat Stats;
			RecFile->SetOffset(i+1);
			stat(RecFile->Name(), &Stats);
			recordSize += Stats.st_size;
			if (i < arrLength - 1){
			    sizeArray [i + 1] = Stats.st_size;
			}
			else {
				WARNING("Note: the file size array is too small; length: %i", arrLength);
			}
//			MESSAGE(VERBOSE_METADATA,"Record, file size: %lu", (unsigned long) Stats.st_size);
			numberOfFiles = i+1;
		}
	}
	RecFile->cFileName::~cFileName();
	sizeArray[0] = recordSize;  // the first field keeps the accumulated size
	return numberOfFiles;
}

void cMediaDatabase::checkDeletedHDDRecordings(){
	MESSAGE(VERBOSE_OBJECTS, "Check for deleted HDD recordings");
	if (!VideoDirectory || ! *VideoDirectory){
		ERROR("Error: The recordings directory is not specified.");
		return;
	}
    if(this->mDatabase->execStatement("SELECT * FROM %s WHERE %s > 0",
                                      SQLITE_TABLE_ITEMFINDER, SQLITE_COL_OBJECTID)){
        ERROR("Error while executing statement to check deleted recordings");
    }
	cString Column, Value;
	cRows* Rows = this->mDatabase->getResultRows();
    cRow* Row;
    while(Rows->fetchRow(&Row)){
		char* locationDb = NULL;
		int objectId = -1;
		bool deleteObject = false;
        while(Row->fetchColumn(&Column, &Value)){
			if(!strcasecmp(Column, SQLITE_COL_OBJECTID)){
				if (*Value){
					objectId = atoi(*Value);
				}
			}
            else if(!strcasecmp(Column, SQLITE_COL_ITEMFINDER)){
				locationDb = strdup(Value);
				if (strlen(VideoDirectory) < strlen(locationDb) && strncmp(VideoDirectory, locationDb, strlen(VideoDirectory)) == 0){
					bool inRecList = false;
					cRecording* Recording = NULL;
					for(Recording = Recordings.First(); Recording && !inRecList; Recording = Recordings.Next(Recording)){
						// Iterating the records           
						if (strcmp (Recording->FileName(), locationDb) == 0){
							inRecList = true;
//						    MESSAGE(VERBOSE_RECORDS, "File name is identical with HDD file %s", Recording->FileName());
						}
					}
					if (inRecList){
						MESSAGE(VERBOSE_RECORDS, "The DB movie entry '%s' exists on HDD", locationDb);
					}
					else {
						MESSAGE(VERBOSE_RECORDS, "The DB movie entry '%s' does NOT exist on HDD and has to be deleted", locationDb);
						deleteObject = true;
						
					}
				}
            }
        }
		if (deleteObject){
			MESSAGE(VERBOSE_RECORDS, "The DB movie entry with ID %i (%s) does NOT exist on HDD and has to be deleted", objectId, locationDb);
			cUPnPMovieMediator* mediator = (cUPnPMovieMediator*) cUPnPObjectFactory::getInstance()->findMediatorByClass(UPNP_CLASS_MOVIE);
			cUPnPClassObject* movieObject = this->getObjectByID(objectId);
			if (mediator && movieObject){
				MESSAGE(VERBOSE_RECORDS, "Try to delete the movie from the db");
				if (mediator->deleteObject(movieObject) == 0){
					MESSAGE(VERBOSE_RECORDS, "The object with ID %i was deleted from the db", objectId);
				}
				else {
					MESSAGE(VERBOSE_RECORDS, "*** Error with deleting object %i from db", objectId);
				}
			}
		}
    }
}
#endif

cUPnPObjects* cMediaDatabase::getChildrenList(int objId){
	cUPnPClassObject* Object = cUPnPObjectFactory::getInstance()->getObject(objId);
	if (Object && Object->isContainer()){		                  
        return Object->getContainer()->getObjectList();
	}
	return NULL;
}

bool cMediaDatabase::purgeContainerObjects(int objId){
	cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
	cUPnPObjects* Children = getChildrenList (objId);
	if (Children && Children->Count() > 0){
		int noChildren = Children->Count();
		std::vector<int> objectVector;
		int ctr = 0;
		cUPnPClassObject* Child = Children->First();
		for(; ctr++ < noChildren && Child; ){
			objectVector.push_back((int) Child->getID());
			if (ctr < noChildren){
				Child = Children->Next(Child);
			}
		}
		MESSAGE(VERBOSE_EPG_UPDATES, "Delete %i objects from container with ID %i", (int)objectVector.size(), objId);
		for (int i = 0; i < (int)objectVector.size(); i++){
			cUPnPObjectID ID = objectVector.at(i);
			cUPnPClassObject* obj = factory->getObject(ID);
			if (obj){
				factory->deleteObject(obj);	
			}
		}
		objectVector.clear();
		MESSAGE(VERBOSE_EPG_UPDATES, "The record timer items deleted");
	}		
	return true;
}

/**
 * A c t i o n
 */
void cMediaDatabase::Action(){
	MESSAGE(VERBOSE_EPG_UPDATES, "The media database thread was started.");
	cUPnPConfig* config = cUPnPConfig::get();
    time_t LastEPGUpdate = 0;
	int NotUsed = 0;
	int ctr = 0;
	int secSleep = 15;
	bool epgRead = false;
	bool dbNormalMode = false;
#ifndef WITHOUT_EPG
    cVdrEpgInfo* vdrEpgInfo= new cVdrEpgInfo(this);   
#endif
#ifndef WITHOUT_AUDIO
	MESSAGE(VERBOSE_EPG_UPDATES, "AUDIO IS INCLUDED");
#endif
#ifdef WITHOUT_AUDIO
	MESSAGE(VERBOSE_EPG_UPDATES, "AUDIO is EXCLUDED");
#endif
    while(this->Running()){
#ifndef WITHOUT_RECORDS
        if(Recordings.StateChanged(NotUsed)){
            MESSAGE(VERBOSE_EPG_UPDATES, "Recordings changed. Updating...");
            loadRecordings();
        }
		if (ctr == 2){
			purgeContainerObjects(REC_TIMER_TV_ID);
			purgeContainerObjects(REC_TIMER_RADIO_ID);
			loadRecordTimerItems();
		}
		if (ctr > 4){
			purgeObsoleteRecordTimerItems();
			if ((ctr % 2) == 0){
				updateRecordTimerItems();
				updateRecordings();
			}
		}
#endif
#ifndef WITHOUT_EPG
		if (config->mEpgShow){
			if (ctr >= 1 && this->mIsInitialised && !epgRead){
				vdrEpgInfo->Read();
				epgRead = true;
			}
			if (ctr > 3 && epgRead){			
				vdrEpgInfo->purgeObsoleteEpgObjects();
				vdrEpgInfo->updateEpgItems(ctr);			
			}
		}
#endif
		time_t schedulesModified = cSchedules::Modified();
#ifndef WITHOUT_AUDIO
		if (ctr > 2 && schedulesModified > LastEPGUpdate && this->mIsInitialised){
			MESSAGE(VERBOSE_EPG_UPDATES, "Schedule changed. Updating radio...");
			updateRadioChannelEPG();
		}
#endif		
#ifndef WITHOUT_TV
        if (ctr > 2 && schedulesModified > LastEPGUpdate && this->mIsInitialised){
            MESSAGE(VERBOSE_EPG_UPDATES, "Schedule changed. Updating...");
            updateChannelEPG();
        }
#endif
		if (ctr > 2 && schedulesModified > LastEPGUpdate && this->mIsInitialised){
			LastEPGUpdate = schedulesModified;
		}
        cCondWait::SleepMs(secSleep * 1000);   // sleep number of seconds
		if (ctr <= 2 && !dbNormalMode && epgRead){
			dbNormalMode = true;
			char* Error;
			if (sqlite3_exec(this->mDatabase->getSqlite3(), "PRAGMA synchronous = NORMAL", NULL, NULL, &Error)){
				ERROR("Error while setting PRAGMA NORMAL: %s", Error);
			}
			if (sqlite3_exec(this->mDatabase->getSqlite3(), "PRAGMA journal_mode = TRUNCATE", NULL, NULL, &Error)){
				ERROR("Error while setting journal mode to truncate: %s", Error);
			}
			secSleep = 60;
		}
		ctr++;
    }
	MESSAGE(VERBOSE_EPG_UPDATES, "Metadata ACTION loop is no more running");
}

/**
  *  B R O W S E
  */
int cMediaDatabase::browse(
    OUT cUPnPResultSet** Results,
    IN const char* ID,
    IN bool BrowseMetadata,
    IN const char* Filter,
    IN unsigned int Offset,
    IN unsigned int Count,
    IN const char* SortCriteria){

    *Results = new cUPnPResultSet;
    (*Results)->mNumberReturned = 0;
    (*Results)->mTotalMatches = 0;
    (*Results)->mResult = NULL;

    MESSAGE(VERBOSE_DIDL, "===== Browsing =====");
    MESSAGE(VERBOSE_DIDL, "ID: %s", ID);
    MESSAGE(VERBOSE_DIDL, "Browse %s", BrowseMetadata?"metadata":"children");
    MESSAGE(VERBOSE_DIDL, "Filter: %s", Filter);
    MESSAGE(VERBOSE_DIDL, "Offset: %d", Offset);
    MESSAGE(VERBOSE_DIDL, "Count: %d", Count);
    MESSAGE(VERBOSE_DIDL, "Sort: %s", SortCriteria);

    cUPnPObjectID ObjectID = atoi(ID);

    cStringList* FilterList = cFilterCriteria::parse(Filter);
    cList<cSortCrit>* SortCriterias = cSortCriteria::parse(SortCriteria);

    if (!SortCriterias){
        return UPNP_CDS_E_INVALID_SORT_CRITERIA;
    }
 	cUPnPClassObject* Object = cUPnPObjectFactory::getInstance()->getObject(ObjectID);
    if (Object){
		cUPnPConfig* config = cUPnPConfig::get();
        IXML_Document* DIDLDoc = NULL;
        if (ixmlParseBufferEx(UPNP_DIDL_SKELETON, &DIDLDoc) == IXML_SUCCESS){

            IXML_Node* Root = ixmlNode_getFirstChild((IXML_Node*) DIDLDoc);
            switch(BrowseMetadata){
                case true:
                    ixmlNode_appendChild(Root, Object->createDIDLFragment(DIDLDoc, FilterList));
                    delete FilterList;
                    (*Results)->mNumberReturned = 1;
                    (*Results)->mTotalMatches = 1;
                    (*Results)->mResult = ixmlDocumenttoString(DIDLDoc);
                    ixmlDocument_free(DIDLDoc);
                    return UPNP_E_SUCCESS;
                case false:
                    if (Object->isContainer()){
                        cUPnPClassContainer* Container = Object->getContainer();
                        cUPnPObjects* Children = Container->getObjectList();

                        if (SortCriterias){
                            for(cSortCrit* SortBy = SortCriterias->First(); SortBy; SortBy = SortCriterias->Next(SortBy)){
                                MESSAGE(VERBOSE_DIDL, "Sorting by %s %s", SortBy->Property, SortBy->SortDescending?"ascending":"descending");
                                Children->SortBy(SortBy->Property, SortBy->SortDescending);
                            }
                        }
                        cUPnPClassObject* Child = Children->First();
                        if (Count==0 || Count > Container->getChildCount()){
                            Count = Container->getChildCount();
						}
                        
                        MESSAGE(VERBOSE_DIDL, "BrowseDirectChildren: Number of children: %d", Count);
						if (Count > 0){
							while(Offset-- && (Child = Children->Next(Child))){}

							for(; Count && Child; Child = Children->Next(Child), Count--){
								int childId = (int)Child->getID();
								if ((childId != EPG_ID && childId != RADIO_ID && childId != EPG_RADIO_ID && childId != AUDIO_ID &&
									childId != REC_TIMER_ID && childId != REC_TIMER_RADIO_ID) ||
									(childId == EPG_ID && config->mEpgShow) || (childId == RADIO_ID && config->mRadioShow) ||
									(childId == EPG_RADIO_ID && config->mRadioShow) || (childId == AUDIO_ID && config->mRadioShow) ||
									(childId == REC_TIMER_ID && !config->mOpressTimers) || 
									(childId == REC_TIMER_RADIO_ID && !config->mOpressTimers && config->mRadioShow)){
									ixmlNode_appendChild(Root, Child->createDIDLFragment(DIDLDoc, FilterList));
									(*Results)->mNumberReturned++;
									(*Results)->mTotalMatches++;
								}
							}
						}
                        delete FilterList;
                        delete SortCriterias;
                    }
                    else {
                        (*Results)->mNumberReturned = 0;
                        (*Results)->mTotalMatches = 0;						
                    }
                    (*Results)->mResult = ixmlDocumenttoString(DIDLDoc);
                    ixmlDocument_free(DIDLDoc);
                    return UPNP_E_SUCCESS;
            }
        }
        else {
            ERROR("Unable to parse DIDL skeleton");
            return UPNP_CDS_E_CANT_PROCESS_REQUEST;
        }
    }
    else {
		if (BrowseMetadata){
			ERROR("No such object: %s", ID);
			return UPNP_CDS_E_NO_SUCH_OBJECT; // No such object;
		}
		else {  // BrowseDirectChildren: empty container
			(*Results)->mNumberReturned = 0;
            (*Results)->mTotalMatches = 0;
			(*Results)->mResult = NULL;
			return UPNP_E_SUCCESS;
		}
    }
    return UPNP_SOAP_E_ACTION_FAILED;
}

#ifndef WITHOUT_EPG
bool cMediaDatabase::purgeAllEpgChannels(){
	bool success = true;
	cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
	cUPnPObjectID* EpgTvID = new cUPnPObjectID(EPG_TV_ID);
	cUPnPClassContainer* epgTvContainer = (cUPnPClassContainer*) factory->getObject(*EpgTvID);
	delete EpgTvID;
	if (epgTvContainer == NULL){
		WARNING("No UPnP EPG TV container object was found");
	}
	else {
		cUPnPObjects* tvObjects = epgTvContainer->getObjectList();
		if (tvObjects){
			success = deleteEpgContainer(tvObjects);
		}
	}

	cUPnPClassContainer* epgRadioContainer = NULL;
	cUPnPObjectID* EpgRadioID = new cUPnPObjectID(EPG_RADIO_ID);
	epgRadioContainer = (cUPnPClassContainer*) factory->getObject(*EpgRadioID);
	delete EpgRadioID;
	if (epgRadioContainer == NULL){
		WARNING("No UPnP EPG RADIO container object was found");
	}
	else {
		cUPnPObjects* radioObjects = epgRadioContainer->getObjectList();
		if (radioObjects){
			success = deleteEpgContainer(radioObjects) && success;
		}
	}
	return success;
}

bool cMediaDatabase::deleteEpgContainer(cUPnPObjects* bcObjects){
	std::vector<int> delVector;
	cUPnPClassObject* bcObj = NULL;
	int ctr = 0;
	int radioCount = bcObjects->Count();
	if (radioCount > 0){
		for (bcObj = bcObjects->First(); bcObj && ctr < radioCount; ctr++){
			delVector.push_back((int)bcObj->getID());
			if (ctr < radioCount){						
				bcObj = bcObjects->Next(bcObj);
			}
		}
	}
	return purgeSelectedChannels(&delVector);
}

bool cMediaDatabase::purgeSelectedChannels(std::vector<int>* delVector){
	cUPnPObjectFactory* factory = cUPnPObjectFactory::getInstance();
	bool success = true;
	int debugCtr = 0;
	for (int i = 0; i < (int) delVector->size(); i++){
		cUPnPObjectID* objId = new cUPnPObjectID(delVector->at(i));
		cUPnPClassObject* delObj = factory->getObject(*objId);
		if (delObj){
			success = factory->deleteObject(delObj) == 0 && success;
			if (success){
				debugCtr++;
//				MESSAGE(VERBOSE_PARSERS, "The EPG channel with ID %i was deleted", delVector->at(i));
			}
			else {
				WARNING("The EPG channel with ID %i could not be deleted", delVector->at(i));
			}
		}
		delete objId;
	}
	if (debugCtr > 0){
	   MESSAGE(VERBOSE_PARSERS, "Number of deleted EPG channels: %i", debugCtr);
	}
	delVector->clear();
	return success;
}
#endif