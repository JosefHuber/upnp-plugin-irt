/*
 * File:   upnpservice.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 21. August 2009, 18:38
 * Last modification: November 6, 2012
 */

#include "upnp/service.h"
#include "../common.h"
#include "util.h"

cUpnpService::cUpnpService(UpnpDevice_Handle DeviceHandle) {
    this->mDeviceHandle = DeviceHandle;
}

int cUpnpService::parseIntegerValue(IN IXML_Document* Document, IN const char* Item, OUT int* Value){
    char* Val = NULL;
    int Error = 0;

    Val = ixmlGetFirstDocumentItem(Document, Item, &Error);

    if(Error != 0){
        ERROR("Error while parsing integer value for item=%s", Item);
        Error = -1;
    }
    else if(Val == NULL || !Val){
        WARNING("Value %s empty!", Item);
        *Value = 0;
    }
    else {
        *Value = atoi(Val);
        free(Val);
    }
    return Error;
}

int cUpnpService::parseStringValue(IN IXML_Document* Document, IN const char* Item, OUT char** Value){
    char* Val = NULL;
    int Error = 0;

    Val = ixmlGetFirstDocumentItem(Document, Item, &Error);

    if(Error != 0){
        ERROR("Error while parsing string value for item=%s", Item);
        Error = -1;
    }
    else if(Val == NULL || !Val){
//        WARNING("Value %s empty!", Item);
		MESSAGE(VERBOSE_CDS, "Value %s empty!", Item);
        *Value = NULL;
    }
    else {
        *Value = strdup(Val);
        free(Val);
    }

    return Error;
}

void cUpnpService::setError(Upnp_Action_Request* Request, int Error){
    Request->ErrCode = Error;
    switch(Error){
        case UPNP_SOAP_E_INVALID_ACTION:
            strn0cpy(Request->ErrStr,_("Invalid action"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_INVALID_ARGS:
            strn0cpy(Request->ErrStr,_("Invalid args"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_INVALID_VAR:
            strn0cpy(Request->ErrStr,_("Invalid var"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_ACTION_FAILED:
            strn0cpy(Request->ErrStr,_("Action failed"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_ARGUMENT_INVALID:
            strn0cpy(Request->ErrStr,_("Argument value invalid"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_ARGUMENT_OUT_OF_RANGE:
            strn0cpy(Request->ErrStr,_("Argument value out of range"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_ACTION_NOT_IMPLEMENTED:
            strn0cpy(Request->ErrStr,_("Optional action not implemented"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_OUT_OF_MEMORY:
            strn0cpy(Request->ErrStr,_("Out of memory"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_HUMAN_INTERVENTION:
            strn0cpy(Request->ErrStr,_("Human intervention required"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_STRING_TO_LONG:
            strn0cpy(Request->ErrStr,_("String argument to long"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_NOT_AUTHORIZED:
            strn0cpy(Request->ErrStr,_("Action not authorized"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_SIGNATURE_FAILURE:
            strn0cpy(Request->ErrStr,_("Signature failure"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_SIGNATURE_MISSING:
            strn0cpy(Request->ErrStr,_("Signature missing"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_NOT_ENCRYPTED:
            strn0cpy(Request->ErrStr,_("Not encrypted"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_INVALID_SEQUENCE:
            strn0cpy(Request->ErrStr,_("Invalid sequence"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_INVALID_CONTROL_URL:
            strn0cpy(Request->ErrStr,_("Invalid control URL"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_NO_SUCH_SESSION:
            strn0cpy(Request->ErrStr,_("No such session"),LINE_SIZE);
            break;
        case UPNP_SOAP_E_OUT_OF_SYNC:
        default:
            strn0cpy(Request->ErrStr,_("Unknown error code. Contact the device manufacturer"),LINE_SIZE);
            break;
    }
}