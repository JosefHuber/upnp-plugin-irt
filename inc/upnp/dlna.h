/* 
 * File:   dlna.h
 * Author: savop
 *
 * Created on 18. April 2009, 23:27
 */

#ifndef _DLNA_H
#define	_DLNA_H

#include "../common.h"
#include "profiles.h"
#include <list>

using namespace std;

/**
 * Enable DLNA compliant media transfer
 *
 * This class enables media transmission with DLNA conformity. Its compliant with
 * version 1.5 of the DLNA guidelines.
 *
 */
class cDlna {
    friend class cUPnPServer;
public:
    /**
     * Returns the instance of DLNA object
     *
     * This will create a DLNA object instance. It will return the same instance
     * on subsequent calls.
     *
     * @return the DLNA object instance
     */
    static cDlna* getInstance(void);
    virtual ~cDlna();
    //const char* getProtocolInfo(UPnPObjectID OID);
    /**
     * Device description document
     *
     * This will return the device description document with service type
     * definitions as well as some DLNA specific information
     *
     * @return The description document
     */
    const char* getDeviceDescription(
        const char* URLBase         ///< the URLBase to be set in the document
    );
    /**
     * Registeres a DLNA profile
     *
     * Registeres a DLNA profile with specific optional options
     *
     * @see common.h
     */
    void registerProfile(
        DLNAProfile* Profile       ///< the DLNA profile
    );
    /**
     * Registeres all known DLNA profiles
     *
     * Registeres all well known DLNA profiles with its known options
     */
    void registerProfiles();
    /**
     * CSV list of supported protocols
     *
     * Returns a comma separated list with all supported protocols. This
     * means, it returns the list of protocols of the registered profiles.
     *
     * @return CSV list of registered protocols
     */
    const char* getSupportedProtocols();
    /**
     * Protocol info of a specific DLNA profile
     *
     * Returns the protocol info string of a specific DLNA profile with its
     * options and flags.
     *
     * @return the protocol info string of the profile
     */
    const char* getProtocolInfo(
        DLNAProfile *Prof,          ///< the Profile of which the protocol info shall be returned
        int Op = -1,                ///< operation mode
        const char* Ps = NULL,      ///< play speed (CSV list)
        int Ci = -1,                ///< conversion indication flag
        unsigned int Flags = 0      ///< DLNA flags
    );
private:
    cDlna();
    void init(void);
    static cDlna* mInstance;
    list<DLNAProfile*> mRegisteredProfiles;
};

#endif	/* _DLNA_H */

