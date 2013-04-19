/*
 * File:   object.cpp
 * Author: savop
 * Author: J. Huber, IRT GmbH
 * Created on 11. September 2009, 20:39
 * Modified on April 9, 2013
 */

#include <upnp/upnptools.h>
#include <vdr/recording.h>
#include <vector>
//#include "database.h"
#include <vdr/tools.h>
#include "metadata.h"
#include "object.h"
#include "../common.h"
#include "resources.h"

/**********************************************\
*                                              *
*  Mediator factory                            *
*                                              *
\**********************************************/

cUPnPObjectFactory* cUPnPObjectFactory::mInstance = NULL;

cUPnPObjectFactory* cUPnPObjectFactory::getInstance(){
    if(!cUPnPObjectFactory::mInstance)
        cUPnPObjectFactory::mInstance = new cUPnPObjectFactory();

    if(cUPnPObjectFactory::mInstance) return cUPnPObjectFactory::mInstance;
    else return NULL;
}

cUPnPObjectFactory::cUPnPObjectFactory(){
    this->mDatabase = cSQLiteDatabase::getInstance();
}

void cUPnPObjectFactory::registerMediator(const char* UPnPClass, cMediatorInterface* Mediator){
    if (UPnPClass == NULL){
        ERROR("cUPnPObjectFactory::registerMediator: Class is undefined");
        return;
    }
    if (Mediator == NULL){
        ERROR("Mediator is undefined");
        return;
    }
    MESSAGE(VERBOSE_SDK, "cUPnPObjectFactory::registerMediator: Registering mediator for class '%s'", UPnPClass);
    this->mMediators[UPnPClass] = Mediator;
    MESSAGE(VERBOSE_SDK, "Now %d mediators registered", this->mMediators.size());
    return;
}

void cUPnPObjectFactory::unregisterMediator(const char* UPnPClass, bool freeMediator){
    if (UPnPClass == NULL){
        ERROR("Class is undefined");
        return;
    }
    tMediatorMap::iterator MediatorIterator = this->mMediators.find(UPnPClass);
    if (MediatorIterator==this->mMediators.end()){
        ERROR("No such mediator found for class '%s'", UPnPClass);
        return;
    }
    MESSAGE(VERBOSE_SDK, "Unregistering mediator for class '%s'", UPnPClass);
    this->mMediators.erase(MediatorIterator);
    if(freeMediator) delete MediatorIterator->second;
    MESSAGE(VERBOSE_SDK, "Remaining registered %d mediators: ", this->mMediators.size());
    return;
}

cMediatorInterface* cUPnPObjectFactory::findMediatorByID(cUPnPObjectID ID){
	pthread_mutex_lock(&(this->mDatabase->mutex_object));
	cString Class = NULL;
	sqlite3_stmt* clsSelStmt = this->mDatabase->getObjectClassSelectStatement();
	bool actionSuccess = sqlite3_bind_int (clsSelStmt, 1, (unsigned int) ID) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("findMediatorByID: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(clsSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				Class = strdup0((const char*) sqlite3_column_text(clsSelStmt, 0));
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("findMediatorByID: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(clsSelStmt);
	sqlite3_reset(clsSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_object));
	if ((const void *) Class == NULL){
		WARNING("No mediator for object with ID %d available after db query", (unsigned int) ID);
		return NULL;
	}
    return this->findMediatorByClass(Class);
}

cMediatorInterface* cUPnPObjectFactory::findMediatorByClass(const char* Class){
    if (!Class){
		ERROR("findMediatorByClass: No class specified"); 
		return NULL;
	}
    tMediatorMap::iterator MediatorIterator = this->mMediators.find(Class);
    if (MediatorIterator==this->mMediators.end()){
        ERROR("No matching mediator for class '%s'",Class);
        return NULL;
    }
    else {
        return MediatorIterator->second;
    }
}

cUPnPClassObject* cUPnPObjectFactory::getObject(cUPnPObjectID ID){
    cMediatorInterface* Mediator = this->findMediatorByID(ID);
    if (Mediator) {
		return Mediator->getObject(ID);
	}
    else {
		WARNING("findMediatorByID failed with ID %d", (unsigned int) ID);
        return NULL;
    }
}

cUPnPClassObject* cUPnPObjectFactory::createObject(const char* UPnPClass, const char* Title, bool Restricted){
    cMediatorInterface* Mediator = this->findMediatorByClass(UPnPClass);
	if (Mediator == NULL){
		ERROR("Got no mediator for class '%s'", UPnPClass);
		return NULL;
	}
    return Mediator->createObject(Title, Restricted);
}

int cUPnPObjectFactory::deleteObject(cUPnPClassObject* Object){
    cMediatorInterface* Mediator = this->findMediatorByClass(Object->getClass());
    return Mediator->deleteObject(Object);
}

int cUPnPObjectFactory::clearObject(cUPnPClassObject* Object){
    cMediatorInterface* Mediator = this->findMediatorByClass(Object->getClass());
    return Mediator->clearObject(Object);
}

int cUPnPObjectFactory::saveObject(cUPnPClassObject* Object){
    cMediatorInterface* Mediator = this->findMediatorByClass(Object->getClass());
	if (Mediator == NULL){
		 MESSAGE(VERBOSE_CUSTOM_OUTPUT, "Can not save the object, because there is no mediator available with %s", Object->getTitle());
		 return -1;
	}
    return Mediator->saveObject(Object);
}


/**********************************************\
 *                                              *
 *  Mediators                                   *
 *                                              *
 \**********************************************/

 /**********************************************\
 *                                              *
 *  Object mediator                             *
 *                                              *
 \**********************************************/

cUPnPObjectMediator::cUPnPObjectMediator(cMediaDatabase* MediaDatabase) : mMediaDatabase(MediaDatabase){
    this->mDatabase = cSQLiteDatabase::getInstance();
}

cUPnPObjectMediator::~cUPnPObjectMediator(){
    delete this->mDatabase;
    delete this->mMediaDatabase;
}

int cUPnPObjectMediator::saveObject(cUPnPClassObject* Object){
    bool successful = true;
    this->mDatabase->startTransaction();
    if (Object->getID() == -1){
		successful = false;
	}
    else if (this->objectToDatabase(Object)){
		successful = false;
	}

    if (successful){
        this->mDatabase->commitTransaction();
        Object->setModified();
        this->mMediaDatabase->cacheObject(Object);
        this->mMediaDatabase->updateSystemID();
//		MESSAGE(VERBOSE_METADATA, "The object with ID %s was cached.", *(Object->getID())); 
        return 0;
    }

    this->mDatabase->rollbackTransaction();
    return -1;
}

int cUPnPObjectMediator::deleteObject(cUPnPClassObject* Object){
	bool actionSuccess = deleteObjectProc ((int)Object->getID());

//	MESSAGE(VERBOSE_METADATA, "Delete the cached resources with object ID: %s", *(Object->getID())); 
	cUPnPResources::getInstance()->deleteCachedResources(Object);  // this is done with: delete Object

    #ifdef SQLITE_CASCADE_DELETES
    this->clearObject(Object);
    #endif
    delete Object; 
	Object = NULL;
	return (actionSuccess) ? 0 : -1;
}

int cUPnPObjectMediator::clearObject(cUPnPClassObject* Object){
    cUPnPClassContainer* Container = Object->getContainer();
    if (Container){
        cList<cUPnPClassObject>* List = Container->getObjectList();
        for(cUPnPClassObject* Child = List->First(); Child; Child = List->Next(Child)){
            if(this->deleteObject(Child)) return -1;
        }
    }
    return 0;
}

int cUPnPObjectMediator::initializeObject(cUPnPClassObject* Object, const char* Class, const char* Title, bool Restricted){
    int objId = this->mMediaDatabase->getNextObjectID();
	if (objId == 0){
		WARNING("The method getNextObjectID failed with the UPnP class %s. Instead a random ID is used.", Class);
		time_t actTime;
	    time (&actTime);
		objId = 0x01000000 + (0x00ffffff & ((unsigned long) actTime));
	}
	cUPnPObjectID *objectID = new cUPnPObjectID (objId);
    if (Object->setID(*objectID)){
        ERROR("Error while setting ID");
        return -1;
    }
	delete objectID;

    cUPnPClassObject* Root = this->mMediaDatabase->getObjectByID(ROOT_ID);
    if (Root){
        Root->getContainer()->addObject(Object);
    }
    else {
		MESSAGE(VERBOSE_MODIFICATIONS, "Got no root object for object with ID %i", objId);
        Object->setParent(NULL);
    }
    if (Object->setClass(Class)){
        ERROR("Error while setting class");
        return -1;
    }
    if (Object->setTitle(Title)){
        ERROR("Error while setting title");
        return -1;
    }
    if (Object->setRestricted(Restricted)){
        ERROR("Error while setting restriction");
        return -1;
    }

	pthread_mutex_lock(&(this->mDatabase->mutex_object));
	sqlite3_stmt* objInsStmt = this->mDatabase->getObjectInsertStatement();
	bool actionSuccess = sqlite3_bind_int (objInsStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (objInsStmt, 2, ((unsigned int) Object->getParentID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(objInsStmt, 3, (Object->getClass()) ? Object->getClass() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(objInsStmt, 4, (Object->getTitle()) ? Object->getTitle() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (objInsStmt, 5, Object->isRestricted() ? 1 : 0) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("newResource: Error with sqlite3_bind for object insertion; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}

	if (actionSuccess && sqlite3_step(objInsStmt) != SQLITE_DONE){
		ERROR("newResource: sqlite3_step failed with insertion of object data for ID: %i; %s", (int) Object->getID(), 
			sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(objInsStmt);
	sqlite3_reset(objInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_object));
	return (actionSuccess) ? 0 : -1;
}

cUPnPClassObject* cUPnPObjectMediator::getObject(cUPnPObjectID){ 
	WARNING("Getting instance of class 'Object' forbidden"); 
	return NULL; 
}

cUPnPClassObject* cUPnPObjectMediator::createObject(const char*, bool){
	WARNING("Getting instance of class 'Object' forbidden"); 
	return NULL; 
}

int cUPnPObjectMediator::objectToDatabase(cUPnPClassObject* Object){
//    MESSAGE(VERBOSE_MODIFICATIONS, "Updating object #%s", *Object->getID());
	pthread_mutex_lock(&(this->mDatabase->mutex_object));
	sqlite3_stmt* objUpdStmt = this->mDatabase->getObjectUpdateStatement();
	bool actionSuccess = sqlite3_bind_int(objUpdStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (objUpdStmt, 2, ((unsigned int) Object->getParentID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(objUpdStmt, 3, (Object->getClass()) ? Object->getClass() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(objUpdStmt, 4, (Object->getTitle()) ? Object->getTitle() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (objUpdStmt, 5, (Object->isRestricted()) ? 1 : 0) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(objUpdStmt, 6, (Object->getCreator()) ? Object->getCreator() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (objUpdStmt, 7, Object->getWriteStatus()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (objUpdStmt, 8, ((unsigned int) Object->mLastID)) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPObjectMediator::objectToDatabase: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(objUpdStmt) != SQLITE_DONE){
		ERROR("cUPnPObjectMediator::objectToDatabase: sqlite3_step failed with an object update; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(objUpdStmt);
	sqlite3_reset(objUpdStmt);
    pthread_mutex_unlock(&(this->mDatabase->mutex_object));

	if (!actionSuccess){
		cString Format = "UPDATE %s SET %s WHERE %s='%s'";
		cString Set=NULL;
		char *Value=NULL;
		cString Properties[] = {
			SQLITE_COL_OBJECTID,
			SQLITE_COL_PARENTID,
			SQLITE_COL_CLASS,
			SQLITE_COL_TITLE,
			SQLITE_COL_RESTRICTED,
			SQLITE_COL_CREATOR,
			SQLITE_COL_WRITESTATUS,
			NULL
		};
		for (cString* Property = Properties; *(*Property)!=NULL; Property++){
			if (!Object->getProperty(*Property, &Value)){
				ERROR("No such property '%s' in object with ID '%s'",*(*Property),*Object->getID());
				return -1;
			}
			Set = cSQLiteDatabase::sprintf("%s%s%s=%Q", *Set?*Set:"", *Set?",":"", *(*Property), Value);
		}
		if(this->mDatabase->execStatement(Format, SQLITE_TABLE_OBJECTS, *Set, SQLITE_COL_OBJECTID, *Object->mLastID)){
			ERROR("Error while executing statement cUPnPObjectMediator::objectToDatabase");
			return -1;
		}
	}

    // The update was successful --> the current ID is now also the LastID
    Object->mLastID = Object->mID;
    return 0;
}

int cUPnPObjectMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){
	pthread_mutex_lock(&(this->mDatabase->mutex_object));
	sqlite3_stmt* objSelStmt = this->mDatabase->getObjectSelectStatement();
	bool actionSuccess = sqlite3_bind_int (objSelStmt, 1, ((unsigned int) ID)) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPObjectMediator::databaseToObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}

	int ctr = 0;
	bool objFound = false;
	bool methError = false;
	int parentId = -1;
	const char *upnpClass = NULL;
	const char *title = NULL;
	const char *creator = NULL;

	if (Object->setID(ID)){
		ERROR("cUPnPObjectMediator::databaseToObject: Error while setting object ID");
		methError = true;
	}

	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(objSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				if (Object->setID(ID)){
					ERROR("cUPnPObjectMediator::databaseToObject: Error while setting object ID");
					methError = true;
				}
				parentId = sqlite3_column_int(objSelStmt, 0);
				upnpClass = (const char *) sqlite3_column_text(objSelStmt, 1);
				if (!methError && (upnpClass == NULL || Object->setClass(upnpClass))){
					ERROR("cUPnPObjectMediator::databaseToObject: Error while setting class: %s", upnpClass);
					methError = true;
				}
				title = (const char *) sqlite3_column_text(objSelStmt, 2);
				if (!methError && (title == NULL || Object->setTitle(title))){
					ERROR("cUPnPObjectMediator::databaseToObject: Error while setting title: %s", title);
					methError = true;
				}
				Object->setRestricted(sqlite3_column_int(objSelStmt, 3) == 1 ? true : false);
				creator = (const char *) sqlite3_column_text(objSelStmt, 4);
				if (!methError && (Object->setCreator(creator ? creator : ""))){
					ERROR("cUPnPObjectMediator::databaseToObject: Error while setting creator");
					methError = true;
				}
				if(Object->setWriteStatus(sqlite3_column_int(objSelStmt, 5))){
					ERROR("cUPnPObjectMediator::databaseToObject: Error while setting write status");
					methError = true;
				}
				objFound = true;
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("cUPnPObjectMediator::databaseToObject: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}

	sqlite3_clear_bindings(objSelStmt);
	sqlite3_reset(objSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_object));
	if (!objFound){
		ERROR("cUPnPObjectMediator::databaseToObject: No object in database with ID %s", *ID);
		return -1;
	}
    if (methError){
		ERROR("cUPnPObjectMediator::databaseToObject: Error with database select statement, object ID %s", *ID);
		return -1;
	}

    this->mMediaDatabase->cacheObject(Object);
	if (parentId >= 0){
		cUPnPObjectID RefID = parentId;
		cUPnPClassContainer* ParentObject;
		if (RefID == -1){
			ParentObject = NULL;
		}
		else {
			ParentObject = (cUPnPClassContainer*)this->mMediaDatabase->getObjectByID(RefID);
			if (!ParentObject){
				ERROR("cUPnPObjectMediator::databaseToObject: No such parent with ID '%s' found.", *RefID);
			}
		}
//		MESSAGE(VERBOSE_SQL, "databaseToObject set parent for ID %s", *ID);
		if (Object->setParent(ParentObject) < 0){  // the parent object of a root container is set to NULL
			ERROR("cUPnPObjectMediator, databaseToObject: setParent Faile for ID %i", (int) ID);
		}
	}
	else if (((int)ID) != 0){
		ERROR("cUPnPObjectMediator::databaseToObject: Invalid parent ID for ID %s", *ID);
	}

    cUPnPResources::getInstance()->getResourcesOfObject(Object);
    return 0;
}

bool cUPnPObjectMediator::deleteObjectProc(int objId){
	pthread_mutex_lock(&(this->mDatabase->mutex_object));
	sqlite3_stmt* objDelStmt = this->mDatabase->getObjectDeleteStatement();
	bool actionSuccess = sqlite3_bind_int (objDelStmt, 1, objId) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("deleteObjectProc: sqlite3_bind error %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(objDelStmt) != SQLITE_DONE){
		ERROR("deleteObjectProc: sqlite3_step failed with deletion in table object: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(objDelStmt);
	sqlite3_reset(objDelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_object));
	return actionSuccess;
}


 /**********************************************\
 *                                              *
 *  Item mediator                               *
 *                                              *
 \**********************************************/

cUPnPItemMediator::cUPnPItemMediator(cMediaDatabase* MediaDatabase) : cUPnPObjectMediator(MediaDatabase){
}

int cUPnPItemMediator::objectToDatabase(cUPnPClassObject* Object){
    if (cUPnPObjectMediator::objectToDatabase(Object)){
		ERROR("cUPnPItemMediator: Could not store the object in the database with the ObjectMediator.");
		return -1;
	}

	pthread_mutex_lock(&(this->mDatabase->mutex_item));
	sqlite3_stmt* itmInsStmt = this->mDatabase->getItemInsertStatement();
	bool actionSuccess = sqlite3_bind_int (itmInsStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (itmInsStmt, 2, ((unsigned int) ((cUPnPClassItem *) Object)->getReferenceID())) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPItemMediator: Could not bind the arguments to the precompiled insert statement; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(itmInsStmt) != SQLITE_DONE){
		ERROR("cUPnPItemMediator: Could not insert/replace ID %i to the Items table: %s", (int) Object->getID(),
			sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(itmInsStmt);
	sqlite3_reset(itmInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_item));
    return (actionSuccess) ? 0 : -1;
}

int cUPnPItemMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){
    if (cUPnPObjectMediator::databaseToObject(Object, ID)){
        ERROR("cUPnPItemMediator: Error while loading object");
        return -1;
    }

	sqlite3_stmt* itmSelStmt = this->mDatabase->getItemSelectStatement();
    cUPnPClassItem* Item = (cUPnPClassItem*) Object;
	bool actionSuccess = sqlite3_bind_int (itmSelStmt, 1, ((unsigned int) ID)) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPItemMediator::databaseToObject: Could not bind the ID to the precompiled statement; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	int ctr = 0;
	int referenceId = -1;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(itmSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				referenceId = sqlite3_column_int(itmSelStmt, 0);				
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
		}
	}
	sqlite3_clear_bindings(itmSelStmt);
	sqlite3_reset(itmSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_item));

	cUPnPObjectID RefID = referenceId;	
	cUPnPClassItem* RefObject = NULL;
	if (RefID != -1){
		RefObject = (cUPnPClassItem*)this->mMediaDatabase->getObjectByID(RefID);
		if (!RefObject){
			ERROR("cUPnPItemMediator::databaseToObject: No such reference item with ID '%s' found.",*RefID);
		}
	}
	Item->setReference(RefObject);
	return (actionSuccess) ? 0 : -1;
}

cUPnPClassItem* cUPnPItemMediator::getObject(cUPnPObjectID ID){
    cUPnPClassItem* Object = new cUPnPClassItem;
    if (this->databaseToObject(Object, ID)){
		return NULL;
	}
    return Object;
}

cUPnPClassItem* cUPnPItemMediator::createObject(const char* Title, bool Restricted){
//    MESSAGE(VERBOSE_MODIFICATIONS, "Creating Item '%s'",Title);
    cUPnPClassItem* Object = new cUPnPClassItem;
    if(this->initializeObject(Object, UPNP_CLASS_ITEM, Title, Restricted)) return NULL;
    return Object;
}

int cUPnPItemMediator::deleteObject(cUPnPClassObject* Object){
	pthread_mutex_lock(&(this->mDatabase->mutex_item));
	sqlite3_stmt* itmDelStmt = this->mDatabase->getItemDeleteStatement();
	bool actionSuccess = sqlite3_bind_int (itmDelStmt, 1, (int) Object->getID()) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPItemMediator::deleteObject: sqlite3_bind error %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(itmDelStmt) != SQLITE_DONE){
		ERROR("sqlite3_step failed with deletion in table items: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(itmDelStmt);
	sqlite3_reset(itmDelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_item));
	return (actionSuccess) ? 0 : -1;
}

/**********************************************\
*                                              *
*  Epg item mediator                           *
*                                              *
\**********************************************/

cUPnPEpgItemMediator::cUPnPEpgItemMediator(cMediaDatabase* MediaDatabase) : cUPnPItemMediator(MediaDatabase){
	this->mBceSelStmt = NULL;
	this->mBceDelStmt = NULL;
	this->mBceInsStmt = NULL;
}

cUPnPClassEpgItem* cUPnPEpgItemMediator::createObject(const char* Title, bool Restricted){
//    MESSAGE(VERBOSE_MODIFICATIONS, "Creating an Epg item '%s'", Title);
    cUPnPClassEpgItem* Object = new cUPnPClassEpgItem;
    if (this->initializeObject(Object, UPNP_CLASS_EPGITEM, Title, Restricted)){
		return NULL;
	}
    return Object;
}

cUPnPClassEpgItem* cUPnPEpgItemMediator::getObject(cUPnPObjectID ID){
    cUPnPClassEpgItem* Object = new cUPnPClassEpgItem;
    if (this->databaseToObject(Object, ID)){
		return NULL;
	}
    return Object;
}

int cUPnPEpgItemMediator::objectToDatabase(cUPnPClassObject* Object){
    if (cUPnPItemMediator::objectToDatabase(Object)){
		return -1;
	}
	pthread_mutex_lock(&(this->mDatabase->mutex_bcEvent));
	if (this->mBceInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s,%s,%s,%s,%s,%s,%s,%s) VALUES (:ID,@SS,@SH,@GR,@VN,:TI,:DU,:ST,:EI)",
								SQLITE_TABLE_BCEVENTS, SQLITE_COL_OBJECTID, SQLITE_COL_BCEV_SYNOPSIS, SQLITE_COL_BCEV_SHORTTITLE, SQLITE_COL_BCEV_GENRES,
								SQLITE_COL_BCEV_VERSION, SQLITE_COL_BCEV_TABLEID, SQLITE_COL_BCEV_DURATION, SQLITE_COL_BCEV_STARTTIME, SQLITE_COL_BCEV_ID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mBceInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for BC event insertion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
    cUPnPClassEpgItem* epgItem = (cUPnPClassEpgItem*)Object;

	bool actionSuccess = sqlite3_bind_int (this->mBceInsStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mBceInsStmt, 2, (epgItem->getSynopsis()) ? epgItem->getSynopsis() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mBceInsStmt, 3, (epgItem->getShortTitle()) ? epgItem->getShortTitle() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mBceInsStmt, 4, (epgItem->getGenres()) ? epgItem->getGenres() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mBceInsStmt, 5, (epgItem->getVersion()) ? epgItem->getVersion() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mBceInsStmt, 6, epgItem->getTableId()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mBceInsStmt, 7, epgItem->getDuration()) == SQLITE_OK;
	unsigned int stTime = (epgItem->getStartTime()) ? (unsigned int) atol(epgItem->getStartTime()) : 0;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mBceInsStmt, 8, stTime) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mBceInsStmt, 9, epgItem->getEventId()) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPEpgItemMediator::objectToDatabase: Error with sqlite3_bind for BC event insertion");
	}
	if (actionSuccess && sqlite3_step(this->mBceInsStmt) != SQLITE_DONE){
		ERROR("cUPnPEpgItemMediator::objectToDatabase: sqlite3_step failed with insertion of data: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	sqlite3_clear_bindings(this->mBceInsStmt);
	sqlite3_reset(this->mBceInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_bcEvent));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPEpgItemMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){	
    if (cUPnPItemMediator::databaseToObject(Object,ID)){
        ERROR("cUPnPEpgItemMediator: Error while loading object");
        return -1;
    }
	pthread_mutex_lock(&(this->mDatabase->mutex_bcEvent));
	if (this->mBceSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s,%s,%s,%s FROM %s WHERE %s=:ID", SQLITE_COL_BCEV_DURATION, SQLITE_COL_BCEV_STARTTIME,
			             SQLITE_COL_BCEV_ID, SQLITE_COL_BCEV_SHORTTITLE, SQLITE_COL_BCEV_SYNOPSIS, SQLITE_COL_BCEV_GENRES, 
						 SQLITE_TABLE_BCEVENTS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mBceSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a broadcast event selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	cUPnPClassEpgItem* epgItem = (cUPnPClassEpgItem*) Object;

	bool actionSuccess = sqlite3_bind_int (this->mBceSelStmt, 1, ((unsigned int) ID)) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPEpgItemMediator::databaseToObject: Cannot bind the ID; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(this->mBceSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				if (epgItem->setDuration(sqlite3_column_int(this->mBceSelStmt, 0))){
					ERROR("cUPnPEpgItemMediator: Error while setting the duration");
				}
				if(epgItem->setStartTime(itoa(sqlite3_column_int(this->mBceSelStmt, 1)))){
					ERROR("Error while setting the start time");
				}
				if(epgItem->setEventId(sqlite3_column_int(this->mBceSelStmt, 2))){
					ERROR("Error while setting the event ID");
				}
				if(epgItem->setShortTitle((const char*)sqlite3_column_text(this->mBceSelStmt, 3))){
					ERROR("Error while setting the short title");
				}
				if(epgItem->setSynopsis((const char*)sqlite3_column_text(this->mBceSelStmt, 4))){
					ERROR("Error while setting the synopsis");
				}
				if(epgItem->setGenres((const char*)sqlite3_column_text(this->mBceSelStmt, 5))){
					ERROR("Error while setting the genres");
				}
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
		}
	}
	sqlite3_clear_bindings(this->mBceSelStmt);
	sqlite3_reset(this->mBceSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_bcEvent));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPEpgItemMediator::deleteObject(cUPnPClassObject* Object){
//	MESSAGE(VERBOSE_METADATA, "Method cUPnPEpgItemMediator::deleteObject with ID %s", *Object->getID());
   if (this->mBceDelStmt == NULL){
		cString resStatement = cString::sprintf("DELETE FROM %s WHERE %s=:ID", SQLITE_TABLE_BCEVENTS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mBceDelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a broadcast event deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	int ret = 0;
	int objId = (int) Object->getID();
	this->mDatabase->startTransaction();
	bool actionSuccess = deleteObjectProc(objId);

	pthread_mutex_lock(&(this->mDatabase->mutex_bcEvent));
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mBceDelStmt, 1, objId) == SQLITE_OK;
	if (actionSuccess && sqlite3_step(this->mBceDelStmt) != SQLITE_DONE){
		ERROR("cUPnPEpgItemMediator::deleteObject: sqlite3_step failed with deletion in table BCEvents: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mBceDelStmt);
	sqlite3_reset(this->mBceDelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_bcEvent));
	if (!actionSuccess){
		this->mDatabase->rollbackTransaction();
		ret = -1;
	}
	else {
		this->mDatabase->commitTransaction();
	}
    return ret;
}

/**********************************************\
*                                              *
*  Record timer item mediator                  *
*                                              *
\**********************************************/
cUPnPRecordTimerItemMediator::cUPnPRecordTimerItemMediator(cMediaDatabase* MediaDatabase) : cUPnPItemMediator(MediaDatabase){
	this->mRtrInsStmt = NULL;
	this->mRtrSelStmt = NULL;
	this->mRtrDelStmt = NULL;
}

int cUPnPRecordTimerItemMediator::objectToDatabase(cUPnPClassObject* Object){
    if (cUPnPItemMediator::objectToDatabase(Object)){
		return -1;
	}
	pthread_mutex_lock(&(this->mDatabase->mutex_recTimer));
	if (this->mRtrInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s) VALUES (:ID,:SS,@CI,@DY,:ST,:SP,:PR,:LT,@FL,@AX,:IR)",
								SQLITE_TABLE_RECORDTIMERS, SQLITE_COL_OBJECTID, SQLITE_COL_STATUS, SQLITE_COL_CHANNELID, SQLITE_COL_DAY,
								SQLITE_COL_START, SQLITE_COL_STOP, SQLITE_COL_PRIORITY, SQLITE_COL_LIVETIME, SQLITE_COL_FILE, 
								SQLITE_COL_AUX, SQLITE_COL_ISRADIOCHANNEL);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mRtrInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a record timer insertion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
    cUPnPClassRecordTimerItem* rtItem = (cUPnPClassRecordTimerItem*)Object;
	
	bool actionSuccess = sqlite3_bind_int (this->mRtrInsStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int(this->mRtrInsStmt, 2, rtItem->getStatus()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mRtrInsStmt, 3, (rtItem->getChannelId()) ? rtItem->getChannelId() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mRtrInsStmt, 4, (rtItem->getDay()) ? rtItem->getDay() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mRtrInsStmt, 5, rtItem->getStart()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mRtrInsStmt, 6, rtItem->getStop()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mRtrInsStmt, 7, rtItem->getPriority()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mRtrInsStmt, 8, rtItem->getLifetime()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text (this->mRtrInsStmt, 9, (rtItem->getFile()) ? rtItem->getFile() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text (this->mRtrInsStmt, 10, (rtItem->getAux()) ? rtItem->getAux() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mRtrInsStmt, 11, (rtItem->isRadioChannel()) ? 1 : 0) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPRecordTimerItemMediator::objectToDatabase: Error with sqlite3_bind for record timer insertion");
	}
	if (actionSuccess && sqlite3_step(this->mRtrInsStmt) != SQLITE_DONE){
		ERROR("cUPnPRecordTimerItemMediator::objectToDatabase: sqlite3_step failed with insertion of data: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	sqlite3_clear_bindings(this->mRtrInsStmt);
	sqlite3_reset(this->mRtrInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_recTimer));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPRecordTimerItemMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){ 
	if (cUPnPItemMediator::databaseToObject(Object, ID)){
        ERROR("cUPnPRecordTimerItemMediator: Error while loading object");
        return -1;
    }
	pthread_mutex_lock(&(this->mDatabase->mutex_recTimer));
	if (this->mRtrSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s,%s,%s,%s,%s,%s,%s,%s FROM %s WHERE %s=:ID", SQLITE_COL_STATUS, SQLITE_COL_CHANNELID, 
			             SQLITE_COL_DAY, SQLITE_COL_START, SQLITE_COL_STOP, SQLITE_COL_PRIORITY, SQLITE_COL_LIVETIME, SQLITE_COL_FILE, 
						 SQLITE_COL_AUX, SQLITE_COL_ISRADIOCHANNEL, SQLITE_TABLE_RECORDTIMERS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mRtrSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a record timer selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	cUPnPClassRecordTimerItem* rtItem = (cUPnPClassRecordTimerItem*) Object;
	
	bool actionSuccess = sqlite3_bind_int (this->mRtrSelStmt, 1, ((unsigned int) ID)) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPRecordTimerItemMediator::databaseToObject: Cannot bind the ID; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(this->mRtrSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				if (rtItem->setStatus(sqlite3_column_int(this->mRtrSelStmt, 0))){
					ERROR("cUPnPRecordTimerItemMediator: Error while setting the status");
				}
				if(rtItem->setChannelId((const char*)sqlite3_column_text(this->mRtrSelStmt, 1))){
					ERROR("Error while setting the channel ID");
				}
				if(rtItem->setDay((const char*)sqlite3_column_text(this->mRtrSelStmt, 2))){
					ERROR("Error while setting the day");
				}
				if(rtItem->setStart(sqlite3_column_int(this->mRtrSelStmt, 3))){
					ERROR("Error while setting the start time");
				}
				if(rtItem->setStop(sqlite3_column_int(this->mRtrSelStmt, 4))){
					ERROR("Error while setting the stop time");
				}
				if (rtItem->setPriority(sqlite3_column_int(this->mRtrSelStmt, 5))){
					ERROR("cUPnPRecordTimerItemMediator: Error while setting the priority");
				}
				if (rtItem->setLifetime(sqlite3_column_int(this->mRtrSelStmt, 6))){
					ERROR("cUPnPRecordTimerItemMediator: Error while setting the live time");
				}
				if(rtItem->setFile((const char*)sqlite3_column_text(this->mRtrSelStmt, 7))){
					ERROR("Error while setting the file name");
				}		
				if(rtItem->setAux((const char*)sqlite3_column_text(this->mRtrSelStmt, 8))){
					ERROR("Error while setting the additional information");
				}
				if (rtItem->setRadioChannel((sqlite3_column_int(this->mRtrSelStmt, 9)) ? true : false)){
					ERROR("cUPnPRecordTimerItemMediator: Error while setting the is radio channel flag");
				}
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
		}
	}
	sqlite3_clear_bindings(this->mRtrSelStmt);
	sqlite3_reset(this->mRtrSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_recTimer));
	return (actionSuccess) ? 0 : -1;
}

cUPnPClassRecordTimerItem* cUPnPRecordTimerItemMediator::createObject(const char* Title, bool Restricted){
    cUPnPClassRecordTimerItem* Object = new cUPnPClassRecordTimerItem;
    if (this->initializeObject(Object, UPNP_CLASS_BOOKMARKITEM, Title, Restricted)){
		ERROR("cUPnPRecordTimerItemMediator::createObject: initializeObject failed");
		return NULL;
	}
    return Object;
}

cUPnPClassRecordTimerItem* cUPnPRecordTimerItemMediator::getObject(cUPnPObjectID ID){
    cUPnPClassRecordTimerItem* Object = new cUPnPClassRecordTimerItem;
    if (this->databaseToObject(Object, ID)){
		return NULL;
	}
    return Object;
}

int cUPnPRecordTimerItemMediator::deleteObject(cUPnPClassObject* Object){
	MESSAGE(VERBOSE_METADATA, "cUPnPRecordTimerItemMediator::deleteObject %i", (int) Object->getID());
	sqlite3_stmt* objDelStmt = this->mDatabase->getObjectDeleteStatement();

	if (this->mRtrDelStmt == NULL){
		cString resStatement = cString::sprintf("DELETE FROM %s WHERE %s=:ID", SQLITE_TABLE_RECORDTIMERS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mRtrDelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a record timer deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	int ret = 0;
	int objId = (int) Object->getID();
	this->mDatabase->startTransaction();
	pthread_mutex_lock(&(this->mDatabase->mutex_object));
	bool actionSuccess = sqlite3_bind_int (objDelStmt, 1, objId) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPRecordTimerItemMediator::deleteObject: sqlite3_bind error %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(objDelStmt) != SQLITE_DONE){
		ERROR("cUPnPRecordTimerItemMediator::deleteObject: sqlite3_step failed with deletion in table object: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(objDelStmt);
	sqlite3_reset(objDelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_object));

	pthread_mutex_lock(&(this->mDatabase->mutex_recTimer));
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mRtrDelStmt, 1, objId) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPRecordTimerItemMediator::deleteObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(this->mRtrDelStmt) != SQLITE_DONE){
		ERROR("cUPnPRecordTimerItemMediator::deleteObject: sqlite3_step failed with deletion in table RecordTimers: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mRtrDelStmt);
	sqlite3_reset(this->mRtrDelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_recTimer));
	 
	if (!actionSuccess){
		this->mDatabase->rollbackTransaction();
		ret = -1;
	}
	else {
		this->mDatabase->commitTransaction();
	}
    return ret;
}

int cUPnPRecordTimerItemMediator::saveObject(cUPnPClassObject* Object){
	if (cUPnPObjectMediator::saveObject(Object)){
		ERROR("cUPnPRecordTimerItemMediator::saveObject failed");
		return -1;
	}
	return objectToDatabase (Object);
}


 /**********************************************\
 *                                              *
 *  Container mediator                          *
 *                                              *
 \**********************************************/

cUPnPContainerMediator::cUPnPContainerMediator(cMediaDatabase* MediaDatabase) : cUPnPObjectMediator(MediaDatabase){
}

int cUPnPContainerMediator::objectToDatabase(cUPnPClassObject* Object){
    if (cUPnPObjectMediator::objectToDatabase(Object)){
		return -1;
	}
    cUPnPClassContainer* Container = (cUPnPClassContainer*)Object;
    pthread_mutex_lock(&(this->mDatabase->mutex_container));
    sqlite3_stmt* conInsStmt = this->mDatabase->getContainerInsertStatement();
	bool actionSuccess = sqlite3_bind_int (conInsStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(conInsStmt, 2, (Container->getContainerType()) ? Container->getContainerType() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (conInsStmt, 3, (Container->isSearchable()) ? 1 : 0) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (conInsStmt, 4, (int) Container->getUpdateID()) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPContainerMediator::objectToDatabase: Error with sqlite3_bind for container insertion");
	}
	if (actionSuccess && sqlite3_step(conInsStmt) != SQLITE_DONE){
		ERROR("cUPnPContainerMediator::objectToDatabase: sqlite3_step failed with insertion of data; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(conInsStmt);
	sqlite3_reset(conInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_container));
	if (!actionSuccess){
		return -1;
	}

	pthread_mutex_lock(&(this->mDatabase->mutex_searchClass));
	sqlite3_stmt* sClInsStmt = this->mDatabase->getSearchClassInsertStatement();
    for (unsigned int i=0; i<Container->getSearchClasses()->size(); i++){
        cClass Class = Container->getSearchClasses()->at(i);
		bool actionSuccess = sqlite3_bind_int (sClInsStmt, 1, ((unsigned int) Container->getID())) == SQLITE_OK;
		actionSuccess = actionSuccess && sqlite3_bind_text(sClInsStmt, 2, (*Class.ID) ? *Class.ID : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
		actionSuccess = actionSuccess && sqlite3_bind_int (sClInsStmt, 3, Class.includeDerived ? 1 : 0) == SQLITE_OK;
		if (!actionSuccess){
			ERROR("cUPnPContainerMediator::objectToDatabase: Error with sqlite3_bind for search class insertion");
		}
		if (actionSuccess && sqlite3_step(sClInsStmt) != SQLITE_DONE){
			ERROR("objectToDatabase: sqlite3_step failed with insertion into search class; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
			actionSuccess = false;
		}
		sqlite3_clear_bindings(sClInsStmt);
		sqlite3_reset(sClInsStmt);

        //Columns = cSQLiteDatabase::sprintf("%s,%s,%s", SQLITE_COL_OBJECTID, SQLITE_COL_CLASS, SQLITE_COL_CLASSDERIVED);
        //Values = cSQLiteDatabase::sprintf("%Q,%Q,%d", *Container->getID(), *Class.ID, Class.includeDerived?1:0);
        //if(this->mDatabase->execStatement(Format, SQLITE_TABLE_SEARCHCLASS, *Columns, *Values)){
        //    ERROR("cUPnPContainerMediator::objectToDatabase: Error while executing a searchclass statement");
        //    return -1;
        //}
    }
	pthread_mutex_unlock(&(this->mDatabase->mutex_searchClass));
    // Create classes not necessary at the moment
    return 0;
}

int cUPnPContainerMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){
    if (cUPnPObjectMediator::databaseToObject(Object, ID)){
        ERROR("cUPnPContainerMediator::databaseToObject: Error while loading object");
        return -1;
    }
    cUPnPClassContainer* Container = (cUPnPClassContainer*)Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_container));
    sqlite3_stmt* conSelStmt = this->mDatabase->getContainerSelectStatement();
	bool actionSuccess = sqlite3_bind_int (conSelStmt, 1, ((unsigned int) ID)) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPContainerMediator::databaseToObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	bool objFound = false;
	bool methodError = false;
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(conSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				if(Container->setContainerType((const char*) sqlite3_column_text(conSelStmt, 0))){
					ERROR("Error while setting container type");
					methodError = true;
				}
				if(Container->setUpdateID((unsigned int) sqlite3_column_int(conSelStmt, 1))){
					ERROR("Error while setting update ID");
					methodError = true;
				}
				if(Container->setSearchable(sqlite3_column_int(conSelStmt, 2) == 1 ? true : false)){
					ERROR("Error while setting searchable");
					methodError = true;
				}
				objFound = true;
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("cUPnPContainerMediator::databaseToObject: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(conSelStmt);
	sqlite3_reset(conSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_container));
	if (methodError || !actionSuccess){
		return -1;
	}
	if (!objFound){
		return 0;
	}

    pthread_mutex_lock(&(this->mDatabase->mutex_container));
    sqlite3_stmt* objParSelStmt = this->mDatabase->getObjectParentSelectStatement();
	actionSuccess = sqlite3_bind_int (objParSelStmt, 1, ((unsigned int) ID)) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPContainerMediator::databaseToObject: sqlite3 bind error with parent ID for ID %d: %s", (unsigned int) ID, 
			sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}

	ctr = 0;
	std::vector<int> objVector;
	while (actionSuccess && ctr++ < MAX_ROW_COUNT){
		int stepRes = sqlite3_step(objParSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				objVector.push_back(sqlite3_column_int(objParSelStmt, 0));
				break;
			case SQLITE_DONE:
				ctr = MAX_ROW_COUNT; // stop loop
				break;
			default:
				ERROR("databaseToObject, search with parent ID: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(objParSelStmt);
	sqlite3_reset(objParSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_container));

//	MESSAGE(VERBOSE_OBJECTS, "Got %d objects with parent ID %d", (int)objVector.size(), ((unsigned int) ID));
    for (int i = 0; i < (int)objVector.size(); i++){
		cUPnPClassObject *dbObj = this->mMediaDatabase->getObjectByID(objVector.at(i));
		if (dbObj){
			Container->addObject(dbObj);
		}
		else {
			ERROR("The object with ID %i does not exist", objVector.at(i));
		}
	}
	objVector.clear();
//	MESSAGE(VERBOSE_OBJECTS, "The container has now %i objects; ID %d", Container->countObjects(), ((unsigned int) ID));

	pthread_mutex_lock(&(this->mDatabase->mutex_searchClass));
    sqlite3_stmt* sclSelStmt = this->mDatabase->getSearchClassSelectStatement();
	actionSuccess = sqlite3_bind_int (sclSelStmt, 1, ((unsigned int) ID)) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPContainerMediator::databaseToObject: sqlite3 bind error with search class ID: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}

	ctr = 0;
    std::vector<cClass> SearchClasses;
	while (actionSuccess && ctr++ < MAX_ROW_COUNT){
		int stepRes = sqlite3_step(sclSelStmt);
		if (stepRes == SQLITE_ROW){					
			cClass Class;
			Class.ID = strdup0((const char*) sqlite3_column_text(sclSelStmt, 0));
			Class.includeDerived = (sqlite3_column_int(sclSelStmt, 1) == 1) ? true : false;
			SearchClasses.push_back(Class);
		}
		else if (stepRes == SQLITE_DONE){
			ctr = MAX_ROW_COUNT; // stop loop
		}
		else {
			ERROR("databaseToObject, search class select: Unexpected return value from method sqlite3_step: %i", stepRes);
		}
	}
	sqlite3_clear_bindings(sclSelStmt);
	sqlite3_reset(sclSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_searchClass));

    if (Container->setSearchClasses(SearchClasses)){
        ERROR("Error while setting search classes");
        return -1;
    }
    return 0;
}

cUPnPClassContainer* cUPnPContainerMediator::createObject(const char* Title, bool Restricted){
    MESSAGE(VERBOSE_MODIFICATIONS, "Creating Container '%s'",Title);
    cUPnPClassContainer* Object = new cUPnPClassContainer;
    if (this->initializeObject(Object, UPNP_CLASS_CONTAINER, Title, Restricted)){
		return NULL;
	}
    return Object;
}

cUPnPClassContainer* cUPnPContainerMediator::getObject(cUPnPObjectID ID){
//    MESSAGE(VERBOSE_METADATA, "cUPnPContainerMediator:: Getting Container with ID '%s'",*ID);
    cUPnPClassContainer* Object = new cUPnPClassContainer;
    if (this->databaseToObject(Object, ID)){
		return NULL;
	}
    return Object;
}

/**********************************************\
 *                                              *
 *  EpgContainer mediator                       *
 *                                              *
 \**********************************************/
cUPnPEpgContainerMediator::cUPnPEpgContainerMediator(cMediaDatabase* MediaDatabase) : cUPnPContainerMediator(MediaDatabase){
	this->mChnInsStmt = NULL;
	this->mChnSelStmt = NULL;
	this->mChnDelStmt = NULL;
}

cUPnPClassEpgContainer* cUPnPEpgContainerMediator::createObject(const char* Title, bool Restricted){
    cUPnPClassEpgContainer* Object = new cUPnPClassEpgContainer;
    if (this->initializeObject(Object, UPNP_CLASS_EPGCONTAINER, Title, Restricted)){
		return NULL;
	}
    return Object;
}

int cUPnPEpgContainerMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){
	if (cUPnPContainerMediator::databaseToObject(Object, ID)){
        ERROR("cUPnPEpgContainerMediator::databaseToObject: Error while loading container");
        return -1;
    }
	cUPnPClassEpgContainer* epgContainer = (cUPnPClassEpgContainer*) Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_epgChannel));
	if (this->mChnSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s FROM %s WHERE %s=:ID", SQLITE_COL_CHANNELID, SQLITE_COL_CHANNELNAME2, SQLITE_COL_ISRADIOCHANNEL,
			                                   SQLITE_TABLE_EPGCHANNELS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mChnSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for an EPG channel selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	
	bool actionSuccess = sqlite3_bind_int (this->mChnSelStmt, 1, (unsigned int) ID) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPEpgContainerMediator::databaseToObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(this->mChnSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				epgContainer->setChannelId((const char*) sqlite3_column_text(this->mChnSelStmt, 0));
				epgContainer->setChannelName((const char*) sqlite3_column_text(this->mChnSelStmt, 1));
				epgContainer->setRadioChannel(sqlite3_column_int(this->mChnSelStmt, 2) == 1);
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("cUPnPEpgContainerMediator::databaseToObject: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(this->mChnSelStmt);
	sqlite3_reset(this->mChnSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_epgChannel));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPEpgContainerMediator::objectToDatabase(cUPnPClassObject* Object){
    if (cUPnPContainerMediator::objectToDatabase(Object)){
		ERROR("cUPnPContainerMediator: object to database failed");
		return -1;
	}
	pthread_mutex_lock(&(this->mDatabase->mutex_epgChannel));
	if (this->mChnInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s,%s,%s) VALUES (:ID,@CI,@CN,:IC)", SQLITE_TABLE_EPGCHANNELS,
			                                    SQLITE_COL_OBJECTID, SQLITE_COL_CHANNELID, SQLITE_COL_CHANNELNAME2, SQLITE_COL_ISRADIOCHANNEL);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mChnInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for an EPG channel insertion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	
	cUPnPClassEpgContainer* Container = (cUPnPClassEpgContainer*)Object;
	bool actionSuccess = sqlite3_bind_int (this->mChnInsStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mChnInsStmt, 2, (Container->getChannelId()) ? Container->getChannelId() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mChnInsStmt, 3, (Container->getChannelName()) ? Container->getChannelName() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int(this->mChnInsStmt, 4, (Container->isRadioChannel()) ? 1 : 0) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("EPG container, objectToDatabase: SQL bind error %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(this->mChnInsStmt) != SQLITE_DONE){
		ERROR("cUPnPEpgContainerMediator::objectToDatabase: sqlite3_step failed with insertion of data");
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mChnInsStmt);
	sqlite3_reset(this->mChnInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_epgChannel));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPEpgContainerMediator::deleteObject(cUPnPClassObject* Object){
//	MESSAGE(VERBOSE_OBJECTS, "cUPnPEpgContainerMediator: delete container with ID %s", *Object->getID());
	sqlite3_stmt* objDelStmt = this->mDatabase->getObjectDeleteStatement();
	if (this->mChnDelStmt == NULL){
		cString resStatement = cString::sprintf("DELETE FROM %s WHERE %s=:ID", SQLITE_TABLE_EPGCHANNELS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mChnDelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for an EPG channel deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	int ret = 0;
	int objId = (int) Object->getID();
	this->mDatabase->startTransaction();
	pthread_mutex_lock(&(this->mDatabase->mutex_object));
	bool actionSuccess = sqlite3_bind_int (objDelStmt, 1, objId) == SQLITE_OK;
	if (actionSuccess && sqlite3_step(objDelStmt) != SQLITE_DONE){
		ERROR("cUPnPEpgContainerMediator::deleteObject: sqlite3_step failed with deletion in table object: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(objDelStmt);
	sqlite3_reset(objDelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_object));

	pthread_mutex_lock(&(this->mDatabase->mutex_epgChannel));
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mChnDelStmt, 1, objId) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPEpgContainerMediator::deleteObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(this->mChnDelStmt) != SQLITE_DONE){
		ERROR("cUPnPEpgContainerMediator::deleteObject: sqlite3_step failed with deletion in table RecordTimers: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mChnDelStmt);
	sqlite3_reset(this->mChnDelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_epgChannel));


	if (!actionSuccess){
		this->mDatabase->rollbackTransaction();
		ret = -1;
	}
	else {
		this->mDatabase->commitTransaction();
	}
    return ret;
}

cUPnPClassEpgContainer* cUPnPEpgContainerMediator::getObject(cUPnPObjectID ID){
    cUPnPClassEpgContainer* Object = new cUPnPClassEpgContainer;
    if (this->databaseToObject(Object, ID)){
		return NULL;
	}
    return Object;
}

 /**********************************************\
 *                                              *
 *  Audio item mediator                         *
 *                                              *
 \**********************************************/

cUPnPAudioItemMediator::cUPnPAudioItemMediator(cMediaDatabase* MediaDatabase) :
    cUPnPItemMediator(MediaDatabase){}

cUPnPClassAudioItem* cUPnPAudioItemMediator::createObject(const char* Title, bool Restricted){
    cUPnPClassAudioItem* Object = new cUPnPClassAudioItem;
    if (this->initializeObject(Object, UPNP_CLASS_AUDIO, Title, Restricted)){
		return NULL;
	}
    return Object;
}

cUPnPClassAudioItem* cUPnPAudioItemMediator::getObject(cUPnPObjectID ID){
    cUPnPClassAudioItem* Object = new cUPnPClassAudioItem;
    if (this->databaseToObject(Object, ID)){
		return NULL;
	}
    return Object;
}

int cUPnPAudioItemMediator::objectToDatabase(cUPnPClassObject* Object){
    if (cUPnPItemMediator::objectToDatabase(Object)){
		return -1;
	}
//    cUPnPClassAudioItem* audioItem = (cUPnPClassAudioItem*) Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_audioItem));
    sqlite3_stmt* audInsStmt = this->mDatabase->getAudioInsertStatement();
	bool actionSuccess = sqlite3_bind_int (audInsStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPAudioItemMediator::objectToDatabase: Error with sqlite3_bind for audio item insertion; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(audInsStmt) != SQLITE_DONE){
		ERROR("cUPnPAudioItemMediator::objectToDatabase: sqlite3_step failed with insertion of data; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(audInsStmt);
	sqlite3_reset(audInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_audioItem));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPAudioItemMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){
//	MESSAGE(VERBOSE_MODIFICATIONS, "cUPnPAudioItemMediator::databaseToObject for ID %i", (int)ID);
    if (cUPnPItemMediator::databaseToObject(Object,ID)){
        ERROR("cUPnPAudioItemMediator: Error while loading object");
        return -1;
    }
//    cUPnPClassAudioItem* audioItem = (cUPnPClassAudioItem*)Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_audioItem));
	sqlite3_stmt* audSelStmt = this->mDatabase->getAudioSelectStatement();
	bool actionSuccess = sqlite3_bind_int (audSelStmt, 1, (unsigned int) ID) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPAudioItemMediator::databaseToObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	bool methodError = false;
	int ctr = 0;
	const char* genre = NULL;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(audSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				genre = (const char*) sqlite3_column_text(audSelStmt, 0);
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("databaseToObject: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(audSelStmt);
	sqlite3_reset(audSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_audioItem));
	if (methodError){
		return -1;
	}
	return (actionSuccess) ? 0 : -1;
}

 /**********************************************\
 *                                              *
 *  Video item mediator                         *
 *                                              *
 \**********************************************/

cUPnPVideoItemMediator::cUPnPVideoItemMediator(cMediaDatabase* MediaDatabase) :
    cUPnPItemMediator(MediaDatabase){}

cUPnPClassVideoItem* cUPnPVideoItemMediator::createObject(const char* Title, bool Restricted){
    cUPnPClassVideoItem* Object = new cUPnPClassVideoItem;
    if (this->initializeObject(Object, UPNP_CLASS_VIDEO, Title, Restricted)){
		return NULL;
	}
    return Object;
}

cUPnPClassVideoItem* cUPnPVideoItemMediator::getObject(cUPnPObjectID ID){
    cUPnPClassVideoItem* Object = new cUPnPClassVideoItem;
    if (this->databaseToObject(Object, ID)){
		return NULL;
	}
    return Object;
}

int cUPnPVideoItemMediator::objectToDatabase(cUPnPClassObject* Object){
    if (cUPnPItemMediator::objectToDatabase(Object)){
		ERROR("cUPnPVideoItemMediator objectToDatabase failed with item mediator objectToDatabase"); 
		return -1;
	}
    cUPnPClassVideoItem* VideoItem = (cUPnPClassVideoItem*)Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_videoItem));
    sqlite3_stmt* vidInsStmt = this->mDatabase->getVideoInsertStatement();
	bool actionSuccess = sqlite3_bind_int (vidInsStmt, 1, ((unsigned int) Object->getID())) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 2, (VideoItem->getGenre()) ? VideoItem->getGenre() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 3, (VideoItem->getLongDescription()) ? VideoItem->getLongDescription() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 4, (VideoItem->getProducers()) ? VideoItem->getProducers() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 5, (VideoItem->getRating()) ? VideoItem->getRating() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 6, (VideoItem->getActors()) ? VideoItem->getActors() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 7, (VideoItem->getDirectors()) ? VideoItem->getDirectors() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 8, (VideoItem->getDescription()) ? VideoItem->getDescription() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 9, (VideoItem->getPublishers()) ? VideoItem->getPublishers() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 10, (VideoItem->getLanguage()) ? VideoItem->getLanguage() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(vidInsStmt, 11, (VideoItem->getRelations()) ? VideoItem->getRelations() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPVideoItemMediator::objectToDatabase: Error with sqlite3_bind for video item insertion; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(vidInsStmt) != SQLITE_DONE){
		ERROR("cUPnPVideoItemMediator::objectToDatabase: sqlite3_step failed with insertion of data; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(vidInsStmt);
	sqlite3_reset(vidInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_videoItem));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPVideoItemMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){
//	 MESSAGE(VERBOSE_MODIFICATIONS, "cUPnPVideoItemMediator::databaseToObject for ID %i", (int)ID);
    if (cUPnPItemMediator::databaseToObject(Object, ID)){
        ERROR("cUPnPVideoItemMediator, Error while loading object");
        return -1;
    }
    cUPnPClassVideoItem* VideoItem = (cUPnPClassVideoItem*) Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_videoItem));
    sqlite3_stmt* vidSelStmt = this->mDatabase->getVideoSelectStatement();
	bool actionSuccess = sqlite3_bind_int (vidSelStmt, 1, (unsigned int) ID) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPVideoItemMediator::databaseToObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	bool objFound = false;
	bool methodError = false;
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(vidSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				if (VideoItem->setGenre((const char*) sqlite3_column_text(vidSelStmt, 0))){
					ERROR("Error while setting genre");
					methodError = true;
				}	
				if (VideoItem->setLongDescription((const char*) sqlite3_column_text(vidSelStmt, 1))){
					ERROR("Error while setting long description");
					methodError = true;
				}
				if (VideoItem->setProducers((const char*) sqlite3_column_text(vidSelStmt, 2))){
					ERROR("Error while setting producers");
					methodError = true;
				}
				if (VideoItem->setRating((const char*) sqlite3_column_text(vidSelStmt, 3))){
					ERROR("Error while setting rating");
					methodError = true;
				}
				if (VideoItem->setActors((const char*) sqlite3_column_text(vidSelStmt, 4))){
					ERROR("Error while setting actors");
					methodError = true;
				}
				if (VideoItem->setDirectors((const char*) sqlite3_column_text(vidSelStmt, 5))){
					ERROR("Error while setting directors");
					methodError = true;
				}
				if (VideoItem->setDescription((const char*) sqlite3_column_text(vidSelStmt, 6))){
					ERROR("Error while setting description");
					methodError = true;
				}
				if (VideoItem->setPublishers((const char*) sqlite3_column_text(vidSelStmt, 7))){
					ERROR("Error while setting publishers");
					methodError = true;
				}
				if (VideoItem->setLanguage((const char*) sqlite3_column_text(vidSelStmt, 8))){
					ERROR("Error while setting language");
					methodError = true;
				}
				if (VideoItem->setRelations((const char*) sqlite3_column_text(vidSelStmt, 9))){
					ERROR("Error while setting relations");
					methodError = true;
				}
				objFound = true;
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("databaseToObject: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(vidSelStmt);
	sqlite3_reset(vidSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_videoItem));
	if (!actionSuccess || methodError || !objFound){
		ERROR("cUPnPVideoItemMediator::databaseToObject failure with ID %i", (int)ID);
		return -1;
	}
    return 0;
}

 /**********************************************\
 *                                              *
 *  Audio broadcast item mediator               *
 *                                              *
 \**********************************************/

cUPnPAudioBroadcastMediator::cUPnPAudioBroadcastMediator(cMediaDatabase* MediaDatabase) :cUPnPAudioItemMediator(MediaDatabase){
	this->mAbcInsStmt = NULL;
	this->mAbcSelStmt = NULL;
	this->mAbcDelStmt = NULL;
}

cUPnPClassAudioBroadcast* cUPnPAudioBroadcastMediator::createObject(const char* Title, bool Restricted){
 //   MESSAGE(VERBOSE_MODIFICATIONS, "Creating Audio broadcast '%s'",Title);
    cUPnPClassAudioBroadcast* Object = new cUPnPClassAudioBroadcast;
    if (this->initializeObject(Object, UPNP_CLASS_AUDIOBC, Title, Restricted)){
		return NULL;
	}
    return Object;
}

cUPnPClassAudioBroadcast* cUPnPAudioBroadcastMediator::getObject(cUPnPObjectID ID){
    cUPnPClassAudioBroadcast* Object = new cUPnPClassAudioBroadcast;
    if (this->databaseToObject(Object, ID)){
		return NULL;
	}
    return Object;
}

int cUPnPAudioBroadcastMediator::objectToDatabase(cUPnPClassObject* Object){
    if (cUPnPAudioItemMediator::objectToDatabase(Object)){
		return -1;
	}
    cUPnPClassAudioBroadcast* AudioBroadcast = (cUPnPClassAudioBroadcast*) Object;
//	LOCK_THREAD;
	pthread_mutex_lock(&(this->mDatabase->mutex_audioBC));
	if (this->mAbcInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s,%s) VALUES (:ID,@RA,:CN)", SQLITE_TABLE_AUDIOBROADCASTS,
			                     SQLITE_COL_OBJECTID, SQLITE_COL_RADIOSTATIONID, SQLITE_COL_CHANNELNR);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mAbcInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a audio broadcast insertion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	
	bool actionSuccess = sqlite3_bind_int (this->mAbcInsStmt, 1, (unsigned int) Object->getID()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mAbcInsStmt, 2, (AudioBroadcast->getChannelName()) ? AudioBroadcast->getChannelName() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mAbcInsStmt, 3, AudioBroadcast->getChannelNr()) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPAudioBroadcastMediator::objectToDatabase: Error with sqlite3_bind for insertion; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(this->mAbcInsStmt) != SQLITE_DONE){
		ERROR("cUPnPAudioBroadcastMediator::objectToDatabase: sqlite3_step failed with insertion of data for ID: %i; %s", (int) Object->getID(),
			sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mAbcInsStmt);
	sqlite3_reset(this->mAbcInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_audioBC));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPAudioBroadcastMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){	
    if (cUPnPAudioItemMediator::databaseToObject(Object,ID)){
        ERROR("cUPnPAudioBroadcastMediator: Error while loading object");
        return -1;
    }
    cUPnPClassAudioBroadcast* AudioBroadcast = (cUPnPClassAudioBroadcast*)Object;
//	LOCK_THREAD;
	pthread_mutex_lock(&(this->mDatabase->mutex_audioBC));
	if (this->mAbcSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s FROM %s WHERE %s=:ID", SQLITE_COL_CHANNELNR,
			                          SQLITE_COL_RADIOSTATIONID, SQLITE_TABLE_AUDIOBROADCASTS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mAbcSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a audio broadcast selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	bool actionSuccess = sqlite3_bind_int (this->mAbcSelStmt, 1, (unsigned int) ID) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPAudioBroadcastMediator::databaseToObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	bool objFound = false;
	bool methodError = false;
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(this->mAbcSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				if (AudioBroadcast->setChannelNr(sqlite3_column_int(this->mAbcSelStmt, 0))){
					ERROR("Error while setting channel number");
					methodError = true;
				}
				if(AudioBroadcast->setChannelName((const char*) sqlite3_column_text(this->mAbcSelStmt, 1))){
					ERROR("Error while setting channel name");
					methodError = true;
				}			
				objFound = true;
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("databaseToObject: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(this->mAbcSelStmt);
	sqlite3_reset(this->mAbcSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_audioBC));
//	MESSAGE(VERBOSE_METADATA, "Method cUPnPAudioBroadcastMediator::databaseToObject was called, got: %s", (AudioBroadcast->getChannelName()) ? AudioBroadcast->getChannelName() : "");
	if (!actionSuccess || methodError || !objFound){
		return -1;
	}
    return 0;
}

int cUPnPAudioBroadcastMediator::deleteObject(cUPnPClassObject* Object){
//	MESSAGE(VERBOSE_METADATA, "Method cUPnPAudioBroadcastMediator::deleteObject with ID %s", *Object->getID());
   if (this->mAbcDelStmt == NULL){
		cString resStatement = cString::sprintf("DELETE FROM %s WHERE %s=:ID", SQLITE_TABLE_AUDIOBROADCASTS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mAbcDelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a audio broadcast item deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	int ret = 0;
	int objId = (int) Object->getID();
	bool actionSuccess = deleteObjectProc(objId);
//	LOCK_THREAD;
	pthread_mutex_lock(&(this->mDatabase->mutex_audioBC));
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mAbcDelStmt, 1, objId) == SQLITE_OK;
	if (actionSuccess && sqlite3_step(this->mAbcDelStmt) != SQLITE_DONE){
		ERROR("cUPnPAudioBroadcastMediator::deleteObject: sqlite3_step failed with deletion in table audio broadcast: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mAbcDelStmt);
	sqlite3_reset(this->mAbcDelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_audioBC));

	delete Object;  // remove object from parent container
	Object = NULL;
	if (!actionSuccess){
		ERROR("cUPnPAudioBroadcastMediator::deleteObject: FAIL");
		ret = -1;
	}
    return ret;
}

 /**********************************************\
 *                                              *
 *  Video broadcast item mediator               *
 *                                              *
 \**********************************************/

cUPnPVideoBroadcastMediator::cUPnPVideoBroadcastMediator(cMediaDatabase* MediaDatabase) : cUPnPVideoItemMediator(MediaDatabase){
	this->mVbcInsStmt = NULL;
	this->mVbcSelStmt = NULL;
	this->mVbcDelStmt = NULL;
}

cUPnPClassVideoBroadcast* cUPnPVideoBroadcastMediator::createObject(const char* Title, bool Restricted){
//	MESSAGE(VERBOSE_METADATA, "Creating Video broadcast '%s'",Title);
    cUPnPClassVideoBroadcast* Object = new cUPnPClassVideoBroadcast;
    if (this->initializeObject(Object, UPNP_CLASS_VIDEOBC, Title, Restricted)){
		return NULL;
	}
    return Object;
}

cUPnPClassVideoBroadcast* cUPnPVideoBroadcastMediator::getObject(cUPnPObjectID ID){
    cUPnPClassVideoBroadcast* Object = new cUPnPClassVideoBroadcast;
    if (this->databaseToObject(Object, ID)){
		ERROR("cUPnPVideoBroadcastMediator: Database to object failed with ID %s", *ID);
		return NULL;
	}
    return Object;
}

int cUPnPVideoBroadcastMediator::objectToDatabase(cUPnPClassObject* Object){
    if (cUPnPVideoItemMediator::objectToDatabase(Object)){
		ERROR("cUPnPVideoBroadcastMediator:: The item mediator objectToDatabase failed");
		return -1;
	}
	
    cUPnPClassVideoBroadcast* VideoBroadcast = (cUPnPClassVideoBroadcast*)Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_videoBC));
	if (this->mVbcInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s,%s,%s,%s) VALUES (:ID,@IC,@RG,@CN,@CR)", SQLITE_TABLE_VIDEOBROADCASTS,
			                   SQLITE_COL_OBJECTID, SQLITE_COL_ICON, SQLITE_COL_REGION, SQLITE_COL_CHANNELNAME, SQLITE_COL_CHANNELNR);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mVbcInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a video broadcast insertion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	bool actionSuccess = sqlite3_bind_int (this->mVbcInsStmt, 1, (unsigned int) Object->getID()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mVbcInsStmt, 2, (VideoBroadcast->getIcon()) ? VideoBroadcast->getIcon() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mVbcInsStmt, 3, (VideoBroadcast->getRegion()) ? VideoBroadcast->getRegion() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mVbcInsStmt, 4, (VideoBroadcast->getChannelName()) ? VideoBroadcast->getChannelName() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mVbcInsStmt, 5, itoa(VideoBroadcast->getChannelNr()), -1, SQLITE_TRANSIENT) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPVideoBroadcastMediator::objectToDatabase: Error with sqlite3_bind for insertion; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(this->mVbcInsStmt) != SQLITE_DONE){
		ERROR("cUPnPVideoBroadcastMediator::objectToDatabase: sqlite3_step failed with insertion of data for ID: %i; %s", (int) Object->getID(),
			sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mVbcInsStmt);
	sqlite3_reset(this->mVbcInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_videoBC));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPVideoBroadcastMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){
//	MESSAGE(VERBOSE_METADATA, "cUPnPVideoBroadcastMediator::databaseToObject, ID %i", (int)ID);
    if (cUPnPVideoItemMediator::databaseToObject(Object,ID)){
        ERROR("cUPnPVideoBroadcastMediator, Error while loading object");
        return -1;
    }
    cUPnPClassVideoBroadcast* VideoBroadcast = (cUPnPClassVideoBroadcast*)Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_videoBC));
	if (this->mVbcSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s,%s FROM %s WHERE %s=:ID", SQLITE_COL_ICON, 
												SQLITE_COL_REGION, SQLITE_COL_CHANNELNR, SQLITE_COL_CHANNELNAME, 
												SQLITE_TABLE_VIDEOBROADCASTS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mVbcSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a video broadcast selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	bool actionSuccess = sqlite3_bind_int (this->mVbcSelStmt, 1, (unsigned int) ID) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPVideoBroadcastMediator, databaseToObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	bool objFound = false;
	bool methodError = false;
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(this->mVbcSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				if (VideoBroadcast->setIcon((const char*) sqlite3_column_text(this->mVbcSelStmt, 0))){
					ERROR("Error while setting icon");
					methodError = true;
				}
				if (VideoBroadcast->setRegion((const char*) sqlite3_column_text(this->mVbcSelStmt, 1))){
					ERROR("Error while setting region");
					methodError = true;
				}
				if (VideoBroadcast->setChannelNr(atoi((const char*) sqlite3_column_text(this->mVbcSelStmt, 2)))){
					ERROR("Error while setting channel number");
					methodError = true;
				}
				if (VideoBroadcast->setChannelName((const char*) sqlite3_column_text(this->mVbcSelStmt, 3))){
					ERROR("Error while setting channel name");
					methodError = true;
				}	
				objFound = true;
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("cUPnPVideoBroadcastMediator, databaseToObject: Unexpected return value with method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(this->mVbcSelStmt);
	sqlite3_reset(this->mVbcSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_videoBC));
	if (!actionSuccess || methodError || !objFound){
		ERROR("cUPnPVideoBroadcastMediator, databaseToObject: Failure with ID %i", (int)ID);
		return -1;
	}
    return 0;
}

int cUPnPVideoBroadcastMediator::deleteObject(cUPnPClassObject* Object){
//	MESSAGE(VERBOSE_METADATA, "Method cUPnPVideoBroadcastMediator::deleteObject with ID %s", *Object->getID());
   if (this->mVbcDelStmt == NULL){
		cString resStatement = cString::sprintf("DELETE FROM %s WHERE %s=:ID", SQLITE_TABLE_VIDEOBROADCASTS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mVbcDelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a video broadcast item deletion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	int ret = 0;
	int objId = (int) Object->getID();

	bool actionSuccess = deleteObjectProc(objId);

	pthread_mutex_lock(&(this->mDatabase->mutex_videoBC));
	actionSuccess = actionSuccess && sqlite3_bind_int (this->mVbcDelStmt, 1, objId) == SQLITE_OK;
	if (actionSuccess && sqlite3_step(this->mVbcDelStmt) != SQLITE_DONE){
		ERROR("cUPnPVideoBroadcastMediator::deleteObject: sqlite3_step failed with deletion in table video broadcast: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mVbcDelStmt);
	sqlite3_reset(this->mVbcDelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_videoBC));

	delete Object;  // romove from parent container
	Object = NULL;
	if (!actionSuccess){
		ERROR("cUPnPVideoBroadcastMediator::deleteObject: FAILED");
		ret = -1;
	}
    return ret;
}

/**********************************************\
*                                              *
*  AudioRecord item mediator                   *
*                                              *
\**********************************************/

cUPnPAudioRecordMediator::cUPnPAudioRecordMediator(cMediaDatabase* MediaDatabase) :
    cUPnPAudioItemMediator(MediaDatabase){}

cUPnPClassAudioRecord* cUPnPAudioRecordMediator::createObject(const char* Title, bool Restricted){
    MESSAGE(VERBOSE_MODIFICATIONS, "Creating AudioRecord '%s'",Title);
    cUPnPClassAudioRecord* Object = new cUPnPClassAudioRecord;
    if (this->initializeObject(Object, UPNP_CLASS_MUSICTRACK, Title, Restricted)) {
		return NULL;
	}
    return Object;
}

cUPnPClassAudioRecord* cUPnPAudioRecordMediator::getObject(cUPnPObjectID ID){
//    MESSAGE(VERBOSE_METADATA, "Getting AudioRecord with ID '%s'",*ID);
    cUPnPClassAudioRecord* Object = new cUPnPClassAudioRecord;
    if (this->databaseToObject(Object, ID)) {
		return NULL;
	}
    return Object;
}

int cUPnPAudioRecordMediator::objectToDatabase(cUPnPClassObject* Object){
//	MESSAGE(VERBOSE_METADATA, "AudioRecord mediator: object to database"); 
    if (cUPnPAudioItemMediator::objectToDatabase(Object)){
		return -1;
	}
    cUPnPClassAudioRecord* Movie = (cUPnPClassAudioRecord*)Object;
    cString Format = "INSERT OR REPLACE INTO %s (%s) VALUES (%s);";
    cString Columns=NULL, Values=NULL;
    char *Value=NULL;
    cString Properties[] = {
        SQLITE_COL_OBJECTID,
        SQLITE_COL_STORAGEMEDIUM,
		SQLITE_COL_CHANNELNAME,
        NULL
    };
    for (cString* Property = Properties; *(*Property); Property++){
        Columns = cSQLiteDatabase::sprintf("%s%s%s", *Columns?*Columns:"", *Columns?",":"", *(*Property));
        if(!Movie->getProperty(*Property, &Value)){
            ERROR("No such property '%s' in object with ID '%s'",*(*Property),* Movie->getID());
            return -1;
        }
        Values = cSQLiteDatabase::sprintf("%s%s%Q", *Values?*Values:"", *Values?",":"", Value);
    }
	pthread_mutex_lock(&(this->mDatabase->mutex_audioRec));
    if (this->mDatabase->execStatement(Format, SQLITE_TABLE_AUDIORECORDS, *Columns, *Values)){
		pthread_mutex_unlock(&(this->mDatabase->mutex_audioRec));
        ERROR("AudioRecord mediator: Error while executing statement");
        return -1;
    }
	pthread_mutex_unlock(&(this->mDatabase->mutex_audioRec));
    return 0;
}

int cUPnPAudioRecordMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){
    if (cUPnPAudioItemMediator::databaseToObject(Object, ID)){
        ERROR("Error while loading object");
        return -1;
    }
//    cUPnPClassAudioRecord* Movie = (cUPnPClassAudioRecord*)Object;
    cString Format = "SELECT * FROM %s WHERE %s=%s";
    cString Column = NULL, Value = NULL;
    cRows* Rows; cRow* Row;
	pthread_mutex_lock(&(this->mDatabase->mutex_audioRec));
    if (this->mDatabase->execStatement(Format, SQLITE_TABLE_AUDIORECORDS, SQLITE_COL_OBJECTID, *ID)){
		pthread_mutex_unlock(&(this->mDatabase->mutex_audioRec));
        ERROR("Error while executing statement");
        return -1;
    }
    Rows = this->mDatabase->getResultRows();
    if (!Rows->fetchRow(&Row)){
		pthread_mutex_unlock(&(this->mDatabase->mutex_audioRec));
        MESSAGE(VERBOSE_SQL, "No item properties found in the table %s", SQLITE_TABLE_AUDIORECORDS);
        return 0;
    }
    while (Row->fetchColumn(&Column, &Value)){
        //if(!strcasecmp(Column, SQLITE_COL_STORAGEMEDIUM)){
        //    if(Movie->setStorageMedium(Value)){
        //        ERROR("Error while setting region");
        //        return -1;
        //    }
        //}
    }
	pthread_mutex_unlock(&(this->mDatabase->mutex_audioRec));
    return 0;
}

/**********************************************\
*                                              *
*  Movie item mediator                         *
*                                              *
\**********************************************/

cUPnPMovieMediator::cUPnPMovieMediator(cMediaDatabase* MediaDatabase) : cUPnPVideoItemMediator(MediaDatabase){
	this->mMovSelStmt = NULL;
	this->mMovInsStmt = NULL;
}

cUPnPClassMovie* cUPnPMovieMediator::createObject(const char* Title, bool Restricted){
//    MESSAGE(VERBOSE_MODIFICATIONS, "Creating movie '%s'",Title);
    cUPnPClassMovie* Object = new cUPnPClassMovie;
    if (this->initializeObject(Object, UPNP_CLASS_MOVIE, Title, Restricted)){
		return NULL;
	}
    return Object;
}

cUPnPClassMovie* cUPnPMovieMediator::getObject(cUPnPObjectID ID){
 //   MESSAGE(VERBOSE_METADATA, "Getting movie with ID '%s'",*ID);
    cUPnPClassMovie* Object = new cUPnPClassMovie;
    if (this->databaseToObject(Object, ID)){
		return NULL;
	}
    return Object;
}

int cUPnPMovieMediator::objectToDatabase(cUPnPClassObject* Object){
//	MESSAGE(VERBOSE_METADATA, "Movie mediator: object to database"); 
    if (cUPnPVideoItemMediator::objectToDatabase(Object)){
		return -1;
	}
    cUPnPClassMovie* Movie = (cUPnPClassMovie*)Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_movie));

	if (this->mMovInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s,%s,%s) VALUES (:ID,@SM,:RC,@CN)", SQLITE_TABLE_MOVIES,
			                   SQLITE_COL_OBJECTID, SQLITE_COL_STORAGEMEDIUM, SQLITE_COL_DVDREGIONCODE, SQLITE_COL_CHANNELNAME);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mMovInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a movie insertion: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	bool actionSuccess = sqlite3_bind_int (this->mMovInsStmt, 1, (unsigned int) Object->getID()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mMovInsStmt, 2, (Movie->getStorageMedium()) ? Movie->getStorageMedium() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_int(this->mMovInsStmt, 3, Movie->getDVDRegionCode()) == SQLITE_OK;
	actionSuccess = actionSuccess && sqlite3_bind_text(this->mMovInsStmt, 4, (Movie->getChannelName()) ? Movie->getChannelName() : "", -1, SQLITE_TRANSIENT) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPMovieMediator::objectToDatabase: Error with sqlite3_bind for insertion; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	if (actionSuccess && sqlite3_step(this->mMovInsStmt) != SQLITE_DONE){
		ERROR("cUPnPMovieMediator::objectToDatabase: sqlite3_step failed with insertion of data; %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
		actionSuccess = false;
	}
	sqlite3_clear_bindings(this->mMovInsStmt);
	sqlite3_reset(this->mMovInsStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_movie));
	return (actionSuccess) ? 0 : -1;
}

int cUPnPMovieMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID){	
    if (cUPnPVideoItemMediator::databaseToObject(Object,ID)){
        ERROR("cUPnPMovieMediator, databaseToObject: Error while loading object");
        return -1;
    }
    cUPnPClassMovie* Movie = (cUPnPClassMovie*)Object;
	pthread_mutex_lock(&(this->mDatabase->mutex_movie));
	if (this->mMovSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s FROM %s WHERE %s=:ID",  
												SQLITE_COL_STORAGEMEDIUM, SQLITE_COL_DVDREGIONCODE, SQLITE_COL_CHANNELNAME, 
												SQLITE_TABLE_MOVIES, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase->getSqlite3(), zSql, strlen(zSql)+1, &(this->mMovSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a movie selection: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));	
		}
	}
	bool actionSuccess = sqlite3_bind_int (this->mMovSelStmt, 1, (unsigned int) ID) == SQLITE_OK;
	if (!actionSuccess){
		ERROR("cUPnPMovieMediator::databaseToObject: sqlite3 bind error: %s", sqlite3_errmsg(this->mDatabase->getSqlite3()));
	}
	bool objFound = false;
	bool methodError = false;
	int ctr = 0;
	while (actionSuccess && ctr++ < 5){
		int stepRes = sqlite3_step(this->mMovSelStmt);
		switch (stepRes){
			case SQLITE_ROW:
				if (Movie->setStorageMedium((const char*) sqlite3_column_text(this->mMovSelStmt, 0))){
					ERROR("Error while setting the storage medium");
					methodError = true;
				}
				if (Movie->setDVDRegionCode(sqlite3_column_int(this->mMovSelStmt, 1))){
					ERROR("Error while setting the region code");
					methodError = true;
				}
				if (Movie->setChannelName((const char*) sqlite3_column_text(this->mMovSelStmt, 2))){
					ERROR("Error while setting the channel name");
					methodError = true;
				}				
				objFound = true;
				break;
			case SQLITE_DONE:
				ctr = 999; // stop loop
				break;
			default:
				ERROR("databaseToObject: Unexpected return value from method sqlite3_step: %i", stepRes);
				break;
		}
	}
	sqlite3_clear_bindings(this->mMovSelStmt);
	sqlite3_reset(this->mMovSelStmt);
	pthread_mutex_unlock(&(this->mDatabase->mutex_movie));
	if (!objFound || methodError){
		return -1;
	}
//	MESSAGE(VERBOSE_METADATA, "Movie mediator: database to object for '%s' successful", Object->getTitle()); 
    return 0;
}
