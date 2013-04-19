/*
 * File:   common.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 19. April 2009, 15:22
 * Modified on January 24, 2013
 */

#ifndef _COMMON_H
#define	_COMMON_H

#include "util.h"
#include <libintl.h>
#include <string.h>
#include <vdr/tools.h>
#include <vdr/i18n.h>
#include <vdr/remux.h>

/****************************************************
 *
 * Table of contents
 *
 * This file includes all (or at least most) constant
 * definitions for this plugin. As was growing very
 * fast, I decided to insert this table of contents
 * for faster navigations. However, you have to scroll
 * on your own.
 *
 * 0.  Global constants
 * 1.  VDR and the VDR subsystem
 * 1.1 Versioning
 * 1.2 Logging
 * 1.3 Plugin constants
 * 1.4 Plugin setup
 * 2.  UPnP
 * 2.1 UPnP Namespaces
 * 2.2 Directory hierarchy
 * 2.3 internal Webserver
 * 2.4 Device description
 * 2.5 Connection Manager Service (CMS)
 * 2.6 Content Directory Service (CDS)
 * 2.7 UPnP AV Transport (AVT)
 * 2.8 Media classes
 * 2.9 Storage Media
 * 2.10 Known Errors
 * 2.11 Write Status
 * 2.12 DIDL Properties
 * 3.  DLNA
 * 3.1 Protocol Info Fields
 * 3.2 Protocol Info Flags
 * 3.3 Media Profiles
 * 3.4 Container types
 * 3.5 Device types
 * 4.  SQLite
 * 4.1 Database setup
 *
 ****************************************************/

/****************************************************
 *
 * 0. Global constants
 *
 ****************************************************/

#define IN
#define OUT
#define INOUT

#define TOSTRING(s)             #s

#define FALSE                   0
#define TRUE                    1

#define bool_t                  uint8_t

/**
 * Translation with gettext()
 */
#ifndef _
#define _(s)                    gettext(s)
#endif

#define KB(i)                   i*1024
#define MB(i)                   i*KB(1024)

#define SIZEOF_UUID_STRING      37 // 00000000-0000-0000-0000-000000000000 = 32 + 4 + 1

#define strdup0(s)              (s!=NULL?strdup(s):NULL)

#define att(s)                  strchr(s,'@')!=NULL?strchr(s,'@')+1:NULL
#define prop(s)                 substr(s, 0, strchr(s,'@')-s)

#ifdef WITHOUT_AUDIO
    #define WITHOUT_RADIO
    #define WITHOUT_CUSTOM_AUDIO
#endif

#ifdef WITHOUT_VIDEO
    #define WITHOUT_TV
    #define WITHOUT_RECORDS
    #define WITHOUT_CUSTOM_VIDEO
#endif

/* Check if a channel is a radio station. */
#define ISRADIO(x) ((x)->Vpid()==0||(x)->Vpid()==1||(x)->Vpid()==0x1fff)

void message(int level, const char* File, int Line, const char* Format, ...) __attribute__ ((format (printf, 4, 5)));

/****************************************************
 *
 * 1. VDR and the VDR subsystem
 *
 ****************************************************/

//#define VDR_RECORDFILE_PATTERN_PES  "%s/%03d.vdr"
//#define VDR_RECORDFILE_PATTERN_TS   "%s/%05d.ts"
#define VDR_MAX_FILES_PER_TSRECORDING   65535
#define VDR_MAX_FILES_PER_PESRECORDING  255
//#define VDR_FILENAME_BUFSIZE        2048

/****************************************************
 *
 * 1.1 Versioning
 *
 ****************************************************/

#define VERSION_INT(maj, min, mic) (maj<<16 | min<<8 | mic)
#define VERSION_DOT(maj, min, mic) maj ##.## min ##.## mic
#define VERSION_STR(maj, min, mic) TOSTRING(maj.min.mic)

/* If any changes on the version number are commited, please change the version
 * string in the main file "upnp.c" as well to avoid errors with the makefile */
#define PLUGIN_VERSION_MAJOR    0
#define PLUGIN_VERSION_MINOR    0
#define PLUGIN_VERSION_MICRO    3
/* The plugin version as dot-separated string */
#define PLUGIN_VERSION          VERSION_STR(PLUGIN_VERSION_MAJOR, \
                                        PLUGIN_VERSION_MINOR, \
                                        PLUGIN_VERSION_MICRO)
/* The plugin version as integer representation */
#define PLUGIN_VERSION_INT      VERSION_INT(PLUGIN_VERSION_MAJOR, \
                                               PLUGIN_VERSION_MINOR, \
                                               PLUGIN_VERSION_MICRO)

/****************************************************
 *
 * 1.2 Logging
 *
 ****************************************************/

/**
 * Log errors
 *
 * Errors are critical problems which cannot handled by the server and needs
 * the help by the user.
 */
#define ERROR(s...)         esyslog("UPnP server error:" s)
/**
 * Log warnings
 *
 * Warnings indicate problems with the server which can be handled
 * by the server itself or are not critical to the servers functionality
 */
#define WARNING(s...)       isyslog("UPnP server warning: " s)
/**
 * Log messages
 *
 * Messages are additional information about the servers behavior. This will
 * be useful for debugging.
 */
#ifndef DEBUG
#define MESSAGE(l,s...)       message(l,__FILE__, __LINE__, "UPnP server message: " s)
#else
#define MESSAGE(l,s...)       dsyslog("UPnP server message: " s)
#endif


/**
 * Define at which level the different messages will be printed
 *
 * The log levels reach from 1 to 5, where 1 is the highest log
 * priority and 5 the lowest. 0 will deactivate the logging of
 * the message. If you want to specifiy a certain level, add
 * 'v' options as many as required for the log level.
 *
 * @example
 * - \-v       Show warnings
 * - \-vv      Log level 1
 * - \-vvv     Log level 2
 * - \-vvvv    Log level 3
 * - \-vvvvv   Log level 4
 * - \-vvvvvv  Log level 5
 */
#define VERBOSE_SQL               4                 ///< SQL messages (open DB, close DB etc.)
#define VERBOSE_SQL_FETCHES       0                 ///< SQL fetch messages (SELECT only)
#define VERBOSE_SQL_STATEMENTS    0                 ///< all SQL statements
#define VERBOSE_DIDL              4                 ///< print DIDL related messages
#define VERBOSE_LIVE_TV           3                 ///< print live TV related messages
#define VERBOSE_RECORDS           3                 ///< print messages related to records
#define VERBOSE_CUSTOMFILES       3                 ///< print messages related to custom files and sometimes records
#define VERBOSE_SDK               1                 ///< print important messages of the program flow
#define VERBOSE_EPG_UPDATES       3                 ///< show information on EPG changes
#define VERBOSE_WEBSERVER         2                 ///< print actions done by the webserver
#define VERBOSE_MODIFICATIONS     2                 ///< show modifications to objects or anything else
#define VERBOSE_METADATA          4                 ///< print additional metadata information
#define VERBOSE_CUSTOM_OUTPUT     5                 ///< everything else...
#define VERBOSE_PARSERS           5                 ///< print the parsers output
#define VERBOSE_BUFFERS           4                 ///< print the buffer output
#define VERBOSE_CDS               2                 ///< print messages from the content directory
#define VERBOSE_CMS               2                 ///< print messages from the connection manager
#define VERBOSE_OBJECTS           3                 ///< print messages related to objects

/****************************************************
 *
 * 1.3 Plugin constants
 *
 ****************************************************/

/* The authors of the plugin */
#define PLUGIN_AUTHORS          "J. Huber (IRT GmbH), basierend auf Andreas Guenther und Denis Loh UPnP Plugin"
/* The web site of the plugin */
#define PLUGIN_WEB_PAGE         "http://upnp.methodus.de"
/* A small discription of the plugin, which is also used as the device description */
#define PLUGIN_DESCRIPTION      "UPnP/DLNA compliant Media Server functionality for VDR"
/* The short plugin name. This is used as the main menu of VDR */
#define PLUGIN_SHORT_NAME       "DLNA/UPnP"
/* A somewhat longer name, a.k.a device name */
#define PLUGIN_NAME             "VDR DLNA/UPnP Media Server"
/* Where the plugin can be downloaded */
#define PLUGIN_DOWNLOAD_PAGE    PLUGIN_WEB_PAGE

/****************************************************
 *
 * 1.4 Plugin setup
 *
 ****************************************************/

#define SETUP_SERVER_ENABLED    "Server.Enabled"
#define SETUP_SERVER_INT        "Server.Int"
#define SETUP_SERVER_PORT       "Server.Port"
#define SETUP_SERVER_AUTO       "Server.AutoDetect"
#define SETUP_SERVER_ADDRESS    "Server.Address"
#define SETUP_WEBSERVER_DIR     "Webserver.Directory"
#define SETUP_DATABASE_DIR      "Database.Directory"
#define SETUP_PREVIEW_EPG_DAYS  "Epg.Preview"
#define SETUP_EPG_DATA_FILE     "Epg.Datafile"
#define SETUP_AMOUNT_CHANNELS   "Channels.Amount"

/* The server port range where the server interacts with clients */
#define SERVER_MIN_PORT         49152
#define SERVER_MAX_PORT         65535

#define RECEIVER_LIVEBUFFER_SIZE     MB(1)
#define RECEIVER_OUTPUTBUFFER_SIZE   MB(1)
#define RECEIVER_RINGBUFFER_MARGIN   10*TS_SIZE

/****************************************************
 *
 * 2. UPnP
 *
 ****************************************************/

/*The maximum size of the device description file
 *must NOT exceed 20KB including HTTP headers
 */
#define UPNP_DEVICE_DESCRIPTION_MAX_LEN KB(20)
/* The maximum size of the SOAP requests */
#define UPNP_SOAP_MAX_LEN               KB(20)
/* The max age of announcements in seconds */
#define UPNP_ANNOUNCE_MAX_AGE           1800
/* Max resources per object including
 * preview images and thumbnails
 */
#define UPNP_MAX_RESOURCES_PER_OBJECT   16

enum UPNP_RESOURCE_TYPES {
    UPNP_RESOURCE_CHANNEL,
    UPNP_RESOURCE_RECORDING,
    UPNP_RESOURCE_FILE,
    UPNP_RESOURCE_URL
};

/****************************************************
 *
 * 2.1 Namespaces
 *
 ****************************************************/

#define UPNP_XMLNS_UPNP         "urn:schemas-upnp-org:metadata-1-0/upnp/"
#define UPNP_XMLNS_DIDL         "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"
#define UPNP_XMLNS_DLNA_META    "urn:schemas-dlna-org:metadata-1-0/"
#define UPNP_XMLNS_UPNP_DEV     "urn:schemas-upnp-org:device-1-0"
#define UPNP_XMLNS_DLNA_DEV     "urn:schemas-dlna-org:device-1-0"
#define UPNP_XMLNS_DUBLINCORE   "http://purl.org/dc/elements/1.1/"

#define UPNP_XMLNS_PREFIX_UPNP  "upnp"
#define UPNP_XMLNS_PREFIX_DC    "dc"
#define UPNP_XMLNS_PREFIX_DIDL  ""
#define UPNP_XMLNS_PREFIX_DLNA  "dlna"

/****************************************************
 *
 * 2.2 Directory hierarchy
 *
 ****************************************************/

#define UPNP_DIR_CONTROL        "/control"
#define UPNP_DIR_EVENT          "/event"
#define UPNP_DIR_XML            "/xml"
#define UPNP_DIR_SHARES         "/shares"
#define UPNP_DIR_PRESENTATION   "/http"
#define UPNP_DIR_ICONS          "/icons"

/****************************************************
 *
 * 2.3 internal webserver
 *
 ****************************************************/

#define UPNP_WEB_MAX_FILE_HANDLES    512

#define UPNP_WEB_PRESENTATION_URL    "/index.html"
#define UPNP_WEB_SERVER_ROOT_DIR     UPNP_DIR_PRESENTATION

enum    UPNP_WEB_METHODS {
    UPNP_WEB_METHOD_BROWSE,
    UPNP_WEB_METHOD_SHOW,
    UPNP_WEB_METHOD_STREAM,
    UPNP_WEB_METHOD_SEARCH,
    UPNP_WEB_METHOD_DOWNLOAD
};

/****************************************************
 *
 * 2.4 Device description
 *
 ****************************************************/

/*The device type of the server*/
#define UPNP_DEVICE_TYPE                "urn:schemas-upnp-org:device:MediaServer:1"
/*Path to device description*/
#define UPNP_DEVICE_DESCRIPTION_PATH    UPNP_WEB_SERVER_ROOT_DIR "/ms_desc.xml"
/*Values to identify device and services*/
#define UPNP_DEVICE_UDN                 "uuid:b120ba52-d88d-4500-9b64-888971d83fd3"
/* The friendly device name, human readable */
#define UPNP_DEVICE_FRIENDLY_NAME       PLUGIN_NAME
/* The guys who wrote the crap */
#define UPNP_DEVICE_MANUFACTURER        PLUGIN_AUTHORS
/* The website of the manufacturer, in this case the plugin website */
#define UPNP_DEVICE_MANUFACTURER_URL    "www.irt.de"
// #define UPNP_DEVICE_MANUFACTURER_URL    PLUGIN_WEB_PAGE
/* There is just the one and only model of the plugin, the plugin itself */
#define UPNP_DEVICE_MODEL_DESCRIPTION   PLUGIN_DESCRIPTION
/* The plugin name... */
#define UPNP_DEVICE_MODEL_NAME          "MIfoP DLNA Broadcast Server"
//#define UPNP_DEVICE_MODEL_NAME          PLUGIN_NAME
/* The plugin version */
#define UPNP_DEVICE_MODEL_NUMBER        PLUGIN_VERSION
/* The website of the plugin, this might be different to the manufactures homepage
 * and should redirect to a download mirror where the plugin can be obtained.
 */
#define UPNP_DEVICE_MODEL_URL           PLUGIN_DOWNLOAD_PAGE
/* The serial number of the plugin. This is the integer value of the version */
#define UPNP_DEVICE_SERIAL_NUMBER       "VDR_DLNAUPNP_" PLUGIN_VERSION

#define UPNP_DEVICE_ICON_JPEG_SM        UPNP_DIR_ICONS "/upnpIconSm.jpeg"
#define UPNP_DEVICE_ICON_JPEG_LRG       UPNP_DIR_ICONS "/upnpIconLrg.jpeg"
#define UPNP_DEVICE_ICON_PNG_SM         UPNP_DIR_ICONS "/upnpIconSm.png"
#define UPNP_DEVICE_ICON_PNG_LRG        UPNP_DIR_ICONS "/upnpIconLrg.png"

/****************************************************
 *
 * 2.5 DIDL Properties
 *
 ****************************************************/

#define UPNP_OBJECT_ITEM               "item"
#define UPNP_OBJECT_CONTAINER          "container"

#define UPNP_PROP_OBJECTID             "@id"
#define UPNP_PROP_PARENTID             "@parentID"
#define UPNP_PROP_TITLE                "dc:title"
#define UPNP_PROP_CREATOR              "dc:creator"
#define UPNP_PROP_RESTRICTED           "@restricted"
#define UPNP_PROP_WRITESTATUS          "upnp:writeStatus"
#define UPNP_PROP_CLASS                "upnp:class"
#define UPNP_PROP_CLASSNAME            UPNP_PROP_CLASS "@name"
#define UPNP_PROP_CREATECLASS          "upnp:createClass"
#define UPNP_PROP_SEARCHCLASS          "upnp:searchClass"
#define UPNP_PROP_SCLASSDERIVED        UPNP_PROP_SEARCHCLASS "@includeDerived"
#define UPNP_PROP_SCLASSNAME           UPNP_PROP_SEARCHCLASS "@name"
#define UPNP_PROP_CCLASSDERIVED        UPNP_PROP_CREATECLASS "@includeDerived"
#define UPNP_PROP_CCLASSNAME           UPNP_PROP_CREATECLASS "@name"
#define UPNP_PROP_REFERENCEID          UPNP_OBJECT_ITEM "@refID"
#define UPNP_PROP_SEARCHABLE           UPNP_OBJECT_CONTAINER "@searchable"
#define UPNP_PROP_CHILDCOUNT           UPNP_OBJECT_CONTAINER "@childcount"
#define UPNP_PROP_RESOURCE             "res"
#define UPNP_PROP_PROTOCOLINFO         UPNP_PROP_RESOURCE "@protocolInfo"
#define UPNP_PROP_SIZE                 UPNP_PROP_RESOURCE "@size"
#define UPNP_PROP_DURATION             UPNP_PROP_RESOURCE "@duration"
#define UPNP_PROP_BITRATE              UPNP_PROP_RESOURCE "@bitrate"
#define UPNP_PROP_SAMPLEFREQUENCE      UPNP_PROP_RESOURCE "@sampleFreq"
#define UPNP_PROP_BITSPERSAMPLE        UPNP_PROP_RESOURCE "@bitsPerSample"
#define UPNP_PROP_NOAUDIOCHANNELS      UPNP_PROP_RESOURCE "@nrAudioChannels"
#define UPNP_PROP_COLORDEPTH           UPNP_PROP_RESOURCE "@colorDepth"
#define UPNP_PROP_RESOLUTION           UPNP_PROP_RESOURCE "@resolution"
#define UPNP_PROP_GENRE                "upnp:genre"
#define UPNP_PROP_LONGDESCRIPTION      "upnp:longDescription"
#define UPNP_PROP_PRODUCER             "upnp:producer"
#define UPNP_PROP_RATING               "upnp:rating"
#define UPNP_PROP_ACTOR                "upnp:actor"
#define UPNP_PROP_DIRECTOR             "upnp:director"
#define UPNP_PROP_DESCRIPTION          "dc:description"
#define UPNP_PROP_PUBLISHER            "dc:publisher"
#define UPNP_PROP_LANGUAGE             "dc:language"
#define UPNP_PROP_RELATION             "dc:relation"
#define UPNP_PROP_STORAGEMEDIUM        "upnp:storageMedium"
#define UPNP_PROP_DVDREGIONCODE        "upnp:DVDRegionCode"
#define UPNP_PROP_CHANNELNAME          "upnp:channelName"
#define UPNP_PROP_SCHEDULEDSTARTTIME   "upnp:scheduledStartTime"
#define UPNP_PROP_SCHEDULEDENDTIME     "upnp:scheduledEndTime"
#define UPNP_PROP_ICON                 "upnp:icon"
#define UPNP_PROP_REGION               "upnp:region"
#define UPNP_PROP_CHANNELNR            "upnp:channelNr"
#define UPNP_PROP_RIGHTS               "dc:rights"
#define UPNP_PROP_RADIOCALLSIGN        "upnp:radioCallSign"
#define UPNP_PROP_RADIOSTATIONID       "upnp:radioStationID"
#define UPNP_PROP_RADIOBAND            "upnp:radioBand"
#define UPNP_PROP_CONTRIBUTOR          "dc:contributor"
#define UPNP_PROP_DATE                 "dc:date"
#define UPNP_PROP_ALBUM                "upnp:album"
#define UPNP_PROP_ARTIST               "upnp:artist"
#define UPNP_PROP_DLNA_CONTAINERTYPE   "dlna:container"

#define UPNP_DIDL_SKELETON             "<DIDL-Lite "\
                                       "xmlns=\"" UPNP_XMLNS_DIDL "\" "\
                                       "xmlns:dc=\"" UPNP_XMLNS_DUBLINCORE "\" "\
                                       "xmlns:upnp=\"" UPNP_XMLNS_UPNP "\" "\
                                       "xmlns:dlna=\"" UPNP_XMLNS_DLNA_META "\">"\
                                       "</DIDL-Lite>"

/****************************************************
 *
 * 2.6 Connection Manager Service (CMS)
 *
 ****************************************************/

/*Path to service description of conection manager service*/
#define UPNP_CMS_SCPD_URL            UPNP_DIR_XML "/cms_scpd.xml"
#define UPNP_CMS_CONTROL_URL         UPNP_DIR_CONTROL "/cms_control"
#define UPNP_CMS_EVENT_URL           UPNP_DIR_EVENT "/cms_event"
#define UPNP_CMS_SERVICE_ID          "urn:upnp-org:serviceId:ConnectionManager"
#define UPNP_CMS_SERVICE_TYPE        "urn:schemas-upnp-org:service:ConnectionManager:1"

/* Compatibility usage only --> See DLNA Profiles */
#define UPNP_CMS_SUPPORTED_PROTOCOLS "http-get:*:video/mpeg:*," \
                                     "http-get:*:audio/mpeg:*"

/****************************************************
 *
 * The UPnP CMS actions
 *
 * This constant definitions represent all actions
 * compliant with UPnP ConnectionManager:1
 *
 ****************************************************/

#define UPNP_CMS_ACTION_GETPROTOCOLINFO "GetProtocolInfo"
#define UPNP_CMS_ACTION_GETCURRENTCONNECTIONIDS "GetCurrentConnectionIDs"
#define UPNP_CMS_ACTION_GETCURRENTCONNECTIONINFO "GetCurrentConnectionInfo"
#define UPNP_CMS_ACTION_PREPAREFORCONNECTION "PrepareForConnection"
#define UPNP_CMS_ACTION_CONNECTIONCOMPLETE "ConnectionComplete"

/****************************************************
 *
 * 2.7 Content Directory Service (CDS)
 *
 ****************************************************/

/*Path to service description of content directory service*/
#define UPNP_CDS_SCPD_URL            UPNP_DIR_XML "/cds_scpd.xml"
#define UPNP_CDS_CONTROL_URL         UPNP_DIR_CONTROL "/cds_control"
#define UPNP_CDS_EVENT_URL           UPNP_DIR_EVENT "/cds_event"
#define UPNP_CDS_SERVICE_ID          "urn:upnp-org:serviceId:ContentDirectory"
#define UPNP_CDS_SERVICE_TYPE        "urn:schemas-upnp-org:service:ContentDirectory:1"

#define UPNP_CDS_SEARCH_CAPABILITIES    ""
#define UPNP_CDS_SORT_CAPABILITIES      UPNP_PROP_TITLE ","\
                                        UPNP_PROP_CREATOR ","\
                                        UPNP_PROP_WRITESTATUS ","\
                                        UPNP_PROP_DESCRIPTION ","\
                                        UPNP_PROP_GENRE ","\
                                        UPNP_PROP_LONGDESCRIPTION ","\
                                        UPNP_PROP_PUBLISHER

#define UPNP_DURATION_FORMAT            "%5d:%02d:%02d"
#define UPNP_DURATION_FRAME_FORMAT      "%5d:%02d:%02d.%03d"
#define UPNP_MAX_METADATA_LENGTH        1024
#define AVDETECTOR_TIME_BASE            1000

/****************************************************
 *
 * The UPnP CDS actions
 *
 * This constant definitions represent all actions
 * compliant with UPnP ContentDirectory:1
 *
 ****************************************************/

#define UPNP_CDS_ACTION_SEARCHCAPABILITIES  "GetSearchCapabilities"
#define UPNP_CDS_ACTION_SORTCAPABILITIES    "GetSortCapabilities"
#define UPNP_CDS_ACTION_SYSTEMUPDATEID      "GetSystemUpdateID"
#define UPNP_CDS_ACTION_BROWSE              "Browse"
#define UPNP_CDS_ACTION_SEARCH              "Search"
#define UPNP_CDS_ACTION_CREATEOBJECT        "CreateObject"
#define UPNP_CDS_ACTION_DESTROYOBJECT       "DestroyObject"
#define UPNP_CDS_ACTION_UPDATEOBJECT        "UpdateObject"
#define UPNP_CDS_ACTION_IMPORTRESOURCE      "ImportResource"
#define UPNP_CDS_ACTION_EXPORTRESOURCE      "ExportResource"
#define UPNP_CDS_ACTION_STOPTRANSFERRES     "StopTransferResource"
#define UPNP_CDS_ACTION_TRANSFERPROGRESS    "GetTransferProgress"
#define UPNP_CDS_ACTION_DELETERESOURCE      "DeleteResource"
#define UPNP_CDS_ACTION_CREATEREFERENCE     "CreateReference"

/****************************************************
 *
 * 2.8 UPnP AV Transport (AVT)
 *
 ****************************************************/

#define UPNP_AVT_SCPD_URL               UPNP_DIR_XML "/avt_scpd.xml"
#define UPNP_AVT_CONTROL_URL            UPNP_DIR_CONTROL "/avt_control"
#define UPNP_AVT_EVENT_URL              UPNP_DIR_EVENT "/avt_event"
#define UPNP_AVT_SERVICE_ID             "urn:upnp-org:serviceID:AVTransport"
#define UPNP_AVT_SERVICE_TYPE           "urn:schemas-upnp-org:service:AVTransport:1"

/****************************************************
 *
 * The UPnP AVT actions
 *
 * This constant definitions represent all actions
 * compliant with UPnP AVTransport:1
 *
 ****************************************************/

/****************************************************
 *
 * 2.9 Media classes
 *
 ****************************************************/

#define UPNP_CLASS_OBJECT       "object"
#define UPNP_CLASS_ITEM         UPNP_CLASS_OBJECT "." "item"
#define UPNP_CLASS_CONTAINER    UPNP_CLASS_OBJECT "." "container"
#define UPNP_CLASS_IMAGE        UPNP_CLASS_ITEM "." "imageItem"
#define UPNP_CLASS_AUDIO        UPNP_CLASS_ITEM "." "audioItem"
#define UPNP_CLASS_VIDEO        UPNP_CLASS_ITEM "." "videoItem"
#define UPNP_CLASS_PLAYLIST     UPNP_CLASS_ITEM "." "playlistItem"
#define UPNP_CLASS_TEXT         UPNP_CLASS_ITEM "." "textItem"
#define UPNP_CLASS_EPGITEM      UPNP_CLASS_ITEM "." "epgItem"
#define UPNP_CLASS_BOOKMARKITEM UPNP_CLASS_ITEM "." "bookmarkItem"
#define UPNP_CLASS_PHOTO        UPNP_CLASS_IMAGE "." "photo"
#define UPNP_CLASS_MUSICTRACK   UPNP_CLASS_AUDIO "." "musikTrack"
#define UPNP_CLASS_AUDIOBC      UPNP_CLASS_AUDIO "." "audioBroadcast"
#define UPNP_CLASS_AUDIOBOOK    UPNP_CLASS_AUDIO "." "audioBook"
#define UPNP_CLASS_MOVIE        UPNP_CLASS_VIDEO "." "movie"
#define UPNP_CLASS_VIDEOBC      UPNP_CLASS_VIDEO "." "videoBroadcast"
#define UPNP_CLASS_MUSICVIDCLIP UPNP_CLASS_VIDEO "." "musicVideoClip"
#define UPNP_CLASS_PERSON       UPNP_CLASS_CONTAINER "." "person"
#define UPNP_CLASS_PLAYLISTCONT UPNP_CLASS_CONTAINER "." "playlistContainer"
#define UPNP_CLASS_ALBUM        UPNP_CLASS_CONTAINER "." "album"
#define UPNP_CLASS_GENRE        UPNP_CLASS_CONTAINER "." "genre"
#define UPNP_CLASS_STORAGESYS   UPNP_CLASS_CONTAINER "." "storageSystem"
#define UPNP_CLASS_STORAGEVOL   UPNP_CLASS_CONTAINER "." "storageVolume"
#define UPNP_CLASS_STORAGEFOLD  UPNP_CLASS_CONTAINER "." "storageFolder"
#define UPNP_CLASS_EPGCONTAINER UPNP_CLASS_CONTAINER "." "epgContainer"
#define UPNP_CLASS_MUSICARTIST  UPNP_CLASS_PERSON "." "musicArtist"
#define UPNP_CLASS_MUSICALBUM   UPNP_CLASS_ALBUM "." "musicAlbum"
#define UPNP_CLASS_PHOTOALBUM   UPNP_CLASS_ALBUM "." "photoAlbum"
#define UPNP_CLASS_MUSICGENRE   UPNP_CLASS_GENRE "." "musicGenre"
#define UPNP_CLASS_MOVIEGENRE   UPNP_CLASS_GENRE "." "movieGenre"

/****************************************************
 *
 * 2.10 Storage media
 *
 ****************************************************/

#define UPNP_STORAGE_UNKNOWN			"UNKNOWN"
#define UPNP_STORAGE_DV                         "DV"
#define UPNP_STORAGE_MINI_DV			"MINI-DV"
#define UPNP_STORAGE_VHS			"VHS"
#define UPNP_STORAGE_W_VHS			"W-VHS"
#define UPNP_STORAGE_S_VHS			"S-VHS"
#define UPNP_STORAGE_D_VHS			"D-VHS"
#define UPNP_STORAGE_VHSC			"VHSC"
#define UPNP_STORAGE_VIDEO8			"VIDEO8"
#define UPNP_STORAGE_HI8			"HI8"
#define UPNP_STORAGE_CD_ROM			"CD-ROM"
#define UPNP_STORAGE_CD_DA			"CD-DA"
#define UPNP_STORAGE_CD_R			"CD-R"
#define UPNP_STORAGE_CD_RW			"CD-RW"
#define UPNP_STORAGE_VIDEO_CD			"VIDEO-CD"
#define UPNP_STORAGE_SACD			"SACD"
#define UPNP_STORAGE_MD_AUDIO			"MD-AUDIO"
#define UPNP_STORAGE_MD_PICTURE			"MD-PICTURE"
#define UPNP_STORAGE_DVD_ROM			"DVD-ROM"
#define UPNP_STORAGE_DVD_VIDEO			"DVD-VIDEO"
#define UPNP_STORAGE_DVD_R_MINUS		"DVD-R"
#define UPNP_STORAGE_DVD_RW_PLUS		"DVD+RW"
#define UPNP_STORAGE_DVD_RW_MINUS		"DVD-RW"
#define UPNP_STORAGE_DVD_RAM			"DVD-RAM"
#define UPNP_STORAGE_DVD_AUDIO			"DVD-AUDIO"
#define UPNP_STORAGE_DAT			"DAT"
#define UPNP_STORAGE_LD                         "LD"
#define UPNP_STORAGE_HDD			"HDD"
#define UPNP_STORAGE_MICRO_MV			"MICRO-MV"
#define UPNP_STORAGE_NETWORK			"NETWORK"

/****************************************************
 *
 * 2.11 Known Errors
 *
 ****************************************************/

/* Errors 401-404, 501 are already defined in
 * Intel SDK, however 403 MUST NOT USED.
 */

/****** 600 Common Action Errors ******/

#define UPNP_SOAP_E_ARGUMENT_INVALID            600
#define UPNP_SOAP_E_ARGUMENT_OUT_OF_RANGE       601
#define UPNP_SOAP_E_ACTION_NOT_IMPLEMENTED      602
#define UPNP_SOAP_E_OUT_OF_MEMORY               603
#define UPNP_SOAP_E_HUMAN_INTERVENTION          604
#define UPNP_SOAP_E_STRING_TO_LONG              605
#define UPNP_SOAP_E_NOT_AUTHORIZED              606
#define UPNP_SOAP_E_SIGNATURE_FAILURE           607
#define UPNP_SOAP_E_SIGNATURE_MISSING           608
#define UPNP_SOAP_E_NOT_ENCRYPTED               609
#define UPNP_SOAP_E_INVALID_SEQUENCE            610
#define UPNP_SOAP_E_INVALID_CONTROL_URL         611
#define UPNP_SOAP_E_NO_SUCH_SESSION             612

/****** 700 Action specific Errors ******/

#define UPNP_CDS_E_NO_SUCH_OBJECT               701
#define UPNP_CDS_E_INVALID_CURRENT_TAG          702
#define UPNP_CDS_E_INVALID_NEW_TAG              703
#define UPNP_CDS_E_REQUIRED_TAG                 704
#define UPNP_CDS_E_READ_ONLY_TAG                705
#define UPNP_CDS_E_PARAMETER_MISMATCH           706
#define UPNP_CDS_E_INVALID_SEARCH_CRITERIA      708
#define UPNP_CDS_E_INVALID_SORT_CRITERIA        709
#define UPNP_CDS_E_NO_SUCH_CONTAINER            710
#define UPNP_CDS_E_RESTRICTED_OBJECT            711
#define UPNP_CDS_E_BAD_METADATA                 712
#define UPNP_CDS_E_RESTRICTED_PARENT            713
#define UPNP_CDS_E_NO_SUCH_SOURCE_RESOURCE      714
#define UPNP_CDS_E_RESOURCE_ACCESS_DENIED       715
#define UPNP_CDS_E_TRANSFER_BUSY                716
#define UPNP_CDS_E_NO_SUCH_FILE_TRANSFER        717
#define UPNP_CDS_E_NO_SUCH_DESTINATION_RESOURCE 718
#define UPNP_CDS_E_DEST_RESOURCE_ACCESS_DENIED  719
#define UPNP_CDS_E_CANT_PROCESS_REQUEST         720

#define UPNP_CMS_E_INCOMPATIBLE_PROTOCOL_INFO   701
#define UPNP_CMS_E_INCOMPATIBLE_DIRECTIONS      702
#define UPNP_CMS_E_INSUFFICIENT_RESOURCES       703
#define UPNP_CMS_E_LOCAL_RESTRICTIONS           704
#define UPNP_CMS_E_ACCESS_DENIED                705
#define UPNP_CMS_E_INVALID_CONNECTION_REFERENCE 706
#define UPNP_CMS_E_NOT_IN_NETWORK               707

/****************************************************
 *
 * 2.12 Write Status
 *
 ****************************************************/

enum UPnPWriteStatus {
    WS_UNKNOWN=0,
    WS_WRITABLE,
    WS_PROTECTED,
    WS_NOT_WRITABLE,
    WS_MIXED
};

/****************************************************
 *
 * 3. DLNA
 *
 ****************************************************/

#define DLNA_PROTOCOL_VERSION_MAJOR     1
#define DLNA_PROTOCOL_VERSION_MINOR     5
#define DLNA_PROTOCOL_VERSION_MICRO     0

#define DLNA_PROTOCOL_VERSION_INT       VERSION_INT(DLNA_PROTOCOL_VERSION_MAJOR, \
                                                    DLNA_PROTOCOL_VERSION_MINOR, \
                                                    DLNA_PROTOCOL_VERSION_MICRO)

#define DLNA_PROTOCOL_VERSION_STR       VERSION_STR(DLNA_PROTOCOL_VERSION_MAJOR, \
                                                    DLNA_PROTOCOL_VERSION_MINOR, \
                                                    DLNA_PROTOCOL_VERSION_MICRO)

/****************************************************
 *
 * 3.1 Protocol info fields
 *
 ****************************************************/

/**
 * ATTENTION
 *
 * The following operation field assumes that s0 is NOT changing. Only changes to sN are permitted.
 * If s0 and/or sN changes these fields must be set to false. Use DLNA_FLAG_*_BASED_SEEK flags instead.
 */
#define DLNA_OPERATION_NONE                 00          ///< No seek operations supported
#define DLNA_OPERATION_TIME_SEEK_RANGE      10          ///< is the server supporting time based seeks?
#define DLNA_OPERATION_RANGE                01          ///< or byte based seeks?

#define DLNA_CONVERSION_TRANSCODED          1           ///< the content was converted from one media format to another
#define DLNA_CONVERSION_NONE                0           ///< the content is available without conversion

#define DLNA_SUPPORTED_PLAYSPEEDS           "2,4,8,-2,-4,-8"; ///< 1 is required, but omited in the PS parameter

#define DLNA_TRANSFER_PROTOCOL_HTTP         1           ///< use http tranfer
#define DLNA_TRANSFER_PROTOCOL_RTP          2           ///< use rtp tranfer

/****************************************************
 *
 * 3.2 Protocol info flags
 *
 ****************************************************/

#define DLNA_FLAG_SENDER_PACED              1 << 31     ///< is the server setting the pace (i.e. RTP)?
#define DLNA_FLAG_TIME_BASED_SEEK           1 << 30     ///< is the server supporting time based seeks?
#define DLNA_FLAG_BYTE_BASED_SEEK           1 << 29     ///< or byte based seeking?
#define DLNA_FLAG_PLAY_CONTAINER            1 << 28     ///< is it possible to play all contents of a container?
#define DLNA_FLAG_S0_INCREASE               1 << 27     ///< is the beginning changing (time shift)?
#define DLNA_FLAG_SN_INCREASE               1 << 26     ///< is the end changing (live-TV)?
#define DLNA_FLAG_RTSP_PAUSE                1 << 25     ///< is pausing rtp streams permitted?
#define DLNA_FLAG_STREAMING_TRANSFER        1 << 24     ///< is the transfer a stream (Audio/AV)?
#define DLNA_FLAG_INTERACTIVE_TRANSFER      1 << 23     ///< is the transfer interactiv (printings)?
#define DLNA_FLAG_BACKGROUND_TRANSFER       1 << 22     ///< is the tranfer done in background (downloaded)?
#define DLNA_FLAG_CONNECTION_STALLING       1 << 21     ///< can the connection be paused on HTTP streams?
#define DLNA_FLAG_VERSION_1_5               1 << 20     ///< does the server complies with DLNA V1.5
#define DLNA_FLAG_CLEARTEXT_CONTENT         1 << 16     ///< (Link Protection) currently not used
#define DLNA_FLAG_CLEARTEXT_BYTE_FULL_SEEK  1 << 15     ///< (Link Protection) currently not used
#define DLNA_FLAG_CLEARTEXT_LIMITED_SEEK    1 << 14     ///< (Link Protection) currently not used

#define DLNA_SUPPORTED_FLAGS                DLNA_FLAG_BYTE_BASED_SEEK | \
                                            DLNA_FLAG_SN_INCREASE | \
                                            DLNA_FLAG_STREAMING_TRANSFER | \
                                            DLNA_FLAG_BACKGROUND_TRANSFER | \
                                            DLNA_FLAG_CONNECTION_STALLING | \
                                            DLNA_FLAG_VERSION_1_5

/****************************************************
 *
 * 3.3 Media profiles
 *
 ****************************************************/

/** @see dlna/profiles.h */

/****************************************************
 *
 * 3.4 Container types
 *
 ****************************************************/

enum DLNAContainerTypes {
    TUNER_1_0
};

#define DLNA_CONTAINER_TUNER    "Tuner_1_0"             // The DLNA container type for a tuner

/****************************************************
 *
 * 3.5 Device types
 *
 ****************************************************/

#define DLNA_DEVICE_DMS_1_0     "DMS-1.00"
#define DLNA_DEVICE_DMS_1_5     "DMS-1.50"

/****************************************************
 *
 * 4. SQLite
 *
 ****************************************************/
/****************************************************
 *
 * 4.1 Database setup
 *
 ****************************************************/

#define SQLITE_DB_FILE          "metadata.db"

/****************************************************
 *
 * Please see database.h for further definitions,
 * SQL statements and triggers
 *
 ****************************************************/

#endif	/* _COMMON_H */

