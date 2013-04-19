/*
 * File:   contentdirectory.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 21. August 2009, 16:12
 * Last Modification: November 6, 2012
 */

#include <upnp/ixml.h>
#include <upnp/upnptools.h>
#include <exception>
#include "upnp/contentdirectory.h"
#include "../common.h"
#include "util.h"

cContentDirectory::cContentDirectory(UpnpDevice_Handle DeviceHandle, cMediaDatabase* MediaDatabase)
: cUpnpService(DeviceHandle) {
    this->mMediaDatabase = MediaDatabase;
}

cContentDirectory::~cContentDirectory() {}

int cContentDirectory::subscribe(Upnp_Subscription_Request* Request){
    IXML_Document* PropertySet = NULL;

    /* The system update ID */
    UpnpAddToPropertySet(&PropertySet, "SystemUpdateID", itoa(this->mMediaDatabase->getSystemUpdateID()));
    /* The container update IDs as CSV list */
    UpnpAddToPropertySet(&PropertySet, "ContainerUpdateIDs", this->mMediaDatabase->getContainerUpdateIDs());
    /* The transfer IDs, which are not supported, i.e. empty */
    UpnpAddToPropertySet(&PropertySet, "TransferIDs", "");
    // Accept subscription
    int ret = UpnpAcceptSubscriptionExt(this->mDeviceHandle, Request->UDN, Request->ServiceId, PropertySet, Request->Sid);

    if(ret != UPNP_E_SUCCESS){
        ERROR("Subscription failed (Error code: %d)", ret);
    }

    ixmlDocument_free(PropertySet);
    return ret;
}

void cContentDirectory::Action(){
    static int Retry = 5;
    MESSAGE(VERBOSE_CDS, "Start Content directory thread");
    while(this->Running()){
        IXML_Document* PropertySet = NULL;
        UpnpAddToPropertySet(&PropertySet, "SystemUpdateID", itoa(this->mMediaDatabase->getSystemUpdateID()));
        int ret = UpnpNotifyExt(this->mDeviceHandle, UPNP_DEVICE_UDN, UPNP_CMS_SERVICE_ID, PropertySet);
        ixmlDocument_free(PropertySet);

        if(ret != UPNP_E_SUCCESS){
            Retry--;
            ERROR("State change notification failed (Error code: %d)",ret);
            ERROR("%d of %d notifications failed", (5-Retry), 5);
        }
        else {
            Retry = 5;
        }
        if (!Retry){
            ERROR("Maximum retries of notifications reached. Stopping...");
            this->Cancel();
        }
        // Sleep 2 seconds
        cCondWait::SleepMs(2000);
    }
}

int cContentDirectory::execute(Upnp_Action_Request* Request){
    if (Request == NULL) {
        ERROR("CMS Action Handler - request is null");
        return UPNP_E_BAD_REQUEST;
    }
//	MESSAGE(VERBOSE_CDS, "Content directory service, execute %s", Request->ActionName);
    if(!strcmp(Request->ActionName, UPNP_CDS_ACTION_BROWSE))
        return this->browse(Request);
    if(!strcmp(Request->ActionName, UPNP_CDS_ACTION_SEARCHCAPABILITIES))
        return this->getSearchCapabilities(Request);
    if(!strcmp(Request->ActionName, UPNP_CDS_ACTION_SORTCAPABILITIES))
        return this->getSortCapabilities(Request);
    if(!strcmp(Request->ActionName, UPNP_CDS_ACTION_SYSTEMUPDATEID))
        return this->getSystemUpdateID(Request);

    return UPNP_E_BAD_REQUEST;
}


int cContentDirectory::browse(Upnp_Action_Request* Request){
//    MESSAGE(VERBOSE_CDS, "Browse requested by %s.", inet_ntoa(Request->CtrlPtIPAddr));

    char* ObjectID = NULL;
    if(this->parseStringValue(Request->ActionRequest, "ObjectID", &ObjectID)){
        ERROR("Invalid arguments. ObjectID missing or wrong");
        this->setError(Request, UPNP_SOAP_E_INVALID_ARGS);
        return Request->ErrCode;
    }
	if (ObjectID == NULL){
		WARNING("The browse 'ObjectID' argument was not set. '0' was used.");
		ObjectID = (char*) malloc(2);
		strcpy (ObjectID, "0");
	}

    char* BrowseFlag = NULL;
    bool BrowseMetadata = false;
    if(this->parseStringValue(Request->ActionRequest, "BrowseFlag", &BrowseFlag)){
        ERROR("Invalid arguments. Browse flag missing or wrong");
        this->setError(Request, UPNP_SOAP_E_INVALID_ARGS);
        return Request->ErrCode;
    }
    if(!strcasecmp(BrowseFlag, "BrowseMetadata")){
        BrowseMetadata = true;
    }
    else if(!strcasecmp(BrowseFlag, "BrowseDirectChildren")){
        BrowseMetadata = false;
    }
    else {
        ERROR("Invalid argument. Browse flag invalid");
        this->setError(Request, UPNP_SOAP_E_INVALID_ARGS);
        return Request->ErrCode;
    }

    char* Filter = NULL;
    if(this->parseStringValue(Request->ActionRequest, "Filter", &Filter)){
        ERROR("Invalid arguments. Filter missing or wrong");
        this->setError(Request, UPNP_SOAP_E_INVALID_ARGS);
        return Request->ErrCode;
    }
	if (Filter == NULL){
//		WARNING("The browse 'Filter' argument was not set. '*' was used.");
		MESSAGE(VERBOSE_CDS, "The browse 'Filter' argument was not set. '*' was used.");
		Filter = (char*) malloc(2);
		strcpy (Filter, "*");
	}

    int StartingIndex = 0;
//    MESSAGE(VERBOSE_CDS, "Parsing StartingIndex");
    if(this->parseIntegerValue(Request->ActionRequest, "StartingIndex", &StartingIndex)){
       ERROR("Invalid arguments. Starting index missing or wrong");
       this->setError(Request, UPNP_SOAP_E_INVALID_ARGS);
       return Request->ErrCode;
    }

    int RequestedCount = 0;
//    MESSAGE(VERBOSE_CDS, "Parsing RequestedCount");
    if(this->parseIntegerValue(Request->ActionRequest, "RequestedCount", &RequestedCount)){
        ERROR("Invalid arguments. Requested count missing or wrong");
        this->setError(Request, UPNP_SOAP_E_INVALID_ARGS);
        return Request->ErrCode;
    }

    char* SortCriteria = NULL;
    if(this->parseStringValue(Request->ActionRequest, "SortCriteria", &SortCriteria)){
        ERROR("Invalid arguments. Sort criteria missing or wrong");
        this->setError(Request, UPNP_SOAP_E_INVALID_ARGS);
        return Request->ErrCode;
    }

    cUPnPResultSet* ResultSet;

    int ret = this->mMediaDatabase->browse(&ResultSet, ObjectID, BrowseMetadata, Filter, StartingIndex, RequestedCount, SortCriteria);
    if(ret!=UPNP_E_SUCCESS){
        ERROR("Error while browsing. Code: %d", ret);
        this->setError(Request, ret);
        return Request->ErrCode;
    }

	char* escapedResult = NULL;
	if (ResultSet->mResult){		
		escapeXMLCharacters(ResultSet->mResult, &escapedResult);

		if(!escapedResult){                             
			ERROR("Escaping XML data failed");
			this->setError(Request, UPNP_SOAP_E_ACTION_FAILED);
			return Request->ErrCode;
		}
	}

    cString Result = cString::sprintf(
            "<u:%sResponse xmlns:u=\"%s\"> \
                <Result>%s</Result> \
                <NumberReturned>%d</NumberReturned> \
                <TotalMatches>%d</TotalMatches> \
                <UpdateID>%d</UpdateID> \
             </u:%sResponse>",
            Request->ActionName,
            UPNP_CDS_SERVICE_TYPE,
            (ResultSet->mResult) ? escapedResult : strdup(""),
            ResultSet->mNumberReturned,
            ResultSet->mTotalMatches,
            this->mMediaDatabase->getSystemUpdateID(),
            Request->ActionName
            );

    Request->ActionResult = ixmlParseBuffer(Result);
    Request->ErrCode = UPNP_E_SUCCESS;

    free(escapedResult);

    return Request->ErrCode;

}

int cContentDirectory::getSystemUpdateID(Upnp_Action_Request* Request){
    cString Result = cString::sprintf(
            "<u:%sResponse xmlns:u=\"%s\"> \
                <Id>%d</Id> \
             </u:%sResponse>",
            Request->ActionName,
            UPNP_CDS_SERVICE_TYPE,
            this->mMediaDatabase->getSystemUpdateID(),
            Request->ActionName
            );

    Request->ActionResult = ixmlParseBuffer(Result);
    Request->ErrCode = UPNP_E_SUCCESS;

    return Request->ErrCode;
}

int cContentDirectory::getSearchCapabilities(Upnp_Action_Request* Request){
    MESSAGE(VERBOSE_CDS, "Sorry, no search capabilities yet");

    cString Result = cString::sprintf(
            "<u:%sResponse xmlns:u=\"%s\"> \
                <SearchCaps>%s</SearchCaps> \
             </u:%sResponse>",
            Request->ActionName,
            UPNP_CDS_SERVICE_TYPE,
            UPNP_CDS_SEARCH_CAPABILITIES,
            Request->ActionName
            );

    Request->ActionResult = ixmlParseBuffer(Result);
    Request->ErrCode = UPNP_E_SUCCESS;

    return Request->ErrCode;
}

int cContentDirectory::getSortCapabilities(Upnp_Action_Request* Request){
    MESSAGE(VERBOSE_CDS, "Sorry, no sort capabilities yet");

    cString Result = cString::sprintf(
            "<u:%sResponse xmlns:u=\"%s\"> \
                <SortCaps>%s</SortCaps> \
             </u:%sResponse>",
            Request->ActionName,
            UPNP_CDS_SERVICE_TYPE,
            UPNP_CDS_SORT_CAPABILITIES,
            Request->ActionName
            );

    Request->ActionResult = ixmlParseBuffer(Result);
    Request->ErrCode = UPNP_E_SUCCESS;

    return Request->ErrCode;
}

void cContentDirectory::setError(Upnp_Action_Request* Request, int Error){
    Request->ErrCode = Error;
    switch(Error){
        case UPNP_CDS_E_BAD_METADATA:
            strn0cpy(Request->ErrStr,_("Bad metadata"),LINE_SIZE);
            break;
        case UPNP_CDS_E_CANT_PROCESS_REQUEST:
            strn0cpy(Request->ErrStr,_("Cannot process the request"),LINE_SIZE);
            break;
        case UPNP_CDS_E_DEST_RESOURCE_ACCESS_DENIED:
            strn0cpy(Request->ErrStr,_("Destination resource access denied"),LINE_SIZE);
            break;
        case UPNP_CDS_E_INVALID_CURRENT_TAG:
            strn0cpy(Request->ErrStr,_("Invalid current tag"),LINE_SIZE);
            break;
        case UPNP_CDS_E_INVALID_NEW_TAG:
            strn0cpy(Request->ErrStr,_("Invalid new tag"),LINE_SIZE);
            break;
        case UPNP_CDS_E_INVALID_SEARCH_CRITERIA:
            strn0cpy(Request->ErrStr,_("Invalid or unsupported search criteria"),LINE_SIZE);
            break;
        case UPNP_CDS_E_INVALID_SORT_CRITERIA:
            strn0cpy(Request->ErrStr,_("Invalid or unsupported sort criteria"),LINE_SIZE);
            break;
        case UPNP_CDS_E_NO_SUCH_CONTAINER:
            strn0cpy(Request->ErrStr,_("No such container"),LINE_SIZE);
            break;
        case UPNP_CDS_E_NO_SUCH_DESTINATION_RESOURCE:
            strn0cpy(Request->ErrStr,_("No such destination resource"),LINE_SIZE);
            break;
        case UPNP_CDS_E_NO_SUCH_FILE_TRANSFER:
            strn0cpy(Request->ErrStr,_("No such file transfer"),LINE_SIZE);
            break;
        case UPNP_CDS_E_NO_SUCH_OBJECT:
            strn0cpy(Request->ErrStr,_("No such objectID"),LINE_SIZE);
            break;
        case UPNP_CDS_E_NO_SUCH_SOURCE_RESOURCE:
            strn0cpy(Request->ErrStr,_("No such source resource"),LINE_SIZE);
            break;
        case UPNP_CDS_E_PARAMETER_MISMATCH:
            strn0cpy(Request->ErrStr,_("Parameter mismatch"),LINE_SIZE);
            break;
        case UPNP_CDS_E_READ_ONLY_TAG:
            strn0cpy(Request->ErrStr,_("Read only tag"),LINE_SIZE);
            break;
        case UPNP_CDS_E_REQUIRED_TAG:
            strn0cpy(Request->ErrStr,_("Required tag"),LINE_SIZE);
            break;
        case UPNP_CDS_E_RESOURCE_ACCESS_DENIED:
            strn0cpy(Request->ErrStr,_("Resource access denied"),LINE_SIZE);
            break;
        case UPNP_CDS_E_RESTRICTED_OBJECT:
            strn0cpy(Request->ErrStr,_("Restricted object"),LINE_SIZE);
            break;
        case UPNP_CDS_E_RESTRICTED_PARENT:
            strn0cpy(Request->ErrStr,_("Restricted parent"),LINE_SIZE);
            break;
        case UPNP_CDS_E_TRANSFER_BUSY:
            strn0cpy(Request->ErrStr,_("Transfer busy"),LINE_SIZE);
            break;
        default:
            cUpnpService::setError(Request, Error);
            break;
    }
    return;
}