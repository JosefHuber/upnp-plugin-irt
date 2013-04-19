/* 
 * File:   connectionmanager.cpp
 * Author: savop
 * 
 * Created on 21. August 2009, 18:35
 */

#include <string.h>
#include <upnp/ixml.h>
#include <upnp/upnptools.h>
#include <vdr/tools.h>
#include "upnp/connectionmanager.h"
#include "../common.h"
#include "upnp/dlna.h"

cVirtualConnection::cVirtualConnection() : mRcsID(-1) {}

cConnectionManager::cConnectionManager(UpnpDevice_Handle DeviceHandle) : cUpnpService(DeviceHandle) {
    this->mVirtualConnections = new cList<cVirtualConnection>;
    this->mDefaultConnection = this->createVirtualConnection();
    this->mSupportedProtocols = cDlna::getInstance()->getSupportedProtocols();
}

cConnectionManager::~cConnectionManager() {
    delete this->mDefaultConnection;
    delete this->mVirtualConnections;
}

int cConnectionManager::subscribe(Upnp_Subscription_Request* Request){
    IXML_Document* PropertySet = NULL;
    /* The protocol infos which this server supports */
    UpnpAddToPropertySet(&PropertySet, "SourceProtocolInfo", this->mSupportedProtocols);
    /* Not set, this field is only used by Media Renderers */
    UpnpAddToPropertySet(&PropertySet, "SinkProtocolInfo", "");
    /* The current connection IDs of all virtual connections */
    const char* IDs = this->getConnectionIDsCVS();
    if(!IDs){
        return UPNP_E_INTERNAL_ERROR;
    }
    UpnpAddToPropertySet(&PropertySet, "CurrentConnectionIDs", IDs);
    // Accept subscription
    int ret = UpnpAcceptSubscriptionExt(this->mDeviceHandle, Request->UDN, Request->ServiceId, PropertySet, Request->Sid);

    if(ret != UPNP_E_SUCCESS){
        ERROR("Subscription failed (Error code: %d)", ret);
    }

    ixmlDocument_free(PropertySet);
    return ret;
}

int cConnectionManager::execute(Upnp_Action_Request* Request){
    if (Request == NULL) {
        ERROR("CMS Action Handler - request is null");
        return UPNP_E_BAD_REQUEST;
    }
    if(!strcmp(Request->ActionName, UPNP_CMS_ACTION_GETPROTOCOLINFO))
        return this->getProtocolInfo(Request);
    if(!strcmp(Request->ActionName, UPNP_CMS_ACTION_GETCURRENTCONNECTIONIDS))
        return this->getCurrentConnectionIDs(Request);
    if(!strcmp(Request->ActionName, UPNP_CMS_ACTION_GETCURRENTCONNECTIONINFO))
        return this->getCurrentConnectionInfo(Request);
    if(!strcmp(Request->ActionName, UPNP_CMS_ACTION_PREPAREFORCONNECTION))
        return this->prepareForConnection(Request);
    if(!strcmp(Request->ActionName, UPNP_CMS_ACTION_CONNECTIONCOMPLETE))
        return this->connectionComplete(Request);

    return UPNP_E_BAD_REQUEST;
}

int cConnectionManager::getProtocolInfo(Upnp_Action_Request* Request){
//    MESSAGE(VERBOSE_CMS, "Protocol info requested by %s.", inet_ntoa(Request->CtrlPtIPAddr));
    cString Result = cString::sprintf(
            "<u:%sResponse xmlns:u=\"%s\"> \
                <Source>%s</Source> \
                <Sink></Sink> \
             </u:%sResponse>",
            Request->ActionName,
            UPNP_CMS_SERVICE_TYPE,
            *this->mSupportedProtocols,
            Request->ActionName
            );
    Request->ActionResult = ixmlParseBuffer(Result);
    Request->ErrCode = UPNP_E_SUCCESS;
    return Request->ErrCode;
}

int cConnectionManager::getCurrentConnectionIDs(Upnp_Action_Request* Request){
//    MESSAGE(VERBOSE_CMS, "Current connection IDs requested by %s.", inet_ntoa(Request->CtrlPtIPAddr));
    cString Result;
    const char* IDs = this->getConnectionIDsCVS();
    if(!IDs){
        Request->ErrCode = UPNP_E_INTERNAL_ERROR;
        return Request->ErrCode;
    }
    Result = cString::sprintf(
            "<u:%sResponse xmlns:u=\"%s\"> \
                <ConnectionIDs>%s</ConnectionIDs> \
             </u:%sResponse>",
            Request->ActionName,
            UPNP_CMS_SERVICE_TYPE,
            IDs,
            Request->ActionName
            );
    Request->ActionResult = ixmlParseBuffer(Result);
    Request->ErrCode = UPNP_E_SUCCESS;
    return Request->ErrCode;
}

int cConnectionManager::getCurrentConnectionInfo(Upnp_Action_Request* Request){
//    MESSAGE(VERBOSE_CMS, "Current connection info requested by %s.", inet_ntoa(Request->CtrlPtIPAddr));
    int ConnectionID;

    if(this->parseIntegerValue(Request->ActionRequest, "ConnectionID", &ConnectionID) != 0){
        ERROR("Invalid arguments. ConnectionID missing or wrong");
        this->setError(Request, 402);
        return Request->ErrCode;
    }

    cVirtualConnection* Connection;
    for(Connection = this->mVirtualConnections->First(); Connection && Connection->mConnectionID != ConnectionID; Connection = this->mVirtualConnections->Next(Connection)){}

    if(Connection){
        cString Result = cString::sprintf(
                "<u:%sResponse xmlns:u=\"%s\">\
                    <ProtocolInfo>%s</ProtocolInfo>\
                    <PeerConnectionManager>%s</PeerConnectionManager>\
                    <PeerConnectionID>%d</PeerConnectionID>\
                    <Direction>%s</Direction>\
                    <RcsID>%d</RcsID>\
                    <AVTransportID>%d</AVTransportID>\
                    <Status>%s</Status>\
                 </u:%sResponse>",
                Request->ActionName,
                UPNP_CMS_SERVICE_TYPE,
                *Connection->mRemoteProtocolInfo,
                *Connection->mRemoteConnectionManager,
                -1,
                cVirtualConnection::getDirectionString(OUTPUT),
                Connection->mRcsID,
                Connection->mAVTransportID,
                cVirtualConnection::getStatusString(Connection->mStatus),
                Request->ActionName
            );
        Request->ActionResult = ixmlParseBuffer(Result);
        Request->ErrCode = UPNP_E_SUCCESS;
    }
    else {
        ERROR("No valid connection found with given ID=%d!", ConnectionID);
        this->setError(Request, 706);
    }

    return Request->ErrCode;

}

int cConnectionManager::prepareForConnection(Upnp_Action_Request* Request){
//    MESSAGE(VERBOSE_CMS, "Request for a new connection by %s.", inet_ntoa(Request->CtrlPtIPAddr));
    //char* Result = NULL;
    char* RemoteProtocolInfo = NULL;
    char* PeerConnectionManager = NULL;
    int   PeerConnectionID = 0;
    char* DirectionStr = NULL;
    int   Direction;

    if(this->parseStringValue(Request->ActionRequest, "RemoteProtocolInfo", &RemoteProtocolInfo) != 0){
        ERROR("Invalid argument RemoteProtocolInfo: Missing or wrong");
        this->setError(Request, 402);
        return Request->ErrCode;
    }

    if(this->parseStringValue(Request->ActionRequest, "PeerConnectionManager", &PeerConnectionManager) != 0){
        ERROR("Invalid argument PeerConnectionManager: Missing or wrong");
        this->setError(Request, 402);
        return Request->ErrCode;
    }

    if(this->parseStringValue(Request->ActionRequest, "Direction", &DirectionStr) != 0 && (Direction = cVirtualConnection::getDirection(DirectionStr)) == -1){
        ERROR("Invalid argument Direction: Missing or wrong");
        this->setError(Request, 402);
        return Request->ErrCode;
    }

    if(this->parseIntegerValue(Request->ActionRequest, "PeerConnectionID", &PeerConnectionID) != 0){
        ERROR("Invalid argument PeerConnectionID: Missing or wrong");
        this->setError(Request, 402);
        return Request->ErrCode;
    }


    /* TODO:
       Create Connection
       Notify AVTransport that a new connection was established
       Send back the response */
    this->setError(Request, UPNP_SOAP_E_ACTION_NOT_IMPLEMENTED);
    return Request->ErrCode;
}

int cConnectionManager::connectionComplete(Upnp_Action_Request* Request){
//    MESSAGE(VERBOSE_CMS, "Request for closing an open connection by %s.", inet_ntoa(Request->CtrlPtIPAddr));
    //char* Result = NULL;
    int ConnectionID;

    if(this->parseIntegerValue(Request->ActionRequest, "ConnectionID", &ConnectionID) != 0){
        ERROR("Invalid argument ConnectionID: Missing or wrong");
        this->setError(Request, 402);
        return Request->ErrCode;
    }

    // TODO:
    // Close and clear any open resources
    // Close and delete the connection
    // Free other resources left
    this->setError(Request, UPNP_SOAP_E_ACTION_NOT_IMPLEMENTED);
    return Request->ErrCode;
}

//bool cConnectionManager::setProtocolInfo(const char* ProtocolInfo){
//    if(strcmp(this->mSupportedProtocols, ProtocolInfo)){
//        // ProtocolInfo changed, save and invoke a event notification
//        this->mSupportedProtocols = ProtocolInfo;
//
//        IXML_Document* PropertySet = NULL;
//        UpnpAddToPropertySet(&PropertySet, "SourceProtocolInfo", this->mSupportedProtocols);
//        int ret = UpnpNotifyExt(this->mDeviceHandle, UPNP_DEVICE_UDN, UPNP_CMS_SERVICE_ID, PropertySet);
//        ixmlDocument_free(PropertySet);
//
//        if(ret != UPNP_E_SUCCESS){
//            ERROR("State change notification failed (Error code: %d)",ret);
//            return false;
//        }
//    }
//    return true;
//}

cVirtualConnection* cConnectionManager::createVirtualConnection(const char* RemoteProtocolInfo, const char* RemoteConnectionManager, int RemoteConnectionID, eDirection Direction){
    static int lastConnectionID = 0;
    MESSAGE(VERBOSE_CMS, "Create virtual connection");
    if(lastConnectionID == 2147483647) lastConnectionID = 1;
    cVirtualConnection* Connection = new cVirtualConnection;
    // AVT is available
    Connection->mAVTransportID = 0;
    // The ProtocolInfo of the remote device (i.e. Media Renderer)
    Connection->mRemoteProtocolInfo = RemoteProtocolInfo;
    // The responsible connection manager
    Connection->mRemoteConnectionManager = RemoteConnectionManager;
    // The virtual connection direction is output
    Connection->mDirection = Direction;
    // The remote connection ID, -1 says ID is unknown
    Connection->mRemoteConnectionID = RemoteConnectionID;
    // Connection status, assume that its ok.
    Connection->mStatus = OK;
    // new assigned ConnectionID
    Connection->mConnectionID = lastConnectionID++;

    // Notify the subscribers
    IXML_Document* PropertySet = NULL;
    const char* IDs = this->getConnectionIDsCVS();
    if(!IDs){
        return NULL;
    }
    UpnpAddToPropertySet(&PropertySet, "CurrentConnectionIDs", IDs);
    int ret = UpnpNotifyExt(this->mDeviceHandle, UPNP_DEVICE_UDN, UPNP_CMS_SERVICE_ID, PropertySet);
    ixmlDocument_free(PropertySet);

    if(ret != UPNP_E_SUCCESS){
        ERROR("State change notification failed (Error code: %d)",ret);
        return NULL;
    }
    MESSAGE(VERBOSE_CMS, "Notification of connection creation sent");
    this->mVirtualConnections->Add(Connection);
    return Connection;
}

bool cConnectionManager::destroyVirtualConnection(int ConnectionID){
    if(ConnectionID == 0){
        ERROR("Cannot delete default connection with ID 0!");
        return false;
    }

    cVirtualConnection* Connection;
    for(Connection = this->mVirtualConnections->First(); Connection && Connection->mConnectionID != ConnectionID; Connection = this->mVirtualConnections->Next(Connection)){}

    if(Connection){
        this->mVirtualConnections->Del(Connection);
        // Notify the subscribers
        IXML_Document* PropertySet = NULL;
        const char* IDs = this->getConnectionIDsCVS();
        if(!IDs){
            return false;
        }
        UpnpAddToPropertySet(&PropertySet, "CurrentConnectionIDs", IDs);
        int ret = UpnpNotifyExt(this->mDeviceHandle, UPNP_DEVICE_UDN, UPNP_CMS_SERVICE_ID, PropertySet);
        ixmlDocument_free(PropertySet);

        if(ret != UPNP_E_SUCCESS){
            ERROR("State change notification failed (Error code: %d)",ret);
            return false;
        }
        return true;
    }
    ERROR("No connection with ID=%d found!", ConnectionID);
    return false;
}

const char* cConnectionManager::getConnectionIDsCVS(){
    cString IDs;
    for(cVirtualConnection* Connection = this->mVirtualConnections->First(); Connection; Connection = this->mVirtualConnections->Next(Connection)){
        IDs = cString::sprintf("%s,%d", (*IDs)?*IDs:"", Connection->mConnectionID);
    }
    return IDs;
}

void cConnectionManager::setError(Upnp_Action_Request* Request, int Error){
    Request->ErrCode = Error;
    switch(Error){
        case 701:
            strn0cpy(Request->ErrStr,_("Incompatible protocol info"),LINE_SIZE);
            break;
        case 702:
            strn0cpy(Request->ErrStr,_("Incompatible directions"),LINE_SIZE);
            break;
        case 703:
            strn0cpy(Request->ErrStr,_("Insufficient network resources"),LINE_SIZE);
            break;
        case 704:
            strn0cpy(Request->ErrStr,_("Local restrictions"),LINE_SIZE);
            break;
        case 705:
            strn0cpy(Request->ErrStr,_("Access denied"),LINE_SIZE);
            break;
        case 706:
            strn0cpy(Request->ErrStr,_("Invalid connection reference"),LINE_SIZE);
            break;
        case 707:
            strn0cpy(Request->ErrStr,_("Not in network"),LINE_SIZE);
            break;
        default:
            cUpnpService::setError(Request, Error);
            break;
    }
}

const char* cVirtualConnection::getDirectionString(eDirection Direction){
    switch(Direction){
        case INPUT:
            return "Input";
        case OUTPUT:
            return "Output";
        default:
            return NULL;
    }
}

const char* cVirtualConnection::getStatusString(eConnectionStatus Status){
    switch(Status){
        case OK:
            return "OK";
        case CONTENT_FORMAT_MISMATCH:
            return "ContentFormatMismatch";
        case INSUFFICIENT_BANDWIDTH:
            return "InsufficientBandwidth";
        case UNRELIABLE_CHANNEL:
            return "UnreliableChannel";
        case UNKNOWN:
            return "Unknown";
        default:
            return NULL;
    }
}

int cVirtualConnection::getConnectionStatus(const char* eConnectionStatus){
    if(!strcasecmp(eConnectionStatus,"OK"))
        return OK;
    if(!strcasecmp(eConnectionStatus,"ContentFormatMismatch"))
        return CONTENT_FORMAT_MISMATCH;
    if(!strcasecmp(eConnectionStatus,"InsufficientBandwidth"))
        return INSUFFICIENT_BANDWIDTH;
    if(!strcasecmp(eConnectionStatus,"UnreliableChannel"))
        return UNRELIABLE_CHANNEL;
    if(!strcasecmp(eConnectionStatus,"Unknown"))
        return UNKNOWN;
    return -1;
}

int cVirtualConnection::getDirection(const char* Direction){
    if(!strcasecmp(Direction, "Output"))
        return OUTPUT;
    if(!strcasecmp(Direction, "Input"))
        return INPUT;
    return -1;
}