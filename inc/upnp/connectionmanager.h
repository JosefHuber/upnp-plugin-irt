/*
 * File:   connectionmanager.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 21. August 2009, 18:35
 * Last modification: April 4, 2013
 */

#ifndef _CONNECTIONMANAGER_H
#define	_CONNECTIONMANAGER_H

#include "service.h"

/**
 * Connection status
 *
 * The connection status of a certain virtual connection
 */
enum eConnectionStatus {
    OK,
    CONTENT_FORMAT_MISMATCH,
    INSUFFICIENT_BANDWIDTH,
    UNRELIABLE_CHANNEL,
    UNKNOWN
};

/**
 * Direction
 *
 * The direction of a virtual connection. Input means client to server, Output
 * server to client
 */
enum eDirection {
    OUTPUT,
    INPUT
};

/**
 * Virtual connection
 *
 * A virtual connection managed by the connection manager service
 */
class cVirtualConnection : public cListObject {
    friend class cConnectionManager;
private:
    cString mRemoteProtocolInfo;
    cString mRemoteConnectionManager;
    eDirection  mDirection;
    int         mRemoteConnectionID;
    int         mConnectionID;
    int         mAVTransportID;
    const int   mRcsID;
    eConnectionStatus mStatus;
    cVirtualConnection();
    static const char* getStatusString(eConnectionStatus Status);
    static const char* getDirectionString(eDirection Direction);
    static int getDirection(const char* Direction);
    static int getConnectionStatus(const char* ConnectionStatus);
};

/**
 * The connection manager service
 *
 * This is the connection manager service according to the UPnP AV ConnectionManager:1 specification.
 * It handles all incoming connection, creates and destroys connections to clients.
 */
class cConnectionManager : public cUpnpService {
public:
    /**
     * Constructor of a Connection manager
     *
     * This creates an instance of a <em>Connection Manager Service</em> and provides
     * interfaces for executing actions and subscribing on events.
     */
    cConnectionManager(
        UpnpDevice_Handle DeviceHandle          ///< the UPnP device handle of this root device
    );
    virtual ~cConnectionManager();
    /*! @copydoc cUpnpService::subscribe(Upnp_Subscription_Request* Request) */
    virtual int subscribe(Upnp_Subscription_Request* Request);
    /*! @copydoc cUpnpService::execute(Upnp_Action_Request* Request) */
    virtual int execute(Upnp_Action_Request* Request);
    /*! @copydoc cUpnpService::setError(Upnp_Action_Request* Request, int Error) */
    virtual void setError(Upnp_Action_Request* Request, int Error);
private:
    int getProtocolInfo(Upnp_Action_Request* Request);
    int getCurrentConnectionIDs(Upnp_Action_Request* Request);
    int getCurrentConnectionInfo(Upnp_Action_Request* Request);
    int prepareForConnection(Upnp_Action_Request* Request);
    int connectionComplete(Upnp_Action_Request* Request);
    cVirtualConnection* createVirtualConnection(const char* RemoteProtocolInfo = NULL, const char* RemoteConnectionManager = NULL, int RemoteConnectionID = -1, eDirection Direction = OUTPUT);
    bool destroyVirtualConnection(int ConnectionID);
    const char* getConnectionIDsCVS();
    cVirtualConnection* mDefaultConnection;
    cList<cVirtualConnection>* mVirtualConnections;
    cString mSupportedProtocols;
};

#endif	/* _CONNECTIONMANAGER_H */

