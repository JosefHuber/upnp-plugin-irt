/*
 * File:   object.h
 * Author: savop
 * Author: J. Huber, IRT GmbH
 * Created on 11. September 2009, 20:39
 * Modified on February 21, 2013
 */

#ifndef _OBJECT_H
#define	_OBJECT_H

#include "database.h"
#include "../common.h"
#include "util.h"
#include <string.h>
#include <vdr/tools.h>
#include <map>
#include <vector>
#include <upnp/ixml.h>
#include "split.h"

#define ROOT_ID    0
#define VIDEO_ID   1
#define AUDIO_ID   2
#define TV_ID      3
#define V_RECORDS_ID 4
#define RADIO_ID   5
#define CUSTOMVIDEOS_ID 6
#define EPG_ID   7
#define A_RECORDS_ID 8
#define EPG_TV_ID 9
#define EPG_RADIO_ID 10
#define REC_TIMER_ID 11
#define REC_TIMER_TV_ID 12
#define REC_TIMER_RADIO_ID 13

#define AUDIO_COMPONENTS_THRESHOLD   3
/**
 * UPnP Object ID
 *
 * This is a UPnP Object ID representation.
 */
struct cUPnPObjectID {
    int _ID;                    ///< The UPnP Object ID
    /**
     * Constructor
     *
     * Creates invalid ID
     */
    cUPnPObjectID():_ID(-1){}
    /**
     * Constructor
     *
     * Creates from long integer
     */
    cUPnPObjectID(
        long ID         ///< new ID
    ){ _ID = (int)ID; }
    /**
     * Constructor
     *
     * Creates from integer
     */
    cUPnPObjectID(
        int ID          ///< new ID
    ){ _ID = ID; }
    /** Set the object ID */
    cUPnPObjectID &operator=(
        long ID         ///< new ID
    ){ _ID = ID; return *this; }
    /** @overload cUPnPObjectID &operator=(long ID) */
    cUPnPObjectID &operator=(
        int ID          ///< new ID
    ){ _ID = ID; return *this; }
    /** @overload cUPnPObjectID &operator=(long ID) */
    cUPnPObjectID &operator=(
        const cUPnPObjectID& ID     ///< new ID
    ){ if(this != &ID){ _ID = ID._ID; } return *this; }
    /** Pre increment the ID */
    cUPnPObjectID &operator++(){ _ID++; return *this; }
    /** Post increment the ID */
    cUPnPObjectID operator++(int){ cUPnPObjectID old = *this; _ID++; return old; }
    /** Post decrement the ID */
    cUPnPObjectID operator--(int){ cUPnPObjectID old = *this; _ID--; return old; }
    /** Pre decrement the ID */
    cUPnPObjectID &operator--(){ _ID--; return *this; }
    /** Not equal */
    bool operator!=(
        long ID         ///< compare with this ID
    ){ return _ID != ID; }
    /** Equal */
    bool operator==(
        long ID         ///< compare with this ID
    ){ return _ID == ID; }
    /** @overload bool operator!=(long ID) */
    bool operator!=(
        int ID          ///< compare with this ID
    ){ return _ID != ID; }
    /** @overload bool operator==(long ID) */
    bool operator==(
        int ID          ///< compare with this ID
    ){ return _ID == ID; }
    /** @overload bool operator!=(long ID) */
    bool operator!=(
        const cUPnPObjectID& ID ///< compare with this ID
    ){ return *this == ID; }
    /** @overload bool operator==(long ID) */
    bool operator==(
        const cUPnPObjectID& ID ///< compare with this ID
    ){ return *this == ID; }
    /** Casts to unsigned int */
    operator unsigned int(){ return (unsigned int)_ID; }
    /** Casts to int */
    operator int(){ return _ID; }
    /** Casts to long */
    operator long(){ return (long)_ID; }
    /** Casts to string */
    const char* operator*(){ char* buf; return asprintf(&buf,"%d",_ID)?buf:NULL; }
};

/**
 * Structure of a UPnP Class
 *
 * This represents a UPnP class
 */
struct cClass {
    cString ID;             ///< The upnp class ID
    bool includeDerived;    ///< flag, to indicate if derived classes are allowed
    /**
     * Compares two classes
     *
     * @param cmp the other class to compare with
     */
    bool operator==(const cClass &cmp){ return (!strcasecmp(cmp.ID,ID) && includeDerived==cmp.includeDerived); }
    /*! @copydoc operator==(const cClass &cmp) */
    bool operator!=(const cClass &cmp){ return !(*this==cmp); }
};

class cUPnPClassObject;
class cUPnPObjectMediator;
class cUPnPContainerMediator;
class cUPnPClassContainer;
class cUPnPResource;

/**
 * List of UPnP Objects
 *
 * This is a <code>cList</code> of UPnP objects.
 * The list can be sorted by using a specific property.
 */
class cUPnPObjects : public cList<cUPnPClassObject> {
public:
    cUPnPObjects();
    virtual ~cUPnPObjects();
    /**
     * Sorts the list
     *
     * This sorts the list by a specific property and a certain direction
     */
    void SortBy(
        const char* Property,       ///< the property used for sorting
        bool Descending = false     ///< the direction of the sort
    );
};

/**
 * The UPnP Class Object
 *
 * This is an UPnP object class representation with all its properties.
 * All UPnP containers and items are derived from this class.
 */
class cUPnPClassObject : public cListObject {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPClassContainer;
private:
    cUPnPObjectID           mLastID;
	/**
	 * Check a character at position i in a string for UTF-8 compatibility.
	 * Expected is a two byte encoded character that starts with 0xC3.
	 * @param text the text to check
	 * @param i the position within the text
	 * @param c3Char if set the previous character was a 0xc3 code
	 * @param retval the position of a possibly wrong encoded character or -1 for a valid UTF-8 string
	 * @param substituted the substituted string
	 * @return the possibly modified c3Char flag
	 */
	bool checkUTF8Char (const char* text, int i, bool c3Char, int* retval, std::string *substituted);
	/**
	 * Check a character at position i in a string for UTF-8 compatibility.
	 * Expected is a two byte encoded character that starts with 0xC2.
	 * @param text the text to check
	 * @param i the position within the text
	 * @param c2Char if set the previous character was a 0xc2 code
	 * @param retval the position of a wrong character, -1 for a valid string
	 * @param substituted the possibly substituted string
	 * @return the possibly modified c2Char flag
	 */
	bool checkUTF8C2Char (const char* text, int i, bool c2Char, int* retval, std::string *substituted);
	/**
	 * Check a character at position i in a string for UTF-8 compatibility
	 * @param text the text to check
	 * @param i the position within the text
	 * @param c2Char if set the previous character was a 0xc2 UTF-8 start indicator
	 * @param c3Char if set the previous character was a 0xc3 UTF-8 start indicator
	 * @param retval the position of a possibly wrong encoded character or -1 for a valid UTF-8 string
	 * @param substituted the possibly substituted string
	 * @return a possibly modified c3Char flag
	 */
	bool checkUTF8Follower (const char* text, int i, bool* c2Char, bool c3Char, int* retval, std::string *substituted);
protected:
    time_t                  mLastModified;              ///< The last modification of this property
    cUPnPObjectID           mID;                        ///< The object ID
    cUPnPClassObject*       mParent;                    ///< The parent object
    cString                 mClass;                     ///< Class (Who am I?)
    cString                 mTitle;                     ///< Object title
    cString                 mCreator;                   ///< Creator of this object
    bool                    mRestricted;                ///< Ability of changing metadata?
    int                     mWriteStatus;               ///< Ability of writing resources?
    cList<cUPnPResource>*   mResources;                 ///< The resources of this object
    cHash<cUPnPResource>*   mResourcesID;               ///< The resources of this object as hashmap
    IXML_Document*          mDIDLFragment;              ///< The DIDL fragment of the object
    cString                 mSortCriteria;              ///< The sort criteria to sort with
    bool                    mSortDescending;            ///< The direction of the sort
    cUPnPClassObject();
    /**
     * Set the Object ID
     *
     * This is only allowed by mediators and the media database. Manually editing
     * the object ID may result in unpredictable behavior.
     *
     * @param ID the ObjectID of this object
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setID(cUPnPObjectID ID);
    /**
     * Set the Parent Object
     *
     * This is only allowed by mediators and the media database. Manually editing
     * the parent may result in unpredictable behavior.
     *
     * @param Parent the parent of this object
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setParent(cUPnPClassContainer* Parent);
    /**
     * Set the UPnP class for an UPnP object
     *
     * This is only allowed by mediators and the media database. Manually editing
     * the object class may result in unpredictable behavior.
     *
     * @param Class the UPnP class of this object
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setClass(const char* Class);
    /**
     * The method checks the storage medium, where the movie or audio record resides.
     * Valid media are defined in \link common.h \endlink
     *
     * @see common.h
     * @param StorageMedium the medium where the movie is located
     * @return returns the storage medium or NULL if not valid
     */
    char* checkStorageMedium(char* StorageMedium);
    /**
     * Set the modification time
     *
     * This sets the last modification time to the current timestamp. This is
     * used to indicate when the object was updated the last time.
     */
    void setModified(void){ this->mLastModified = time(NULL); }
    /**
     * Check a string for the maximum length. If it is too long, truncate it.
     * @return a possibly modified string
     */
    char* checkString(const char* text);
    /**
     * Check whether the text string is conforming to an UTF-8 encoding.
	 * Only a two byte encoding starting with 0xc2 or 0xc3 is assumed and only a subset of the allowed
     * characters in the two byte range is investigated.
     * @param text the text to check
	 * @param buf  the possibly substituted text if not allowed characters are detected
     * @return -1 if the text is conforming to UTF-8 encoding,
     *       	 >= 0 indicates a possibly wrong encoded character at that position
     */
    int checkSpecialChars (const char* text, char** buf);
	/**
	 * Get the substring of a text appended by three dots
	 * @param text the text to be shortened
	 * @param length the length of the substring to be obtained
	 * @return a shortened string appended by dots
	 */
	char* getSubstring (const char* text, int length);
public:
    /**
     * Last modified
     *
     * Returns when the object was modified the last time.
     *
     * @return last modification timestamp
     */
    time_t  modified() const { return this->mLastModified; }
    virtual ~cUPnPClassObject();
    /**
     * Compares a object
     *
     * This compares a given object with this object
     * It uses the SortCriteria to compare them.
     *
     * @return returns
     * - \bc >0, if the object comes after this one
     * - \bc 0, if the objects have the same property
     * - \bc <0, if the object comes before this one
     * @param ListObject the object to compare with
     */
    virtual int Compare(const cListObject& ListObject) const;
    /**
     * Get the properties of the object
     *
     * This returns a property list with all the properties which can be obtained
     * or set with \c getProperty or \c setProperty.
     *
     * @return a stringlist with the properties
     */
    virtual cStringList* getPropertyList();
    /**
     * Gets a property
     *
     * Returns the value of a specified property. The value is converted into a
     * string.
     *
     * @return returns
     * - \bc true, if the property exists
     * - \bc false, otherwise
     * @param Property the property which should be returned
     * @param Value the value of that property
     */
    virtual bool getProperty(const char* Property, char** Value) const ;
    /**
     * Sets a property
     *
     * Sets the value of a specified property. The value is converted from string
     * into the propper data type.
     *
     * @return returns
     * - \bc true, if the property exists
     * - \bc false, otherwise
     * @param Property the property which should be set
     * @param Value the value of that property
     */
    virtual bool setProperty(const char* Property, const char* Value);
    /**
     * Converts to container
     *
     * This will convert the object into a container if it is one. If not, it
     * returns \bc NULL.
     *
     * @return returns
     * - \bc NULL, if it is not a container
     * - a container representation of this object
     */
    virtual cUPnPClassContainer* getContainer(){ return NULL; }
    /**
     * Create the DIDL fragment
     *
     * This creates the DIDL-Lite fragment of the UPnP object. The fragment is written to the
     * specified \em IXML document. The details of the output can be controlled via
     * the filter stringlist.
     *
     * @return the DIDL fragment of the object
     * @param Document the IXML document where to write the contents
     * @param Filter the string list with the filter criteria
     */
    virtual IXML_Node* createDIDLFragment(IXML_Document* Document, cStringList* Filter) = 0;
    /**
     * Is this a container?
     *
     * Returns whether this object is a container or not
     *
     * @return returns
     * - \bc true, if it is a container
     * - \bc false, otherwise
     */
    bool isContainer(){ return this->getContainer()==NULL?false:true; }
    /**
     * Set the sort criteria
     *
     * This sets a certain criteria which the object can be compared with.
     *
     * @param Property the property to sort after
     * @param Descending sort the objects in descending order
     */
    void setSortCriteria(const char* Property, bool Descending = false);
    /**
     * Clears the sort criteria
     *
     * Clears the property of the sort criteria and sets the descending flag to
     * false.
     */
    void clearSortCriteria();
    /******* Setter *******/
    /**
     * Set the title
     *
     * This sets the title of the object. It is a required metadata information.
     * It must not be \bc NULL or an empty string.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Title the title of the object
     */
    int setTitle(const char* Title);
    /**
     * Set the creator
     *
     * The creator of an object is primarily the creator or owner of the object
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Creator the creator of the object
     */
    int setCreator(const char* Creator);
    /**
     * Set the restriction
     *
     * This sets the restriction flag. If the object is restricted, no modifications
     * to its metadata by the user are allowed.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Restricted \bc true, to disallow modification, \bc false to allow it
     */
    int setRestricted(bool Restricted);
    /**
     * Set the write status
     *
     * This sets the write status of a resource. With this indicator, you can set
     * the modifiabilty of resources by a control point.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Status the write status
     */
    int setWriteStatus(int Status);
    /**
     * Set the resources
     *
     * This sets the list of resources of an object. The list usally contain a
     * single resource. However, multiple resources a also very common.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Resources the resource list of this object
     */
    int setResources(cList<cUPnPResource>* Resources);
    /**
     * Add resource to list
     *
     * This adds the specified resource to the resource list of the object.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Resource the resource to be added
     */
    int addResource(cUPnPResource* Resource);
    /**
     * Remove resource from list
     *
     * This removes the specified resource from the resource list of the object.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Resource the resource to be removed
     */
    int removeResource(cUPnPResource* Resource);
    /******* Getter *******/
    /**
     * Get the object ID
     *
     * This returns the object ID of the object.
     *
     * @return the object ID
     */
    cUPnPObjectID getID() const { return this->mID; }
    /**
     * Get the parent ID
     *
     * This returns the ID of the parent container object, associated with this object.
     * It is \bc -1, if the object is the root object.
     *
     * @return the parent ID
     */
    cUPnPObjectID getParentID() const { return this->mParent?this->mParent->getID():cUPnPObjectID(-1); }
    /**
     * Get the parent object
     *
     * This returns the parent container object, associated with this object. It is
     * \bc NULL, if the object is the root object.
     *
     * @return the parent object
     */
    cUPnPClassContainer* getParent() const { return (cUPnPClassContainer*)this->mParent; }
    /**
     * Get the title
     *
     * This returns the title of the object. This may be the title of an item or
     * the folder name in case of a container.
     *
     * @return the title of the object
     */
    const char* getTitle() const { return this->mTitle; }
    /**
     * Get the object class
     *
     * This returns the object class of the object. The classes are defined by
     * the UPnP Working Committee. However, custom classes which are derived from
     * a standardized class are also possible.
     *
     * @return the class of the object
     */
    const char* getClass() const { return this->mClass; }
    /**
     * Get the creator
     *
     * This returns the creator of the object. Usually, this is the primary
     * content creator or the owner of the object
     *
     * @return the creator of the object
     */
    const char* getCreator() const { return this->mCreator; }
    /**
     * Is the resource restricted?
     *
     * Returns \bc true, if the object is restricted or \bc false, otherwise.
     * When the object is restricted, then modifications to the metadata of the
     * object are disallowed.
     *
     * @return returns
     * - \bc true, if the object is restricted
     * - \bc false, otherwise
     */
    bool isRestricted() const { return this->mRestricted; }
    /**
     * Get write status
     *
     * This returns the write status of the object. It gives information, if the
     * resource is modifiable.
     *
     * @return the write status
     */
    int getWriteStatus() const { return this->mWriteStatus; }
    /**
     * Get a resource by its ID
     *
     * Returns the resource with the specified resource ID.
     *
     * @return the resource by ID
     * @param ResourceID the resource ID of the demanded resource
     */
    cUPnPResource* getResource(unsigned int ResourceID) const { return this->mResourcesID->Get(ResourceID); }
    /**
     * Get the resources
     *
     * This returns a list with resources associated with this object.
     *
     * @return the resources of this object
     */
    cList<cUPnPResource>* getResources() const { return this->mResources; }
};

/**
 * The UPnP class Item
 *
 * This is a UPnP item class representation with all its properties.
 */
class cUPnPClassItem : public cUPnPClassObject {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPItemMediator;
    friend class cUPnPEpgItemMediator;
protected:
    cUPnPClassItem*           mReference;                                       ///< The reference item
    /**
     * Constructor of an UPnP item
     *
     * This creates a new instance of an UPnP item.
     */
    cUPnPClassItem();
    /**
     * Create the first part of a Digital Item Declaration Language (DIDL) fragment.
     * with the object ID, parent ID and restricted properties. The title property is excluded.
     * @param Document The DIDL fragment
     * @return the pointer to the <code>IXML_Element</code> object
     */
    IXML_Element* createDIDLSubFragment1(IXML_Document* Document);
    /**
     * Create the second part of a DIDL fragment
     * with the creator, write status, reference ID and the resources properties.
     * @param Filter the string list with the filter criteria
	 * @param eItem the pointer to the <code>IXML_Element</code> instance obtained with the method <code>createDIDLSubFragment1</code>
	 * @return the pointer to the <code>IXML_Node</code> object
     */
    IXML_Node* createDIDLSubFragment2(cStringList* Filter, IXML_Element* eItem);
    /**
     * Create the resource part of a DIDL fragment.
     * @param Filter the string list with the filter criteria
	 * @param eItem the pointer to the <code>IXML_Element</code> instance obtained with the method <code>createDIDLSubFragment1</code>
	 * @return the pointer to the <code>IXML_Node</code> object
     */
    IXML_Node* createDIDLResFragment(cStringList* Filter, IXML_Element* eItem);
public:
    virtual ~cUPnPClassItem(){};
    virtual cStringList* getPropertyList();
    virtual IXML_Node* createDIDLFragment(IXML_Document* Document, cStringList* Filter);
    virtual bool setProperty(const char* Property, const char* Value);
    virtual bool getProperty(const char* Property, char** Value) const;
    /******** Setter ********/
    /**
     * Set a reference item
     *
     * This sets a reference item. Its comparable with symlinks in *nix systems.
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Reference the reference item
     */
    int setReference(cUPnPClassItem* Reference);
    /******** Getter ********/
    /**
     * Get the referenced item
     *
     * This returns the referenced item of this item.
     *
     * @return the referenced item
     */
    cUPnPClassItem* getReference() const { return this->mReference; }
    /**
     * Get the reference ID
     *
     * This returns the object ID of the referenced item or \b -1, if
     * no reference exists.
     *
     * @return the reference ID
     */
    cUPnPObjectID   getReferenceID() const { return this->mReference?this->mReference->getID():cUPnPObjectID(-1); }
};

typedef std::vector<cClass> tClassVector;

/**
 * The UPnP Container Class
 *
 * This is a UPnP container class representation with all its properties.
 */
class cUPnPClassContainer : public cUPnPClassObject {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPContainerMediator;
protected:
    cString                     mContainerType;                                 ///< DLNA container type
    tClassVector                mSearchClasses;                                 ///< Classes which are searchable
    tClassVector                mCreateClasses;                                 ///< Classes which are creatable
    bool                        mSearchable;                                    ///< Is the Container searchable?
    unsigned int                mUpdateID;                                      ///< The containerUpdateID
    cUPnPObjects*               mChildren;                                      ///< List of children
    cHash<cUPnPClassObject>*    mChildrenID;                                    ///< List of children as hash map
    /**
     * Get the number of the cached objects in this container. The function can be called for testing purposes.
     * @return the number of cached objects
     */
    int countObjects();
    /**
     * Update the container
     *
     * This performs an update, which acutally increases the containerUpdateID.
     */
    void update();
    /**
     * Sets the containerUpdateID
     *
     * This method should only be used when the containerUpdateID is loaded from
     * the database.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param UID the containerUpdateID
     */
    int setUpdateID(unsigned int UID);
    /**
     * Constructor of a container
     *
     * This creates a new instance of a container
     */
    cUPnPClassContainer();
public:
    virtual ~cUPnPClassContainer();
    virtual cStringList* getPropertyList();
    virtual IXML_Node* createDIDLFragment(IXML_Document* Document, cStringList* Filter);
    virtual bool setProperty(const char* Property, const char* Value);
    virtual bool getProperty(const char* Property, char** Value) const;
    virtual cUPnPClassContainer* getContainer(){ return this; }
    /**
     * Add a child
     *
     * This adds the specified child to this container. The parent container of the
     * child will be set to this container.
     *
     * @param Object the child to be added
     */
    void addObject(cUPnPClassObject* Object);
    /**
     * Remove a child
     *
     * This removes the specified child from the list of children. The child will
     * also loose its parent container, so that there is no link between left.
     *
     * @param Object the child to be removed
     */
    void removeObject(cUPnPClassObject* Object);
    /**
     * Get a child by ID
     *
     * Returns the child, which is specified by the \c ObjectID.
     *
     * @return the child with the specified ID
     * @param ID the \c ObjectID of the child
     */
    cUPnPClassObject* getObject(cUPnPObjectID ID) const;
    /**
     * Get the list of children
     *
     * This returns a list of the children of the container.
     *
     * @return the list of children
     */
    cUPnPObjects* getObjectList() const { return this->mChildren; }
    /**
     * Add a search class
     *
     * This adds a search class to the search classes vector
     *
     * @return returns
     * - \bc 0, if adding was successful
     * - \bc <0, otherwise
     * @param SearchClass the new class to be added
     */
    int addSearchClass(cClass SearchClass);
    /**
     * Remove a search class
     *
     * This removes a search class from the search classes vector
     *
     * @return returns
     * - \bc 0, if deleting was successful
     * - \bc <0, otherwise
     * @param SearchClass the class to be deleted
     */
    int delSearchClass(cClass SearchClass);
    /**
     * Add a create class
     *
     * This adds a create class to the create classes vector
     *
     * @return returns
     * - \bc 0, if adding was successful
     * - \bc <0, otherwise
     * @param CreateClass the new class to be added
     */
    int addCreateClass(cClass CreateClass);
    /**
     * Remove a create class
     *
     * This removes a create class from the create classes vector
     *
     * @return returns
     * - \bc 0, if deleting was successful
     * - \bc <0, otherwise
     * @param CreateClass the class to be deleted
     */
    int delCreateClass(cClass CreateClass);
    /******** Setter ********/
    /**
     * Set the UPnP/DLNA container type
     *
     * This sets the UPnP/DLNA container type. It must be a valid container type value.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Type the DLNA container type
     */
    int setContainerType(const char* Type);
    /**
     * Sets the search classes
     *
     * This sets the search classes, which allows the user to search only for
     * these classes in the current container and its children. If the vector
     * is empty the search can return any match. If the additional flag \bc
     * derived is set, then also any derived classes are matched.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param SearchClasses a vector container the allowed search classes
     */
    int setSearchClasses(std::vector<cClass> SearchClasses);
    /**
     * Sets the create classes
     *
     * This sets the create classes, which allows the user to create new objects
     * in this container, if \em restricted is \bc false.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param CreateClasses a vector containing the create classes
     */
    int setCreateClasses(std::vector<cClass> CreateClasses);
    /**
     * Sets the searchable flag
     *
     * This sets the searchable flag, which allows or disallows search on this
     * container.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Searchable \bc true, to enable or \bc false, to disable searching
     */
    int setSearchable(bool Searchable);
    /******** Getter ********/
    /**
     * Get the DLNA container type
     *
     * This returns the DLNA container type. Currently there are only these possible
     * values beside \bc NULL:
     * - \bc TUNER_1_0
     *
     * @return the DLNA container type
     */
    const char*   getContainerType() const { return this->mContainerType; }
    /**
     * Get the search classes
     *
     * This returns a vector container all possible search classes. This are classes,
     * which can be used for searching in this container.
     *
     * @return a vector with all search classes
     */
    const tClassVector* getSearchClasses() const { return &(this->mSearchClasses); }
    /**
     * Get the create classes
     *
     * This returns a vector containing all possible create classes. This are classes,
     * which can be created in this container. For instance a TV container can only create
     * items of the class VideoBroadcast. The vector is empty when creation of new items
     * by the user is not allowed.
     *
     * @return a vector with create classes
     */
    const tClassVector* getCreateClasses() const { return &(this->mCreateClasses); }
    /**
     * Is this container searchable
     *
     * This returns \bc true, if the container can be search via \em Search or
     * \bc false, otherwise.
     *
     * @return returns
     * - \bc true, if the container is searchable
     * - \bc false, otherwise
     */
    bool          isSearchable() const { return this->mSearchable; }
    /**
     * Get the number of children
     *
     * This returns the total number of children of this container
     *
     * @return the number of childen
     */
    unsigned int getChildCount() const { return this->mChildren->Count(); }
    /**
     * Get the containerUpdateID
     *
     * This returns the containerUpdateID
     *
     * @return the containerUpdateID of this container
     */
    unsigned int getUpdateID() const { return this->mUpdateID; }
    /**
     * Has the container been updated?
     *
     * This returns \bc true, if the container was recently updated or
     * \bc false, otherwise
     *
     * @return returns
     * - \bc true, if the container was updated
     * - \bc false, otherwise
     */
    bool         isUpdated();
};

/**
 * The UPnP EPG Container class
 *
 * The UPnP EPG container class can be used to represent broadcast channels as UPnP containers for
 * UPnP EPG items. UPnP items within this container instances can be used for information purposes
 * or to schedule a broadcast event related recording task.
 * Note: EPG items can not be used by an DLNA/UPnP client to consume the actual broadcast content.
 */
class cUPnPClassEpgContainer : public cUPnPClassContainer {
	friend class cUPnPEpgContainerMediator;
	friend class cVdrEpgInfo;

protected:
    cString  mChannelId;               ///< the channel ID
    cString  mChannelName;             ///< the channel name
    bool     mIsRadioChannel;          ///< is set if the associated channel is a radio channel

    /**
     * Constructor of a container
     *
     * This creates a new instance of this container.
     */
    cUPnPClassEpgContainer();

public:
    virtual ~cUPnPClassEpgContainer();
    virtual cStringList* getPropertyList();
    virtual bool getProperty(const char* Property, char** Value) const;
	/**
	 * Set the channel ID
	 * @param channelId a pointer to the channel ID variable
	 */
    int setChannelId (const char* channelId);
	/**
	 * Set the channel name
	 * @param channelName a pointer to the channel name variable
	 */
    int setChannelName (const char* channelName);
     /**
     * Set the radio/video channel mode.
     * @param val if set the channel is a radio channel, if not it is a TV channel
     */
    int setRadioChannel (const bool val);
	/**
	 * Get the channel ID
	 * @return a pointer to the channel Id variable
	 */
    const char* getChannelId() const { return this->mChannelId; }
	/**
	 * Get the channel name
	 * @return a pointer to the channel name variable
	 */
    const char* getChannelName() const { return this->mChannelName; }
	/**
	 * Is this a radio channel?
	 * @return true if this is a radio channel, false if not.
	 */
    bool isRadioChannel() const { return this->mIsRadioChannel; }
};

/**
 * The UPnP Video Item Class
 *
 * This is a UPnP video item class representation with its properties.
 */
class cUPnPClassVideoItem : public cUPnPClassItem {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPVideoItemMediator;
protected:
    cString             mGenre;                                                 ///< Genre of the video
    cString             mDescription;                                           ///< Description
    cString             mLongDescription;                                       ///< a longer description
    cString             mPublishers;                                            ///< CSV of Publishers
    cString             mLanguage;                                              ///< RFC 1766 Language code
    cString             mRelations;                                             ///< Relation to other contents
    cString             mProducers;                                             ///< CSV of Producers
    cString             mRating;                                                ///< Rating (for parential control)
    cString             mActors;                                                ///< CSV of Actors
    cString             mDirectors;                                             ///< CSV of Directors
    /**
     * Constructor of a video item
     *
     * This creates a new instance of a video item
     */
    cUPnPClassVideoItem();
    /**
     * Create a DIDL fragment and add video related attributes to it.
	 * @param eItem the pointer to the IXML_Element obtained with the method createDIDLSubFragment1
	 * @param Filter the string list with the filter criteria
	 * @return the pointer to the IXML_Node object
     */
    IXML_Node* createDIDLVideoSubFragment(IXML_Element* eItem, cStringList* Filter);
    /**
     * Add a property from a comma separated string to the DIDL fragment.
     * @param css the comma separated string
     * @param splitter a cSplit instance
     * @param Filter the string list with the filter criteria
     * @param eItem the pointer to the IXML_Element
     * @param propType the UPnP property type to be added
     */
    void addSplittedProperties(const char* css, cSplit* splitter, cStringList* Filter, IXML_Element* eItem, const char* propType);

public:
    virtual ~cUPnPClassVideoItem();
    virtual IXML_Node* createDIDLFragment(IXML_Document* Document, cStringList* Filter);
    virtual cStringList* getPropertyList();
    virtual bool setProperty(const char* Property, const char* Value);
    virtual bool getProperty(const char* Property, char** Value) const;
    /******** Setter ********/
    /**
     * Set a long description
     *
     * A long description may hold information about the content or the story
     * of a video
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param LongDescription the content or story of a video
     */
    int setLongDescription(const char* LongDescription);
    /**
     * Set a description
     *
     * A description may hold short information about the content or the story
     * of a video. Unlike a long description, this contains just a very short
     * brief like a subtitle or the episode title.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Description the description of a video
     */
    int setDescription(const char* Description);
    /**
     * Set the publishers
     *
     * This is a CSV list of publishers, who distributes the video.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Publishers a CSV list of publishers
     */
    int setPublishers(const char* Publishers);
    /**
     * Set genre(s)
     *
     * This is a CSV list of genres of a video. This may be something like
     * "Western" or "SciFi". Actually, there is no standardized rule for
     * a genre name, which results in an ambiguous definition of certain
     * genre, like Thriller and Horror.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Genre a CSV list of genre
     */
    int setGenre(const char* Genre);
    /**
     * Set the language
     *
     * This sets the language of a video. It is defined by RFC 1766.
     * A valid language definition is \em "de-DE" or \em "en-US".
     *
     * @see http://www.ietf.org/rfc/rfc1766.txt
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Language the language (RFC 1766)
     */
    int setLanguage(const char* Language);
    /**
     * Sets a relation URL
     *
     * This sets a CSV list of relation URLs, where to find additional
     * information about the movie. The URLs may not contain commas and they
     * must be properly escaped as in RFC 2396
     *
     * @see http://www.ietf.org/rfc/rfc2396.txt
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Relations a CSV list with relations
     */
    int setRelations(const char* Relations);
    /**
     * Sets the directors
     *
     * This sets a CSV list of directors.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Directors a CSV list of directors
     */
    int setDirectors(const char* Directors);
    /**
     * Sets the actors
     *
     * This sets a CSV list of actors in a video. This usually contain the main actors.
     * However, also other actors appearing in the video can be mentioned here.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Actors a CSV list of actors
     */
    int setActors(const char* Actors);
    /**
     * Sets the producers
     *
     * This sets a CSV list of producers of a video. These are the people who are
     * involed in the production of a video
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Producers a CSV list of producers
     */
    int setProducers(const char* Producers);
    /**
     * Sets the rating
     *
     * This is a rating, which can be used for parential control issues.
     *
     * @see http://en.wikipedia.org/wiki/Motion_picture_rating_system
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Rating the rating of a video
     */
    int setRating(const char* Rating);
    /******** Getter ********/
    /**
     * Get the genres
     *
     * This returns a CSV list of genre
     *
     * @return the genre of a video
     */
    const char* getGenre() const { return this->mGenre; }
    /**
     * Get the long description
     *
     * This returns the long description of a video
     *
     * @return the long description of a video
     */
    const char* getLongDescription() const { return this->mLongDescription; }
    /**
     * Get the description
     *
     * This returns the description of a video
     *
     * @return the description of a video
     */
    const char* getDescription() const { return this->mDescription; }
    /**
     * Get the publishers
     *
     * This returns a CSV list of publishers of the video
     *
     * @return a CSV list of publishers
     */
    const char* getPublishers() const { return this->mPublishers; }
    /**
     * Get the language
     *
     * This returns the language of the video
     *
     * @return the language
     */
    const char* getLanguage() const { return this->mLanguage; }
    /**
     * Get the relations
     *
     * This returns a CSV list of relation URLs.
     *
     * @return a CSV list of relation URLs
     */
    const char* getRelations() const { return this->mRelations; }
    /**
     * Get the actors
     *
     * This returns a CSV list of actors in the video
     *
     * @return a CSV list of actors
     */
    const char* getActors() const { return this->mActors; }
    /**
     * Get the producers
     *
     * This returns a CSV list of producers of a video
     *
     * @return a CSV list of producers
     */
    const char* getProducers() const { return this->mProducers; }
    /**
     * Get the directors
     *
     * This returns a CSV list of directors
     *
     * @return a CSV list of directors
     */
    const char* getDirectors() const { return this->mDirectors; }
    /**
     * Get the rating
     *
     * This returns the rating used for parental control.
     *
     * @return the rating of a video
     */
    const char* getRating() const { return this->mRating; }
};

/**
 * The UPnP Audio Item Class
 *
 * This is an UPnP audio item class representation with its properties.
 */
class cUPnPClassAudioItem : public cUPnPClassItem {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPAudioItemMediator;
protected:
    /**
     * Constructor of a audio item
     *
     * This creates a new instance of a audio item
     */
    cUPnPClassAudioItem();

public:
    virtual ~cUPnPClassAudioItem();
};

/**
 * The UPnP Audio Record Class
 *
 * Instances of this class represent audio records. Its properties channel name and storage medium.
 */
class cUPnPClassAudioRecord : public cUPnPClassAudioItem {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPAudioRecordMediator;
protected:
    cString      mStorageMedium;                ///< The storage medium where the audio record is stored
    cString      mChannelName; 		            ///< The channel name or provider name

    /**
     * Constructor of an Audio Record
     *
     * This creates a new instance of a <code>cUPnPClassAudioRecord</code>.
     */
    cUPnPClassAudioRecord();
public:
    virtual ~cUPnPClassAudioRecord();
    virtual cStringList* getPropertyList();
    virtual bool setProperty(const char* Property, const char* Value);
    virtual bool getProperty(const char* Property, char** Value) const;
    /**
     * Sets the storage medium
     *
     * This will set the storage medium, where the audio record resides. Valid media
     * are defined in \link common.h \endlink
     *
     * @see common.h
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise

     */
    int setStorageMedium(const char* StorageMedium);
    /**
     * Set the channel name
     *
     * This sets the channel name or the provider of the channel.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param ChannelName the channel name
     */
    int setChannelName(const char* ChannelName);
    /**
     * Get the storage medium
     *
     * This returns the storage medium, where the movie resides.
     *
     * @return the storage medium
     */
    const char* getStorageMedium() const { return this->mStorageMedium; }
    /**
     * Get the channel name
     *
     * This returns the channel name or provider name respectively.
     *
     * @return the channel name
     */
    const char* getChannelName() const { return this->mChannelName; }
};

/**
 * The UPnP Movie class
 *
 * Instances of this class represent movies or TV records.
 * Properties are region code, storage medium and channel name.
 */
class cUPnPClassMovie : public cUPnPClassVideoItem {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPMovieMediator;
protected:
    int          mDVDRegionCode;                ///< The Region code of the movie (0 - 8)
    cString      mStorageMedium;                ///< The storage medium where the movie is stored
    cString      mChannelName; 		            ///< The channel name or provider name

    /**
     * Constructor of a movie
     *
     * This creates a new instance of <code>cUPnPClassMovie</code>.
     */
    cUPnPClassMovie();
public:
    virtual ~cUPnPClassMovie();
    virtual IXML_Node* createDIDLFragment(IXML_Document* Document, cStringList* Filter);
    virtual cStringList* getPropertyList();
    virtual bool setProperty(const char* Property, const char* Value);
    virtual bool getProperty(const char* Property, char** Value) const;
    /******** Setter ********/
    /**
     * Sets the DVD region code
     *
     * For more information on this, see http://en.wikipedia.org/wiki/DVD_region_code
     *
     * The integer representation for \em ALL is 9.
     *
     * @see http://en.wikipedia.org/wiki/DVD_region_code
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param RegionCode the region code of this movie
     */
    int setDVDRegionCode(int RegionCode);
    /**
     * Sets the storage medium
     *
     * This will set the storage medium, where the movie resides. Valid media
     * are defined in \link common.h \endlink
     *
     * @see common.h
      * @param StorageMedium the medium where the movie is located
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setStorageMedium(const char* StorageMedium);
    /**
     * Set the channel name
     *
     * This sets the channel name or the provider of the channel.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param ChannelName the channel name
     */
    int setChannelName(const char* ChannelName);
    /******** Getter ********/
    /**
     * Get the DVD region code
     *
     * This returns the DVD region code. For more information,
     * see http://en.wikipedia.org/wiki/DVD_region_code
     *
     * The integer representation for \em ALL is 9.
     *
     * @see http://en.wikipedia.org/wiki/DVD_region_code
     * @return the DVD region code
     */
    int getDVDRegionCode() const { return this->mDVDRegionCode; }
    /**
     * Get the storage medium
     *
     * This returns the storage medium, where the movie resides.
     *
     * @return the storage medium
     */
    const char* getStorageMedium() const { return this->mStorageMedium; }
    /**
     * Get the channel name
     *
     * This returns the channel name or provider name respectively.
     *
     * @return the channel name
     */
    const char* getChannelName() const { return this->mChannelName; }
};

/**
 * The UPnP Audio Broadcast class
 *
 * Instances of this class represent audio broadcast channels.
 * Its properties are channel number and channel name.
 */
class cUPnPClassAudioBroadcast : public cUPnPClassAudioItem {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPAudioBroadcastMediator;
protected:
    int                 mChannelNr;             ///< The channel number
    cString             mChannelName;           ///< The channel name or provider name
    /**
     * Constructor of a Audio broadcast
     *
     * This creates a new instance of <code>cUPnPClassAudioBroadcast</code>.
     */
    cUPnPClassAudioBroadcast();
public:
    virtual ~cUPnPClassAudioBroadcast();
    virtual IXML_Node* createDIDLFragment(IXML_Document* Document, cStringList* Filter);
    virtual cStringList* getPropertyList();
    virtual bool setProperty(const char* Property, const char* Value);
    virtual bool getProperty(const char* Property, char** Value) const;
    /******** Setter ********/
    /**
     * Set channel number
     *
     * This sets the channel number, so that it can be used for directly navigation
     * or channel up and down navigation respectively.
     * @param ChannelNr the channel number; valid ones are greater 0
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setChannelNr(int ChannelNr);
    /**
     * Set the channel name
     *
     * This sets the channel name or the provider of the channel.
     * @param ChannelName the channel name
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setChannelName(const char* ChannelName);
    /******** Getter ********/
    /**
     * Get the channel number
     *
     * This returns the channel number.
     *
     * @return the channel number; valid numbers are greater 0
     */
    int getChannelNr() const { return this->mChannelNr; }
    /**
     * Get the channel name
     *
     * This returns the channel name or provider name respectively.
     *
     * @return the channel name
     */
    const char* getChannelName() const { return this->mChannelName; }
};


/**
 * The UPnP Video Broadcast Class
 *
 * Instances of this class represent TV channels.
 * Its properties are icon URI, region, channel number and channel name.
 */
class cUPnPClassVideoBroadcast : public cUPnPClassVideoItem {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPVideoBroadcastMediator;
protected:
    cString             mIcon;                  ///< The channel icon of the channel
    cString             mRegion;                ///< The region where the channel can be received
    int                 mChannelNr;             ///< The channel number
    cString             mChannelName;           ///< The channel name or provider name
    /**
     * Constructor of a video broadcast
     *
     * This creates a new instance of <code>cUPnPClassVideoBroadcast</code>.
     */
    cUPnPClassVideoBroadcast();
public:
    virtual ~cUPnPClassVideoBroadcast();
	/**
     * Create a DIDL fragment including the video broadcast related attributes.
	 * If the 'broadcastPrepend' flag with the upnp-plugin start options is set, the broadcast event title
	 * is prepended by the channel number and channel name.
	 * @param Document  an <code>IXML_Document</code> instance
	 * @param Filter the string list with the filter criteria
	 * @return the pointer to the resulting <code>IXML_Node</code> object
     */
    virtual IXML_Node* createDIDLFragment(IXML_Document* Document, cStringList* Filter);
    virtual cStringList* getPropertyList();
    virtual bool setProperty(const char* Property, const char* Value);
    virtual bool getProperty(const char* Property, char** Value) const;
    /******** Setter ********/
    /**
     * Set the channel icon
     *
     * This sets the channel icon of this channel. The resource must be a valid
     * URI which can be obtained via the internal webserver.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param IconURI the URI to the icon file
     */
    int setIcon(const char* IconURI);
    /**
     * Set the channel region
     *
     * This sets the region of a channel, where it can be received.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param Region the location where the channel can be received
     */
    int setRegion(const char* Region);
    /**
     * Set channel number
     *
     * This sets the channel number, so that it can be used for directly navigation
     * or channel up and down navigation respectively.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param ChannelNr the channel number
     */
    int setChannelNr(int ChannelNr);
    /**
     * Set the channel name
     *
     * This sets the channel name or the provider of the channel.
     *
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     * @param ChannelName the channel name
     */
    int setChannelName(const char* ChannelName);
    /******** Getter ********/
    /**
     * Get the channel icon
     *
     * This returns the channel icon of the channel.
     *
     * @return the channel icon
     */
    const char* getIcon() const { return this->mIcon; }
    /**
     * Get the region
     *
     * This returns the region, where the channel can be received
     *
     * @return the channel region
     */
    const char* getRegion() const { return this->mRegion; }
    /**
     * Get the channel number
     *
     * This returns the channel number.
     *
     * @return the channel number
     */
    int getChannelNr() const { return this->mChannelNr; }
    /**
     * Get the channel name
     *
     * This returns the channel name or provider name respectively
     *
     * @return the channel name
     */
    const char* getChannelName() const { return this->mChannelName; }
};

/**
 * The UPnP EPG Item Class
 *
 * This is an UPnP EPG item class representation.
 * The items can be used to show the broadcast event information and/or to schedule a broadcast event related recording task.
 * Such a recording task can be ordered by an UPnP media player associated control point when 'playing' the EPG item.
 * The accompanying small video confirms the schedule of the recording order.
 * In addition to the recording task a record timer item is generated whith which the task can be cancelled by any
 * UPnP client at a later date.
 */
class cUPnPClassEpgItem : public cUPnPClassItem {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPEpgItemMediator;

protected:
    int          mEventId;                      ///< The event ID associated to the broadcast event
    int          mDuration;                     ///< The duration of the broadcast event
    int          mTableId;                      ///< The table ID of the broadcast event
    int          mSerialNumber;                 ///< A serial number for the broadcast event
    int          mChannelNumber;                ///< The associated channel serial number for the broadcast event
	cString      mShortTitle;                   ///< The short title of the broadcast event
	cString      mSynopsis;                     ///< The synopsis of the broadcast event
	cString      mGenres;                       ///< The genre(s) for the broadcast event
	cString      mStartTime;                    ///< The start time of the broadcast event
	cString      mVersion;                      ///< The version associated to the broadcast event

    /**
     * Construct an EPG item
     *
     * This creates a new instance of <code>cUPnPClassEpgItem</code>.
     */
    cUPnPClassEpgItem();

public:
    virtual ~cUPnPClassEpgItem();
    virtual IXML_Node* createDIDLFragment(IXML_Document* Document, cStringList* Filter);
    virtual cStringList* getPropertyList();
    virtual bool setProperty(const char* Property, const char* Value);
    virtual bool getProperty(const char* Property, char** Value) const;
    /**
     * Set the event ID. The ID is an unique unsigned 32 bit number for the event.
	 * @param eventId the event ID
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setEventId(int eventId);
    /**
     * Set the duration of the broadcast event in seconds.
	 * @param duration the duration
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setDuration(int duration);
    /**
     * Set the ID of the event table.
	 * @param tableId the ID of the event table
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setTableId(int tableId);
    /**
     * Set the short title of the broadcast event
	 * @param shortTitle the short title
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setShortTitle (const char* shortTitle);
    /**
     * Set the synopsis of the broadcast event.
	 * @param synopsis the long description
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setSynopsis (const char* synopsis);
    /**
     * Set the genre(s) for the broadcast event as numbers defined with spec EN 300 468
	 * @param genres the genres as space separated numbers
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setGenres (const char* genres);
    /**
     * Set the start time of the broadcast event. The string represents a time_t number in UTC.
	 * @param startTime the start time
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setStartTime (const char* startTime);
    /**
     * Set the version. The string represents a hex number which specifies the version of the event in the table.
	 * @param version the version
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
     */
    int setVersion (const char* version);
    /**
     * Get the broadcast event ID.
	 * @return the event ID
     */
    int getEventId() const { return this->mEventId; }
    /**
     * Get the duration of the broadcast event in seconds.
	 * @return the duration in seconds
     */
    int getDuration() const { return this->mDuration; }
    /**
     * Get the ID of the event table.
	 * @return the table ID
     */
    int getTableId() const { return this->mTableId; }
    /**
     * Get the short title.
	 * @return the short title
     */
    const char* getShortTitle() const {return this->mShortTitle; }
    /**
     * Get the synopsis.
	 * @return the long description
     */
    const char* getSynopsis() const {return this->mSynopsis; }
    /**
     * Get the genres for the broadcast event as space separated numbers defined in EN 300 468.
	 * @return the genres as content descriptor numbers defined in EN 300 468
     */
    const char* getGenres() const {return this->mGenres; }
    /**
     * Get the start time of the broadcast event as a string representing a time_t number in UTC.
	 * @return the start time
     */
    const char* getStartTime() const {return this->mStartTime; }
    /**
     * Get the version. The string represents a hex number which specifies the version of the event in the table.
	 * @return the version
     */
    const char* getVersion() const {return this->mVersion; }
};
/**
 * The UPnP Record Timer Item Class
 *
 * Instances of this UPnP record timer item class represent scheduled recording tasks.
 * They can be used to display the actual recording orders and/or to cancel them.
 * Cancellation can be done by 'playing' these items with an UPnP media player. The associated small video
 * sequence confirms the canellation of the recording task.
 */
class cUPnPClassRecordTimerItem : public cUPnPClassItem {
    friend class cMediaDatabase;
    friend class cUPnPObjectMediator;
    friend class cUPnPRecordTimerItemMediator;

protected:
	int mStatus;                           ///< the status flag(s) of the timer
	int mPriority;                         ///< the priority of the timer
	int mLivetime;                         ///< the minimum live time of the record in units of days
	int mStart;                            ///< the timer's start time
	int mStop;                             ///< the timer's stop time
	bool mIsRadioChannel;                  ///< is set for a radio channel, not set for a TV channel
	cString mChannelId;                    ///< the channel ID associated to the record
	cString mDay;                          ///< the day the recording starts. Format: yyyy-mm-dd
	cString mFile;                         ///< the file name of the record
	cString mAux;                          ///< additional informations
    /**
	 * Construct an instance of <code>cUPnPClassRecordTimerItem</code>.
	 */
	cUPnPClassRecordTimerItem();
public:
	virtual ~cUPnPClassRecordTimerItem();
    virtual IXML_Node* createDIDLFragment(IXML_Document* Document, cStringList* Filter);
	/**
	 * Get the status flag(s) of the timer.
	 * @return the status flag(s) of the timer
	 */
	int getStatus () const { return this->mStatus; }
	/**
	 * Get the priority of the timer.
	 * @return the priority of the timer
	 */
	int getPriority() const { return this->mPriority; }
	/**
	 * Get the minimum live time in units of days the record has to remain on disk.
	 * @return the live time in units of days
	 */
	int getLifetime() const { return this->mLivetime; }
	/**
	 * Is the channel a radio channel?
	 * @return true if it is a radio channel, false if not.
	 */
    bool isRadioChannel() const { return this->mIsRadioChannel; }
	/**
	 * Get the channel ID associated to the record.
	 * @return the channel ID
	 */
	const char* getChannelId() const {return this->mChannelId; }
	/**
	 * Get the day the recording starts. Format: yyyy-mm-dd
	 * @return the day the recording starts
	 */
	const char* getDay() const {return this->mDay; }
    /**
     * Get the start time.
	 * @return the start time as a decimal coded binary value
     */
    int getStart() const {return this->mStart; }
    /**
     * Get the stop time.
	 * @return the stop time as a decimal coded binary value
     */
    int getStop() const {return this->mStop; }
    /**
     * Get the file name of the record.
	 * @return the file name of the record
     */
    const char* getFile() const {return this->mFile; }
    /**
     * Get additional informations.
	 * @return the additional informations if any
     */
    const char* getAux() const {return this->mAux; }
	/**
	 * Set the status flag(s) of the timer
	 * @param status the status flags of the timer
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setStatus (int status);
	/**
	 * Set the priority of the timer
	 * @param priority the priority of the timer
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setPriority(int priority);
	/**
	 * Set the minimum live time of the record in units of days
	 * @param livetime the minimum live time
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setLifetime(int livetime);
	/**
	 * Set the channel type radio or TV
	 * @param isRadio is set if the channel is a radio channel, if not it is a TV channel
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setRadioChannel (bool isRadio);
	/**
	 * Set the start time of the record
	 * @param start the start time, a decimal coded binary value
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setStart(int start);
	/**
	 * Set the stop time of the timer
	 * @param stop the stop time, a decimal coded binary value
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setStop (int stop);
	/**
	 * Set the file name of the record
	 * @param file the file name
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setFile (const char* file);
	/**
	 * Set the additional informations for the timer
	 * @param aux the additional informations
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setAux (const char* aux);
	/**
	 * Set the channel ID associated to the record
	 * @param channelId a pointer to the channel Id for the record
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setChannelId(const char* channelId);
	/**
	 * Set the day the recording starts
	 * @param day a pointer to the day
     * @return returns
     * - \bc 0, if setting was successful
     * - \bc <0, otherwise
	 */
	int setDay(const char* day);
};
#endif	/* _OBJECT_H */

