/*
 * File:   database.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 3. September 2009, 22:20
 * Last modification: April 9, 2013
 */

#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "database.h"
#include "../common.h"
#include "object.h"
#include "../upnp.h"
#include "config.h"

cSQLiteDatabase* cSQLiteDatabase::mInstance = NULL;

cSQLiteDatabase::cSQLiteDatabase(){
	initializeParams();
}

cSQLiteDatabase::~cSQLiteDatabase(){
    sqlite3_close(this->mDatabase);
}

cSQLiteDatabase* cSQLiteDatabase::getInstance(){
    if (cSQLiteDatabase::mInstance == NULL){
        cSQLiteDatabase::mInstance = new cSQLiteDatabase;
        DatabaseLocker.Wait();
        cSQLiteDatabase::mInstance->initialize();
    }

    if(cSQLiteDatabase::mInstance != NULL){
        return cSQLiteDatabase::mInstance;
	}
    return NULL;
}

int cSQLiteDatabase::exec(const char* Statement){
    char* Error;
    if(!this->mDatabase){
        ERROR("Database not open. Cannot continue");
        return -1;
    }
    this->mRows = new cRows;
    MESSAGE(VERBOSE_SQL_STATEMENTS,"SQLite: %s", Statement);
    if(sqlite3_exec(this->mDatabase, Statement, cSQLiteDatabase::getResultRow, (cSQLiteDatabase*)this, &Error)!=SQLITE_OK){
        ERROR("Database error: %s", Error);
        ERROR("Statement was: %s", Statement);
        delete this->mRows; this->mRows = NULL;
        sqlite3_free(Error);
        return -1;
    }

    sqlite3_free(Error);
    return 0;
}

const char* cSQLiteDatabase::sprintf(const char* Format, ...){
    va_list vlist;
    va_start(vlist, Format);
    char* SQLStatement = sqlite3_vmprintf(Format, vlist);
    va_end(vlist);
    return SQLStatement;
}

int cSQLiteDatabase::execStatement(const char* Statement, ...){
    va_list vlist;
    va_start(vlist, Statement);
    char* SQLStatement = sqlite3_vmprintf(Statement, vlist);
    va_end(vlist);
    int ret = this->exec(SQLStatement);
    sqlite3_free(SQLStatement);
    return ret;
}

int cSQLiteDatabase::getResultRow(void* DB, int NumCols, char** Values, char** ColNames){
    cRow* Row = new cRow;
    Row->ColCount = NumCols;
    Row->Columns = new char*[NumCols];
    Row->Values = new char*[NumCols];
    for(int i=0; i < NumCols; i++){
        Row->Columns[i] = strdup0(ColNames[i]);
        Row->Values[i] = strdup0(Values[i]);
    }
    cSQLiteDatabase* Database = (cSQLiteDatabase*)DB;
    Database->mRows->Add(Row);
    return 0;
}

cRows::cRows(){
    this->mLastRow = NULL;
}

cRows::~cRows(){
    this->mLastRow = NULL;
}

bool cRows::fetchRow(cRow** Row){
    if(this->mLastRow==NULL){
        this->mLastRow = this->First();
    }
    else {
        this->mLastRow = this->Next(this->mLastRow);
    }
    if(this->mLastRow != NULL){
        *Row = this->mLastRow;
        return true;
    }
    else {
        *Row = NULL;
        return false;
    }
    return false;
}

cRow::cRow(){
    this->currentCol = 0;
    this->ColCount = 0;
    this->Columns = NULL;
    this->Values = NULL;
}

cRow::~cRow(){
    delete [] this->Columns;
    delete [] this->Values;
    this->Columns = NULL;
    this->Values = NULL;
}

bool cRow::fetchColumn(cString* Column, cString* Value){
    char *Col, *Val;
    bool ret = this->fetchColumn(&Col, &Val);
    if(ret){
        *Column = cString(Col,true);
        *Value = cString(Val,true);
    }
    return ret;
}


bool cRow::fetchColumn(char** Column, char** Value){
    if (currentCol>=this->ColCount){
        return false;
    }
    MESSAGE(VERBOSE_SQL_FETCHES,"Fetching column %s='%s' (%d/%d)", this->Columns[currentCol], this->Values[currentCol], currentCol+1, this->ColCount);
    *Column = strdup0(this->Columns[currentCol]);
    if (this->Values[currentCol]){
        *Value = strcasecmp(this->Values[currentCol],"NULL")?strdup(this->Values[currentCol]):NULL;
    }
    else {
        *Value = NULL;
    }
    currentCol++;
    return true;
}

int cSQLiteDatabase::initialize(){
    int ret;
    const char* dbdir = (cUPnPConfig::get()->mDatabaseFolder) ? cUPnPConfig::get()->mDatabaseFolder : cPluginUpnp::getConfigDirectory();
    cString File = cString::sprintf("%s/%s", dbdir, SQLITE_DB_FILE);
    if ((ret = sqlite3_open(File, &this->mDatabase))){
        ERROR("Unable to open database file %s (Error code: %d)!", *File, ret);
        sqlite3_close(this->mDatabase);
        return -1;
    }
	char* Error;
	if (sqlite3_exec(this->mDatabase, "PRAGMA synchronous = OFF", NULL, NULL, &Error)){
		ERROR("Error while setting PRAGMA OFF: %s", Error);
	}
	if (sqlite3_exec(this->mDatabase, "PRAGMA journal_mode = MEMORY", NULL, NULL, &Error)){
		ERROR("Error while setting journal mode to memory: %s", Error);
	}
    MESSAGE(VERBOSE_SDK,"Database file %s opened. SQLITE version: %s", *File, sqlite3_libversion());
    if (this->initializeTables()){
        ERROR("Error while creating tables");
        return -1;
    }
    else if(this->initializeTriggers()){
        ERROR("Error while setting triggers");
        return -1;
    }
    return 0;
}

void cSQLiteDatabase::startTransaction(){
    if (this->mActiveTransaction){
        if (this->mAutoCommit){
            this->commitTransaction();
        }
        else {
            this->rollbackTransaction();
        }
    }
    this->execStatement("BEGIN TRANSACTION");
//    MESSAGE(VERBOSE_SQL,"Start new transaction");
    this->mActiveTransaction = true;
}

void cSQLiteDatabase::commitTransaction(){
    this->execStatement("COMMIT TRANSACTION");
//    MESSAGE(VERBOSE_SQL,"Commited transaction");
    this->mActiveTransaction = false;
}

void cSQLiteDatabase::rollbackTransaction(){
    this->execStatement("ROLLBACK TRANSACTION");
    MESSAGE(VERBOSE_SQL,"Rolled back transaction");
    this->mActiveTransaction = false;
}

int cSQLiteDatabase::initializeParams(){
    this->mActiveTransaction = false;
    this->mDatabase = NULL;
    this->mLastRow = NULL;
    this->mRows = NULL;
	this->mObjInsStmt = NULL;
	this->mObjDelStmt = NULL;
	this->mObjSelStmt = NULL;
	this->mObjParSelStmt = NULL;
	this->mObjClsSelStmt = NULL;
	this->mObjUpdStmt = NULL;
	this->mConSelStmt = NULL;
	this->mConInsStmt = NULL;
	this->mPKSelStmt  = NULL;
	this->mSysUpdStmt = NULL;
	this->mSysSelStmt = NULL;
	this->mItmSelStmt = NULL;
	this->mItmInsStmt = NULL;
	this->mItmDelStmt = NULL;
	this->mIfdSelStmt = NULL;
	this->mIfdInsStmt = NULL;
	this->mResSelObjStmt = NULL;
	this->mSClSelStmt = NULL;
	this->mSClInsStmt = NULL;
	this->mVidInsStmt = NULL;
	this->mVidSelStmt = NULL;
	this->mAudInsStmt = NULL;
	this->mAudSelStmt = NULL;
	int ret = 0;
	if (pthread_mutex_init(&mutex_container, NULL) != 0){
		ERROR("pthread_mutex_init failed for UPnP container mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_object, NULL) != 0){
		ERROR("pthread_mutex_init failed for UPnP object mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_item, NULL) != 0){
		ERROR("pthread_mutex_init failed for item mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_audioItem, NULL) != 0){
		ERROR("pthread_mutex_init failed for audio item mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_videoItem, NULL) != 0){
		ERROR("pthread_mutex_init failed for video item mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_bcEvent, NULL) != 0){
		ERROR("pthread_mutex_init failed for broadcast event mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_movie, NULL) != 0){
		ERROR("pthread_mutex_init failed for movie mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_audioRec, NULL) != 0){
		ERROR("pthread_mutex_init failed for audio record mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_epgChannel, NULL) != 0){
		ERROR("pthread_mutex_init failed for epg channel mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_recTimer, NULL) != 0){
		ERROR("pthread_mutex_init failed for record timer mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_audioBC, NULL) != 0){
		ERROR("pthread_mutex_init failed for audio broadcast mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_videoBC, NULL) != 0){
		ERROR("pthread_mutex_init failed for video broadcast mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_searchClass, NULL) != 0){
		ERROR("pthread_mutex_init failed for search class mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_resource, NULL) != 0){
		ERROR("pthread_mutex_init failed with resource mutex");
		ret = -1;
	}
	if (pthread_mutex_init(&mutex_primKey, NULL) != 0){
		ERROR("pthread_mutex_init failed with primary key mutex");
		ret = -1;
	}
	return ret;
}

int cSQLiteDatabase::initializeTables(){
    int ret = 0;
    this->startTransaction();
    if(this->execStatement(SQLITE_CREATE_TABLE_ITEMFINDER)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_SYSTEM)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_PRIMARY_KEYS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_ALBUMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_AUDIOBROADCASTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_AUDIOITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_CONTAINER)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_IMAGEITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_ITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_MOVIES)==-1) ret = -1; 
    if(this->execStatement(SQLITE_CREATE_TABLE_AUDIORECORDS)==-1) ret = -1; 
    if(this->execStatement(SQLITE_CREATE_TABLE_OBJECTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_PHOTOS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_PLAYLISTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_RESOURCES)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_SEARCHCLASS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_VIDEOBROADCASTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_CREATE_TABLE_VIDEOITEMS)==-1) ret = -1;
    if(ret){
        this->rollbackTransaction();
    }
    else {
        this->commitTransaction();
    }
    return ret;
}

int cSQLiteDatabase::initializeTriggers(){
    int ret = 0;
    this->startTransaction();
    if(this->execStatement(SQLITE_TRIGGER_UPDATE_SYSTEM)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_UPDATE_OBJECTID)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_D_AUDIOITEMS_AUDIOBROADCASTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_D_CONTAINERS_ALBUMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_D_CONTAINERS_PLAYLISTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_D_IMAGEITEMS_PHOTOS)==-1) ret = -1;

    if(this->execStatement(SQLITE_TRIGGER_D_VIDEOITEMS_MOVIES)==-1) ret = -1;   
    if(this->execStatement(SQLITE_TRIGGER_D_AUDIOITEMS_AUDIORECORDS)==-1) ret = -1;   
    if(this->execStatement(SQLITE_TRIGGER_D_VIDEOITEMS_VIDEOBROADCASTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_AUDIOITEMS_AUDIOBROADCASTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_CONTAINERS_ALBUMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_CONTAINERS_PLAYLISTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_CONTAINERS_SEARCHCLASSES)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_IMAGEITEMS_PHOTOS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_ITEMS_AUDIOITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_ITEMS_IMAGEITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_ITEMS_ITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_ITEMS_VIDEOITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_OBJECTS_OBJECTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_OBJECT_CONTAINERS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_OBJECT_ITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_OBJECT_RESOURCES)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_I_VIDEOITEMS_MOVIES)==-1) ret = -1; 
    if(this->execStatement(SQLITE_TRIGGER_I_AUDIOITEMS_AUDIORECORDS)==-1) ret = -1;  
    if(this->execStatement(SQLITE_TRIGGER_I_VIDEOITEMS_VIDEOBROADCASTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_AUDIOITEMS_AUDIOBROADCASTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_CONTAINERS_ALBUMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_CONTAINERS_PLAYLISTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_CONTAINERS_SEARCHCLASSES)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_IMAGEITEMS_PHOTOS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_ITEMS_AUDIOITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_ITEMS_IMAGEITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_ITEMS_ITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_ITEMS_VIDEOITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_OBJECTS_OBJECTS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_OBJECT_CONTAINERS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_OBJECT_ITEMS)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_OBJECT_RESOURCES)==-1) ret = -1;
    if(this->execStatement(SQLITE_TRIGGER_U_VIDEOITEMS_MOVIES)==-1) ret = -1; 
    if(this->execStatement(SQLITE_TRIGGER_U_AUDIOITEMS_AUDIORECORDS)==-1) ret = -1; 
    if(this->execStatement(SQLITE_TRIGGER_U_VIDEOITEMS_VIDEOBROADCASTS)==-1) ret = -1;
    if(ret){
        this->rollbackTransaction();
    }
    else {
        this->commitTransaction();
    }
    return ret;
}

long cSQLiteDatabase::getLastInsertRowID() const {
    return (long)sqlite3_last_insert_rowid(this->mDatabase);
}

sqlite3_stmt* cSQLiteDatabase::getObjectInsertStatement(){
	if (this->mObjInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT INTO %s (%s,%s,%s,%s,%s) VALUES (:ID,:PA,@CS,@TL,:RD)",
               SQLITE_TABLE_OBJECTS, SQLITE_COL_OBJECTID, SQLITE_COL_PARENTID, SQLITE_COL_CLASS, SQLITE_COL_TITLE, SQLITE_COL_RESTRICTED);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mObjInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a object insertion: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mObjInsStmt;
}

sqlite3_stmt* cSQLiteDatabase::getObjectDeleteStatement(){
	if (this->mObjDelStmt == NULL){
		cString resStatement = cString::sprintf("DELETE FROM %s WHERE %s=:ID", SQLITE_TABLE_OBJECTS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mObjDelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a object delete: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mObjDelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getObjectSelectStatement(){
	if (this->mObjSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s,%s,%s,%s FROM %s WHERE %s=:ID", SQLITE_COL_PARENTID, 
			SQLITE_COL_CLASS, SQLITE_COL_TITLE, SQLITE_COL_RESTRICTED, SQLITE_COL_CREATOR, SQLITE_COL_WRITESTATUS,
			SQLITE_TABLE_OBJECTS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mObjSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a object select: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mObjSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getObjectParentSelectStatement(){
	if (this->mObjParSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s=:ID", SQLITE_COL_OBJECTID,
			                 SQLITE_TABLE_OBJECTS, SQLITE_COL_PARENTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mObjParSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a object parent select: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mObjParSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getObjectClassSelectStatement(){
	if (this->mObjClsSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s=:ID", SQLITE_COL_CLASS,
			                 SQLITE_TABLE_OBJECTS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mObjClsSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a object class select: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mObjClsSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getObjectUpdateStatement(){
	if (this->mObjUpdStmt == NULL){
		cString resStatement = cString::sprintf("UPDATE %s SET %s=:OI,%s=:PI,%s=@CS,%s=@TL,%s=:RD,%s=@CR,%s=:WS WHERE %s=:OL", 
	                        SQLITE_TABLE_OBJECTS, SQLITE_COL_OBJECTID, SQLITE_COL_PARENTID, SQLITE_COL_CLASS, SQLITE_COL_TITLE,
							SQLITE_COL_RESTRICTED, SQLITE_COL_CREATOR, SQLITE_COL_WRITESTATUS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mObjUpdStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a object update: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mObjUpdStmt;
}

sqlite3_stmt* cSQLiteDatabase::getContainerSelectStatement(){
	if (this->mConSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s FROM %s WHERE %s=:ID", SQLITE_COL_DLNA_CONTAINERTYPE, 
			                 SQLITE_COL_CONTAINER_UID, SQLITE_COL_SEARCHABLE, SQLITE_TABLE_CONTAINERS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mConSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a container select: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mConSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getContainerInsertStatement(){
    if (this->mConInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s,%s,%s) VALUES (:ID,@DL,:SE,:UI)", SQLITE_TABLE_CONTAINERS,
			                    SQLITE_COL_OBJECTID, SQLITE_COL_DLNA_CONTAINERTYPE, SQLITE_COL_SEARCHABLE, SQLITE_COL_CONTAINER_UID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mConInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a container insertion: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mConInsStmt;
}

sqlite3_stmt* cSQLiteDatabase::getPKSelectStatement(){
	if (this->mPKSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s=:ID",
			                 SQLITE_COL_KEY, SQLITE_TABLE_PRIMARY_KEYS, SQLITE_COL_KEYID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mPKSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement with a primary key selection: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mPKSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getSystemUpdateStatement(){
	if (this->mSysUpdStmt == NULL){
		cString resStatement = cString::sprintf("UPDATE %s SET %s=%s+1 WHERE %s='%s'",
			                 SQLITE_TABLE_SYSTEM, SQLITE_COL_VALUE, SQLITE_COL_VALUE, SQLITE_COL_KEY_SYSTEM, KEY_SYSTEM_UPDATE_ID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mSysUpdStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for the system update: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mSysUpdStmt;
}

sqlite3_stmt* cSQLiteDatabase::getSystemSelectStatement(){
	if (this->mSysSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s='%s'",
			                 SQLITE_COL_VALUE, SQLITE_TABLE_SYSTEM, SQLITE_COL_KEY_SYSTEM, KEY_SYSTEM_UPDATE_ID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mSysSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for the system select: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mSysSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getItemSelectStatement(){
    if (this->mItmSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s=:ID", SQLITE_COL_REFERENCEID,
			                                    SQLITE_TABLE_ITEMS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mItmSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for an item selection: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mItmSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getItemInsertStatement(){
    if (this->mItmInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s) VALUES (:ID,:RI)", SQLITE_TABLE_ITEMS,
			                                    SQLITE_COL_OBJECTID, SQLITE_COL_REFERENCEID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mItmInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for an item insertion: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mItmInsStmt;
}

sqlite3_stmt* cSQLiteDatabase::getItemDeleteStatement(){
	if (this->mItmDelStmt == NULL){
		cString resStatement = cString::sprintf("DELETE FROM %s WHERE %s=:ID", SQLITE_TABLE_ITEMS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mItmDelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a item delete: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mItmDelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getItemFinderSelectStatement(){
    if (this->mIfdSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s=@FI", SQLITE_COL_OBJECTID,
			                                    SQLITE_TABLE_ITEMFINDER, SQLITE_COL_ITEMFINDER);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mIfdSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a selection from table ItemFinder: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mIfdSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getItemFinderInsertStatement(){
    if (this->mIfdInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s) VALUES (:ID,@FI)", SQLITE_TABLE_ITEMFINDER,
			                                    SQLITE_COL_OBJECTID, SQLITE_COL_ITEMFINDER);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mIfdInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for an item finder insertion: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mIfdInsStmt;
}

sqlite3_stmt* cSQLiteDatabase::getResourceObjSelectStatement(){
	if (this->mResSelObjStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s=:LN", SQLITE_COL_RESOURCEID, SQLITE_TABLE_RESOURCES, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mResSelObjStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a resource ID selection: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mResSelObjStmt;
}

sqlite3_stmt* cSQLiteDatabase::getSearchClassSelectStatement(){
	if (this->mSClSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s FROM %s WHERE %s=:ID", SQLITE_COL_CLASS, SQLITE_COL_CLASSDERIVED, 
			                                    SQLITE_TABLE_SEARCHCLASS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mSClSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a search class select: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mSClSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getSearchClassInsertStatement(){
	if (this->mSClInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s,%s) VALUES (:ID,@CS,:CD)", SQLITE_TABLE_SEARCHCLASS,
			                                    SQLITE_COL_OBJECTID, SQLITE_COL_CLASS, SQLITE_COL_CLASSDERIVED);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mSClInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a search class insertion: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mSClInsStmt;
}

sqlite3_stmt* cSQLiteDatabase::getVideoInsertStatement(){
    if (this->mVidInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s) VALUES (:ID,@GR,@LD,@PD,@RT,@AC,@DR,@DS,@PB,@LG,@RL)", 
			                    SQLITE_TABLE_VIDEOITEMS, SQLITE_COL_OBJECTID, SQLITE_COL_GENRE, SQLITE_COL_LONGDESCRIPTION, 
								SQLITE_COL_PRODUCER, SQLITE_COL_RATING, SQLITE_COL_ACTOR, SQLITE_COL_DIRECTOR, SQLITE_COL_DESCRIPTION, 
								SQLITE_COL_PUBLISHER, SQLITE_COL_LANGUAGE, SQLITE_COL_RELATION);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mVidInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a video item insertion: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mVidInsStmt;
}

sqlite3_stmt* cSQLiteDatabase::getVideoSelectStatement(){
    if (this->mVidSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s,%s,%s,%s,%s,%s,%s,%s,%s,%s FROM %s WHERE %s=:ID", SQLITE_COL_GENRE, 
			                     SQLITE_COL_LONGDESCRIPTION, SQLITE_COL_PRODUCER, SQLITE_COL_RATING, SQLITE_COL_ACTOR, SQLITE_COL_DIRECTOR,
								 SQLITE_COL_DESCRIPTION, SQLITE_COL_PUBLISHER, SQLITE_COL_LANGUAGE, SQLITE_COL_RELATION,
			                     SQLITE_TABLE_VIDEOITEMS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mVidSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a video item selection: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mVidSelStmt;
}

sqlite3_stmt* cSQLiteDatabase::getAudioInsertStatement(){
    if (this->mAudInsStmt == NULL){
		cString resStatement = cString::sprintf("INSERT OR REPLACE INTO %s (%s) VALUES (:ID)", 
								                SQLITE_TABLE_AUDIOITEMS,  SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mAudInsStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for a audio item insertion: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mAudInsStmt;
}

sqlite3_stmt* cSQLiteDatabase::getAudioSelectStatement(){
    if (this->mAudSelStmt == NULL){
		cString resStatement = cString::sprintf("SELECT %s FROM %s WHERE %s=:ID", SQLITE_COL_GENRE,
			                                    SQLITE_TABLE_AUDIOITEMS, SQLITE_COL_OBJECTID);
		const char *zSql = *resStatement;
		const char **pzTail = NULL;	
		if (sqlite3_prepare_v2(this->mDatabase, zSql, strlen(zSql)+1, &(this->mAudSelStmt), pzTail) != SQLITE_OK ){
			ERROR("Error when compiling the SQL statement for an audio item selection: %s", sqlite3_errmsg(this->mDatabase));	
		}
	}
	return this->mAudSelStmt;
}