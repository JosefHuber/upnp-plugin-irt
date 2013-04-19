/* 
 * File:   upnpservice.h
 * Author: savop
 *
 * Created on 21. August 2009, 18:38
 */

#ifndef _UPNPSERVICE_H
#define	_UPNPSERVICE_H

#include <upnp/upnp.h>

/**
 * UPnP Service interface
 * 
 * This is a service interface implemented by a UPnP service like CDS oder CMS
 * 
 * It comes with some tool functions which are commonly useful for processing
 * an event or action.
 */
class cUpnpService {
public:
    /**
     * Constructor of a service
     *
     * @private
     * @param DeviceHandle the UPnP device handle of this root device
     */
    cUpnpService(
        UpnpDevice_Handle DeviceHandle      ///< the UPnP device handle of this root device
    );
    virtual ~cUpnpService(){};
    /**
     * Subscribes to an event
     *
     * This is a callback function to register a new subscriber for an event.
     *
     * @return An integer representing one of the following:
     * - \bc UPNP_E_SUCCESS, if subscription was okay
     * - or any other non null value in case of an error
     *
     * @param Request Information about the subscription
     */
    virtual int subscribe(
        Upnp_Subscription_Request* Request      ///< Information about the subscription
    ) = 0;
    /**
     * Executes an action
     *
     * This executes an action initialized by a control point. The result is
     * stored in the first parameter.
     *
     * @return An integer representing one of the following:
     * - \bc UPNP_E_SUCCESS, if subscription was okay
     * - or any other non null value in case of an error
     *
     * @param Request Input and output parameters of an action
     */
    virtual int execute(
        Upnp_Action_Request* Request            ///< Input and output parameters of an action
    ) = 0;
protected:
    /**
     * Sets an error on an action request
     *
     * This function puts a error message into the action request structure
     * according to its error code
     *
     * @param Request the action request, to set the error for
     * @param Error the error code of which the message should be obtained
     */
    virtual void setError(
        Upnp_Action_Request* Request,           ///< the action request, to set the error for
        int Error                               ///< the error code of which the message should be obtained
    );
    /**
     * Parses an integer value
     *
     * This tool function parses an integer value from a given \em IXML document. It is searching
     * for the very first occurance of the demanded item.
     *
     * @return Returns
     * - \bc 0, if parsing was successful
     * - \bc <0, if an error occured
     *
     * @param Document the document, which is parsed
     * @param Item the demanded item
     * @param Value the value of the item
     */
    int parseIntegerValue(
        IN IXML_Document* Document,             ///< the document, which is parsed
        IN const char* Item,                    ///< the demanded item
        OUT int* Value                          ///< the value of the item
    );
    /**
     * Parses a string value
     *
     * This tool function parses a string value from a given \em IXML document. It is searching
     * for the very first occurance of the demanded item.
     *
     * @return Returns
     * - \bc 0, if parsing was successful
     * - \bc <0, if an error occured
     *
     * @param Document the document, which is parsed
     * @param Item the demanded item
     * @param Value the value of the item
     */
    int parseStringValue(
        IN IXML_Document* Document,             ///< the document, which is parsed
        IN const char* Item,                    ///< the demanded item
        OUT char** Value                        ///< the value of the item
    );

    UpnpDevice_Handle mDeviceHandle;            ///< the UPnP device handle of the root device
};

#endif	/* _UPNPSERVICE_H */

