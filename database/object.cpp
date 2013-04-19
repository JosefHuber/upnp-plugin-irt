/*
 * File:   object.cpp
 * Author: savop
 * Author: J. Huber, IRT GmbH
 * Created on 11. September 2009, 20:39
 * Last modification on March 22, 2013
 */

#include <upnp/upnptools.h>
#include <vdr/recording.h>
#include <vector>
#include <vdr/tools.h>
#include <upnp/ixml.h>
#include <upnp/upnp.h>
#include "metadata.h"
#include "object.h"
#include "resources.h"
#include "config.h"

static int CompareUPnPObjects(const void *a, const void *b){
  const cUPnPClassObject *la = *(const cUPnPClassObject **)a;
  const cUPnPClassObject *lb = *(const cUPnPClassObject **)b;
  return la->Compare(*lb);
}

cUPnPObjects::cUPnPObjects(){}

cUPnPObjects::~cUPnPObjects(){}

void cUPnPObjects::SortBy(const char* Property, bool Descending){
    int n = Count();
    cUPnPClassObject *a[n];
    cUPnPClassObject *object = (cUPnPClassObject *)objects;
    int i = 0;
    while (object && i < n) {
        object->setSortCriteria(Property, Descending);
        a[i++] = object;
        object = (cUPnPClassObject *)object->Next();
    }
    qsort(a, n, sizeof(cUPnPClassObject *), CompareUPnPObjects);
    objects = lastObject = NULL;
    for (i = 0; i < n; i++) {
        a[i]->Unlink();
        count--;
        Add(a[i]);
    }
}

 /**********************************************\
 *                                              *
 *  UPnP Objects                                *
 *                                              *
 \**********************************************/

 /**********************************************\
 *                                              *
 *  Object                                      *
 *                                              *
 \**********************************************/

cUPnPClassObject::cUPnPClassObject(){
    this->mID = -1;
    this->mLastID = -1;
    this->mResources = new cList<cUPnPResource>;
    this->mResourcesID = new cHash<cUPnPResource>;
    this->mParent = NULL;
    this->mClass = NULL;
    this->mCreator = NULL;
    this->mTitle = NULL;
    this->mWriteStatus = WS_UNKNOWN;
    this->mRestricted = true;
    this->mDIDLFragment = NULL;
    this->mSortCriteria = NULL;
    this->mLastModified = 0; //NULL;
}

cUPnPClassObject::~cUPnPClassObject(){
//	MESSAGE(VERBOSE_METADATA, "UPnP object destruction: Delete the cached resources with object ID: %s", *(this->getID())); 
//	cUPnPResources::getInstance()->deleteCachedResources(this);    // is done with the object mediator 'deleteObject'
    if(this->mParent) {
		this->mParent->getContainer()->removeObject(this);
	}
	if (this->mResources){
       this->mResources->Clear();
	}
	if (this->mResourcesID){
      this->mResourcesID->Clear();
	}
    delete this->mResources;
    delete this->mResourcesID;
    free(this->mDIDLFragment);
}

int cUPnPClassObject::Compare(const cListObject& ListObject) const {
    char* Value1 = NULL; char* Value2 = NULL; int ret = 0;
    cUPnPClassObject* Object = (cUPnPClassObject*)&ListObject;
    if(Object->getProperty(this->mSortCriteria, &Value1) &&
       this->getProperty(this->mSortCriteria, &Value2)){
        ret = strcmp(Value1, Value2);
        if(this->mSortDescending) ret *= -1;
    }
    return ret;
}

void cUPnPClassObject::setSortCriteria(const char* Property, bool Descending){
    this->mSortCriteria = Property;
    this->mSortDescending = Descending;
}

void cUPnPClassObject::clearSortCriteria(){
    this->mSortCriteria = NULL;
    this->mSortDescending = false;
}

int cUPnPClassObject::setID(cUPnPObjectID ID){
//    MESSAGE(VERBOSE_MODIFICATIONS, "Set ID from %s to %s", *this->getID(),*ID);
    if((int)ID < 0){
        ERROR("Invalid object ID '%s'",*ID);
        return -1;
    }
    this->mLastID = (this->mID==-1) ? ID : this->mID;
    this->mID = ID;
    return 0;
}

int cUPnPClassObject::setParent(cUPnPClassContainer* Parent){
    if (Parent == NULL){
        MESSAGE(VERBOSE_MODIFICATIONS, "Object '%s' elected as root object", *this->getID());
    }
    // unregister from old parent
    if (Parent && this->mParent && Parent != this->mParent && ((int) Parent->getID()) != ((int) this->mParent->getID())){
		this->mParent->getContainer()->removeObject(this);
    }
    this->mParent = Parent;
    return 0;
}

int cUPnPClassObject::setClass(const char* Class){
    if(     !strcasecmp(Class, UPNP_CLASS_ALBUM) ||
            !strcasecmp(Class, UPNP_CLASS_AUDIO) ||
            !strcasecmp(Class, UPNP_CLASS_AUDIOBC) ||
            !strcasecmp(Class, UPNP_CLASS_AUDIOBOOK) ||
            !strcasecmp(Class, UPNP_CLASS_CONTAINER) ||
			!strcasecmp(Class, UPNP_CLASS_EPGCONTAINER) ||
            !strcasecmp(Class, UPNP_CLASS_GENRE) ||
            !strcasecmp(Class, UPNP_CLASS_IMAGE) ||
            !strcasecmp(Class, UPNP_CLASS_ITEM) ||
			!strcasecmp(Class, UPNP_CLASS_EPGITEM) ||
			!strcasecmp(Class, UPNP_CLASS_BOOKMARKITEM) ||
            !strcasecmp(Class, UPNP_CLASS_MOVIE) ||
            !strcasecmp(Class, UPNP_CLASS_MOVIEGENRE) ||
            !strcasecmp(Class, UPNP_CLASS_MUSICALBUM) ||
            !strcasecmp(Class, UPNP_CLASS_MUSICARTIST) ||
            !strcasecmp(Class, UPNP_CLASS_MUSICGENRE) ||
            !strcasecmp(Class, UPNP_CLASS_MUSICTRACK) ||
            !strcasecmp(Class, UPNP_CLASS_MUSICVIDCLIP) ||
            !strcasecmp(Class, UPNP_CLASS_OBJECT) ||
            !strcasecmp(Class, UPNP_CLASS_PERSON) ||
            !strcasecmp(Class, UPNP_CLASS_PHOTO) ||
            !strcasecmp(Class, UPNP_CLASS_PHOTOALBUM) ||
            !strcasecmp(Class, UPNP_CLASS_PLAYLIST) ||
            !strcasecmp(Class, UPNP_CLASS_PLAYLISTCONT) ||
            !strcasecmp(Class, UPNP_CLASS_STORAGEFOLD) ||
            !strcasecmp(Class, UPNP_CLASS_STORAGESYS) ||
            !strcasecmp(Class, UPNP_CLASS_STORAGEVOL) ||
            !strcasecmp(Class, UPNP_CLASS_TEXT) ||
            !strcasecmp(Class, UPNP_CLASS_VIDEO) ||
            !strcasecmp(Class, UPNP_CLASS_VIDEOBC)
            ){
        this->mClass = strdup0(Class);
        return 0;
    }
    else {
        ERROR("Invalid or unsupported class '%s'", Class);
        return -1;
    }
}

int cUPnPClassObject::setTitle(const char* Title){
    if(Title==NULL){
        ERROR("Title is empty but required");
        return -1;
    }
    this->mTitle = skipspace(stripspace(strdup(Title)));
    return 0;
}

int cUPnPClassObject::setCreator(const char* Creator){
    this->mCreator = strdup0(Creator);
    return 0;
}

int cUPnPClassObject::setRestricted(bool Restricted){
    this->mRestricted = Restricted;
    return 0;
}

int cUPnPClassObject::setWriteStatus(int WriteStatus){
    if(     WriteStatus == WS_MIXED ||
            WriteStatus == WS_NOT_WRITABLE ||
            WriteStatus == WS_PROTECTED ||
            WriteStatus == WS_UNKNOWN ||
            WriteStatus == WS_WRITABLE){
        this->mWriteStatus = WriteStatus;
        return 0;
    }
    else {
        ERROR("Invalid write status '%d'", WriteStatus);
        return -1;
    }
}

bool cUPnPClassObject::getProperty(const char* Property, char** Value) const {
    cString Val;
    if(!strcasecmp(Property, SQLITE_COL_OBJECTID) || !strcasecmp(Property, UPNP_PROP_OBJECTID)){
        Val = *this->getID();
    }
    else if(!strcasecmp(Property, SQLITE_COL_PARENTID) || !strcasecmp(Property, UPNP_PROP_PARENTID)){
        Val = *this->getParentID();
    }
    else if(!strcasecmp(Property, SQLITE_COL_CLASS) || !strcasecmp(Property, UPNP_PROP_CLASS)){
        Val = this->getClass();
    }
    else if(!strcasecmp(Property, SQLITE_COL_TITLE) || !strcasecmp(Property, UPNP_PROP_TITLE)){
        Val = this->getTitle();
    }
    else if(!strcasecmp(Property, SQLITE_COL_CREATOR) || !strcasecmp(Property, UPNP_PROP_CREATOR)){
        Val = this->getCreator();
    }
    else if(!strcasecmp(Property, SQLITE_COL_RESTRICTED) || !strcasecmp(Property, UPNP_PROP_RESTRICTED)){
        Val = this->isRestricted()?"1":"0";
    }
    else if(!strcasecmp(Property, SQLITE_COL_WRITESTATUS) || !strcasecmp(Property, UPNP_PROP_WRITESTATUS)){
        Val = itoa(this->getWriteStatus());
    }
    else {
        ERROR("Invalid property '%s'", Property);
        return false;
    }
    *Value = strdup0(*Val);
    return true;
}

cStringList* cUPnPClassObject::getPropertyList(){
    cStringList* Properties = new cStringList;
    Properties->Append(strdup(UPNP_PROP_CREATOR));
    Properties->Append(strdup(UPNP_PROP_WRITESTATUS));
    return Properties;
}

bool cUPnPClassObject::setProperty(const char* Property, const char* Value){
    int ret;
    if(!strcasecmp(Property, SQLITE_COL_OBJECTID) || !strcasecmp(Property, UPNP_PROP_OBJECTID)){
        ERROR("Not allowed to set object ID by hand");
        return false;
    }
    else if(!strcasecmp(Property, SQLITE_COL_PARENTID) || !strcasecmp(Property, UPNP_PROP_PARENTID)){
        ERROR("Not allowed to set parent ID by hand");
        return false;
    }
    else if(!strcasecmp(Property, SQLITE_COL_CLASS) || !strcasecmp(Property, UPNP_PROP_CLASS)){
        ERROR("Not allowed to set class by hand");
        return false;
    }
    else if(!strcasecmp(Property, SQLITE_COL_TITLE) || !strcasecmp(Property, UPNP_PROP_TITLE)){
        ret = this->setTitle(Value);
    }
    else if(!strcasecmp(Property, SQLITE_COL_CREATOR) || !strcasecmp(Property, UPNP_PROP_CREATOR)){
        ret = this->setCreator(Value);
    }
    else if(!strcasecmp(Property, SQLITE_COL_RESTRICTED) || !strcasecmp(Property, UPNP_PROP_RESTRICTED)){
        ret = this->setRestricted(atoi(Value)==1?true:false);
    }
    else if(!strcasecmp(Property, SQLITE_COL_WRITESTATUS) || !strcasecmp(Property, UPNP_PROP_WRITESTATUS)){
        ret= this->setWriteStatus(atoi(Value));
    }
    else {
        ERROR("Invalid property with setProperty '%s'", Property);
        return false;
    }
	return ret >= 0;
}

int cUPnPClassObject::addResource(cUPnPResource* Resource){
//    MESSAGE(VERBOSE_MODIFICATIONS, "Adding resource #%d", Resource->getID());
    if (!Resource){
        ERROR("No resource");
        return -1;
    }
    this->mResources->Add(Resource);
    this->mResourcesID->Add(Resource, Resource->getID());
    return 0;
}

int cUPnPClassObject::removeResource(cUPnPResource* Resource){
    if (!Resource){
        ERROR("No resource");
        return -1;
    }
    this->mResourcesID->Del(Resource, Resource->getID());
    this->mResources->Del(Resource);
    return 0;
}

char* cUPnPClassObject::checkStorageMedium(char* StorageMedium){
    if (!StorageMedium){
	   return strdup(UPNP_STORAGE_UNKNOWN);
	}
    else if (
            strcasecmp(StorageMedium,UPNP_STORAGE_CD_DA) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_CD_R) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_CD_ROM) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_CD_RW) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_DAT) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_DV) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_DVD_AUDIO) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_DVD_RAM) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_DVD_ROM) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_DVD_RW_MINUS) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_DVD_RW_PLUS) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_DVD_R_MINUS) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_DVD_VIDEO) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_D_VHS) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_HDD) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_HI8) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_LD) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_MD_AUDIO) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_MD_PICTURE) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_MICRO_MV) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_MINI_DV) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_NETWORK) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_SACD) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_S_VHS) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_UNKNOWN) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_VHS) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_VHSC) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_VIDEO8) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_VIDEO_CD) &&
            strcasecmp(StorageMedium,UPNP_STORAGE_W_VHS)){

        ERROR("Invalid storage medium type: %s", StorageMedium);
        return NULL;
    }
    return strdup(StorageMedium);
}

char* cUPnPClassObject::checkString(const char* text){
	int longDescrLen = 0;
	char* longDescrTrunc = NULL;
	if (text){
		longDescrLen = strlen(text);
		const int maxStrLen = UPNP_MAX_METADATA_LENGTH - 24;
		
		if (longDescrLen > maxStrLen){
			int actLen = maxStrLen;
			bool found = false;
			for (int i = maxStrLen-1; i >= 0 && !found; i--){
				int iChar = text[i] & 0xff;
				if (iChar <= 127){
					actLen = i+1;
					found = true;
				}
			}
			longDescrTrunc = strdup(substr(text, 0, actLen));
		}
		else {
			longDescrTrunc = strdup(text);
		}
	}
	return longDescrTrunc;
}

bool cUPnPClassObject::checkUTF8Char (const char* text, int i, bool c3Char, int* retval, std::string *substituted){
	if (c3Char){
		*substituted += 0xc3;
		*substituted += text[i];
		return false;
	}
	*substituted += "?";
	*retval = (*retval == -1) ? i : *retval;
	if (i > 2){
		MESSAGE(VERBOSE_MODIFICATIONS, "Got an unexpected UTF8 0xc3 follower at position %i: %x %x %x %x", i, (int)(text[i-3] & 0xff),
			(int)(text[i-2] & 0xff), (int)(text[i-1] & 0xff), (int)(text[i] & 0xff));
	}
	else if (i > 0){
		MESSAGE(VERBOSE_MODIFICATIONS, "Got an unexpected UTF8 0xc3 follower at position %i: %x %x", i, 
			   (int)(text[i-1] & 0xff), (int)(text[i] & 0xff));
	}
	else {
		MESSAGE(VERBOSE_MODIFICATIONS, "Got an unexpected UTF8 0xc3 follower at position %i: %x", i, (int)(text[i] & 0xff));
	}
	return c3Char;
}

bool cUPnPClassObject::checkUTF8C2Char (const char* text, int i, bool c2Char, int* retval, std::string *substituted){
	if (c2Char){
		*substituted += 0xc2;
		*substituted += text[i];
		return false;
	}
	*substituted += "?";
	*retval = (*retval == -1) ? i : *retval;
	if (i > 2){
		MESSAGE(VERBOSE_MODIFICATIONS, "Got an unexpected UTF8 0xc2 follower at position %i: %x %x %x %x", i, (int)(text[i-3] & 0xff),
			(int)(text[i-2] & 0xff), (int)(text[i-1] & 0xff), (int)(text[i] & 0xff));
	}
	else if (i > 0){
		MESSAGE(VERBOSE_MODIFICATIONS, "Got an unexpected UTF8 0xc2 follower at position %i: %x %x", i, 
			         (int)(text[i-1] & 0xff), (int)(text[i] & 0xff));
	}	
	else {
	    MESSAGE(VERBOSE_MODIFICATIONS, "Got an unexpected UTF8 0xc2 follower at position %i: %x", i, (int)(text[i] & 0xff));
	}
	return c2Char;
}

bool cUPnPClassObject::checkUTF8Follower (const char* text, int i, bool* c2Char, bool c3Char, int* retval, std::string *substituted){
	if (c3Char){  
		c3Char = checkUTF8Char (text, i, c3Char, retval, substituted);		
	}
	else {
		*c2Char = checkUTF8C2Char (text, i, *c2Char, retval, substituted);
	}
	return c3Char;
}

int cUPnPClassObject::checkSpecialChars (const char* text, char** buf){
	std::string substituted = "";
	const int INITIAL = -1;
	int retval = INITIAL;
	int doubleReplace = 0;
	if (text){
		const int truncateLen = UPNP_MAX_METADATA_LENGTH * 3 / 4;
		const int maxStrLen = strlen(text); 
		int iChar = 0;
		bool c2Char = false;
		bool c3Char = false;
		bool stop = false;
		for (int i = 0; i < maxStrLen && !stop && retval == INITIAL; i++){
			iChar = text[i] & 0xff;
			switch(iChar){
			case 0x20: // space
				if (i + doubleReplace > truncateLen){
					substituted += "...";
					stop = true;
				}
				else {
					substituted += text[i];	
					c3Char = false;
					c2Char = false;
				}
				break;
			case 0x80:  // A accent grave
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x81:  // A accent acute
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x82:  // A accent circumflex
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x84:  // Ae
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x85:  // Angstroem sign
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x88:  // E accent grave
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x89:  // E accent acute
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x8a:  // E accent circumflex
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x91:  // N tilde
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x92:  // O accent grave
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x93:  // O accent acute
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x94:  // O accent circumflex
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x96:  // Oe after 0xc3 or hyphen after 0xc2
				if (c2Char){
					substituted += "- ";
					c2Char = false;
				}
				else {
				   c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				}
				break;
			case 0x99:  // U accent grave
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x9a:  // U accent acute
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x9b:  // U accent circumflex
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x9c:   // Ue
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0x9f:  // sharp s
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xa0:  // a accent grave
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xa1:  // a accent acute
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xa2:  // a accent circumflex after 0xc3 or the registered sign after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xa3:  // a tilde after 0xc3 or pound sign after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xa4:  // ae
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xa5:  // small angstroem after 0xc3 or yen sign after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xa7:  // c cedilla
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xa8:  // e accent grave
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xa9:  // e accent acute after 0xc3 or the copyright sign after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xaa:  // e accent circumflex
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xab:  // e with diaeresis after 0xc3 or the left-pointing double angle quotation mark after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xad:  // i accent acute after 0xc3 or soft hyphen after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xae:  // i accent circumflex
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xaf:  // i with diaeresis
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xb0:  // degree
				c2Char = checkUTF8C2Char (text, i, c2Char, &retval, &substituted);
				break;
			case 0xb1:  // n tilde after 0xc3 or the plus-minus sign after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xb2:  // o accent grave after 0xc3 or superscript two after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xb3:  // o accent acute
				if (c3Char){
					c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				}
				else {
					substituted += "?";
					retval = (retval == INITIAL) ? ((i > 0) ? i - 1 : 0) : retval;
				}
				break;
			case 0xb4:  // o accent circumflex after 0xc3 or accent acute after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xb6:   // oe
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xb7:   // division sign after 0xc3
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xb8:   // o with stroke
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xb9:  // u accent grave after 0xc3 or superscript one after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xba:  // u accent acute after 0xc3 or masculine ordinale indicator after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xbb:  // u accent circumflex after 0xc3 or the right-pointing double angle quotation mark after 0xc2
				c3Char = checkUTF8Follower (text, i, &c2Char, c3Char, &retval, &substituted);
				break;
			case 0xbc:   // ue
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xbe:   // small letter thorn
				c3Char = checkUTF8Char (text, i, c3Char, &retval, &substituted);
				break;
			case 0xbf:   // inverted question mark
				c2Char = checkUTF8C2Char (text, i, c2Char, &retval, &substituted);
				break;
			case 0xc4:
				substituted += "Ae";
				if (retval == INITIAL) doubleReplace++;
				break;
			case 0xd6:
				substituted += "Oe";
				if (retval == INITIAL) doubleReplace++;
				break;
			case 0xdc:
				substituted += "Ue";
				if (retval == INITIAL) doubleReplace++;
				break;
			case 0xe4:
				substituted += "ae";
				if (retval == INITIAL) doubleReplace++;
				break;
			case 0xf6:
				substituted += "oe";
				if (retval == INITIAL) doubleReplace++;
				break;
			case 0xfc:
				substituted += "ue";
				if (retval == INITIAL) doubleReplace++;
				break;
			case 0xdf:
				substituted += "ss";
				if (retval == INITIAL) doubleReplace++;
				break;
			case 0xc2:
				c2Char = true;
				break;
			case 0xc3:
				c3Char = true;
				break;
			case 0xa6:  // after a previous 0xc4
				substituted += "?";
				retval = (retval == INITIAL) ? ((i > 0) ? i - 1 : 0) : retval;
				break;
			default:
				if (iChar > 127){
					if (c2Char){
						retval = (retval == INITIAL) ? i-1 : retval;
					}
					else {
						retval = (retval == INITIAL) ? ((c3Char) ? i-1 : i) : retval;
					}
					substituted += "?";		
					if (i > 2){
						MESSAGE(VERBOSE_MODIFICATIONS, "Got an unexpected character at position %i: %x %x %x %x", i, (int)(text[i-3] & 0xff),
							(int)(text[i-2] & 0xff), (int)(text[i-1] & 0xff), (int)(text[i] & 0xff));
					}
					else if (i > 0){
						MESSAGE(VERBOSE_MODIFICATIONS, "Got an unexpected character at position %i: %x %x", i, 
									 (int)(text[i-1] & 0xff), (int)(text[i] & 0xff));
					}	
					else {
						MESSAGE(VERBOSE_MODIFICATIONS, "Got an unexpected character at position %i: %x", i, (int)(text[i] & 0xff));
					}
				}
				else {
					substituted += text[i];					
				}
				c3Char = false;
				c2Char = false;
				break;
			}
		}
	}
	*buf = strdup(substituted.c_str());
	if (retval != INITIAL){
		retval += doubleReplace;
	}
	return retval; 
 }

char* cUPnPClassObject::getSubstring (const char* text, int length){
	if (text && length > 0){
		char* firstPart = substr(text, 0, length);
		char* ret = (char*)malloc(sizeof(firstPart)*length + 4);
		strcpy (ret, firstPart);
		strcat (ret, "...");
		return ret;
	}
	return strdup("No valid char");
}

 /**********************************************\
 *                                              *
 *  Item                                        *
 *                                              *
 \**********************************************/

cUPnPClassItem::cUPnPClassItem(){
    this->setClass(UPNP_CLASS_ITEM);
    this->mReference = NULL;
}

int cUPnPClassItem::setReference(cUPnPClassItem* Reference){
    this->mReference = Reference;
    return 0;
}

cStringList* cUPnPClassItem::getPropertyList(){
    cStringList* Properties = cUPnPClassObject::getPropertyList();
    Properties->Append(strdup(UPNP_PROP_REFERENCEID));
    return Properties;
}

bool cUPnPClassItem::getProperty(const char* Property, char** Value) const {

    if(!strcasecmp(Property, SQLITE_COL_REFERENCEID) || !strcasecmp(Property, UPNP_PROP_REFERENCEID)){
        *Value = strdup0(*this->getReferenceID());
    }
    else return cUPnPClassObject::getProperty(Property, Value);
    return true;
}

bool cUPnPClassItem::setProperty(const char* Property, const char* Value){
    return cUPnPClassObject::setProperty(Property, Value);
}

IXML_Node* cUPnPClassItem::createDIDLFragment(IXML_Document* Document, cStringList* Filter){
    IXML_Element* eItem = createDIDLSubFragment1(Document);
	char* buf = NULL;
	int len = -1;
	if ((len = checkSpecialChars(this->getTitle(), &buf)) < 0){
        ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(buf));
	}
	else {
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, getSubstring(buf, len));
	}
    return (IXML_Node*)createDIDLSubFragment2(Filter, eItem);
}

IXML_Element* cUPnPClassItem::createDIDLSubFragment1(IXML_Document* Document){
	this->mDIDLFragment = Document;

	MESSAGE(VERBOSE_DIDL, "==(%s)= %s =====", *this->getID(), this->getTitle());
//	MESSAGE(VERBOSE_DIDL, "ParentID: %s", *this->getParentID());
	MESSAGE(VERBOSE_DIDL, "Class: %s", this->getClass());
//	MESSAGE(VERBOSE_DIDL, "Filter: %d", Filter?Filter->Size():-1);
	IXML_Node* Didl = ixmlNode_getFirstChild((IXML_Node*) this->mDIDLFragment);
	IXML_Element* eItem = ixmlDocument_createElement(this->mDIDLFragment, "item");
	ixmlNode_appendChild(Didl, (IXML_Node*) eItem);

	ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_OBJECTID, *this->getID());
	ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_PARENTID, *this->getParentID());
    ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_RESTRICTED, this->isRestricted()?"1":"0");
    return eItem;
}

IXML_Node* cUPnPClassItem::createDIDLSubFragment2(cStringList* Filter, IXML_Element* eItem){
	ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_CLASS, this->getClass());
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_CREATOR, this->getCreator());
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_WRITESTATUS, itoa(this->getWriteStatus()));
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_REFERENCEID, ((int)(this->getReferenceID())<0)?"":*this->getReferenceID());
	return createDIDLResFragment(Filter, eItem);
}

IXML_Node* cUPnPClassItem::createDIDLResFragment(cStringList* Filter, IXML_Element* eItem){
	cUPnPConfig* config = cUPnPConfig::get();
	int ctr = 0;
	for(cUPnPResource* Resource = this->getResources()->First(); ctr++ < 5 && Resource; Resource = this->getResources()->Next(Resource)){
		MESSAGE(VERBOSE_DIDL, "Resource: %s", Resource->getResource());
//		MESSAGE(VERBOSE_DIDL, "Protocolinfo: %s", Resource->getProtocolInfo());
		cString URLBase = cString::sprintf("http://%s:%d", UpnpGetServerIpAddress(), UpnpGetServerPort());
		cString ResourceURL = cString::sprintf("%s%s/get?resId=%d", *URLBase, UPNP_DIR_SHARES, Resource->getID());
		MESSAGE(VERBOSE_DIDL, "Resource-URI: %s", *ResourceURL);

		IXML_Element* eRes = ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_RESOURCE, *ResourceURL);
		if(eRes){
			ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eRes, UPNP_PROP_BITRATE, itoa(Resource->getBitrate()));
			ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eRes, UPNP_PROP_BITSPERSAMPLE, itoa(Resource->getBitsPerSample()));
			ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eRes, UPNP_PROP_COLORDEPTH, itoa(Resource->getColorDepth()));
			bool changeDurationZero = config->mDurationZeroChange && Resource->getResDuration() &&
				                      strcmp(skipspace(Resource->getResDuration()), "0:00:00") == 0;
			ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eRes, UPNP_PROP_DURATION, 
				  (changeDurationZero || Resource->getResDuration() == NULL) ? "0:50:00" : skipspace(Resource->getResDuration()));
			ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eRes, UPNP_PROP_PROTOCOLINFO, Resource->getProtocolInfo());
			off64_t fileSize = Resource->getFileSize();
			if (config->mDurationZeroChange && fileSize == ((off64_t)-1)){
				fileSize = (off64_t) 6000000000;
			}
			ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eRes, UPNP_PROP_SIZE, cString::sprintf("%lld", fileSize));
		}
	}
    return (IXML_Node*)eItem;
}

 /**********************************************\
 *                                              *
 *  Container                                   *
 *                                              *
 \**********************************************/

cUPnPClassContainer::cUPnPClassContainer(){
    this->setClass(UPNP_CLASS_CONTAINER);
    this->mChildren   = new cUPnPObjects;
    this->mChildrenID = new cHash<cUPnPClassObject>;
    this->mContainerType = NULL;
    this->mUpdateID = 0;
    this->mSearchable = false;
}

cUPnPClassContainer::~cUPnPClassContainer(){
	if (this->mChildren){
       delete this->mChildren;
	}
	if (this->mChildrenID){
       delete this->mChildrenID;
	}
}

IXML_Node* cUPnPClassContainer::createDIDLFragment(IXML_Document* Document, cStringList* Filter){
    this->mDIDLFragment = Document;

    MESSAGE(VERBOSE_DIDL, "===(%s)= %s =====", *this->getID(), this->getTitle());
//    MESSAGE(VERBOSE_DIDL, "ParentID: %s", *this->getParentID());
    MESSAGE(VERBOSE_DIDL, "Class: %s", this->getClass());
//    MESSAGE(VERBOSE_DIDL, "Filter: %d", Filter?Filter->Size():-1);

    IXML_Node* Didl = ixmlNode_getFirstChild((IXML_Node*) this->mDIDLFragment);
    IXML_Element* eItem = ixmlDocument_createElement(this->mDIDLFragment, "container");
    ixmlNode_appendChild(Didl, (IXML_Node*) eItem);

    ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_OBJECTID, *this->getID());
    ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_PARENTID, *this->getParentID());
    ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_RESTRICTED, this->isRestricted()?"1":"0");
	char* buf = NULL;
	int len = -1;
	if ((len = checkSpecialChars(this->getTitle(), &buf)) < 0){
        ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(buf));
	}
	else {
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, getSubstring(buf, len));
	}

    ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_CLASS, this->getClass());

    ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_DLNA_CONTAINERTYPE, this->getContainerType());
    ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_CHILDCOUNT, itoa(this->getChildCount()));
    ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_SEARCHABLE, this->isSearchable()?"1":"0");

    const tClassVector* CreateClasses = this->getCreateClasses();
    for(unsigned int i = 0; i < CreateClasses->size(); i++){
        cClass CreateClass = CreateClasses->at(i);
        IXML_Element* eCreateClasses = ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_CREATECLASS, CreateClass.ID);
        if(eCreateClasses)
            ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_CCLASSDERIVED, CreateClass.includeDerived?"1":"0");
    }

    const tClassVector* SearchClasses = this->getSearchClasses();
    for(unsigned int i = 0; i < SearchClasses->size(); i++){
        cClass SearchClass = SearchClasses->at(i);
        IXML_Element* eSearchClasses = ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_SEARCHCLASS, SearchClass.ID);
        if(eSearchClasses)
            ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_SCLASSDERIVED, SearchClass.includeDerived?"1":"0");
    }

    return (IXML_Node*)eItem;
}

int cUPnPClassContainer::setUpdateID(unsigned int UID){
    this->mUpdateID = UID;
    return 0;
}

cStringList* cUPnPClassContainer::getPropertyList(){
    cStringList* Properties = cUPnPClassObject::getPropertyList();
    Properties->Append(strdup(UPNP_PROP_DLNA_CONTAINERTYPE));
    Properties->Append(strdup(UPNP_PROP_SEARCHABLE));
    return Properties;
}

bool cUPnPClassContainer::setProperty(const char* Property, const char* Value){
    int ret;
    if (!strcasecmp(Property, SQLITE_COL_DLNA_CONTAINERTYPE) || !strcasecmp(Property, UPNP_PROP_DLNA_CONTAINERTYPE)){
        ret = this->setContainerType(Value);
    }
    else if(!strcasecmp(Property, SQLITE_COL_SEARCHABLE) || !strcasecmp(Property, UPNP_PROP_SEARCHABLE)){
        ret = this->setSearchable(Value);
    }
    else if(!strcasecmp(Property, SQLITE_COL_CONTAINER_UID)){
        ret = this->setUpdateID((unsigned int)atoi(Value));
    }
    else {
		return cUPnPClassObject::setProperty(Property, Value);
	}
	return ret >= 0;
}

bool cUPnPClassContainer::getProperty(const char* Property, char** Value) const {
    cString Val;
    if (!strcasecmp(Property, SQLITE_COL_DLNA_CONTAINERTYPE) || !strcasecmp(Property, UPNP_PROP_DLNA_CONTAINERTYPE)){
        Val = this->getContainerType();
    }
    else if (!strcasecmp(Property, SQLITE_COL_SEARCHABLE) || !strcasecmp(Property, UPNP_PROP_SEARCHABLE)){
        Val = this->isSearchable()?"1":"0";
    }
    else if (!strcasecmp(Property, SQLITE_COL_CONTAINER_UID)){
        Val = cString::sprintf("%d", this->getUpdateID());
    }
    else return cUPnPClassObject::getProperty(Property, Value);
    *Value = strdup0(*Val);
    return true;
}

void cUPnPClassContainer::addObject(cUPnPClassObject* Object){
//    MESSAGE(VERBOSE_MODIFICATIONS, "Adding object (ID:%s) to container (ID:%s); children until now: %i", *Object->getID(), *this->getID(), this->countObjects());
    Object->setParent(this);
    this->mChildren->Add(Object);
    this->mChildrenID->Add(Object, (unsigned int)Object->getID());
}

int cUPnPClassContainer::countObjects(){
	return this->mChildren->Count();
}

void cUPnPClassContainer::removeObject(cUPnPClassObject* Object){
	if (Object == NULL){
		return;
	}
	if (this->mChildrenID && this->mChildrenID->Get((unsigned int)Object->getID())){
        this->mChildrenID->Del(Object, (unsigned int)Object->getID());
	}
	else {
		MESSAGE(VERBOSE_MODIFICATIONS, "Could not delete from chidren ID list");
	}
	if (this->mChildren && this->mChildren->Count() > 0){
       this->mChildren->Del(Object, false);
	}
    Object->mParent = NULL;
}

cUPnPClassObject* cUPnPClassContainer::getObject(cUPnPObjectID ID) const {
    MESSAGE(VERBOSE_METADATA, "Getting object (ID:%s); container count: %i", *ID, this->mChildren->Count());
    if ((int)ID < 0){
        ERROR("Invalid object ID");
        return NULL;
    }
    return this->mChildrenID->Get((unsigned int)ID);
}

int cUPnPClassContainer::setContainerType(const char* Type){
    if (Type == NULL){
        this->mContainerType = Type;
    }
	else if (isempty(Type)){
		 this->mContainerType = NULL;
	}
    else if (!strcasecmp(Type, DLNA_CONTAINER_TUNER)){
        this->mContainerType = Type;
    }
    else {
        ERROR("Invalid container type '%s'",Type);
        return -1;
    }
    return 0;
}

int cUPnPClassContainer::addSearchClass(cClass SearchClass){
    this->mSearchClasses.push_back(SearchClass);
    return 0;
}

int cUPnPClassContainer::delSearchClass(cClass SearchClass){
    tClassVector::iterator it = this->mSearchClasses.begin();
    cClass Class;
    for(unsigned int i=0; i<this->mSearchClasses.size(); i++){
        Class = this->mSearchClasses[i];
        if(Class == SearchClass){
            this->mSearchClasses.erase(it+i);
            return 0;
        }
    }
    return -1;
}

int cUPnPClassContainer::addCreateClass(cClass CreateClass){
    this->mCreateClasses.push_back(CreateClass);
    return 0;
}

int cUPnPClassContainer::delCreateClass(cClass CreateClass){
    tClassVector::iterator it = this->mCreateClasses.begin();
    cClass Class;
    for (unsigned int i=0; i<this->mCreateClasses.size(); i++){
        Class = this->mCreateClasses[i];
        if (Class == CreateClass){
            this->mCreateClasses.erase(it+i);
            return 0;
        }
    }
    return -1;
}

int cUPnPClassContainer::setSearchClasses(std::vector<cClass> SearchClasses){
    this->mSearchClasses = SearchClasses;
    return 0;
}

int cUPnPClassContainer::setCreateClasses(std::vector<cClass> CreateClasses){
    this->mCreateClasses = CreateClasses;
    return 0;
}

int cUPnPClassContainer::setSearchable(bool Searchable){
    this->mSearchable = Searchable;
    return 0;
}

bool cUPnPClassContainer::isUpdated(){
    static unsigned int lastUpdateID = this->getUpdateID();
    if (lastUpdateID != this->getUpdateID()){
        lastUpdateID = this->getUpdateID();
        return true;
    }
    return false;
}

/**********************************************\
 *                                              *
 *  Epg  Container                              *
 *                                              *
 \**********************************************/
cUPnPClassEpgContainer::cUPnPClassEpgContainer(){
	this->mIsRadioChannel = false;
	this->mChannelName = NULL;
	this->mChannelId = NULL;
	this->setClass(UPNP_CLASS_EPGCONTAINER);
}

cUPnPClassEpgContainer::~cUPnPClassEpgContainer(){
}

cStringList* cUPnPClassEpgContainer::getPropertyList(){
    cStringList* Properties = cUPnPClassObject::getPropertyList();
    return Properties;
}

int cUPnPClassEpgContainer::setChannelId (const char* channelId){
	this->mChannelId = strdup0(channelId);
	return 0;
}

int cUPnPClassEpgContainer::setChannelName (const char* channelName){
	this->mChannelName = strdup0(channelName);
	return 0;
}

int cUPnPClassEpgContainer::setRadioChannel (const bool val){
	this->mIsRadioChannel = val;
	return 0;
}

bool cUPnPClassEpgContainer::getProperty(const char* Property, char** Value) const{
    cString Val = NULL;
    if (!strcasecmp(Property, SQLITE_COL_CHANNELID)){
        Val = this->getChannelId();
    }
	else if (!strcasecmp(Property, SQLITE_COL_CHANNELNAME2)){
        Val = this->getChannelName();
    }
	else if (!strcasecmp(Property, SQLITE_COL_ISRADIOCHANNEL)){
		Val = (this->isRadioChannel()) ? strdup("1") : strdup("0");
	}
    else {
		return cUPnPClassContainer::getProperty(Property, Value);
	}
    *Value = strdup0(*Val);
	return true;
}

 /**********************************************\
 *                                              *
 *  Video item                                  *
 *                                              *
 \**********************************************/

cUPnPClassVideoItem::cUPnPClassVideoItem(){
    this->setClass(UPNP_CLASS_VIDEO);
    this->mGenre = NULL;
    this->mLongDescription = NULL;
    this->mProducers = NULL;
    this->mRating = NULL;
    this->mActors = NULL;
    this->mDirectors = NULL;
    this->mDescription = NULL;
    this->mPublishers = NULL;
    this->mLanguage = NULL;
    this->mRelations = NULL;
}

cUPnPClassVideoItem::~cUPnPClassVideoItem(){
}

IXML_Node* cUPnPClassVideoItem::createDIDLFragment(IXML_Document* Document, cStringList* Filter){
    IXML_Element* eItem = (IXML_Element*) cUPnPClassItem::createDIDLFragment(Document, Filter);
    return createDIDLVideoSubFragment(eItem, Filter);
}

IXML_Node* cUPnPClassVideoItem::createDIDLVideoSubFragment(IXML_Element* eItem, cStringList* Filter){
	char* longDescr = strdup0(this->getLongDescription());
	char* buf = NULL;
	int len = -1;
	if (longDescr && strlen(longDescr) > 0){
		if ((len = checkSpecialChars(longDescr, &buf)) >= 0){
			longDescr = getSubstring(buf, len);
		}
		else {
			longDescr = strdup(buf);
		}
		ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_LONGDESCRIPTION, checkString (longDescr));
	}
	char* descr = strdup0(this->getDescription());
	buf = NULL;
	len = -1;
	if (descr && strlen(descr) > 0){
		if ((len = checkSpecialChars(descr, &buf)) >= 0){
			descr = getSubstring(buf, len);
		}
		else {
			descr = strdup(buf);
		}
		ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_DESCRIPTION, checkString (descr));
	}
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_LANGUAGE, this->getLanguage());
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_RATING, this->getRating());

	cSplit* splitter = new cSplit();
	addSplittedProperties(this->getGenre(), splitter, Filter, eItem, (const char*) UPNP_PROP_GENRE);
	addSplittedProperties(this->getProducers(), splitter, Filter, eItem, (const char*) UPNP_PROP_PRODUCER);
	addSplittedProperties(this->getActors(), splitter, Filter, eItem, (const char*) UPNP_PROP_ACTOR);
	addSplittedProperties(this->getDirectors(), splitter, Filter, eItem, (const char*) UPNP_PROP_DIRECTOR);
	addSplittedProperties(this->getPublishers(), splitter, Filter, eItem, (const char*) UPNP_PROP_PUBLISHER);
	addSplittedProperties(this->getRelations(), splitter, Filter, eItem, (const char*) UPNP_PROP_RELATION);
    return (IXML_Node*) eItem;
}

void cUPnPClassVideoItem::addSplittedProperties(const char* css, cSplit* splitter, cStringList* Filter, IXML_Element* eItem, const char* propType){
	std::vector<std::string>* tokens = splitter->split(css, ",");
	for (int i = 0; i < (int) tokens->size(); i++){
		char* upnpProp = skipspace(tokens->at(i).c_str());
		if (upnpProp && *upnpProp){
			ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, propType, upnpProp);
		}
	}
}

cStringList* cUPnPClassVideoItem::getPropertyList(){
    cStringList* Properties = cUPnPClassItem::getPropertyList();
    Properties->Append(strdup(UPNP_PROP_LONGDESCRIPTION));
    Properties->Append(strdup(UPNP_PROP_PRODUCER));
    Properties->Append(strdup(UPNP_PROP_GENRE));
    Properties->Append(strdup(UPNP_PROP_RATING));
    Properties->Append(strdup(UPNP_PROP_ACTOR));
    Properties->Append(strdup(UPNP_PROP_DIRECTOR));
    Properties->Append(strdup(UPNP_PROP_DESCRIPTION));
    Properties->Append(strdup(UPNP_PROP_PUBLISHER));
    Properties->Append(strdup(UPNP_PROP_LANGUAGE));
    Properties->Append(strdup(UPNP_PROP_RELATION));
    return Properties;
}

bool cUPnPClassVideoItem::getProperty(const char* Property, char** Value) const {
    cString Val;
    if (!strcasecmp(Property,SQLITE_COL_GENRE) || !strcasecmp(Property,UPNP_PROP_GENRE)){
        Val = this->getGenre();
    }
    else if (!strcasecmp(Property,SQLITE_COL_LONGDESCRIPTION) || !strcasecmp(Property,UPNP_PROP_LONGDESCRIPTION)){
        Val = this->getLongDescription();
    }
    else if (!strcasecmp(Property,SQLITE_COL_PRODUCER) || !strcasecmp(Property,UPNP_PROP_PRODUCER)){
        Val = this->getProducers();
    }
    else if (!strcasecmp(Property,SQLITE_COL_RATING) || !strcasecmp(Property,UPNP_PROP_RATING)){
        Val = this->getRating();
    }
    else if (!strcasecmp(Property,SQLITE_COL_ACTOR) || !strcasecmp(Property,UPNP_PROP_ACTOR)){
        Val = this->getActors();
    }
    else if (!strcasecmp(Property,SQLITE_COL_DIRECTOR) || !strcasecmp(Property,UPNP_PROP_DIRECTOR)){
        Val = this->getDirectors();
    }
    else if (!strcasecmp(Property,SQLITE_COL_DESCRIPTION) || !strcasecmp(Property,UPNP_PROP_DESCRIPTION)){
        Val = this->getDescription();
    }
    else if (!strcasecmp(Property,SQLITE_COL_PUBLISHER) || !strcasecmp(Property,UPNP_PROP_PUBLISHER)){
        Val = this->getPublishers();
    }
    else if (!strcasecmp(Property,SQLITE_COL_LANGUAGE) || !strcasecmp(Property,UPNP_PROP_LANGUAGE)){
        Val = this->getLanguage();
    }
    else if (!strcasecmp(Property,SQLITE_COL_RELATION) || !strcasecmp(Property,UPNP_PROP_RELATION)){
        Val = this->getRelations();
    }
    else return cUPnPClassItem::getProperty(Property, Value);
    *Value = strdup0(*Val);
    return true;
}

bool cUPnPClassVideoItem::setProperty(const char* Property, const char* Value){
    bool ret;
    if (!strcasecmp(Property,SQLITE_COL_GENRE) || !strcasecmp(Property,UPNP_PROP_GENRE)){
        ret = this->setGenre(Value);
    }
    else if (!strcasecmp(Property,SQLITE_COL_LONGDESCRIPTION) || !strcasecmp(Property,UPNP_PROP_LONGDESCRIPTION)){
        ret = this->setLongDescription(Value);
    }
    else if (!strcasecmp(Property,SQLITE_COL_PRODUCER) || !strcasecmp(Property,UPNP_PROP_PRODUCER)){
        ret = this->setProducers(Value);
    }
    else if (!strcasecmp(Property,SQLITE_COL_RATING) || !strcasecmp(Property,UPNP_PROP_RATING)){
        ret = this->setRating(Value);
    }
    else if (!strcasecmp(Property,SQLITE_COL_ACTOR) || !strcasecmp(Property,UPNP_PROP_ACTOR)){
        ret = this->setActors(Value);
    }
    else if (!strcasecmp(Property,SQLITE_COL_DIRECTOR) || !strcasecmp(Property,UPNP_PROP_DIRECTOR)){
        ret = this->setDirectors(Value);
    }
    else if (!strcasecmp(Property,SQLITE_COL_DESCRIPTION) || !strcasecmp(Property,UPNP_PROP_DESCRIPTION)){
        ret = this->setDescription(Value);
    }
    else if (!strcasecmp(Property,SQLITE_COL_PUBLISHER) || !strcasecmp(Property,UPNP_PROP_PUBLISHER)){
        ret = this->setPublishers(Value);
    }
    else if (!strcasecmp(Property,SQLITE_COL_LANGUAGE) || !strcasecmp(Property,UPNP_PROP_LANGUAGE)){
        ret = this->setLanguage(Value);
    }
    else if (!strcasecmp(Property,SQLITE_COL_RELATION) || !strcasecmp(Property,UPNP_PROP_RELATION)){
        ret = this->setRelations(Value);
    }
    else {
		return cUPnPClassItem::setProperty(Property, Value);
	}
	return ret >= 0;
}

int cUPnPClassVideoItem::setActors(const char* Actors){
    this->mActors = Actors;
    return 0;
}

int cUPnPClassVideoItem::setGenre(const char* Genre){
    this->mGenre = Genre;
    return 0;
}

int cUPnPClassVideoItem::setDescription(const char* Description){ 
	if (Description){
       this->mDescription = skipspace(stripspace(strdup(Description)));
	}
	else {
		this->mDescription = Description;
	}
    return 0;
}

int cUPnPClassVideoItem::setLongDescription(const char* LongDescription){
	if (LongDescription){
        this->mLongDescription = skipspace(stripspace(strdup(LongDescription)));
	}
	else {
	   this->mLongDescription = LongDescription;
	}
    return 0;
}

int cUPnPClassVideoItem::setProducers(const char* Producers){
    this->mProducers = Producers;
    return 0;
}

int cUPnPClassVideoItem::setRating(const char* Rating){
    this->mRating = Rating;
    return 0;
}

int cUPnPClassVideoItem::setDirectors(const char* Directors){
    this->mDirectors = Directors;
    return 0;
}

int cUPnPClassVideoItem::setPublishers(const char* Publishers){
    this->mPublishers = Publishers;
    return 0;
}

int cUPnPClassVideoItem::setLanguage(const char* Language){
    this->mLanguage = Language;
    return 0;
}

int cUPnPClassVideoItem::setRelations(const char* Relations){
    this->mRelations = Relations;
    return 0;
}

 /**********************************************\
 *                                              *
 *  Audio item                                  *
 *                                              *
 \**********************************************/

cUPnPClassAudioItem::cUPnPClassAudioItem(){
    this->setClass(UPNP_CLASS_AUDIO);
}

cUPnPClassAudioItem::~cUPnPClassAudioItem(){
}

 /**********************************************\
 *                                              *
 *  Audio Broadcast item                        *
 *                                              *
 \**********************************************/

cUPnPClassAudioBroadcast::cUPnPClassAudioBroadcast(){
    this->setClass(UPNP_CLASS_AUDIOBC);
    this->mChannelNr = 0;
}

cUPnPClassAudioBroadcast::~cUPnPClassAudioBroadcast(){
}

IXML_Node* cUPnPClassAudioBroadcast::createDIDLFragment(IXML_Document* Document, cStringList* Filter){
	IXML_Element* eItem = createDIDLSubFragment1(Document);
	cUPnPConfig* config = cUPnPConfig::get();
	char* title = strdup0(this->getTitle());
	char* buf = NULL;
	int len = -1;
	if (!title){
		title = strdup("Missing title");
	}
	else if ((len = checkSpecialChars(title, &buf)) >= 0){
		title = getSubstring(buf, len);
	}
	else {
		title = strdup(buf);
	}
	char* channel = strdup0(getChannelName());
	char* channelNumber = strdup ((const char*) itoa(getChannelNr()));

	buf = NULL;
	len = -1;
	if (!channel){
		channel = strdup("Got no channel");
	}
	else if ((len = checkSpecialChars(channel, &buf)) >= 0){
		channel = getSubstring(buf, len);
	}
	else {
		channel = strdup(buf);
	}
	bool channelAdded = false;
	if (config->mBroadcastPrepend && channel){
		MESSAGE(VERBOSE_DIDL, "Got a audio broadcast item mediator, channel name: %s", channel);
		int length1 = strlen(title);
		int length2 = strlen(channel);
		int length3 = strlen(channelNumber);
		char* titlePrep = (char*) malloc(sizeof(titlePrep) * (length1 + length2 + length3 + 7));
		strcpy (titlePrep, channelNumber);			
		strcat (titlePrep, " - ");
		strcat (titlePrep, channel);
		strcat (titlePrep, " - ");
		strcat (titlePrep, title);
			
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString (titlePrep));
		channelAdded = true;
		MESSAGE(VERBOSE_DIDL, "cUPnPClassAudioBroadcast:  modified title: %s", titlePrep);
		free(titlePrep);
	}
	if (!config->mBroadcastPrepend || !channelAdded) {
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString (title));
	}

	if (config->mChangeRadioClass){
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_CLASS, UPNP_CLASS_VIDEOBC);
	}else {
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_CLASS, this->getClass());
	}
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_CREATOR, this->getCreator());
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_WRITESTATUS, itoa(this->getWriteStatus()));
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_REFERENCEID, ((int)(this->getReferenceID())<0)?"":*this->getReferenceID());
	createDIDLResFragment(Filter, eItem);

	if (channel){
		ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_CHANNELNAME, channel);
	}
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_CHANNELNR, channelNumber);
    return (IXML_Node*) eItem;
}

cStringList* cUPnPClassAudioBroadcast::getPropertyList(){
    cStringList* Properties = cUPnPClassAudioItem::getPropertyList();
    Properties->Append(strdup(UPNP_PROP_CHANNELNAME));
    Properties->Append(strdup(UPNP_PROP_CHANNELNR));
    return Properties;
}

bool cUPnPClassAudioBroadcast::setProperty(const char* Property, const char* Value){
    bool ret;
    if (!strcasecmp(Property, SQLITE_COL_RADIOSTATIONID) || !strcasecmp(Property, UPNP_PROP_CHANNELNAME)){
        ret = this->setChannelName(Value);
    }
    else if (!strcasecmp(Property, SQLITE_COL_CHANNELNR) || !strcasecmp(Property, UPNP_PROP_CHANNELNR)){
        ret = this->setChannelNr(atoi(Value));
    }
    else {
		return cUPnPClassAudioItem::setProperty(Property, Value);
	}
	return ret >= 0;
}

bool cUPnPClassAudioBroadcast::getProperty(const char* Property, char** Value) const {
    cString Val;
    if (!strcasecmp(Property, SQLITE_COL_RADIOSTATIONID) || !strcasecmp(Property, UPNP_PROP_CHANNELNAME)){
        Val = this->getChannelName();
    }
    else if (!strcasecmp(Property, SQLITE_COL_CHANNELNR) || !strcasecmp(Property, UPNP_PROP_CHANNELNR)){
        Val = itoa(this->getChannelNr());
    }
    else {
		return cUPnPClassAudioItem::getProperty(Property, Value);
	}
    *Value = strdup0(*Val);
    return true;
}

int cUPnPClassAudioBroadcast::setChannelNr(int ChannelNr){
    this->mChannelNr = ChannelNr;
    return 0;
}

int cUPnPClassAudioBroadcast::setChannelName(const char* ChannelName){
    this->mChannelName = ChannelName;
    return 0;
}

 /**********************************************\
 *                                              *
 *  Video Broadcast item                        *
 *                                              *
 \**********************************************/

cUPnPClassVideoBroadcast::cUPnPClassVideoBroadcast(){
    this->setClass(UPNP_CLASS_VIDEOBC);
    this->mIcon = NULL;
    this->mRegion = NULL;
    this->mChannelNr = 0;
}

cUPnPClassVideoBroadcast::~cUPnPClassVideoBroadcast(){
}

IXML_Node* cUPnPClassVideoBroadcast::createDIDLFragment(IXML_Document* Document, cStringList* Filter){
	IXML_Element* eItem = createDIDLSubFragment1(Document);
	cUPnPConfig* config = cUPnPConfig::get();
	char* title = strdup0(this->getTitle());
	char* buf = NULL;
	int len = -1;
	if (!title){
		title = strdup("Missing title");
	}
	else if ((len = checkSpecialChars(title, &buf)) >= 0){
		MESSAGE(VERBOSE_DIDL, "cUPnPClassVideoBroadcast: Truncated title length %i", len);
		title = getSubstring(buf, len);
	}
	else {
		title = strdup(buf);
	}
	char* channel = strdup0(getChannelName());
	char* channelNumber = strdup ((const char*) itoa(getChannelNr()));
	buf = NULL;
	if (!channel){
		channel = strdup("No channel name");
	}
	else if ((len = checkSpecialChars(channel, &buf)) >= 0){
		channel = getSubstring(buf, len);
	}
	else {
		channel = strdup(buf);
	}
	bool channelAdded = false;
	if (config->mBroadcastPrepend && channel){
		int length1 = strlen(title);
		int length2 = strlen(channel);
		int length3 = strlen(channelNumber);
		char* titlePrep = (char*) malloc(sizeof(titlePrep) * (length1 + length2 + length3 + 7));
		strcpy (titlePrep, channelNumber);			
		strcat (titlePrep, " - ");
		strcat (titlePrep, channel);
		strcat (titlePrep, " - ");
		strcat (titlePrep, title);
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(titlePrep));

		channelAdded = true;				
		MESSAGE(VERBOSE_DIDL, "Modified video broadcast title: '%s'", titlePrep);
		free(titlePrep);
	}
	if (!config->mBroadcastPrepend || !channelAdded) {
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(title));
	}

    createDIDLSubFragment2(Filter, eItem);
	if (channel){
		ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_CHANNELNAME, channel);
	}
	ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_CHANNELNR, channelNumber);
    ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_ICON, this->getIcon());
    ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_REGION, this->getRegion());
    return (IXML_Node*) eItem;
}

cStringList* cUPnPClassVideoBroadcast::getPropertyList(){
    cStringList* Properties = cUPnPClassVideoItem::getPropertyList();
    Properties->Append(strdup(UPNP_PROP_CHANNELNAME));
    Properties->Append(strdup(UPNP_PROP_CHANNELNR));
    Properties->Append(strdup(UPNP_PROP_ICON));
    Properties->Append(strdup(UPNP_PROP_REGION));
    return Properties;
}

bool cUPnPClassVideoBroadcast::setProperty(const char* Property, const char* Value){
    bool ret;
    if (!strcasecmp(Property, SQLITE_COL_CHANNELNAME) || !strcasecmp(Property, UPNP_PROP_CHANNELNAME)){
        ret = this->setChannelName(Value);
    }
    else if (!strcasecmp(Property, SQLITE_COL_CHANNELNR) || !strcasecmp(Property, UPNP_PROP_CHANNELNR)){
        ret = this->setChannelNr(atoi(Value));
    }
    else if (!strcasecmp(Property, SQLITE_COL_ICON) || !strcasecmp(Property, UPNP_PROP_ICON)){
        ret = this->setIcon(Value);
    }
    else if (!strcasecmp(Property, SQLITE_COL_REGION) || !strcasecmp(Property, UPNP_PROP_REGION)){
        ret = this->setRegion(Value);
    }
    else return cUPnPClassVideoItem::setProperty(Property, Value);
//    return ret<0 ? false : true;
	return ret >= 0;
}

bool cUPnPClassVideoBroadcast::getProperty(const char* Property, char** Value) const {
    cString Val;
    if (!strcasecmp(Property, SQLITE_COL_CHANNELNAME) || !strcasecmp(Property, UPNP_PROP_CHANNELNAME)){
        Val = this->getChannelName();
    }
    else if (!strcasecmp(Property, SQLITE_COL_CHANNELNR) || !strcasecmp(Property, UPNP_PROP_CHANNELNR)){
        Val = itoa(this->getChannelNr());
    }
    else if (!strcasecmp(Property, SQLITE_COL_ICON) || !strcasecmp(Property, UPNP_PROP_ICON)){
        Val = this->getIcon();
    }
    else if (!strcasecmp(Property, SQLITE_COL_REGION) || !strcasecmp(Property, UPNP_PROP_REGION)){
        Val = this->getRegion();
    }
    else return cUPnPClassVideoItem::getProperty(Property, Value);
    *Value = strdup0(*Val);
    return true;
}

int cUPnPClassVideoBroadcast::setChannelName(const char* ChannelName){
    this->mChannelName = ChannelName;
    return 0;
}

int cUPnPClassVideoBroadcast::setChannelNr(int ChannelNr){
    this->mChannelNr = ChannelNr;
    return 0;
}

int cUPnPClassVideoBroadcast::setIcon(const char* IconURI){
    this->mIcon = IconURI;
    return 0;
}

int cUPnPClassVideoBroadcast::setRegion(const char* Region){
    this->mRegion = Region;
    return 0;
}

/**********************************************\
*                                              *
*  AudioRecord item                            *
*                                              *
\**********************************************/

cUPnPClassAudioRecord::cUPnPClassAudioRecord(){
	 this->mChannelName = strdup("");
}

cUPnPClassAudioRecord::~cUPnPClassAudioRecord(){
}

cStringList* cUPnPClassAudioRecord::getPropertyList(){
    cStringList* Properties = cUPnPClassAudioItem::getPropertyList();
    Properties->Append(strdup(UPNP_PROP_STORAGEMEDIUM));
    return Properties;
}

bool cUPnPClassAudioRecord::setProperty(const char* Property, const char* Value){
    bool ret;
    if (!strcasecmp(Property, SQLITE_COL_STORAGEMEDIUM) || !strcasecmp(Property, UPNP_PROP_STORAGEMEDIUM)){
        ret = this->setStorageMedium(Value);
    }
	else if (!strcasecmp(Property, SQLITE_COL_CHANNELNAME) || !strcasecmp(Property, UPNP_PROP_CHANNELNAME)){
        ret = this->setChannelName(Value);
    }
    else {
		return cUPnPClassAudioItem::setProperty(Property, Value);
	}
	return ret >= 0;
}

bool cUPnPClassAudioRecord::getProperty(const char* Property, char** Value) const {
    cString Val;
    if (!strcasecmp(Property, SQLITE_COL_STORAGEMEDIUM) || !strcasecmp(Property, UPNP_PROP_STORAGEMEDIUM)){
        Val = this->getStorageMedium();
    }
	else if (!strcasecmp(Property, SQLITE_COL_CHANNELNAME) || !strcasecmp(Property, UPNP_PROP_CHANNELNAME)){
        Val = this->getChannelName();
    }
    else{
		return cUPnPClassAudioItem::getProperty(Property, Value);
	}
    *Value = strdup0(*Val);
    return true;
}

int cUPnPClassAudioRecord::setChannelName(const char* ChannelName){
    this->mChannelName = ChannelName;
    return 0;
}

int cUPnPClassAudioRecord::setStorageMedium(const char* StorageMedium){
	char* storage = checkStorageMedium ((char*)StorageMedium);
	if (storage){
		this->mStorageMedium = storage;
		return 0;
	}
	return -1;
}

/**********************************************\
*                                              *
*  Movie item                                  *
*                                              *
\**********************************************/

cUPnPClassMovie::cUPnPClassMovie(){
     this->mDVDRegionCode = 2; // Europe
     this->mStorageMedium = UPNP_STORAGE_UNKNOWN;
	 this->mChannelName = strdup("");
}

cUPnPClassMovie::~cUPnPClassMovie(){}

IXML_Node* cUPnPClassMovie::createDIDLFragment(IXML_Document* Document, cStringList* Filter){
//	MESSAGE(VERBOSE_DIDL, "cUPnPClassMovie: create DIDL fragment");
	IXML_Element* eItem = createDIDLSubFragment1(Document);
	cUPnPConfig* config = cUPnPConfig::get();
	char* title = strdup0(this->getTitle());
	char* buf = NULL;
	int len = -1;
	if (title == NULL){
		title = strdup("Missing title");
	}
	else if ((len = checkSpecialChars(title, &buf)) >= 0){
		title = getSubstring(buf, len);
	}
	else {
		title = strdup(buf);
	}
	bool channelAdded = false;
	char* channel = strdup0(getChannelName());
	if (config->mTitleAppend && channel && !isempty(channel)){
		buf = NULL;
		len = -1;
		if ((len = checkSpecialChars(channel, &buf)) >= 0){
			channel = getSubstring(buf, len);
		}
		else {
			channel = strdup(buf);
		}
		int length1 = strlen(title);
		int length2 = strlen(channel);
		char* titlePrep = (char*) malloc(sizeof(titlePrep) * (length1 + length2 + 4));
		strcpy (titlePrep, title);			
		strcat (titlePrep, " - ");
		strcat (titlePrep, channel);
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(titlePrep));
		channelAdded = true;				
		MESSAGE(VERBOSE_DIDL, "Got a movie item mediator, modified title: %s", titlePrep);
		free(titlePrep);
	}
	if (!config->mTitleAppend || !channelAdded) {
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(title));
	}

    createDIDLSubFragment2(Filter, eItem);
	return createDIDLVideoSubFragment(eItem, Filter);
}

cStringList* cUPnPClassMovie::getPropertyList(){
    cStringList* Properties = cUPnPClassVideoItem::getPropertyList();
    Properties->Append(strdup(UPNP_PROP_DVDREGIONCODE));
    Properties->Append(strdup(UPNP_PROP_STORAGEMEDIUM));
    return Properties;
}

bool cUPnPClassMovie::setProperty(const char* Property, const char* Value){
    bool ret;
    if (!strcasecmp(Property, SQLITE_COL_DVDREGIONCODE) || !strcasecmp(Property, UPNP_PROP_DVDREGIONCODE)){
        ret = this->setDVDRegionCode(atoi(Value));
    }
    else if (!strcasecmp(Property, SQLITE_COL_STORAGEMEDIUM) || !strcasecmp(Property, UPNP_PROP_STORAGEMEDIUM)){
        ret = this->setStorageMedium(Value);
    }
	else if (!strcasecmp(Property, SQLITE_COL_CHANNELNAME) || !strcasecmp(Property, UPNP_PROP_CHANNELNAME)){
        ret = this->setChannelName(Value);
    }
    else {
		return cUPnPClassVideoItem::setProperty(Property, Value);
	}
	return ret >= 0;
}

bool cUPnPClassMovie::getProperty(const char* Property, char** Value) const {
    cString Val;
    if (!strcasecmp(Property, SQLITE_COL_DVDREGIONCODE) || !strcasecmp(Property, UPNP_PROP_DVDREGIONCODE)){
        Val = itoa(this->getDVDRegionCode());
    }
    else if (!strcasecmp(Property, SQLITE_COL_STORAGEMEDIUM) || !strcasecmp(Property, UPNP_PROP_STORAGEMEDIUM)){
        Val = this->getStorageMedium();
    }
	else if (!strcasecmp(Property, SQLITE_COL_CHANNELNAME) || !strcasecmp(Property, UPNP_PROP_CHANNELNAME)){
        Val = this->getChannelName();
    }
    else {
		return cUPnPClassVideoItem::getProperty(Property, Value);
	}
    *Value = strdup0(*Val);
    return true;
}

int cUPnPClassMovie::setDVDRegionCode(int RegionCode){
//    http://en.wikipedia.org/wiki/DVD_region_code
//    0 	Informal term meaning "worldwide". Region 0 is not an official setting; discs that bear the region 0 symbol either have no flag set or have region 1–6 flags set.
//    1 	Canada, United States; U.S. territories; Bermuda
//    2 	Western Europe; incl. United Kingdom, Ireland, and Central Europe; Eastern Europe, Western Asia; including Iran, Israel, Egypt; Japan, South Africa, Swaziland, Lesotho; French overseas territories
//    3 	Southeast Asia; South Korea; Taiwan; Hong Kong; Macau
//    4 	Mexico, Central and South America; Caribbean; Australia; New Zealand; Oceania;
//    5 	Ukraine, Belarus, Russia, Continent of Africa, excluding Egypt, South Africa, Swaziland, and Lesotho; Central and South Asia, Mongolia, North Korea.
//    6 	People's Republic of China
//    7 	Reserved for future use (found in use on protected screener copies of MPAA-related DVDs and "media copies" of pre-releases in Asia)
//    8 	International venues such as aircraft, cruise ships, etc.[1]
//    ALL (9)	Region ALL discs have all 8 flags set, allowing the disc to be played in any locale on any player.
    if (0 <= RegionCode && RegionCode <= 9){
        this->mDVDRegionCode = RegionCode;
        return 0;
    }
    else {
        ERROR("Invalid DVD region code: %d", RegionCode);
        return -1;
    }
}

int cUPnPClassMovie::setChannelName(const char* ChannelName){
    this->mChannelName = ChannelName;
    return 0;
}

int cUPnPClassMovie::setStorageMedium(const char* StorageMedium){
	char* storage = checkStorageMedium ((char*)StorageMedium);
	if (storage){
		this->mStorageMedium = storage;
		return 0;
	}
	return -1;
}

 /**********************************************\
 *                                              *
 *  Epg item                                    *
 *                                              *
 \**********************************************/

cUPnPClassEpgItem::cUPnPClassEpgItem(){
	this->mShortTitle = strdup("");                 
	this->mSynopsis = strdup("");   
	this->mGenres = strdup(""); 
	this->mStartTime = strdup("");                
	this->mVersion = strdup("");                  
    this->setClass(UPNP_CLASS_EPGITEM);
}

cUPnPClassEpgItem::~cUPnPClassEpgItem(){
}

bool cUPnPClassEpgItem::getProperty(const char* Property, char** Value) const {
    cString Val;
    if(!strcasecmp(Property, SQLITE_COL_BCEV_ID)){
        Val = itoa(this->getEventId());
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_STARTTIME) || !strcasecmp(Property, UPNP_PROP_SCHEDULEDSTARTTIME)){
        Val = this->getStartTime();
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_DURATION) || !strcasecmp(Property, UPNP_PROP_DURATION)){
        Val = itoa(this->getDuration());
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_TABLEID)){
        Val = itoa(this->getTableId());
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_VERSION)){
        Val = this->getVersion();
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_SHORTTITLE)){
        Val = this->getShortTitle();
    }
	else if(!strcasecmp(Property, SQLITE_COL_BCEV_SYNOPSIS) || !strcasecmp(Property, UPNP_PROP_LONGDESCRIPTION)){
        Val = this->getSynopsis();
    }
    else {
        return cUPnPClassItem::getProperty(Property, Value);
    }
    *Value = strdup0(*Val);
    return true;
}

bool cUPnPClassEpgItem::setProperty(const char* Property, const char* Value){
    if(!strcasecmp(Property, SQLITE_COL_BCEV_SYNOPSIS) || !strcasecmp(Property, UPNP_PROP_LONGDESCRIPTION)){
        this->setSynopsis(Value);
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_SHORTTITLE)){
        this->setShortTitle(Value);
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_VERSION)){
        this->setVersion(Value);
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_ID)){
        this->setEventId(atoi(Value));
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_TABLEID)){
        this->setTableId(atoi(Value));
    }
	else if(!strcasecmp(Property, SQLITE_COL_BCEV_DURATION) || !strcasecmp(Property, UPNP_PROP_DURATION)){
        this->setDuration(atoi(Value));
    }
	else if(!strcasecmp(Property, SQLITE_COL_BCEV_VERSION)){
        this->setVersion(Value);
    }
    else if(!strcasecmp(Property, SQLITE_COL_BCEV_STARTTIME) || !strcasecmp(Property, UPNP_PROP_SCHEDULEDSTARTTIME)){
        this->setStartTime(Value);
    }
    else {
		return cUPnPClassItem::setProperty(Property, Value);
	}
	return true;
}

IXML_Node* cUPnPClassEpgItem::createDIDLFragment(IXML_Document* Document, cStringList* Filter){
    IXML_Element* eItem = createDIDLSubFragment1(Document);
	char* title = strdup0(this->getTitle());
	char* buf = NULL;
	int len = -1;
	if (!title || strlen (title) == 0){
		title = strdup("Missing title");
	}
	else if ((len = checkSpecialChars(title, &buf)) >= 0){
		title = getSubstring(buf, len);
	}
	else {
		title = strdup(buf);
	}
	char* startTime = (char*) this->getStartTime();
	if (strlen(startTime) > 0){ // list the start and stop time before the title
		time_t startT = (time_t) atol(startTime);		
		struct tm * ptr;
		char buf [30];
		char buf2 [10];
		int dur = this->getDuration();
		ptr = localtime (&startT);
		strftime (buf, sizeof(buf), "%d.%m.%Y %H:%M", ptr);
		int length3 = 0;
		if (dur > 0){
			time_t stopT = startT + (time_t)(dur);
			ptr = localtime (&stopT);
			strftime (buf2, sizeof(buf2), "-%H:%M", ptr);
			length3 = strlen(buf2);
		}
		int length1 = strlen(title);
		int length2 = strlen(buf);

		char* titlePrep = (char*) malloc(sizeof(titlePrep) * (length1 + length2 + length3 + 3));
		strcpy (titlePrep, buf);			
        if (length3 > 0){
		   strcat (titlePrep, buf2);
		}
		strcat (titlePrep, "; ");
		strcat (titlePrep, title);
			
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(titlePrep));
		free(titlePrep);
	}
	else {
		MESSAGE(VERBOSE_OBJECTS, "Got no start time");
        ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(title));
	}
	createDIDLSubFragment2(Filter, eItem);

	char* longDescr = strdup0(this->getSynopsis());
	buf = NULL;
	len = -1;
	if (longDescr && strlen(longDescr) > 0){
		if ((len = checkSpecialChars(longDescr, &buf)) >= 0){
			longDescr = getSubstring(buf, len);
		}
		else {
			longDescr = strdup(buf);
		}
		ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_LONGDESCRIPTION, checkString (longDescr));
	}
	char* descr = strdup0(this->getShortTitle());
	buf = NULL;
	len = -1;
	if (descr && strlen(descr) > 0){
		if ((len = checkSpecialChars(descr, &buf)) >= 0){
			descr = getSubstring(buf, len);
		}
		else {
			descr = strdup(buf);
		}
		ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_DESCRIPTION, checkString (descr));
	}
	char* genreNums = strdup0(this->getGenres());
	if (genreNums && !isempty(genreNums)){
		cSplit* splitter = new cSplit();
		std::vector<std::string>* tokens = splitter->split(genreNums, " ");
		int ctr = 0;
		while (ctr < (int) tokens->size() && ctr < 10){			
			char* genreNum = (char*)tokens->at(ctr).c_str();
			int iGenreNum = 0;
			if (genreNum && !isempty(genreNum)){
				sscanf(genreNum, "%x", &iGenreNum);
				char* genre = getDVBContentDescriptor(iGenreNum);
				if (genre && !isempty(genre)){
					ixmlAddFilteredProperty(Filter, this->mDIDLFragment, eItem, UPNP_PROP_GENRE, genre);
				}
			}
			ctr++;
		}
	}
	return (IXML_Node*)eItem;
}

int cUPnPClassEpgItem::setEventId(int eventId){
	this->mEventId = eventId;
	return 0;
}

int cUPnPClassEpgItem::setDuration(int duration){
	this->mDuration = duration;
	return 0;
}

int cUPnPClassEpgItem::setTableId(int tableId){
	this->mTableId = tableId;
	return 0;
}

int cUPnPClassEpgItem::setShortTitle (const char* shortTitle){
	this->mShortTitle = strdup0(shortTitle); 
	return 0;
}

int cUPnPClassEpgItem::setSynopsis (const char* synopsis){
	this->mSynopsis = strdup0(synopsis); 
	return 0;
}

int cUPnPClassEpgItem::setGenres (const char* genres){
	this->mGenres = strdup0(genres); 
	return 0;
}

int cUPnPClassEpgItem::setStartTime (const char* startTime){
	this->mStartTime = strdup0(startTime);
	return 0;
}

int cUPnPClassEpgItem::setVersion (const char* version){
	this->mVersion = strdup0(version); 
	return 0;
}

cStringList* cUPnPClassEpgItem::getPropertyList(){
    cStringList* Properties = cUPnPClassItem::getPropertyList();
    Properties->Append(strdup(UPNP_PROP_LONGDESCRIPTION));
    Properties->Append(strdup(UPNP_PROP_DURATION));
    Properties->Append(strdup(UPNP_PROP_SCHEDULEDSTARTTIME));
    Properties->Append(strdup(UPNP_PROP_LONGDESCRIPTION));
    return Properties;
}

 /**********************************************\
 *                                              *
 *  Record Timer Item                           *
 *                                              *
 \**********************************************/

cUPnPClassRecordTimerItem::cUPnPClassRecordTimerItem(){
	this->mStatus = 0;   // timer is not active
	this->mPriority = 0; // the lowest priority
	this->mLivetime = 0; // the minimum live time of the record in days
	this->mIsRadioChannel = false; // a TV channel has to be recorded
	this->mChannelId = NULL;
	this->mDay = NULL;
	this->mStart = 0;
	this->mStop = 0;
	this->mFile = NULL;
	this->mAux = NULL;
}

cUPnPClassRecordTimerItem::~cUPnPClassRecordTimerItem(){
}

IXML_Node* cUPnPClassRecordTimerItem::createDIDLFragment(IXML_Document* Document, cStringList* Filter){
    IXML_Element* eItem = createDIDLSubFragment1(Document);
	char* title = strdup0(this->getTitle());
	char* buf = NULL;
	int len = -1;
	if (!title){
		title = strdup("Missing title");
	}
	else if ((len = checkSpecialChars(title, &buf)) >= 0){
		title = getSubstring(buf, len);
	}
	else {
		title = strdup(buf);
	}
	char* startTime = strdup0((char*) this->getDay());
	if (startTime && strlen(startTime) >= 10){ // list the start and stop time before the title
		char buf [30];
		strcpy(buf, substr(startTime, 8, 2));
		strcat(buf, ".");
		strcat(buf, substr(startTime, 5, 2));
		strcat(buf, ".");
		strcat(buf, substr(startTime, 0, 4));
		char buf2 [10];
		sprintf (buf2, "%04d", this->getStart());
		strcat(buf, " ");
		strcat(buf, substr(buf2, 0, 2));
		strcat(buf, ":");
		strcat(buf, substr(buf2, 2, 2));
		strcat(buf, "-");
		sprintf (buf2, "%04d", this->getStop());
		strcat(buf, substr(buf2, 0, 2));
		strcat(buf, ":");
		strcat(buf, substr(buf2, 2, 2));
		int length1 = strlen(title);
		int length2 = strlen(buf);

		char* titlePrep = (char*) malloc(sizeof(titlePrep) * (length1 + length2 + 3));
		strcpy (titlePrep, buf);			
		strcat (titlePrep, "; ");
		strcat (titlePrep, title);			
		ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(titlePrep));
		free(titlePrep);
	}
	else {
		MESSAGE(VERBOSE_OBJECTS, "No start time prepended");
        ixmlAddProperty(this->mDIDLFragment, eItem, UPNP_PROP_TITLE, checkString(title));
	}
	createDIDLSubFragment2(Filter, eItem);
	return (IXML_Node*)eItem;
}

int cUPnPClassRecordTimerItem::setStatus (int status){
	this->mStatus = status;
	return 0;
}

int cUPnPClassRecordTimerItem::setPriority(int priority){
	this->mPriority = priority;
	return 0;
}

int cUPnPClassRecordTimerItem::setLifetime(int livetime){
	this->mLivetime = livetime;
	return 0;
}

int cUPnPClassRecordTimerItem::setRadioChannel (bool isRadio){
	this->mIsRadioChannel = isRadio;
	return 0;
}

int cUPnPClassRecordTimerItem::setStart (int start){
	this->mStart = start;
	return 0;
}

int cUPnPClassRecordTimerItem::setStop (int stop){
	this->mStop = stop;
	return 0;
}

int cUPnPClassRecordTimerItem::setFile (const char* file){
	this->mFile = strdup0(file);
	return 0;
}

int cUPnPClassRecordTimerItem::cUPnPClassRecordTimerItem::setAux (const char* aux){
	this->mAux = strdup0(aux);
	return 0;
}

int cUPnPClassRecordTimerItem::setChannelId(const char* channelId){
	this->mChannelId = strdup0(channelId);
	return 0;
}

int cUPnPClassRecordTimerItem::setDay(const char* day){
	this->mDay = strdup0(day);
	return 0;
}