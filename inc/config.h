/*
 * File:   config.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 *
 * Created on 15. August 2009, 13:03
 * Last modification: April 15, 2013
 */

#ifndef _CONFIG_H
#define	_CONFIG_H

/**
 * <h2>Specify the UPnP plugin behaviour using start parameters</h2>
 *
 * In the following example seven options are specified:
 * @example
 *  \-B \-R \-W \-T \-E /var/vdr \-F 280 \-P 14
 * <ul>
 * <li>With a UPnP content directory browse the title of the broadcast event is prepended by the channel number
 * and channel name in the DIDL Lite object.</li>
 * <li>Radio containers and items are shown when browsing.</li>
 * <li>Only free to air channels are selected from the channel configuration list.</li>
 * <li>Recording titles are appended by their channel names in DIDL Lite objects.</li>
 * <li>EPG containers and items are produced and shown. The path to the VDR's EPG file is '/var/vdr'.</li>
 * <li>Take only the first 280 channels from the channel configuration file.</li>
 * <li>Prepare and show EPG items for the next 14 days.</li>
 * </ul>
 */
#include <vdr/tools.h>
#include "../common.h"

/**
 * The configuration settings
 *
 * This holds the configurations for the upnp-plugin. It holds informations about the
 * network settings of the server, some if its status flags and EPG related parameters.
 */
class cUPnPConfig {
private:
    static cUPnPConfig* mInstance;
    cString mParsedArgs;
    cUPnPConfig();
public:
    static int verbosity;                               ///< the verbosity of the plugin, the higher the more messages
                                                        ///< are printed.
    char* mInterface;                                   ///< the network interface, which the server is bound to
    char* mEpgFile;                                     ///< the file name with path where the epg datas are stored
    char* mAddress;                                     ///< the IP address which is used by the server
    int   mPort;                                        ///< the port which the server is listening on
    int   mEnable;                                      ///< indicates, if the server is enabled or not
    int   mAutoSetup;                                   ///< indicates, if the settings are automatically detected
    int   mEpgPreviewDays;                              ///< the number or days the EPG events have to be gathered in advance
    int   mFirstChannelsAmount;                         ///< the amount of channels from channels.conf to be processed
    char* mDatabaseFolder;                              ///< the directory where the metadata.db is located
    char* mHTTPFolder;                                  ///< the directory where the HTTP data is located
    bool  mTitleAppend;                                 ///< if set the title of a stored broadcast movie is appended by the broadcast service name
    bool  mBroadcastPrepend;                            ///< if set the title of a live broadcast is prepended by the broadcast service name
    bool  mEpgShow;                                     ///< if set the EPG containers and items are prepared and shown with contentdirectory::browse()
    bool  mRadioShow;                                   ///< if set the radio items are prepared and shown with contentdirectory::browse()
    bool  mDurationZeroChange;                          ///< if set a resource duration of '0:00:00' is changed to '0:50:00' and a size of 6000000000 bytes.
	bool  mOpressTimers;								///< if set the record timer folders are oppressed with contentdirectory::browse()
    bool  mWithoutCA;                                   ///< if set only the free to air channels are selected from channels.conf
	bool  mChangeRadioClass;  ///< if set change the UPnP class returned in contentdirectory::browse() from object.item.audioitem.audioBroadcast to object.item.videoItem.videoBroadcast
public:
    virtual ~cUPnPConfig();
    /**
     * Get the configuration
     *
     * This returns the instance of the current configuration settings.
     *
     * @return the configuration object
     */
    static cUPnPConfig* get();
    /**
     * Parse setup variable
     *
     * This parses the setup variable with the according value. The value is a
     * string representation and must be converted into the according data type.
     *
     * @return returns
     * - \bc true, if parsing was successful
     * - \bc false, otherwise
     * @param Name the name of the variable
     * @param Value the according value of the variable
     */
    bool  parseSetup(const char* Name, const char* Value);
    /**
     * Processes the commandline arguments
     *
     * This processes the commandline arguments which the user specified at the
     * start of the plugin.
     *
     * @return returns
     * - \bc true, if processing was successful
     * - \bc false, otherwise
     * @param argc the number of arguments in the list
     * @param argv the arguments as a char array
     */
    bool  processArgs(int argc, char* argv[]);
};

#endif	/* _CONFIG_H */

