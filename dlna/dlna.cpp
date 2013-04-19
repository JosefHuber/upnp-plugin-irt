/* 
 * File:   dlna.cpp
 * Author: savop
 * 
 * Created on 18. April 2009, 23:27
 */

#include <stdio.h>
#include <vdr/tools.h>
#include "upnp/dlna.h"

cDlna* cDlna::mInstance = NULL;

cDlna* cDlna::getInstance(void){
    if(cDlna::mInstance == NULL)
        cDlna::mInstance = new cDlna;

    if(cDlna::mInstance != NULL)
        return cDlna::mInstance;
    else return NULL;
}

cDlna::cDlna() {
    this->mRegisteredProfiles.clear();
    this->init();
}

cDlna::~cDlna() {
    this->mRegisteredProfiles.clear();
}


void cDlna::init(void){
    this->registerProfiles();
}

void cDlna::registerProfile(DLNAProfile* Profile){
    this->mRegisteredProfiles.push_back(Profile);
}

void cDlna::registerProfiles(){
    this->registerProfile(&DLNA_PROFILE_MPEG_TS_SD_EU);
    this->registerProfile(&DLNA_PROFILE_AVC_TS_HD_EU);
    this->registerProfile(&DLNA_PROFILE_MPEG_TS_SD_EU_ISO);
    this->registerProfile(&DLNA_PROFILE_AVC_TS_HD_EU_ISO);
}

const char* cDlna::getSupportedProtocols(){
    cString Protocols;
    list<DLNAProfile*>::iterator it;
    for(it=this->mRegisteredProfiles.begin(); it!=this->mRegisteredProfiles.end(); it++){
        Protocols = cString::sprintf("%s%s%s",(*Protocols)?*Protocols:"",(*Protocols)?",":"",this->getProtocolInfo(*it, DLNA_SUPPORTED_FLAGS));
    }
    return Protocols;
}

const char* cDlna::getProtocolInfo(DLNAProfile *Profile, int Op, const char* Ps, int Ci, unsigned int Flags){
    cString DLNA4thField = NULL;
    DLNA4thField = cString::sprintf("DLNA.ORG_PN=%s", Profile->ID);
    if(Op != -1)
        DLNA4thField = cString::sprintf("%s;DLNA.ORG_OP=%d",*DLNA4thField,Op);
    if(Ps != NULL)
        DLNA4thField = cString::sprintf("%s;DLNA.ORG_PS=%s",*DLNA4thField,Ps);
    if(Ci != -1)
        DLNA4thField = cString::sprintf("%s;DLNA.ORG_CI=%d",*DLNA4thField,Ci);
    if(Flags != 0)
        DLNA4thField = cString::sprintf("%s;DLNA.ORG_FLAGS=%.8x%.24x",*DLNA4thField,Flags,0);
	if (strcmp(Profile->ID, "MP3") == 0){
		DLNA4thField = cString::sprintf("%s", "*");
	}
    char* Protocol = strdup(cString::sprintf("http-get:*:%s:%s", Profile->mime, *DLNA4thField));
//    char* Protocol = strdup(cString::sprintf("http-get:*:%s:%s", Profile->mime, "*"));
    return Protocol;
}

const char* cDlna::getDeviceDescription(const char* URLBase){
    cString description = cString::sprintf(
            "<?xml version = \"1.0\" encoding = \"utf-8\"?> \
            <root xmlns=\"%s\" xmlns:%s=\"%s\"> \
                <specVersion> \
                    <major>1</major> \
                    <minor>0</minor> \
                </specVersion> \
                <URLBase>%s</URLBase> \
                <device> \
                    <deviceType>%s</deviceType> \
                    <friendlyName>%s</friendlyName> \
                    <manufacturer>%s</manufacturer> \
                    <manufacturerURL>%s</manufacturerURL> \
                    <modelDescription>%s</modelDescription> \
                    <modelName>%s</modelName> \
                    <modelNumber>%s</modelNumber> \
                    <modelURL>%s</modelURL> \
                    <serialNumber>%s</serialNumber> \
                    <UDN>%s</UDN> \
                    <iconList> \
                        <icon> \
                            <mimetype>%s</mimetype> \
                            <width>%d</width> \
                            <height>%d</height> \
                            <depth>%d</depth> \
                            <url>%s</url> \
                        </icon> \
                        <icon> \
                            <mimetype>%s</mimetype> \
                            <width>%d</width> \
                            <height>%d</height> \
                            <depth>%d</depth> \
                            <url>%s</url> \
                        </icon> \
                        <icon> \
                            <mimetype>%s</mimetype> \
                            <width>%d</width> \
                            <height>%d</height> \
                            <depth>%d</depth> \
                            <url>%s</url> \
                        </icon> \
                        <icon> \
                            <mimetype>%s</mimetype> \
                            <width>%d</width> \
                            <height>%d</height> \
                            <depth>%d</depth> \
                            <url>%s</url> \
                        </icon> \
                    </iconList> \
                    <presentationURL>%s</presentationURL> \
                    <%s:X_DLNADOC>%s</dlna:X_DLNADOC> \
                    <serviceList> \
                        <service> \
                            <serviceType>%s</serviceType> \
                            <serviceId>%s</serviceId> \
                            <SCPDURL>%s</SCPDURL> \
                            <controlURL>%s</controlURL> \
                            <eventSubURL>%s</eventSubURL> \
                        </service> \
                        <service> \
                            <serviceType>%s</serviceType> \
                            <serviceId>%s</serviceId> \
                            <SCPDURL>%s</SCPDURL> \
                            <controlURL>%s</controlURL> \
                            <eventSubURL>%s</eventSubURL> \
                        </service> \
                    </serviceList> \
                </device> \
            </root>",
            UPNP_XMLNS_UPNP_DEV,            // UPnP Device Namespace (2)
            UPNP_XMLNS_PREFIX_DLNA,         // DLNA Namespace prefix (2)
            UPNP_XMLNS_DLNA_DEV,            // DLNA Device Namespace (2)
            URLBase,                        // URLBase (IP:PORT) (7)
            UPNP_DEVICE_TYPE,               // UPnP Device Type (MediaServer:1) (9)
            UPNP_DEVICE_FRIENDLY_NAME,      // UPnP Device Friendly Name (10)
            UPNP_DEVICE_MANUFACTURER,       // UPnP Device Manufacturer (11)
            UPNP_DEVICE_MANUFACTURER_URL,   // UPnP Device Manufacturer URL (12)
            UPNP_DEVICE_MODEL_DESCRIPTION,  // UPnP Device Model Description (13)
            UPNP_DEVICE_MODEL_NAME,         // UPnP Device Model Name (14)
            UPNP_DEVICE_MODEL_NUMBER,       // UPnP Device Model Number (15)
            UPNP_DEVICE_MODEL_URL,          // UPnP Device Model URL (16)
            UPNP_DEVICE_SERIAL_NUMBER,      // UPnP Device Serialnumber (17)
            UPNP_DEVICE_UDN,                // UPnP Device UDN (18)
            DLNA_ICON_JPEG_LRG_24.mime,     // UPnP Device Large Icon JPEG Mimetype (21)
            DLNA_ICON_JPEG_LRG_24.width,    // UPnP Device Large Icon Width (22)
            DLNA_ICON_JPEG_LRG_24.height,   // UPnP Device Large Icon Height (23)
            DLNA_ICON_JPEG_LRG_24.bitDepth, // UPnP Device Large Icon Bit Depth (24)
            UPNP_DEVICE_ICON_JPEG_LRG,      // UPnP Device Large Icon Path (25)
            DLNA_ICON_JPEG_SM_24.mime,      // UPnP Device Small Icon JPEG Mimetype (28)
            DLNA_ICON_JPEG_SM_24.width,     // UPnP Device Small Icon Width (29)
            DLNA_ICON_JPEG_SM_24.height,    // UPnP Device Small Icon Height (30)
            DLNA_ICON_JPEG_SM_24.bitDepth,  // UPnP Device Small Icon Bit Depth (31)
            UPNP_DEVICE_ICON_JPEG_SM,       // UPnP Device Small Icon Path (32)
            DLNA_ICON_PNG_SM_24A.mime,      // UPnP Device Small Icon PNG Mimetype (35)
            DLNA_ICON_PNG_SM_24A.width,     // UPnP Device Small Icon Width (36)
            DLNA_ICON_PNG_SM_24A.height,    // UPnP Device Small Icon Height (37)
            DLNA_ICON_PNG_SM_24A.bitDepth,  // UPnP Device Small Icon Bit Depth (38)
            UPNP_DEVICE_ICON_PNG_SM,        // UPnP Device Small Icon Path (39)
            DLNA_ICON_PNG_LRG_24A.mime,     // UPnP Device Large Icon PNG Mimetype (42)
            DLNA_ICON_PNG_LRG_24A.width,    // UPnP Device Large Icon Width (43)
            DLNA_ICON_PNG_LRG_24A.height,   // UPnP Device Large Icon Height (44)
            DLNA_ICON_PNG_LRG_24A.bitDepth, // UPnP Device Large Icon Bit Depth (45)
            UPNP_DEVICE_ICON_PNG_LRG,       // UPnP Device Large Icon Path (46)
            UPNP_WEB_PRESENTATION_URL,      // UPnP Presentation URL (49)
            UPNP_XMLNS_PREFIX_DLNA,         // DLNA Namespace prefix (50)
            DLNA_DEVICE_DMS_1_5,            // DLNA Device Type/Version (50)
            UPNP_CMS_SERVICE_TYPE,          // UPnP CMS Service Type
            UPNP_CMS_SERVICE_ID,            // UPnP CMS Service ID
            UPNP_CMS_SCPD_URL,              // UPnP CMS Service Description
            UPNP_CMS_CONTROL_URL,           // UPnP CMS Control URL
            UPNP_CMS_EVENT_URL,             // UPnP CMS Event URL
            UPNP_CDS_SERVICE_TYPE,          // UPnP CDS Service Type
            UPNP_CDS_SERVICE_ID,            // UPnP CDS Service ID
            UPNP_CDS_SCPD_URL,              // UPnP CDS Service Description
            UPNP_CDS_CONTROL_URL,           // UPnP CDS Control URL
            UPNP_CDS_EVENT_URL             // UPnP CDS Event URL
//            UPNP_AVT_SERVICE_TYPE,          // UPnP AVT Service Type
//            UPNP_AVT_SERVICE_ID,            // UPnP AVT Service ID
//            UPNP_AVT_SCPD_URL,              // UPnP AVT Service Description
//            UPNP_AVT_CONTROL_URL,           // UPnP AVT Control URL
//            UPNP_AVT_EVENT_URL              // UPnP AVT Event URL
            );
    return strdup0(*description);
}