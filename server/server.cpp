/*
 * File:   server.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 19. April 2009, 17:42
 * Last modification: March 14, 2013
 */

#include <vdr/plugin.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <upnp/upnp.h>
#include "server.h"
#include "util.h"
#include "config.h"
#include "../common.h"
#include "upnp/dlna.h"
#include "object.h"

/****************************************************
 *
 *  The UPnP Server
 *
 *  Handles incoming messages, UPnP connections
 *  and so on.
 *
 ****************************************************/

cConnectionManager* cUPnPServer::mConnectionManager = NULL;
cContentDirectory* cUPnPServer::mContentDirectory = NULL;

cUPnPServer::cUPnPServer() {
    this->mServerAddr = new sockaddr_in;
    // Bugfix: this was necessary because there were
    // some uninitialised bytes in the structure (Please recheck this!)
    memset(this->mServerAddr,0,sizeof(sockaddr_in));
    this->mServerAddr->sin_family = AF_INET;
    this->mServerAddr->sin_port = 0;
    this->mIsRunning = false;
    this->mIsAutoDetectionEnabled = true;
    this->mIsEnabled = false;
    this->mDeviceHandle = NULL;
    this->mMediaDatabase = NULL;
}

cUPnPServer::~cUPnPServer() {
    delete this->mServerAddr; this->mServerAddr = NULL;
    delete this->mMediaDatabase;
}

bool cUPnPServer::init(void){

    MESSAGE(VERBOSE_SDK, "Loading configuration...");
    cUPnPConfig* config = cUPnPConfig::get();
    this->enable(config->mEnable == 1 ? true : false);
    if(!config->mAutoSetup){
        if(config->mInterface)
            if(!this->setInterface(config->mInterface)){
                ERROR("Invalid network interface: %s", config->mInterface);
                return false;
            }
        if(config->mAddress)
            if(!this->setAddress(config->mAddress)){
                ERROR("Invalid IP address: %s", config->mAddress);
                return false;
            }
        if(!this->setServerPort((short)config->mPort)){
            ERROR("Invalid port: %d", config->mPort);
            return false;
        }
    }
    else {
        if(!this->setAutoDetection(config->mAutoSetup == 1 ? true : false)){
            ERROR("Invalid auto detection setting: %d", config->mAutoSetup);
            return false;
        }
        if(!this->autoDetectSettings()){
            ERROR("Error while auto detecting settings.");
            return false;
        }
    }

    MESSAGE(VERBOSE_SDK, "Initializing Intel UPnP SDK on %s:%d",inet_ntoa(this->mServerAddr->sin_addr), ntohs(this->mServerAddr->sin_port));
    int ret = 0;
    ret = UpnpInit(inet_ntoa(this->mServerAddr->sin_addr), ntohs(this->mServerAddr->sin_port));

    if (ret != UPNP_E_SUCCESS) {
        // test if SDK was allready initiated
        if (ret == UPNP_E_INIT) {
            WARNING("SDK was allready initiated (no problem) - Errorcode: %d", ret);
        } else {
            ERROR("Error while init Intel SDK - Errorcode: %d", ret);
            return false;
        }
    }
    else {
        if(!inet_aton(UpnpGetServerIpAddress(),&this->mServerAddr->sin_addr)){
            ERROR("Unable to set IP address");
        }
        this->mServerAddr->sin_port = htons(UpnpGetServerPort());
        MESSAGE(VERBOSE_SDK, "Initializing succesfully at %s:%d", UpnpGetServerIpAddress(), UpnpGetServerPort());
    }

    MESSAGE(VERBOSE_CUSTOM_OUTPUT, "Setting maximum packet size for SOAP requests");
    UpnpSetMaxContentLength(UPNP_SOAP_MAX_LEN);

    const char* httpdir = (cUPnPConfig::get()->mHTTPFolder) ? cUPnPConfig::get()->mHTTPFolder : cPluginUpnp::getConfigDirectory();
    //set the root directory of the webserver
    cString WebserverRootDir = cString::sprintf("%s%s", httpdir, UPNP_WEB_SERVER_ROOT_DIR);

    MESSAGE(VERBOSE_SDK, "Set web server root dir: %s", *WebserverRootDir );
    this->mWebServer = cUPnPWebServer::getInstance(WebserverRootDir);
    MESSAGE(VERBOSE_SDK, "Initializing web server.");
    if (!this->mWebServer->init()) {
        ERROR("Error while setting web server root dir - Errorcode: %d", ret);
        return false;
    }

    //register media server device to SDK
    cString URLBase = cString::sprintf("http://%s:%d", UpnpGetServerIpAddress(), UpnpGetServerPort());

    this->mDeviceDescription = cString(cDlna::getInstance()->getDeviceDescription(URLBase),true);

    MESSAGE(VERBOSE_SDK, "Register Media Server Device");
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
            this->mDeviceDescription, sizeof(this->mDeviceDescription), 1,
            &cUPnPServer::upnpActionCallback,
            &this->mDeviceHandle,
            &this->mDeviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        ERROR("Error while registering device - Errorcode: %d", ret);
        return false;
    }

    MESSAGE(VERBOSE_CUSTOM_OUTPUT, "Unregister server to cleanup previously started servers");
    ret = UpnpUnRegisterRootDevice(this->mDeviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        WARNING("Unregistering old devices failed");
        return false;
    }

    MESSAGE(VERBOSE_CUSTOM_OUTPUT, "Register Media Server Device again");
    ret = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
            this->mDeviceDescription, sizeof(this->mDeviceDescription), 1,
            &cUPnPServer::upnpActionCallback,
            &this->mDeviceHandle,
            &this->mDeviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        ERROR("Error while registering device - Errorcode: %d", ret);
        return false;
    }

    MESSAGE(VERBOSE_SDK, "Initializing media database");
    this->mMediaDatabase = new cMediaDatabase;
    if(!this->mMediaDatabase->init()){
        ERROR("Error while initializing database");
        return false;
    }

    MESSAGE(VERBOSE_SDK, "Initializing connection manager");
    cUPnPServer::mConnectionManager = new cConnectionManager(this->mDeviceHandle);
    MESSAGE(VERBOSE_SDK, "Initializing content directory");
    cUPnPServer::mContentDirectory  = new cContentDirectory(this->mDeviceHandle, this->mMediaDatabase);
    if(!cUPnPServer::mContentDirectory->Start()){
        ERROR("Unable to start content directory thread");
        return false;
    }

    //send first advertisments
    MESSAGE(VERBOSE_SDK, "Send first advertisements to publish start in network");
    ret = UpnpSendAdvertisement(this->mDeviceHandle, UPNP_ANNOUNCE_MAX_AGE);
    if (ret != UPNP_E_SUCCESS) {
        ERROR("Error while sending first advertisments - Errorcode: %d", ret);
        return false;
    }
    return true;
}

bool cUPnPServer::uninit(void) {
    MESSAGE(VERBOSE_SDK, "Shuting down content directory");
    delete cUPnPServer::mContentDirectory; cUPnPServer::mContentDirectory = NULL;

    MESSAGE(VERBOSE_SDK, "Shuting down connection manager");
    delete cUPnPServer::mConnectionManager; cUPnPServer::mConnectionManager = NULL;

    MESSAGE(VERBOSE_SDK, "Closing metadata database");
    delete this->mMediaDatabase; this->mMediaDatabase = NULL;

    MESSAGE(VERBOSE_SDK, "Closing the web server");
    this->mWebServer->uninit();
    delete this->mWebServer;

    MESSAGE(VERBOSE_SDK, "Close Intel SDK");
    // unregiser media server device from UPnP SDK
    int ret = UpnpUnRegisterRootDevice(this->mDeviceHandle);
    if (ret != UPNP_E_SUCCESS) {
        WARNING("No device registered");
    }
    // send intel sdk message to shutdown
    ret = UpnpFinish();

    if (ret == UPNP_E_SUCCESS) {
        MESSAGE(VERBOSE_SDK, "Close Intel SDK Successfull");
        return true;
    } else {
        ERROR("Intel SDK unintialized or already closed - Errorcode: %d", ret);
        return false;
    }
}

int cUPnPServer::upnpActionCallback(Upnp_EventType eventtype, void *event, void *cookie) {
    // only to remove warning while compiling because cookie is unused
    cookie = NULL;
    Upnp_Subscription_Request* eventRequest = NULL;
    Upnp_Action_Request*       actionRequest = NULL;

    //check committed event variable
    if (event == NULL) {
        ERROR("UPnP Callback - NULL request");
        return UPNP_E_BAD_REQUEST;
    }

    switch (eventtype) {
        case UPNP_CONTROL_ACTION_REQUEST:
//			MESSAGE(VERBOSE_SDK, "Got a upnpActionCallback of type UPNP_CONTROL_ACTION_REQUEST");
            actionRequest = (Upnp_Action_Request*) event;

            //check that request is for this device
            if (strcmp(actionRequest->DevUDN, UPNP_DEVICE_UDN) != 0) {
                ERROR("UPnP Callback - actions request not for this device");
                return UPNP_E_BAD_REQUEST;
            }

            //find out which service was called
            if (strcmp(actionRequest->ServiceID, UPNP_CMS_SERVICE_ID) == 0) {
                // proceed action
                return cUPnPServer::mConnectionManager->execute(actionRequest);

            } else if (strcmp(actionRequest->ServiceID, UPNP_CDS_SERVICE_ID) == 0) {
                // proceed action
                return cUPnPServer::mContentDirectory->execute(actionRequest);
            } else {
                ERROR("UPnP Callback - unsupported service called for control");
                return UPNP_E_BAD_REQUEST;
            }
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
//			MESSAGE(VERBOSE_SDK, "Got a upnpActionCallback of type UPNP_EVENT_SUBSCRIPTION_REQUEST");
            eventRequest = (Upnp_Subscription_Request*) event;

            //check that request is for this device
            if (strcmp(eventRequest->UDN, UPNP_DEVICE_UDN) != 0) {
                ERROR("UPnP Callback - event request not for this device");
                return UPNP_E_BAD_REQUEST;
            }

            if (strcmp(eventRequest->ServiceId, UPNP_CMS_SERVICE_ID) == 0) {
                // handle event request
                return cUPnPServer::mConnectionManager->subscribe(eventRequest);

            } else if (strcmp(eventRequest->ServiceId, UPNP_CDS_SERVICE_ID) == 0) {
                // handle event request
                return cUPnPServer::mContentDirectory->subscribe(eventRequest);
            } else {
                ERROR("UPnP Callback - unsupported service called for eventing");
                return UPNP_E_BAD_REQUEST;
            }

            return UPNP_E_BAD_REQUEST;
        default:
            ERROR("UPnP Action Callback - Unsupported Event");
            return UPNP_E_BAD_REQUEST;
    }

    return UPNP_E_BAD_REQUEST;
}

bool cUPnPServer::autoDetectSettings(void){
    int count;
    char** Ifaces = getNetworkInterfaces(&count);
    int i=0;
    bool ret = false;
    MESSAGE(VERBOSE_CUSTOM_OUTPUT, "AUTODETECT: Found %d possible interfaces.", sizeof(Ifaces));
    while(Ifaces[i]){
        if(strcmp(Ifaces[i],"lo")!=0){
            //    true || false == true
            //    false || false == false
            ret = this->setInterface(strdup(Ifaces[i])) || ret;
        }
        i++;
    }
    delete [] Ifaces;
    if(!ret){
        MESSAGE(VERBOSE_CUSTOM_OUTPUT, "AUTODETECT: No suitable interface. Giving up.");
        return false;
    }
    this->setServerPort(0);
    return true;
}

bool cUPnPServer::start(void){
    if(!this->isRunning()){
        // Put all the stuff which shall be started with the server in here
        // if the startup failed due any reason return false!
        MESSAGE(VERBOSE_SDK, "Starting UPnP Server on %s:%d",inet_ntoa(this->getServerAddress()->sin_addr), ntohs(this->getServerAddress()->sin_port));
        MESSAGE(VERBOSE_SDK, "Using DLNA version: %s", DLNA_PROTOCOL_VERSION_STR);
        this->mIsRunning = true;
        // Start Media database thread
        this->mMediaDatabase->Start();
    }
    return true;
}

void cUPnPServer::stop(void){
    if(this->isRunning()){
        MESSAGE(VERBOSE_SDK, "Call upnpServer STOP");
        this->uninit();
        this->mIsRunning = false;
    }
    return;
}

bool cUPnPServer::restart(void){
    MESSAGE(VERBOSE_SDK, "Call upnpServer RESTART");
    this->stop();
    return this->start();
}

void cUPnPServer::enable(bool enabled){
    this->mIsEnabled = enabled;
}

bool cUPnPServer::setInterface(const char* Interface){
    if(Interface != NULL) this->mInterface = Interface;

    if(*this->mInterface!=NULL){
        MESSAGE(VERBOSE_CUSTOM_OUTPUT, "Try to retrieve address for NIC %s",Interface);
        const sockaddr_in* ipAddress = getIPFromInterface(Interface);
        if(ipAddress!=NULL){
            memcpy(&this->mServerAddr->sin_addr,&ipAddress->sin_addr,sizeof(ipAddress->sin_addr));
            MESSAGE(VERBOSE_CUSTOM_OUTPUT, "NIC %s has the following IP: %s", *this->mInterface, inet_ntoa(this->mServerAddr->sin_addr));
            this->stop();
            return true;
        }
        delete ipAddress;
        ERROR("Unable to obtain a valid IP address for NIC %s!",Interface);
    }
    this->mServerAddr = NULL;
    return false;
}

bool cUPnPServer::setServerPort(unsigned short port){
    // check if the port is in user range or 0
    if(port != 0 && port < SERVER_MIN_PORT) return false;
    this->stop();
    this->mServerAddr->sin_port = htons(port);
    return true;
}

bool cUPnPServer::setAddress(const char* Address){
    if(inet_aton(Address, &this->mServerAddr->sin_addr) == 0) return false;
    this->stop();
    return true;
}

bool cUPnPServer::setAutoDetection(bool enable){
    this->mIsAutoDetectionEnabled = enable;
    return true;
}

sockaddr_in* cUPnPServer::getServerAddress() {
    return this->mServerAddr;
}