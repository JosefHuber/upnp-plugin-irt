/*
 * File:   contentdirectory.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 21. August 2009, 16:12
 * Last modification: April 4, 2013
 */

#ifndef _CONTENTDIRECTORY_H
#define	_CONTENTDIRECTORY_H

#include <upnp/upnp.h>
#include "service.h"
#include "metadata.h"

/**
 * The content directory service
 *
 * This is the content directory service according to the UPnP AV ContentDirectory:1 specification.
 * It handles all incoming requests for contents managed by the media server.
 * A frequently invoked request is the UPnP object browse request which delivers informations about
 * TV and radio channels or recordings including the URLs by which the content can be rendered.
 */
class cContentDirectory : public cUpnpService, public cThread {
public:
    /**
     * Constructor of a Content Directory
     *
     * This creates an instance of a <em>Content Directory Service</em> and provides
     * interfaces for executing actions and subscribing on events.
     */
    cContentDirectory(
        UpnpDevice_Handle DeviceHandle,     ///< The UPnP device handle of the root device
        cMediaDatabase* MediaDatabase       ///< the media database where requests are processed
    );
    virtual ~cContentDirectory();
    /*! @copydoc cUpnpService::subscribe(Upnp_Subscription_Request* Request) */
    virtual int subscribe(Upnp_Subscription_Request* Request);
    /*! @copydoc cUpnpService::execute(Upnp_Action_Request* Request) */
    virtual int execute(Upnp_Action_Request* Request);
    /*! @copydoc cUpnpService::setError(Upnp_Action_Request* Request, int Error) */
    virtual void setError(Upnp_Action_Request* Request, int Error);
private:
    cMediaDatabase*             mMediaDatabase;
    void Action();
    int getSearchCapabilities(Upnp_Action_Request* Request);
    int getSortCapabilities(Upnp_Action_Request* Request);
    int getSystemUpdateID(Upnp_Action_Request* Request);
    int browse(Upnp_Action_Request* Request);
};

#endif	/* _CONTENTDIRECTORY_H */

