/* 
 * File:   menusetup.h
 * Author: savop
 *
 * Created on 19. April 2009, 16:50
 */

#ifndef _CMENUSETUPUPNP_H
#define	_CMENUSETUPUPNP_H

#include <vdr/plugin.h>
#include "server.h"
#include "config.h"

/**
 * The VDR setup page
 *
 * This class shows and manages the settings within the VDR setup OSD
 *
 */
class cMenuSetupUPnP : public cMenuSetupPage {
public:
    cMenuSetupUPnP();
//    virtual ~cMenuSetupUPnP();
    /**
     * Processes a keystroke
     *
     * This processes a keystroke which is done by the user and updates the
     * menu accordingly
     *
     * It returns the current state of the VDR after pressing a key
     *
     * @return The current state of the VDR
     */
    virtual eOSState ProcessKey(
        eKeys Key       ///< Key, pressed by the user
    );
protected:
    /**
     * Stores the setup information
     *
     * This stores the setup information in the configuration file
     */
    virtual void Store(void);
    /**
     * Update the menu
     *
     * This updates the menu osd and refreshes the screen.
     */
    void Update(void);
    /**
     * Loads the setup information
     *
     * This loads the setup information from the configuration file
     */
    void Load(void);
private:
    const char* const* getInterfaceList(int *count);
    int getInterfaceIndex(const char* Interface);
    const char* getInterface(int Index);
    cOsdItem *mCtrlBind;
    cOsdItem *mCtrlEnabled;
    cOsdItem *mCtrlPort;
    cOsdItem *mCtrlAutoMode;
    cUPnPServer* mUpnpServer;
    /**
     * Is the server enabled or not
     *
     * The server can be switched on or off. If it is turned off, the server
     * will close open transmissions and ports
     *
     */
    int mEnable;
    int mAutoSetup;
    /**
     * The port to listen to (Default: 0 autodetect)
     *
     * The port the server is bound to. The default setting is 0.
     * So, the server will determine automatically a free random port between
     * 49152 and 65535. If a server should use a specific port it can be set
     * to one out of that range.
     *
     */
    int mPort;
    int mDetectPort;
    /**
     * The Interface the server is bound to
     *
     * If multiple interfaces exist the server can be bound to a specific
     * one
     *
     */
    int mInterfaceIndex;
    /**
     * The socket address of the server
     *
     * The IP address and the port of the server
     */
    char *mAddress;
};

#endif	/* _CMENUSETUPUPNP_H */

