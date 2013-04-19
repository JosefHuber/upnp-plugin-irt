/*
 * File:   mediator.h
 * Author: savop
 * Author: J. Huber, IRT GmbH
 * Created on 11. September 2009, 20:39
 * Last modification on February 19, 2013
 */

#ifndef _MEDIATOR_H
#define	_MEDIATOR_H

#include <pthread.h>

/**
 * Mediator interface
 *
 * This is an interface for mediators used to communicate with the database.
 * A mediator is applied to get, create, save or delete an UPnP object.
 */
class cMediatorInterface {
public:
    virtual ~cMediatorInterface(){};
    /**
     * Create an UPnP object
     *
     * This creates a new UPnP object with the specific title and the restriction.
     *
     * @return the newly created object
     * @param Title the title of that object
     * @param Restricted the restriction of the object
     */
    virtual cUPnPClassObject* createObject(const char* Title, bool Restricted) = 0;
    /**
     * Get an UPnP object
     *
     * Retrieves an UPnP object from the database and stores its information in the
     * object. The object is obtained via its object ID.
     *
     * @return the object, found in the database
     * @param ID the object ID
     */
    virtual cUPnPClassObject* getObject(cUPnPObjectID ID) = 0;
    /**
     * Save the UPnP object
     *
     * This saves the object in the database by updating the values in the database
     * with those in the object.
     *
     * @return returns
     * - \bc <0, in case of an error
     * - \bc 0, otherwise
     * @param Object the <code>cUPnPClassObject</code> instance to be saved
     */
    virtual int saveObject(cUPnPClassObject* Object) = 0;
    /**
     * Delete the UPnP object
     *
     * This deletes the object in the database by removing all of its possible children and then
     * deleting the contents from the database.
     *
     * @return returns
     * - \bc <0, in case of an error
     * - \bc 0, otherwise
     * @param Object the <code>cUPnPClassObject</code> instance to be deleted
     */
    virtual int deleteObject(cUPnPClassObject* Object) = 0;
    /**
     * Clear the UPnP object
     *
     * This clears the object, i.e. all its children will be removed and deleted
     * from the database.
     *
     * @return returns
     * - \bc <0, in case of an error
     * - \bc 0, otherwise
     * @param Object the <code>cUPnPClassObject</code> instance to be cleared
     */
    virtual int clearObject(cUPnPClassObject* Object) = 0;
};

typedef std::map<const char*, cMediatorInterface*, strCmp> tMediatorMap;

/**
 * The object factory
 *
 * This factory can create, delete, clear or save UPnP objects. It uses mediators
 * to communicate with the persistance database to load or persist the objects.
 *
 * If a new type of object shall be stored in the database an according mediator
 * is needed, which knows the internal database structure. It must implement the
 * <code>cMediatorInterface</code> class to work with this factory.
 */
class cUPnPObjectFactory {
private:
    static cUPnPObjectFactory*            mInstance;
    cSQLiteDatabase*                      mDatabase;
    tMediatorMap                          mMediators;
    cMediatorInterface* findMediatorByID(cUPnPObjectID ID);
    cUPnPObjectFactory();

public:
    /**
     * Return the instance of the factory
     *
     * This returns the instance of the factory. When the SQLite media database is
     * initialized successfully, it usually has all known mediators already
     * registered.
     *
     * @return the instance of the factory
     */
    static cUPnPObjectFactory* getInstance();
    /**
     * Register a mediator
     *
     * This registers a new mediator by the associated UPnP class. The mediator
     * must implement the <code>cMediatorInterface</code> class to be used with this
     * factory.
     *
     * @param UPnPClass the class of which the mediator is associated to
     * @param Mediator the mediator itself
     */
    void registerMediator(const char* UPnPClass, cMediatorInterface* Mediator);
    /**
     * Unregisters a mediator
     *
     * This unregisters a mediator if it is not needed any longer. If the optional
     * parameter \c freeMediator is set, the object instance will be free'd after
     * removing it from the list.
     *
     * @param UPnPClass the class of the associated mediator
     * @param freeMediator flag to indicate if the mediator shall be free'd after removing
     */
    void unregisterMediator(const char* UPnPClass, bool freeMediator=true);
    /**
     * @copydoc cMediatorInterface::createObject(const char* Title, bool Restricted)
     *
     * @param UPnPClass the class of the new object
     */
    cUPnPClassObject* createObject(const char* UPnPClass, const char* Title, bool Restricted=true);
    /*! @copydoc cMediatorInterface::getObject(cUPnPObjectID ID) */
    cUPnPClassObject* getObject(cUPnPObjectID ID);
    /*! @copydoc cMediatorInterface::saveObject(cUPnPClassObject* Object) */
    int saveObject(cUPnPClassObject* Object);
    /*! @copydoc cMediatorInterface::deleteObject(cUPnPClassObject* Object) */
    int deleteObject(cUPnPClassObject* Object);
    /*! @copydoc cMediatorInterface::clearObject(cUPnPClassObject* Object) */
    int clearObject(cUPnPClassObject* Object);
    /**
	 * Find a <code>cMediatorInterface</code> instance by an UPnP class.
	 * @param Class the UPnP class
	 * @return the <code>cMediatorInterface</code> instance or NULL if not successful
	 */
    cMediatorInterface* findMediatorByClass(const char* Class);
};

class cMediaDatabase;

/**
 * UPnP Object Mediator
 *
 * This is the interface between UPnP objects and the database. It is possible to
 * create new <code>cUPnPClassObject</code> instances, store them in the database as well as removing them from
 * it.
 */
class cUPnPObjectMediator : public cMediatorInterface {
protected:
    cSQLiteDatabase*        mDatabase;                  ///< the SQLite 3 database wrapper
    cMediaDatabase*         mMediaDatabase;             ///< the media database
    /**
     * Construction of an object mediator
     *
     * This constructs a new object mediator. A direct call of this constructor is actually not allowed because
     * it is prohibited to create instances of the UPnP class Object. Only derived classes can be instantiated.
     */
    cUPnPObjectMediator(
        cMediaDatabase* MediaDatabase                   ///< the <code>cMediaDatabase</code> instance
    );
    /**
     * Initialize an UPnP object
     *
     * This initializes an UPnP object, which means, that it will be created in the database with
     * the required details.
     *
     * @return returns
     * - \bc <0, in case of an error
     * - \bc 0, otherwise
     */
    virtual int initializeObject(
        cUPnPClassObject* Object,                       ///< the object to be initialized
        const char* Class,                              ///< the class of the object
        const char* Title,                              ///< the title of the object
        bool Restricted                                 ///< restriction of the object
    );
    /**
     * Store the UPnP object in the database
     *
     * This stores the information of an object in the database.
     *
     * @return returns
     * - \bc <0, in case of an error
     * - \bc 0, otherwise
     * @param Object the <code>cUPnPClassObject</code> instance to be saved
     */
    virtual int objectToDatabase(cUPnPClassObject* Object);
    /**
     * Load an UPnP object from the database
     *
     * This loads an object from the database.
     *
     * @return returns
     * - \bc <0, in case of an error
     * - \bc 0, otherwise
     * @param Object the <code>cUPnPClassObject</code> instance to be loaded
     * @param ID the object ID of that object
     */
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);
	/**
	 * Delete an UPnP object with ID objId from the SQLite table 'objects'
	 * @param objId the object ID for the content to be deleted
	 * @return true if successfull, false if not
	 */
	bool deleteObjectProc(int objId);	
public:
    virtual ~cUPnPObjectMediator();
    /*! @copydoc cMediatorInterface::createObject(const char* Title, bool Restricted) */
    virtual cUPnPClassObject* createObject(const char* Title, bool Restricted);
    /*! @copydoc cMediatorInterface::getObject(cUPnPObjectID ID) */
    virtual cUPnPClassObject* getObject(cUPnPObjectID ID);
    /*! @copydoc cMediatorInterface::saveObject(cUPnPClassObject* Object) */
    virtual int saveObject(cUPnPClassObject* Object);
    /*! @copydoc cMediatorInterface::deleteObject(cUPnPClassObject* Object) */
    virtual int deleteObject(cUPnPClassObject* Object);
    /*! @copydoc cMediatorInterface::clearObject(cUPnPClassObject* Object) */
    virtual int clearObject(cUPnPClassObject* Object);
};

/**
 * UPnP Item Mediator
 *
 * This is the interface between the UPnP item objects and the database. It is possible to
 * create new <code>cUPnPClassItem</code> instances, store them in the database as well as removing them from
 * it.
 */
class cUPnPItemMediator : public cUPnPObjectMediator {
protected:
    /*! @copydoc cUPnPObjectMediator::objectToDatabase(cUPnPClassObject* Object) */
    virtual int objectToDatabase(cUPnPClassObject* Object);
    /*! @copydoc cUPnPObjectMediator::databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID) */
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);	
public:
    /**
     * Construction of an UPnP item mediator
     *
     * This creates a new item mediator used with database accesses.
     *
     * @param MediaDatabase the <code>cMediaDatabase</code> instance
     */
    cUPnPItemMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPItemMediator(){};
    /*! @copydoc cUPnPObjectMediator::createObject(const char* Title, bool Restricted) */
    virtual cUPnPClassItem* createObject(const char* Title, bool Restricted);
    /*! @copydoc cUPnPObjectMediator::getObject(cUPnPObjectID ID) */
    virtual cUPnPClassItem* getObject(cUPnPObjectID ID);
    /*! @copydoc cMediatorInterface::deleteObject(cUPnPClassObject* Object) */
    virtual int deleteObject(cUPnPClassObject* Object);	
};

/**
 * Audio Item Mediator
 *
 * This is the interface between the audio item objects and the database. It is possible to
 * create new <code>cUPnPClassAudioItem</code> instances, store them in the database as well as removing them from
 * it.
 */
class cUPnPAudioItemMediator : public cUPnPItemMediator {
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);
public:
    /**
     * Construction of an audio item mediator
     *
     * This creates a new audio item mediator used with database accesses.
     *
     * @param MediaDatabase the <code>cMediaDatabase</code> instance
     */
    cUPnPAudioItemMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPAudioItemMediator(){};
    virtual cUPnPClassAudioItem* createObject(const char* Title, bool Restricted);
    virtual cUPnPClassAudioItem* getObject(cUPnPObjectID ID);
};

/**
 * VideoItem Mediator
 *
 * This is the interface between the video item objects and the database. It is possible to
 * create new <code>cUPnPClassVideoItem</code> instances, store them in the database as well as removing them from
 * it.
 */
class cUPnPVideoItemMediator : public cUPnPItemMediator {
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);
public:
    /**
     * Construction of a video item mediator
     *
     * This creates a new video item mediator used with database accesses.
     *
     * @param MediaDatabase the <code>cMediaDatabase</code> instance
     */
    cUPnPVideoItemMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPVideoItemMediator(){};
    virtual cUPnPClassVideoItem* createObject(const char* Title, bool Restricted);
    virtual cUPnPClassVideoItem* getObject(cUPnPObjectID ID);
};

/**
 * Audio Broadcast Mediator
 *
 * This is the interface between the audio broadcast objects and the database. It is possible to
 * create new <code>cUPnPClassAudioBroadcast</code> instances, store them in the database as well as removing them from
 * it.
 */
class cUPnPAudioBroadcastMediator : public cUPnPAudioItemMediator {
private:
    sqlite3_stmt* mAbcInsStmt;      // the compiled SQLite statement for an audio broadcast insertion
	sqlite3_stmt* mAbcSelStmt;      // the compiled SQLite statement for an audio broadcast selection
	sqlite3_stmt* mAbcDelStmt;      // the compiled SQLite statement for an audio broadcast deletion
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);
public:
    /**
     * Construction of an audio broadcast mediator
     *
     * This creates a new audio broadcast mediator used with database accesses.
     *
     * @param MediaDatabase the <code>cMediaDatabase</code> instance
     */
    cUPnPAudioBroadcastMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPAudioBroadcastMediator(){};
    virtual cUPnPClassAudioBroadcast* createObject(const char* Title, bool Restricted);
    virtual cUPnPClassAudioBroadcast* getObject(cUPnPObjectID ID);
    /*! @copydoc cMediatorInterface::deleteObject(cUPnPClassObject* Object) */
    virtual int deleteObject(cUPnPClassObject* Object);
    /*! @copydoc cMediatorInterface::saveObject(cUPnPClassObject* Object) */
//    virtual int saveObject(cUPnPClassObject* Object);	
};

/**
 * Video Broadcast Mediator
 *
 * This is the interface between the video broadcast objects and the database. It is possible to
 * create new <code>cUPnPClassVideoBroadcast</code> instances, store them in the database as well as removing them from
 * it.
 */
class cUPnPVideoBroadcastMediator : public cUPnPVideoItemMediator {
private:
    sqlite3_stmt* mVbcInsStmt;   // the compiled SQLite statement for a video broadcast insertion
	sqlite3_stmt* mVbcSelStmt;   // the compiled SQLite statement for a video broadcast selection
	sqlite3_stmt* mVbcDelStmt;   // the compiled SQLite statement for a video broadcast deletion
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);
public:
    /**
     * Construction of a video broadcast mediator
     *
     * This creates a new video broadcast mediator for database accesses.
     *
     * @param MediaDatabase the media database
     */
    cUPnPVideoBroadcastMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPVideoBroadcastMediator(){};
    virtual cUPnPClassVideoBroadcast* createObject(const char* Title, bool Restricted);
    virtual cUPnPClassVideoBroadcast* getObject(cUPnPObjectID ID);
    /*! @copydoc cMediatorInterface::deleteObject(cUPnPClassObject* Object) */
    virtual int deleteObject(cUPnPClassObject* Object);	
    /*! @copydoc cMediatorInterface::saveObject(cUPnPClassObject* Object) */
 //   virtual int saveObject(cUPnPClassObject* Object);	
};

/**
 * Audio Record Mediator
 *
 * This is the interface between the audio record objects and the database. It is possible to
 * create new <code>cUPnPClassAudioRecord</code> instances, store them in the database as well as removing them from
 * it.
 */
class cUPnPAudioRecordMediator : public cUPnPAudioItemMediator {
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);

public:
    /**
     * Construction of an audio record mediator
     *
     * This creates a new audio record mediator. It can create new
     * instances of <code>cUPnPClassAudioRecord</code> and perform database accesses.
     *
     * @param MediaDatabase the <code>cMediaDatabase</code> instance
     */
    cUPnPAudioRecordMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPAudioRecordMediator(){};
    /*! @copydoc cMediatorInterface::createObject(const char* Title, bool Restricted) */
    virtual cUPnPClassAudioRecord* createObject(const char* Title, bool Restricted);
    virtual cUPnPClassAudioRecord* getObject(cUPnPObjectID ID);
};

/**
 * Movie Mediator
 *
 * This is the interface between the movie objects and the database. It is possible to
 * create new <code>cUPnPClassMovie</code> instances, store them in the database as well as removing them from
 * it.
 */
class cUPnPMovieMediator : public cUPnPVideoItemMediator {
private:
	sqlite3_stmt* mMovSelStmt;     // the compiled SQLite statement for a movie selection
	sqlite3_stmt* mMovInsStmt;     // the compiled SQLite statement for a movie insertion
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);

public:
    /**
     * Construction of a movie mediator
     *
     * This creates a new movie mediator for database accesses.
     *
     * @param MediaDatabase the <code>cMediaDatabase</code> instance
     */
    cUPnPMovieMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPMovieMediator(){};
    /*! @copydoc cMediatorInterface::createObject(const char* Title, bool Restricted) */
    virtual cUPnPClassMovie* createObject(const char* Title, bool Restricted);
    virtual cUPnPClassMovie* getObject(cUPnPObjectID ID);
};

/**
 * EPG Item Mediator
 *
 * This is the interface between UPnP EPG items and the database. With this class it is possible to
 * create new <code>cUPnPClassEpgItem</code> instances, store them in the database as well as removing them from
 * it.
 */
class cUPnPEpgItemMediator : public cUPnPItemMediator {
private:
    sqlite3_stmt* mBceDelStmt;     // the compiled SQLite statement for an EPG broadcast event deletion
    sqlite3_stmt* mBceSelStmt;     // the compiled SQLite statement for an EPG broadcast event selection
    sqlite3_stmt* mBceInsStmt;     // the compiled SQLite statement for an EPG broadcast event insertion
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);
public:
    /**
     * Construction of an EPG item mediator
     *
     * This creates a new EPG item mediator used with database acesses.
     *
     * @param MediaDatabase the media database
     */
    cUPnPEpgItemMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPEpgItemMediator(){};
    virtual cUPnPClassEpgItem* createObject(const char* Title, bool Restricted);
    virtual cUPnPClassEpgItem* getObject(cUPnPObjectID ID);
    /*! @copydoc cMediatorInterface::deleteObject(cUPnPClassObject* Object) */
    virtual int deleteObject(cUPnPClassObject* Object);
    /*! @copydoc cMediatorInterface::saveObject(cUPnPClassObject* Object) */
//    virtual int saveObject(cUPnPClassObject* Object);	
};

/**
 * Record Timer Item Mediator
 *
 * This is the interface between record timer objects and the database. It is possible to
 * create new <code>cUPnPClassRecordTimerItem</code> objects, store them in the database as well as removing them from
 * it.
 */
class cUPnPRecordTimerItemMediator : public cUPnPItemMediator { 
private:
	sqlite3_stmt* mRtrInsStmt;     // the compiled SQLite statement for a record timer insertion
	sqlite3_stmt* mRtrSelStmt;     // the compiled SQLite statement for a record timer selection
	sqlite3_stmt* mRtrDelStmt;     // the compiled SQLite statement for a record timer deletion
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);
public:
	/**
	 * Construction of a record timer item mediator used with database accesses.
	 * @param MediaDatabase the <code>cMediaDatabase</code> instance
	 */
	cUPnPRecordTimerItemMediator(cMediaDatabase* MediaDatabase);
	virtual ~cUPnPRecordTimerItemMediator(){};
    virtual cUPnPClassRecordTimerItem* createObject(const char* Title, bool Restricted);
    virtual cUPnPClassRecordTimerItem* getObject(cUPnPObjectID ID);
    /*! @copydoc cMediatorInterface::deleteObject(cUPnPClassObject* Object) */
    virtual int deleteObject(cUPnPClassObject* Object);
    /*! @copydoc cMediatorInterface::saveObject(cUPnPClassObject* Object) */
    virtual int saveObject(cUPnPClassObject* Object);		
};

/**
 * General UPnP Container Mediator
 *
 * This is the interface between UPnP containers and the database. It is possible to
 * create new <code>cUPnPClassContainer</code> objects, store them in the database as well as removing them from
 * it.
 */
class cUPnPContainerMediator : public cUPnPObjectMediator {
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);
public:
    /**
     * Construction of an UPnP container mediator
     *
     * This creates a new container mediator. It can create new
     * instances of UPnP containers and perform database accesses.
     *
     * @param MediaDatabase the media database
     */
    cUPnPContainerMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPContainerMediator(){};
    /*! @copydoc cMediatorInterface::createObject(const char* Title, bool Restricted) */
    virtual cUPnPClassContainer* createObject(const char* Title, bool Restricted);
    /*! @copydoc cMediatorInterface::getObject(cUPnPObjectID ID) */
    virtual cUPnPClassContainer* getObject(cUPnPObjectID ID);
};

/**
 * EPG Container Mediator
 *
 * This is the interface between UPnP EPG containers and the database. It is possible to
 * create new <code>cUPnPClassEpgContainer</code> objects, store them in the database as well as removing them from
 * it.
 */
class cUPnPEpgContainerMediator : public cUPnPContainerMediator {
private:
     sqlite3_stmt* mChnInsStmt;    // the compiled SQLite statement for an EPG channel insertion
	 sqlite3_stmt* mChnSelStmt;    // the compiled SQLite statement for an EPG channel selection
	 sqlite3_stmt* mChnDelStmt;    // the compiled SQLite statement for an EPG channel deletion
protected:
    virtual int objectToDatabase(cUPnPClassObject* Object);
    virtual int databaseToObject(cUPnPClassObject* Object, cUPnPObjectID ID);
public:
    /**
     * Construction of an EPG container mediator
     *
     * This creates a new UPnP EPG container mediator used with database accesses.
     *
     * @param MediaDatabase the <code>cMediaDatabase</code> instance
     */
    cUPnPEpgContainerMediator(cMediaDatabase* MediaDatabase);
    virtual ~cUPnPEpgContainerMediator(){};
    /*! @copydoc cMediatorInterface::createObject(const char* Title, bool Restricted) */
    virtual cUPnPClassEpgContainer* createObject(const char* Title, bool Restricted);
    /*! @copydoc cMediatorInterface::deleteObject(cUPnPClassObject* Object) */
    virtual int deleteObject(cUPnPClassObject* Object);
    virtual cUPnPClassEpgContainer* getObject(cUPnPObjectID ID);
};
#endif	/* _MEDIATOR_H */