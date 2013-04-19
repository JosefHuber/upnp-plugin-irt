/* 
 * File:   upnpwebserver.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 30. Mai 2009, 18:13
 * Last modification: April 9, 2013
 */

#include <time.h>
#include <vdr/channels.h>
#include <vdr/timers.h>
#include <map>
#include <upnp/upnp.h>
#include "webserver.h"
#include "server.h"
#include "livereceiver.h"
#include "recplayer.h"
#include "fileplayer.h"
#include "search.h"
#include "vdrepg.h"

/* COPIED FROM INTEL UPNP TOOLS */
/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * Neither name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
/** @private */
struct File_Info_
{
  /** The length of the file. A length less than 0 indicates the size
   *  is unknown, and data will be sent until 0 bytes are returned from
   *  a read call. */
  off64_t file_length;

  /** The time at which the contents of the file was modified;
   *  The time system is always local (not GMT). */
  time_t last_modified;

  /** If the file is a directory, {\bf is_directory} contains
   * a non-zero value. For a regular file, it should be 0. */
  int is_directory;

  /** If the file or directory is readable, this contains
   * a non-zero value. If unreadable, it should be set to 0. */
  int is_readable;

  /** The content type of the file. This string needs to be allocated
   *  by the caller using {\bf ixmlCloneDOMString}.  When finished
   *  with it, the SDK frees the {\bf DOMString}. */

  DOMString content_type;

};

/** @private */
struct cWebFileHandle {
    cString      Filename;
    off64_t      Size;
    cFileHandle* FileHandle;
};

/****************************************************
 *
 *  The web server
 *
 *  Handles the virtual directories and the
 *  provision of data
 *
 *  Interface between the channels/recordings of the
 *  VDR and the outer world
 *
 ****************************************************/
cVector<cUPnPClassObject*> cUPnPWebServer::epgVector;
cVector<tRecordTimerPlay*> cUPnPWebServer::inhibitVector;

cUPnPWebServer::cUPnPWebServer(const char* root) : mRootdir(root) {	
	if (pthread_mutex_init(&mutexrt1, NULL) != 0){
		ERROR("pthread_mutex_init failed for mutexrt1");
	}
}

cUPnPWebServer::~cUPnPWebServer(){}

cUPnPWebServer* cUPnPWebServer::mInstance = NULL;

UpnpVirtualDirCallbacks cUPnPWebServer::mVirtualDirCallbacks = {
    cUPnPWebServer::getInfo,
    cUPnPWebServer::open,
    cUPnPWebServer::read,
    cUPnPWebServer::write,
    cUPnPWebServer::seek,
    cUPnPWebServer::close
};

bool cUPnPWebServer::init(){
    MESSAGE(VERBOSE_WEBSERVER, "Initialize callbacks for virtual directories. Web server root dir: %s", this->mRootdir);

    if(UpnpSetWebServerRootDir(this->mRootdir) == UPNP_E_INVALID_ARGUMENT){
        ERROR("The root directory of the webserver is invalid.");
        return false;
    }
    MESSAGE(VERBOSE_WEBSERVER, "Setting up callbacks");

    if(UpnpSetVirtualDirCallbacks(&cUPnPWebServer::mVirtualDirCallbacks) == UPNP_E_INVALID_ARGUMENT){
        ERROR("The virtual directory callbacks are invalid.");
        return false;
    }

    if(UpnpIsWebserverEnabled() == FALSE){
        WARNING("The webserver has not been started. For whatever reason...");
        return false;
    }

    MESSAGE(VERBOSE_WEBSERVER, "Add virtual directories.");
    if(UpnpAddVirtualDir(UPNP_DIR_SHARES) == UPNP_E_INVALID_ARGUMENT){
        ERROR("The virtual directory %s is invalid.",UPNP_DIR_SHARES);
        return false;
    }
    return true;
}

bool cUPnPWebServer::uninit(){
    MESSAGE(VERBOSE_WEBSERVER, "Disabling the internal webserver");
    UpnpEnableWebserver(FALSE);

    return true;
}

cUPnPWebServer* cUPnPWebServer::getInstance(const char* rootdir){
    if(cUPnPWebServer::mInstance == NULL)
        cUPnPWebServer::mInstance = new cUPnPWebServer(rootdir);

    if(cUPnPWebServer::mInstance){
        return cUPnPWebServer::mInstance;
    }
    else return NULL;
}

int cUPnPWebServer::getInfo(const char* filename, File_Info* info){
    MESSAGE(VERBOSE_WEBSERVER, "Getting information of file '%s'", filename);

    propertyMap Properties;
    int Method;
    int Section;

    if(cPathParser::parse(filename, &Section, &Method, &Properties)){
        switch(Section){
            case 0:
                switch(Method){
                    case UPNP_WEB_METHOD_STREAM:
                        {
                            MESSAGE(VERBOSE_WEBSERVER, "Stream request, getInfo");
                            propertyMap::iterator It = Properties.find("resId");
                            unsigned int ResourceID = 0;
                            if(It == Properties.end()){
                                ERROR("No resourceID for stream request");
                                return -1;
                            }
                            else {
                                ResourceID = (unsigned)atoi(It->second);
                                cUPnPResource* Resource = cUPnPResources::getInstance()->getResource(ResourceID);
                                if(!Resource){
                                    ERROR("No such resource with ID (%d)", ResourceID);
                                    return -1;
                                }
                                else {
                                    File_Info_ finfo;

                                    finfo.content_type = ixmlCloneDOMString(Resource->getContentType());
                                    finfo.file_length = Resource->getFileSize();
                                    finfo.is_directory = 0;
                                    finfo.is_readable = 1;
                                    finfo.last_modified = Resource->getLastModification();
                                    memcpy(info, &finfo, sizeof(File_Info_));

                                    MESSAGE(VERBOSE_METADATA, "==== File info of Resource #%d ====", Resource->getID());
                                    MESSAGE(VERBOSE_METADATA, "Size: %lld", finfo.file_length);
                                    MESSAGE(VERBOSE_METADATA, "Dir: %s", finfo.is_directory?"yes":"no");
                                    MESSAGE(VERBOSE_METADATA, "Read: %s", finfo.is_readable?"allowed":"not allowed");
                                    MESSAGE(VERBOSE_METADATA, "Last modified: %s", ctime(&(finfo.last_modified)));
                                    MESSAGE(VERBOSE_METADATA, "Content-type: %s", finfo.content_type);
									MESSAGE(VERBOSE_METADATA, "Task %i %s", Resource->getRecordTimer(), (Resource->getRecordTimer() == DO_TRIGGER_TIMER) ?
										"'Program_Record_Timer'" : (Resource->getRecordTimer() == PURGE_RECORD_TIMER) ?  "'Purge_Record_Timer'" : "None");
									handleRecordTimer(Resource);

#ifdef UPNP_HAVE_CUSTOMHEADERS
                                    UpnpAddCustomHTTPHeader("transferMode.dlna.org: Streaming");
                                    UpnpAddCustomHTTPHeader(
                                        "contentFeatures.dlna.org: "
                                        "DLNA.ORG_OP=00;"
                                        "DLNA.ORG_CI=0;"
                                        "DLNA.ORG_FLAGS=01700000000000000000000000000000"
                                        );
#endif
                                }
                            }
                        }
                        break;
                    case UPNP_WEB_METHOD_BROWSE:
                    //    break;
                    case UPNP_WEB_METHOD_SHOW:
                    //    break;
                    case UPNP_WEB_METHOD_SEARCH:
                    case UPNP_WEB_METHOD_DOWNLOAD:
                    default:
                        ERROR("Unknown or unsupported method ID (%d)", Method);
                        return -1;
                }
                break;
            default:
                ERROR("Unknown or unsupported section ID (%d).", Section);
                return -1;
        }
    }
    else {
        return -1;
    }

    return 0;
}

UpnpWebFileHandle cUPnPWebServer::open(const char* filename, UpnpOpenFileMode mode){
    MESSAGE(VERBOSE_WEBSERVER, "File %s was opened for %s.",filename,mode==UPNP_READ ? "reading" : "writing");

    propertyMap Properties;
    int Method;
    int Section;
    cWebFileHandle* WebFileHandle = NULL;

    if(cPathParser::parse(filename, &Section, &Method, &Properties)){
        switch(Section){
            case 0:
                switch(Method){
                    case UPNP_WEB_METHOD_STREAM:
                        {
                            MESSAGE(VERBOSE_WEBSERVER, "Stream request, open");
                            propertyMap::iterator It = Properties.find("resId");
                            unsigned int ResourceID = 0;
                            if(It == Properties.end()){
                                ERROR("No resourceID for stream request");
                                return NULL;
                            }
                            else {
                                ResourceID = (unsigned)atoi(It->second);
                                cUPnPResource* Resource = cUPnPResources::getInstance()->getResource(ResourceID);
                                if(!Resource){
                                    ERROR("No such resource with ID (%d)", ResourceID);
                                    return NULL;
                                }
                                else {
                                    WebFileHandle = new cWebFileHandle;
                                    WebFileHandle->Filename = Resource->getResource();
                                    WebFileHandle->Size = Resource->getFileSize();
                                    switch(Resource->getResourceType()){
                                        case UPNP_RESOURCE_CHANNEL:
                                            {
                                                char* ChannelID = strtok(strdup(Resource->getResource()),":");
                                                int    StreamID = atoi(strtok(NULL,":"));
                                                MESSAGE(VERBOSE_LIVE_TV, "Try to create Receiver for Channel %s with Stream ID %d; RESOURCE: %s", ChannelID, StreamID, Resource->getResource());
                                                cChannel* Channel = Channels.GetByChannelID(tChannelID::FromString(ChannelID));
                                                if(!Channel){
                                                    ERROR("No such channel with ID %s", ChannelID);
                                                    return NULL;
                                                }
                                                cLiveReceiver* Receiver = cLiveReceiver::newInstance(Channel,0);
                                                if(!Receiver){
                                                    ERROR("Unable to tune channel. No available tuners?");
                                                    return NULL;
                                                }
                                                WebFileHandle->FileHandle = Receiver;
                                            }
                                            break;
                                        case UPNP_RESOURCE_RECORDING:
                                            {
                                                const char* RecordFile = Resource->getResource();
                                                MESSAGE(VERBOSE_RECORDS, "Try to create Player for Record %s", RecordFile);
                                                cRecording* Recording = Recordings.GetByName(RecordFile);
                                                if(!Recording){
                                                    ERROR("No such recording with file name %s", RecordFile);
                                                    return NULL;
                                                }
                                                cRecordingPlayer* RecPlayer = cRecordingPlayer::newInstance(Recording);
                                                if(!RecPlayer){
                                                    ERROR("Unable to start record player. No access?!");
                                                    return NULL;
                                                }
                                                WebFileHandle->FileHandle = RecPlayer;
                                            }
                                            break;
                                        case UPNP_RESOURCE_FILE:
									       {
											    MESSAGE(VERBOSE_RECORDS, "Try to create a file player with %s", Resource->getResource());
                                                cFilePlayer* RecPlayer = cFilePlayer::newInstance(Resource->getResource());
                                                if(!RecPlayer){
                                                    ERROR("Unable to start the file player. No access?!");
                                                    return NULL;
                                                }
                                                WebFileHandle->FileHandle = RecPlayer;
									        }
                                            break;
                                        case UPNP_RESOURCE_URL:
                                        default:
                                            return NULL;
                                    }
                                }
                            }
                        }
                        break;
                    case UPNP_WEB_METHOD_BROWSE:
                    //    break;
                    case UPNP_WEB_METHOD_SHOW:
                    //    break;
                    case UPNP_WEB_METHOD_SEARCH:
                    case UPNP_WEB_METHOD_DOWNLOAD:
                    default:
                        ERROR("Unknown or unsupported method ID (%d)", Method);
                        return NULL;
                }
                break;
            default:
                ERROR("Unknown or unsupported section ID (%d).", Section);
                return NULL;
        }
    }
    else {
        return NULL;
    }
    MESSAGE(VERBOSE_WEBSERVER, "Open the file handle");
    WebFileHandle->FileHandle->open(mode);
    return (UpnpWebFileHandle)WebFileHandle;
}

int cUPnPWebServer::write(UpnpWebFileHandle fh, char* buf, size_t buflen){
    cWebFileHandle* FileHandle = (cWebFileHandle*)fh;
    MESSAGE(VERBOSE_BUFFERS, "Writing to %s", *FileHandle->Filename);
    return FileHandle->FileHandle->write(buf, buflen);
}

int cUPnPWebServer::read(UpnpWebFileHandle fh, char* buf, size_t buflen){
    cWebFileHandle* FileHandle = (cWebFileHandle*)fh;
    MESSAGE(VERBOSE_BUFFERS, "Reading from %s", *FileHandle->Filename);
    return FileHandle->FileHandle->read(buf, buflen);
}

int cUPnPWebServer::seek(UpnpWebFileHandle fh, off_t offset, int origin){
    cWebFileHandle* FileHandle = (cWebFileHandle*)fh;
    MESSAGE(VERBOSE_BUFFERS, "Seeking on %s", *FileHandle->Filename);
    return FileHandle->FileHandle->seek(offset, origin);
}

int cUPnPWebServer::close(UpnpWebFileHandle fh){
    cWebFileHandle *FileHandle = (cWebFileHandle *)fh;
    MESSAGE(VERBOSE_WEBSERVER, "Closing file %s", *FileHandle->Filename);
    FileHandle->FileHandle->close();
    delete FileHandle->FileHandle;
    delete FileHandle;
    return 0;
}

bool cUPnPWebServer::createNewTimer(cUPnPClassEpgItem* epgItem, cUPnPClassContainer* channelObj){
	if (!channelObj){
		WARNING("Can not schedule the record timer because the channel object is null for the event with ID %i", epgItem->getEventId());
		return false;
	}
	time_t tstart = (time_t)atol(epgItem->getStartTime()) - Setup.MarginStart * 60;
	time_t tstop = tstart + epgItem->getDuration() + Setup.MarginStop * 60;

	int start;
	int stop;
	time_t day;         ///< midnight of the day this timer shall hit, or of the first day it shall hit in case of a repeating timer
	int weekdays;       ///< bitmask, lowest bits: SSFTWTM  (the 'M' is the LSB)
	struct tm tm_r;
	struct tm *time = localtime_r(&tstart, &tm_r);
						
	day = cTimer::SetTime(tstart, 0);
	weekdays = 0;
	start = time->tm_hour * 100 + time->tm_min;
	time = localtime_r(&tstop, &tm_r);
	stop = time->tm_hour * 100 + time->tm_min;
	if (stop >= 2400){
		stop -= 2400;
	}
   	int length1 = strlen(epgItem->getTitle());
	char* recTitle = (char*) malloc(sizeof(recTitle) * (length1 + 5));
	strcpy (recTitle, epgItem->getTitle());
	strreplace(recTitle, ':', '|');
	cString buffer = cString::sprintf("%u:%s:%s:%04d:%04d:%d:%d:%s:", 1, ((cUPnPClassEpgContainer*)channelObj)->getChannelId(), 
						*cTimer::PrintDay(day, weekdays, true), start, stop, Setup.DefaultPriority, Setup.DefaultLifetime, recTitle);
	free (recTitle);
	MESSAGE(VERBOSE_METADATA, "Create a new record timer from the broadcast event, options: %s", *buffer);
	cTimer* recTimer = new cTimer;
	recTimer->Parse(buffer);
	cTimer *tCheck = Timers.GetTimer(recTimer);
	if (!tCheck) {
		Timers.Add(recTimer);
		Timers.SetModified();
		MESSAGE(VERBOSE_METADATA, "The new record timer %s was added", *recTimer->ToDescr());
	}
	return true;
}

bool cUPnPWebServer::scheduleEpgTimer(cUPnPClassObject* obj){
	if (!obj){
		WARNING("Can not schedule the record timer because the epg event object is null");
		return false;
	}
	cUPnPClassEpgItem* epgItem = (cUPnPClassEpgItem*) obj;
	cUPnPClassContainer* channelObj = obj->getParent();
	MESSAGE(VERBOSE_METADATA, "The start time of the broadcast event to be recorded is: %s, the duration (sec): %i, the event ID: %i", 
		epgItem->getStartTime(), epgItem->getDuration(), epgItem->getEventId());
	if (channelObj){
		cUPnPObjects* epgs = channelObj->getObjectList();
		if (epgs){
			MESSAGE(VERBOSE_METADATA, "The channel folder has %i epg items", epgs->Count());
			if (epgs->Count() == 1){
				return createNewTimer(epgItem, channelObj);
			}
		}
		pthread_mutex_lock(&(cUPnPWebServer::getInstance()->mutexrt1));
	    epgVector.Insert(obj);
		pthread_mutex_unlock(&(cUPnPWebServer::getInstance()->mutexrt1));
		pthread_t timerThread;
		int iThread = pthread_create (&timerThread, NULL, newTimerFunction, (void *) epgItem);

		if (iThread == 0){
			MESSAGE(VERBOSE_METADATA, "A record timer thread was created");
			return true;
		}
		else {
			ERROR("Could not create a record timer thread");
			return false;
		}
	}
	return false;
}

void * cUPnPWebServer::newTimerFunction(void *epgItemIn){
//	MESSAGE(VERBOSE_METADATA, "Record timer thread function started");
	cUPnPClassEpgItem* epgItem = (cUPnPClassEpgItem*) epgItemIn;
	cUPnPClassContainer* channelObj = ((cUPnPClassObject*) epgItem)->getParent();
	int channelId = (int) channelObj->getID();
	const int delaySec = 7;
	sleep (delaySec);  // in units of seconds
	pthread_mutex_lock(&(cUPnPWebServer::getInstance()->mutexrt1));
	time_t actTime;
	time (&actTime);
	int vectorSize = epgVector.Size();
	if (vectorSize == 1 && ((int) epgItem->getID() == (int) epgVector.At(0)->getID())){
		epgVector.Clear();
	}
	else if (vectorSize > 1){
		int count = 0;
		int actNr = 0;
		for (int j = 0; j < vectorSize; j++){
			cUPnPClassObject* vObj = epgVector.At(j);
			if ((int) epgItem->getID() != (int) vObj->getID() && channelId == (int) vObj->getParent()->getID()){
				count++;					
			}
			else {
				actNr = j;
			}
		}
		epgVector.Remove(actNr);
		if (count > 0){
			cList<cUPnPResource>* resList = epgItem->getResources();
			if (resList){
				int resMax = min((int)20, resList->Count());
				int ctr = 0;
				for (cUPnPResource* res = (cUPnPResource*)resList->First(); res && ctr++ < resMax; ){
					MESSAGE(VERBOSE_METADATA, "Record timer flag was: %i", res->getRecordTimer());
					res->setRecordTimer(DO_TRIGGER_TIMER);  // allow recording with this epg item again
					if (ctr < resMax){
						res = (cUPnPResource*)resList->Next(res);
					}
				}
			}
			else {
				WARNING("Got no resource for the EPG item: %s", epgItem->getTitle());
			}
			bool found = false;
			if (inhibitVector.Size() > 0){
				for (int i = 0; i < inhibitVector.Size(); i++){
					tRecordTimerPlay * rtPlay = (tRecordTimerPlay*) inhibitVector.At(i);
					if (rtPlay->getContainerId() == channelId){
						MESSAGE(VERBOSE_METADATA, "Found the tRecordTimerPlay structure for the container with ID %i", channelId);
						rtPlay->setLastAccess(actTime);
						found = true;
						break;
					}
				}
			}
			if (!found){
				tRecordTimerPlay rtPlay (channelId, actTime);
				inhibitVector.Insert(&rtPlay);
			}
			pthread_mutex_unlock(&(cUPnPWebServer::getInstance()->mutexrt1));
			return epgItemIn;  
		}
	}
	else {
		WARNING("The epg vector size of %i should not happen when installing a record timer", vectorSize);
	}

	pthread_mutex_unlock(&(cUPnPWebServer::getInstance()->mutexrt1));
	for (int i = 0; i < inhibitVector.Size(); i++){
		tRecordTimerPlay * rtPlay = (tRecordTimerPlay*) inhibitVector.At(i);
		if (rtPlay->getContainerId() == channelId && (long) actTime - (long)rtPlay->getLastAccess() < (long) delaySec){
			MESSAGE(VERBOSE_METADATA, "Last EPG folder play item inhibited; times: %ld - %ld", (long) actTime, (long)rtPlay->getLastAccess());
			return epgItemIn;			
		}
	}
	
	createNewTimer(epgItem, channelObj);
	return epgItemIn;
}

bool cUPnPWebServer::handleRecordTimer(cUPnPResource* Resource){
	if (Resource->getRecordTimer() == DO_TRIGGER_TIMER){
		Resource->setRecordTimer(DO_TRIGGER_TIMER + 1);  // do only one timer schedule with a broadcast event
		int objId = Resource->getObjectId(); 
		MESSAGE(VERBOSE_METADATA, "Do trigger a record timer with object ID %i", objId);
		if (objId > 0){
			MESSAGE(VERBOSE_METADATA, "Search the UPnP object with ID %i", objId);
			cUPnPClassObject* obj = cUPnPObjectFactory::getInstance()->getObject(objId);
			return scheduleEpgTimer(obj);
		}
	}
	else if (Resource->getRecordTimer() == PURGE_RECORD_TIMER){
		Resource->setRecordTimer(PURGE_RECORD_TIMER + 1);
		int objId = Resource->getObjectId(); 
		if (objId <= 0){
			ERROR("Can not purge the record timer; got no object ID with the resource");
			return false;
		}
		cUPnPClassObject* obj = cUPnPObjectFactory::getInstance()->getObject(objId);
		if (obj == NULL){
			ERROR("Can not purge the record timer, because there is no UPnP object with ID %i", objId);
			return false;
		}
		cUPnPClassRecordTimerItem* rtItem = (cUPnPClassRecordTimerItem*) obj;
		int timerMax = Timers.Count();
		MESSAGE(VERBOSE_METADATA, "Purge a record timer with an object ID %i, number of timers: %i", Resource->getObjectId(), timerMax);
		if (timerMax <= 0){
			WARNING("Can not purge a record timer because the timer list is empty");
			return false;
		}
		int ctr = 0;
		int found = false;
		for (cTimer* timer = Timers.First(); ctr++ < timerMax && !found && timer; ){
			MESSAGE(VERBOSE_METADATA, "Existing timer: %s", *timer->ToText());
			if (timer->Start() == rtItem->getStart() && strcmp(*timer->Channel()->GetChannelID().ToString(), rtItem->getChannelId()) == 0){
				MESSAGE(VERBOSE_METADATA, "Delete this timer");
				Timers.Del(timer, true);
				Timers.Save();
				cUPnPObjectFactory::getInstance()->deleteObject(obj);	
				found = true;
			}
			if (ctr < timerMax && !found){
				timer = Timers.Next(timer);
			}
		}
		return true;
	}
	return false;
}