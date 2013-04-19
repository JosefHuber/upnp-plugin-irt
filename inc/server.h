/*
 * File:   server.h
 * Author: savop
 *
 * Created on 19. April 2009, 17:42
 */

#ifndef _SERVER_H
#define	_SERVER_H

#include <netinet/in.h>
#include <vdr/recording.h>
#include <vdr/thread.h>
#include <upnp/upnp.h>
#include "util.h"
#include "../common.h"
#include "webserver.h"
#include "metadata.h"
#include "upnp/connectionmanager.h"
#include "upnp/contentdirectory.h"
#include "../upnp.h"

/**
 * The UPnP Server
 *
 * This is the core of the UPnP server. This handles all the components which
 * are needed for a UPnP media server. Incoming messages are passed through it
 * and it determines what to do.
 */
class cUPnPServer {
    friend class cPluginUpnp;
public:
    /**
     * Constructor
     *
     * This will create a new server and initializes the main functionalities
     * The server has to be started manually by invoking cUPnPServer::start().
     */
    cUPnPServer();
    /**
     * Destructor
     *
     * This will destroy the server object. Open ports and connections will be
     * closed.
     */
    virtual ~cUPnPServer();
    /**
     * Enable the server
     *
     * This switch indicates if the server is startable or not
     *
     * If it is set to false, any invocation of start() will do nothing.
     *
     * @param enabled if \bc true, the server will be enabled. If \bc false it is
     * disabled.
     */
    void enable(bool enabled);
    /**
     * Start the UPnP server
     *
     * This will start the UPnP server activities as a background task.
     *
     * @return returns
     * - \bc true, when the server started successfully
     * - \bc false, otherwise
     */
    bool start(void);
    /**
     * Restart the server
     *
     * When the server is not operating properly it can be restarted.
     * It will stop the server functionalities, clear everything and
     * start it again.
     *
     * @return returns
     * - \bc true, when the server restarted successfully
     * - \bc false, otherwise
     */
    bool restart(void);
    /**
     * Stop the server
     *
     * This will stop the server. This means that open connections to
     * any clients and open ports will be closed.
     */
    void stop(void);
    /**
     * Automatically detect settings
     *
     * This will automatically detect the network settings if the autodetection
     * is turned on.
     *
     * @return returns
     * - \bc true, if autoDetection was successful
     * - \bc false, otherwise
     */
    bool autoDetectSettings(void);
    /**
     * Get the server address
     *
     * Returns a server address structure including IP address and port
     *
     * @return The server socket address
     */
    sockaddr_in* getServerAddress(void);
    /**
     * Get the interface the server listens to
     *
     * @return the network interface
     */
    const char* getInterface(void) const { return this->mInterface; }
    /**
     * Set the server port
     *
     * The port must be in the scope of user definied ports (49152 - 65535). If
     * the port is 0, it is autoassigned. You can retrieve the actual port by
     * calling getServerAddress(), which will give you a structure with the port
     * in it.
     *
     * The server must be restarted if the IP or port changes.
     *
     * Returns 1 when the port is valid, 0 otherwise
     *
     * @param port     The port of the server
     * @return returns
     * - \bc true, if the new server port is set
     * - \bc false, otherwise
     */
    bool setServerPort(unsigned short port);
    /**
     * The Interface to listen on
     * 
     * Sets the listener interface, for instance 'eth1' or 'wlan0'.
     *
     * @param Interface     The interface of the server
     * @return returns
     * - \bc true, if the new server address is set
     * - \bc false, otherwise
     */
    bool setInterface(const char* Interface);
    /**
     * Set the server address
     *
     * Specifies the servers IP address. The server needs to restart
     * when the IP is changed. However, it's not possible to detect
     * changes through the system.
     *
     * This method should only be used in cases of fixed IP addresses
     * for example when no DHCP server is available.
     *
     * @param Address     The address of the server
     * @return returns
     * - \bc true, if the new server address is set
     * - \bc false, otherwise
     */
    bool setAddress(const char* Address);
    /**
     * Enables oder Disables auto detection mode
     *
     * If this is set to true, the setup will get it's information via
     * auto detection
     *
     * @param enable \bc true enables and \bc false disables the auto detection
     * @return returns
     * - \bc true, if the new server address is set
     * - \bc false, otherwise
     */
    bool setAutoDetection(bool enable);
    /**
     * Checks if the server is enabled
     *
     * This indicates if the server is currently enabled.
     *
     * @return returns
     * - \bc true, if the server is enabled
     * - \bc false, otherwise
     */
    bool isEnabled(void) const { return this->mIsEnabled; }
    /**
     * Checks if the server is running
     *
     * If the server is enabled, this indicates if it is running.
     *
     * @return returns
     * - \bc true if the server is running
     * - \bc false, otherwise
     */
    bool isRunning(void) const { return this->mIsRunning; }
    /**
     * Is auto detection enabled or not
     *
     * Returns true or false if auto detection is enabled or not
     *
     * @return returns
     * - \bc true, if autodetection is enabled
     * - \bc false, otherwise
     */
    bool isAutoDetectionEnabled() { return this->mIsAutoDetectionEnabled; }
protected:
private:
    /**
     * Inits the server
     *
     * This method initializes all member variables with default values
     */
    bool init(void);
    bool uninit(void); 
    static int upnpActionCallback(Upnp_EventType eventtype, void *event, void *cookie);
    bool mIsRunning;
    bool mIsEnabled;
    sockaddr_in* mServerAddr;
    cString mInterface;
    bool mIsAutoDetectionEnabled;
    cString mDeviceDescription;
    cUPnPWebServer* mWebServer;
    cMediaDatabase* mMediaDatabase;
    UpnpDevice_Handle mDeviceHandle;
    static cConnectionManager* mConnectionManager;
    static cContentDirectory* mContentDirectory;
};
#endif	/* _SERVER_H */