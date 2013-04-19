/* 
 * File:   resources.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 30. September 2009, 15:17
 * Last modification: April 9, 2013
 */

#include <string.h>
#include <vdr/channels.h>
#include "upnp/dlna.h"
#include <vdr/tools.h>
#include "resources.h"
#include "avdetector.h"
#include "vdrepg.h"

cUPnPResource::cUPnPResource(){
    this->mBitrate = 0;
    this->mBitsPerSample = 0;
    this->mColorDepth = 0;
    this->mDuration = NULL;
    this->mImportURI = NULL;
    this->mNrAudioChannels = 0;
    this->mProtocolInfo = NULL;
    this->mResolution = NULL;
    this->mResource = NULL;
    this->mResourceID = 0;
    this->mSampleFrequency = 0;
    this->mSize = 0;
	this->mRecordTimer = 0;
    this->mContentType = NULL;
	this->mObjectId = -1;
	this->mCacheAdded = false;
}

time_t cUPnPResource::getLastModification() const {
    time_t Time;
    const cRecording* Recording;
    const cEvent* Event;
    switch(this->mResourceType){
        case UPNP_RESOURCE_CHANNEL:
        case UPNP_RESOURCE_URL:
            Time = time(NULL);
            break;
        case UPNP_RESOURCE_RECORDING:
            Recording = Recordings.GetByName(this->mResource);
            Event = (Recording)?Recording->Info()->GetEvent():NULL;
            Time = (Event)?Event->EndTime():time(NULL);
            break;
        case UPNP_RESOURCE_FILE:
			Time = time(NULL);
            break;
        default:
            ERROR("Invalid resource type. This resource might be broken");
            Time = -1;
    }
    return Time;
}

 void cUPnPResource::setResDuration(char* duration) {
	 this->mDuration = cString(duration, false);
 }

 void cUPnPResource::setResolution(char* resolution){
	 this->mResolution = cString(resolution, false);
 }

 void cUPnPResource::setNrAudioChannels(unsigned int nrAudioChann){
	 this->mNrAudioChannels = nrAudioChann;
 }

 void cUPnPResource::setRecordTimer (int recTimer){
	 this->mRecordTimer = recTimer;
 }

void cUPnPResource::setObjectId(int objId){
	this->mObjectId = objId;
}

void cUPnPResource::setBitrate(unsigned int bitRate){
	this->mBitrate = bitRate;
}

cUPnPResources* cUPnPResources::mInstance = NULL;

cUPnPResources::cUPnPResources(){
    this->mResources = new cHash<cUPnPResource>;
    this->mMediator = new cUPnPResourceMediator;
    this->mDatabase = cSQLiteDatabase::getInstance();
}

cUPnPResources::~cUPnPResources(){
    delete this->mResources;
    delete this->mMediator;
}

cUPnPResources* cUPnPResources::getInstance(){
    if (!cUPnPResources::mInstance){
        cUPnPResources::mInstance = new cUPnPResources();
	}
    if (cUPnPResources::mInstance){
		return cUPnPResources::mInstance;
	}
    return NULL;
}

int cUPnPResources::loadResources(){
    if(this->mDatabase->execStatement("SELECT %s FROM %s",SQLITE_COL_RESOURCEID,SQLITE_TABLE_RESOURCES)){
        ERROR("Error while executing statement, 'loadResources'");
        return -1;
    }
    cRows* Rows = this->mDatabase->getResultRows(); cRow* Row;
    cString Column = NULL, Value = NULL;
    while(Rows->fetchRow(&Row)){
        while(Row->fetchColumn(&Column, &Value)){
            if(!strcasecmp(Column, SQLITE_COL_RESOURCEID)){
                unsigned int ResourceID = (unsigned int)atoi(Value);
                this->getResource(ResourceID);
            }
        }
    }
    return 0;
}

int cUPnPResources::getResourcesOfObject(cUPnPClassObject* Object){
	if (Object == NULL){
	    ERROR("cUPnPResources::getResourcesOfObject: No valid Object was given");
		return -1;
	}
	sqlite3_stmt* resSelObjStmt = this->mDatabase->getResourceObjSelectStatement();
	if (resSelObjStmt == NULL){
		ERROR("Got no compiled resource selection statement");
		return -1;
	}
//	MESSAGE(VERBOSE_METADATA, "cUPnPResources::getResourcesOfObject with ID %d", ((unsigned int) Object->getID()));
	pthread_mutex_lock(&(this->mDatabase->mutex_resource));
	bool actionSuccess = sqlite3_bind_int (resSelObjStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	if (!actionSuccess){
		WARNING("getResourcesOfObject: Error when binding the SQL statement for a resource ID selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
	}
	int ctr = 0;
	int resourceId = -1;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(resSelObjStmt);
		switch (stepRes){
			case SQLITE_ROW:
				resourceId = sqlite3_column_int(resSelObjStmt, 0);
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
		}
	}
	sqlite3_clear_bindings(resSelObjStmt);
	sqlite3_reset(resSelObjStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_resource));
    if (resourceId >= 0){
		Object->addResource(this->getResource((unsigned int) resourceId));
	}

	if (!actionSuccess){
	MESSAGE(VERBOSE_METADATA, "cUPnPResources::getResourcesOfObject (conventional way) with ID %d", ((unsigned int) Object->getID()));
		if(this->mDatabase->execStatement("SELECT %s FROM %s WHERE %s=%Q",
											SQLITE_COL_RESOURCEID,
											SQLITE_TABLE_RESOURCES,
											SQLITE_COL_OBJECTID,
											*Object->getID())){
			ERROR("Error while executing statement, 'getResourcesOfObject'");
			return -1;
		}
		cRows* Rows = this->mDatabase->getResultRows(); cRow* Row;
		cString Column = NULL, Value = NULL;
		while(Rows->fetchRow(&Row)){
			while(Row->fetchColumn(&Column, &Value)){
				if(!strcasecmp(Column, SQLITE_COL_RESOURCEID)){
					 MESSAGE(VERBOSE_METADATA, "For the upnp object with ID %d the resource with ID %s was found in the database",
							 (unsigned int) Object->getID(), *Value);
					unsigned int ResourceID = (unsigned int)atoi(Value);
					Object->addResource(this->getResource(ResourceID));
				}
			}
		}
	}
    return 0;
}

cUPnPResource* cUPnPResources::getResource(unsigned int ResourceID){
    cUPnPResource* Resource;
    if((Resource = this->mResources->Get(ResourceID))){
//        MESSAGE(VERBOSE_METADATA, "Found cached resource");
        return Resource;
    }
    else if((Resource = this->mMediator->getResource(ResourceID))){
		addCache(Resource);
        return Resource;
    }
    else {
        ERROR("No such resource with ID '%d'", ResourceID);
        return NULL;
    }
}

int cUPnPResources::createFromRecording(cUPnPClassObject* Object, cRecording* Recording){
    if(!Object || !Recording){
        ERROR("createFromRecording: Invalid input arguments");
        return -1;
    }

    cAudioVideoDetector* Detector = new cAudioVideoDetector(Recording);

    if(Detector->detectProperties()){
        ERROR("Error while detecting video or audio properties");
        delete Detector;
        return -1;
    }

    if(!Detector->getDLNAProfile()){
        ERROR("No DLNA profile found for Recording %s", Recording->Name());
        delete Detector;
        return -1;
    }

    const char* ProtocolInfo = cDlna::getInstance()->getProtocolInfo(Detector->getDLNAProfile());
    MESSAGE(VERBOSE_METADATA, "Protocol info: %s", ProtocolInfo);   
    cString ResourceFile     = Recording->FileName();
    cUPnPResource* Resource  = this->mMediator->newResource(Object, UPNP_RESOURCE_RECORDING, ResourceFile, Detector->getDLNAProfile()->mime, ProtocolInfo, false);
	if (Resource == NULL){
		return -1;
	}
    Resource->mBitrate       = Detector->getBitrate();
    Resource->mBitsPerSample = Detector->getBitsPerSample();
    Resource->mDuration      = duration(Detector->getDuration(), AVDETECTOR_TIME_BASE);
    Resource->mResolution    = (Detector->getWidth() && Detector->getHeight()) ? *cString::sprintf("%dx%d", Detector->getWidth(), Detector->getHeight()) : NULL;
    Resource->mSampleFrequency = Detector->getSampleFrequency();
    Resource->mSize          = Detector->getFileSize();
    Resource->mNrAudioChannels = Detector->getNumberOfAudioChannels();
    Resource->mImportURI     = NULL;
    Resource->mColorDepth    = 0;
	Resource->setObjectId((int)Object->getID());
    Object->addResource(Resource);
    this->mMediator->saveResource(Object, Resource);
	addCache(Resource);

    delete Detector;
    return 0;
}

int cUPnPResources::createEpgResourceByCopy(cUPnPClassObject* epgObject, cUPnPResource* timerResource){
    if(!epgObject || !timerResource){
        ERROR("createEpgResourceByCopy: At least one input argument is NULL");
        return -1;
    }
	cUPnPResource* timerResourceNew = this->mMediator->newResource(epgObject, timerResource->getResourceType(), 
				    cString(timerResource->getResource(), false), cString(timerResource->getContentType(), false),
					cString(timerResource->getProtocolInfo(), false), true);
	if (timerResourceNew == NULL){
		return -1;
	}
	timerResourceNew->setRecordTimer(DO_TRIGGER_TIMER);
	timerResourceNew->setResDuration((char*) timerResource->getResDuration());
	timerResourceNew->setResolution ((char*) timerResource->getResolution());
	timerResourceNew->setFileSize (timerResource->getFileSize());
	timerResourceNew->setNrAudioChannels(timerResource->getNrAudioChannels());
	timerResourceNew->setObjectId((int)epgObject->getID());
	timerResourceNew->setBitrate(timerResource->getBitrate());
	epgObject->addResource(timerResourceNew);
	this->mMediator->saveResource(epgObject, timerResourceNew);
	addCache(timerResourceNew);
	return 0;
}

int cUPnPResources::createConfirmationResource(cUPnPClassObject* object, const char* fileName, off_t fileSize, bool isEpgObject){
	cUPnPResource* timerResourceNew = this->mMediator->newResource(object, UPNP_RESOURCE_FILE, 
				    cString(fileName, false), cString("video/mpeg", false),
					cString("http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_SD_EU_ISO", false), isEpgObject);
	if (timerResourceNew == NULL){
		return -1;
	}
	timerResourceNew->setRecordTimer((isEpgObject) ? DO_TRIGGER_TIMER : PURGE_RECORD_TIMER);
	timerResourceNew->setResDuration(strdup("0:00:05"));
	timerResourceNew->setResolution (strdup("720x576"));
	timerResourceNew->setFileSize (fileSize);
	timerResourceNew->setNrAudioChannels(2);
	timerResourceNew->setObjectId((int)object->getID());
	timerResourceNew->setBitrate(15000000);
	object->addResource(timerResourceNew);
	this->mMediator->saveResource(object, timerResourceNew);
	addCache(timerResourceNew);
	return 0;
}

int cUPnPResources::createFromFile(cUPnPClassItem*, cString ){
    MESSAGE(VERBOSE_SDK, "To be done");
    return -1;
}

int cUPnPResources::createFromChannel(cUPnPClassObject* Object, cChannel* Channel){
    if(!Object || !Channel){
        ERROR("Invalid input arguments");
        return -1;
    }

    cAudioVideoDetector* Detector = new cAudioVideoDetector(Channel);

    if(Detector->detectProperties()){
        ERROR("Cannot detect channel properties");
        delete Detector;
        return -1;
    }

    if(!Detector->getDLNAProfile()){
        ERROR("No DLNA profile found for Channel %s", *Channel->GetChannelID().ToString());
        delete Detector;
        return -1;
    }

    const char* ProtocolInfo = cDlna::getInstance()->getProtocolInfo(Detector->getDLNAProfile());
    MESSAGE(VERBOSE_METADATA, "Protocol info: %s", ProtocolInfo);

    // Index which may be used to indicate different resources with same channel ID
    // For instance a different DVB card
    // Not used yet.
    int index = 0;
    
    cString ResourceFile     = cString::sprintf("%s:%d", *Channel->GetChannelID().ToString(), index);
    cUPnPResource* Resource  = this->mMediator->newResource(Object, UPNP_RESOURCE_CHANNEL, ResourceFile, Detector->getDLNAProfile()->mime, ProtocolInfo, false);
	if (Resource == NULL){
		return -1;
	}
    Resource->mBitrate       = Detector->getBitrate();
    Resource->mBitsPerSample = Detector->getBitsPerSample();
    Resource->mDuration      = duration(Detector->getDuration());
    Resource->mResolution    = (Detector->getWidth() && Detector->getHeight()) ? *cString::sprintf("%dx%d",Detector->getWidth(), Detector->getHeight()) : NULL;
    Resource->mSampleFrequency = Detector->getSampleFrequency();
    Resource->mSize          = Detector->getFileSize();
    Resource->mNrAudioChannels = Detector->getNumberOfAudioChannels();
    Resource->mImportURI     = NULL;
    Resource->mColorDepth    = 0;
	Resource->setObjectId((int)Object->getID());
    Object->addResource(Resource);
    this->mMediator->saveResource(Object, Resource);
 	addCache(Resource);

    delete Detector;
    return 0;
}

int cUPnPResources::deleteCachedResources(cUPnPClassObject* delObject){
	if (delObject == NULL){
		return -1;
	}
//	MESSAGE(VERBOSE_METADATA, "deleteCachedResources: Get resources of object");
	cList<cUPnPResource>* resList = delObject->getResources();
	cUPnPResource* Resource;
	if (resList == NULL){
		MESSAGE(VERBOSE_METADATA, "deleteCachedResources: res list is NULL");
		return -1;
	}
	MESSAGE(VERBOSE_METADATA, "deleteCachedResources: check res list count");
	if (resList->Count() <= 0){
		MESSAGE(VERBOSE_METADATA, "deleteCachedResources: No resource(s) available");
		return -1;
	}
	int ctr = 0;
	const int resCount = resList->Count();
	MESSAGE(VERBOSE_METADATA, "deleteCachedResources: Number of resources: %i with object ID %s", resCount, *delObject->getID());
	for (Resource = resList->First(); Resource && ctr++ < resCount; ){
		if (Resource && Resource->getID() && this->mResources && Resource->mCacheAdded){
		    MESSAGE(VERBOSE_METADATA, "deleteCachedResources: Remove the cached resource with ID: %d", Resource->getID());
			this->mResources->Del(Resource, Resource->getID());
			Resource->mCacheAdded = false;
		}
		if (ctr < resCount){
			Resource = resList->Next(Resource);
		}
	}
	return 0;
}

void cUPnPResources::addCache(cUPnPResource* resource){
	if (resource == NULL){
		ERROR("cUPnPResources::addCache: the resource is NULL");
		return;
	}
	this->mResources->Add(resource, resource->getID());
	resource->mCacheAdded = true;
}

cUPnPResourceMediator::cUPnPResourceMediator(){
    this->mDatabase = cSQLiteDatabase::getInstance();

	MESSAGE(VERBOSE_METADATA, "Construct a cUPnPResourceMediator instance");
	cString resStatement = cString::sprintf("UPDATE %s SET %s=%s+1 WHERE %s=:PK", SQLITE_TABLE_PRIMARY_KEYS, SQLITE_COL_KEY, SQLITE_COL_KEY, SQLITE_COL_KEYID);
	const char *zSql = *resStatement;
	const char **pzTail = NULL;	
	if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mPKUpdStmt), pzTail) != SQLITE_OK ){
		ERROR("Error when compiling the SQL statement for a primary key update: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
	}

	resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s=:LN", SQLITE_COL_KEY, SQLITE_TABLE_PRIMARY_KEYS, SQLITE_COL_KEYID);
	zSql = *resStatement;
	pzTail = NULL;	
	if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mPKSelStmt), pzTail) != SQLITE_OK ){
		ERROR("Error when compiling the SQL statement for a primary key select: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
	}

	this->mResSelStmt = NULL;
	this->mResInsStmt = NULL;
	this->mResUpdStmt = NULL;
}

cUPnPResourceMediator::~cUPnPResourceMediator(){}

cUPnPResource* cUPnPResourceMediator::getResource(unsigned int ResourceID){
    cUPnPResource* Resource = new cUPnPResource;
	pthread_mutex_lock(&(this->mDatabase->mutex_resource));
    Resource->mResourceID = ResourceID;
	if (this->mResSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s FROM %s WHERE %s=:ID", SQLITE_COL_PROTOCOLINFO, 
			                 SQLITE_COL_RESOURCE, SQLITE_COL_SIZE, SQLITE_COL_DURATION, SQLITE_COL_BITRATE, SQLITE_COL_SAMPLEFREQUENCE, 
							 SQLITE_COL_BITSPERSAMPLE, SQLITE_COL_NOAUDIOCHANNELS, SQLITE_COL_COLORDEPTH, SQLITE_COL_RESOLUTION,
							 SQLITE_COL_CONTENTTYPE, SQLITE_COL_RESOURCETYPE, SQLITE_COL_RECORDTIMER, SQLITE_COL_OBJECTID,
							 SQLITE_TABLE_RESOURCES, SQLITE_COL_RESOURCEID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mResSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a resource selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}

	bool actionSuccess = sqlite3_bind_int (this->mResSelStmt, 1, ResourceID) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("getResource: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(this->mResSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				Resource->mProtocolInfo = strdup0((const char*) sqlite3_column_text(this->mResSelStmt, 0));
				Resource->mResource = strdup0((const char*) sqlite3_column_text(this->mResSelStmt, 1));
				Resource->mSize = (off64_t) sqlite3_column_int(this->mResSelStmt, 2);
				Resource->mDuration = strdup0((const char*) sqlite3_column_text(this->mResSelStmt, 3));
				Resource->mBitrate = (unsigned int) sqlite3_column_int(this->mResSelStmt, 4);
				Resource->mSampleFrequency = (unsigned int) sqlite3_column_int(this->mResSelStmt, 5);
				Resource->mBitsPerSample = (unsigned int) sqlite3_column_int(this->mResSelStmt, 6);
				Resource->mNrAudioChannels = (unsigned int) sqlite3_column_int(this->mResSelStmt, 7);
				Resource->mColorDepth = (unsigned int) sqlite3_column_int(this->mResSelStmt, 8);
				Resource->mResolution = strdup0((const char*) sqlite3_column_text(this->mResSelStmt, 9));
				Resource->mContentType = strdup0((const char*) sqlite3_column_text(this->mResSelStmt, 10));
				Resource->mResourceType = sqlite3_column_int(this->mResSelStmt, 11);
				Resource->mRecordTimer = sqlite3_column_int(this->mResSelStmt, 12);
				Resource->mObjectId = sqlite3_column_int(this->mResSelStmt, 13);
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("getResource: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(this->mResSelStmt);
	sqlite3_reset(this->mResSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_resource));
    return Resource;
}

int cUPnPResourceMediator::saveResource(cUPnPClassObject* Object, cUPnPResource* Resource){
	if (Resource == NULL){
		return -1;
	}
	pthread_mutex_lock(&(this->mDatabase->mutex_resource));
	if (this->mResUpdStmt == NULL){
		cString resStatement = cString::sprintf("UPDATE %s SET %s=@OI,%s=@PI,%s=@RS,%s=:SZ,%s=@DR,%s=:BR,%s=:SF,%s=:BS,%s=:AC,%s=:CD,%s=@RN,%s=@CT,%s=:RT,%s=:RR WHERE %s=:RI", 
							  SQLITE_TABLE_RESOURCES, SQLITE_COL_OBJECTID, SQLITE_COL_PROTOCOLINFO, SQLITE_COL_RESOURCE, SQLITE_COL_SIZE, SQLITE_COL_DURATION, 
							  SQLITE_COL_BITRATE,  SQLITE_COL_SAMPLEFREQUENCE, SQLITE_COL_BITSPERSAMPLE, SQLITE_COL_NOAUDIOCHANNELS, SQLITE_COL_COLORDEPTH, 
							  SQLITE_COL_RESOLUTION, SQLITE_COL_CONTENTTYPE, SQLITE_COL_RESOURCETYPE, SQLITE_COL_RECORDTIMER, SQLITE_COL_RESOURCEID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mResUpdStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a resource update: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	bool actionSuccess = true;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResUpdStmt, 1, *Object->getID(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResUpdStmt, 2, (*Resource->mProtocolInfo) ? *Resource->mProtocolInfo : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResUpdStmt, 3, (*Resource->mResource) ? *Resource->mResource : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int64 (this->mResUpdStmt, 4, Resource->mSize) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResUpdStmt, 5, (*Resource->mDuration) ? *Resource->mDuration : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResUpdStmt, 6, Resource->mBitrate) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResUpdStmt, 7, Resource->mSampleFrequency) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResUpdStmt, 8, Resource->mBitsPerSample) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResUpdStmt, 9, Resource->mNrAudioChannels) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResUpdStmt, 10, Resource->mColorDepth) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResUpdStmt, 11, (Resource->getResolution() == NULL ) ? "" : *Resource->mResolution, -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResUpdStmt, 12, (*Resource->mContentType) ? *Resource->mContentType : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResUpdStmt, 13, Resource->mResourceType) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResUpdStmt, 14, Resource->getRecordTimer()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResUpdStmt, 15, Resource->mResourceID) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("saveResource: Error with sqlite3_bind for resource update");
	}

	if (actionSuccess && sqlite3_step(this->mResUpdStmt) != SQLITE_DONE){
		ERROR("saveResource: sqlite3_step failed with update of resource data");
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mResUpdStmt);
	sqlite3_reset(this->mResUpdStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_resource));

	if (!actionSuccess){
		ERROR("saveResource failed");
		return -1;
	}
    return 0;
}

cUPnPResource* cUPnPResourceMediator::newResource(cUPnPClassObject* Object, int ResourceType, cString ResourceFile, 
	                                              cString ContentType, cString ProtocolInfo, bool isEpgResource){
    if (this->mDatabase == NULL){
		return NULL;
	}
	bool actionSuccess = true;
	bool resNumFound = false;
	unsigned int lastSetResourceId = 0;
	this->mDatabase->startTransaction();

	pthread_mutex_lock(&(this->mDatabase->mutex_primKey));
	actionSuccess = sqlite3_bind_int (this->mPKUpdStmt, 1, (isEpgResource) ? PK_RESOURCES_EPG_NR : PK_RESOURCES_NR) == SQLITE_OK;
	if (actionSuccess && sqlite3_step(this->mPKUpdStmt) != SQLITE_DONE){
		ERROR("newResource: sqlite3_step failed with update of res nr: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mPKUpdStmt);
	sqlite3_reset(this->mPKUpdStmt);

	actionSuccess = actionSuccess && sqlite3_bind_int (this->mPKSelStmt, 1, (isEpgResource) ? PK_RESOURCES_EPG_NR : PK_RESOURCES_NR) == SQLITE_OK;
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(this->mPKSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				lastSetResourceId = sqlite3_column_int(this->mPKSelStmt, 0);
				resNumFound = true;
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
		}
	}
	sqlite3_clear_bindings(this->mPKSelStmt);
	sqlite3_reset(this->mPKSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_primKey));

    cUPnPResource* Resource = new cUPnPResource;
	if (!resNumFound){
		ERROR("newResource: Got no resource id from table PrimaryKeys");
		actionSuccess = false;
	}
	pthread_mutex_lock(&(this->mDatabase->mutex_resource));
	if (this->mResInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT INTO %s (%s,%s,%s,%s,%s,%s,%s) VALUES (:ID,@OB,@RF,@PI,@CT,:RT,:TM)",
											 SQLITE_TABLE_RESOURCES, SQLITE_COL_RESOURCEID, SQLITE_COL_OBJECTID, SQLITE_COL_RESOURCE,
											 SQLITE_COL_PROTOCOLINFO, SQLITE_COL_CONTENTTYPE, SQLITE_COL_RESOURCETYPE, SQLITE_COL_RECORDTIMER);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mResInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a resource insertion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResInsStmt, 1, lastSetResourceId) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResInsStmt, 2, *Object->getID(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResInsStmt, 3, (*ResourceFile) ? *ResourceFile : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResInsStmt, 4, (*ProtocolInfo) ? *ProtocolInfo : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mResInsStmt, 5, (*ContentType) ? *ContentType : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResInsStmt, 6, ResourceType) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mResInsStmt, 7, Resource->getRecordTimer()) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("newResource: Error with sqlite3_bind for resource insertion; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}

	if (actionSuccess && sqlite3_step(this->mResInsStmt) != SQLITE_DONE){
		ERROR("newResource: sqlite3_step failed with insertion of data, lastSetResID: %d; object ID: %s; %s",lastSetResourceId,
			             *Object->getID(), sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mResInsStmt);
	sqlite3_reset(this->mResInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_resource));

	Resource->mResourceID = lastSetResourceId;
    Resource->mResource = ResourceFile;
    Resource->mProtocolInfo = ProtocolInfo;
    Resource->mContentType = ContentType;
    Resource->mResourceType = ResourceType;
    if(actionSuccess){
		this->mDatabase->commitTransaction();        
    }
    else {
       this->mDatabase->rollbackTransaction(); 
	   Resource = NULL;
	   MESSAGE(VERBOSE_METADATA, "The method cUPnPResourceMediator::newResource failed");
    }    
    return Resource;
}
