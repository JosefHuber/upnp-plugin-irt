/*
 * File:   upnp.h
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 17. April 2009, 20:53
 * Last modification: April 15, 2013
 */

/**
 * @mainpage A UPnP/DLNA Media Server Plugin for the Linux Based VDR
 * <br>
 * This software is an add-on to the linux based Video Disc Recorder (VDR).
 * It allows access to broadcast content and metadata using UPnP-AV clients such as control points and renderers.
 * <br>
 * Informations about UPnP can be obtained e.g. from http://upnp.org.
 * For detailled informations about the VDR visit http://linuxtv.org/vdrwiki/index.php/VDR_Wiki:Portal.
 * <br>
 * The plugin software runs with VDR versions 1.7.21 or newer, including vdr-2.0.0 and
 * was tested with Linux Debian Squeeze and Wheezy distributions.
 * <br>
 * It is based on the git source code management system's commit:
 * git://projects.vdr-developer.org/vdr-plugin-upnp.git with date of 2010-1-27, 6:55 pm and
 * author Denis Loh <denis.loh@gmail.com>.
 * <br>
 * <br>
 * The plugin implements the UPnP Content Directory and Connection Manager
 * Service and is capable to serve media content over IP.
 * Data are stored using a SQLite database.
 * <br>
 * Available media contents and some of its metadata transferred over the broadcast transport stream
 * can be listed using UPnP control points.
 * Content can be rendered by UPnP-AV media renderers.
 * <br>
 * A wide range of UPnP/DLNA clients are commercial available can be used with this media server:
 * TV devices, play stations, computers (e.g. laptops, tablets) and smartphone apps.
 * <br>
 * The data transfer takes place with usual ethernet, WLAN or powerline connections.
 * <br>
 * <br>
 * Some of the UPnP/DLNA plugin features:
 * <ul>
 * <li>The plugin supports SD and HD live-TV, live-radio and recordings of the VDR as well as custom video files.</li>
 * <li>Recording tasks can be scheduled by an UPnP client using so-called UPnP EPG items.</li>
 * <li>With a content directory browse the Electronic Program Guide items come along with title,
 * start and end time, description, long description and genres of the broadcast event.</li>
 * <li>EPG items, originating from the SI data of the broadcast transport stream, are updated periodically.</li>
 * <li>Scheduled recording tasks can be deleted using so-called record timer items.</li>
 * <li>With a content directory browse the 4th field of the parameter protocolInfo of the
 * DIDL Lite object is set according to the DLNA demands with the proper DLNA.ORG_PN parameter,
 * if applicable.</li>
 * </ul>
 * <br>
 * The actual behaviour of the UPnP/DLNA plugin can be determined using the plugin's start parameter options:
 * <ul>
 * <li>-E <epg-file>	Display EPG containers and items when content directory browsing is done.
 * <epg-file> is the path to the epg file produced by the VDR.</li>
 * <li>-R	Display radio channels with content directory browsing.</li>
 * <li>-O	Opress the record timer items.</li>
 * <li>-P <days> The number of days the EPG items are displayed in advance. Default value: 7 days</li>
 * <li>-F <amount>	Show only the first <amount> channels from the VDR's channel list. Default: show all.</li>
 * <li>-W	Show only free to air stations. Default: show all.</li>
 * <li>-B	Prepend the broadcast event title with the channel number and name when browsing.</li>
 * <li>-T	Append the channel name to the recording title when browsing.</li>
 * </ul>
 */
#ifndef _UPNP_H
#define	_UPNP_H

#include <vdr/thread.h>
#include <vdr/plugin.h>
#include "common.h"
#include "server.h"

class cUPnPServer;

/**
 * The UPnP/DLNA plugin
 *
 * This is a UPnP/DLNA media server plugin. It supports live-TV, live-radio and recordings
 * of the VDR as well as custom video files.
 * Recording tasks can be scheduled by an UPnP client using so-called UPnP EPG items.
 * Scheduled recording tasks can be deleted using so-called record timer items.
 * When a content directory browse is done the 4th field of the parameter protocolInfo of the
 * DIDL Lite object is set according to the DLNA demands with the proper DLNA.ORG_PN parameter,
 * if applicable.
 */
class cPluginUpnp : public cPlugin {
private:
    // Add any member variables or functions you may need here.
    cUPnPServer* mUpnpServer;
    static const char*  mConfigDirectory;
public:
    cPluginUpnp(void);
    virtual ~cPluginUpnp();
    /**
     * Get the version of the plugin
     *
     * Returns the version string of the plugin
     *
     * @return a string representation of the plugin version
     */
    virtual const char *Version(void);
    /**
     * Get the description
     *
     * This returns a brief description of the plugin and what it does.
     *
     * @return the description of the plugin
     */
    virtual const char *Description(void);
    /**
     * Get the command line help
     *
     * This returns the command line help output, which comes, when the user
     * types \c --help into the command line.
     *
     * @return the command line help
     */
    virtual const char *CommandLineHelp(void);
    /*! @copydoc cUPnPConfig::processArgs */
    virtual bool ProcessArgs(int argc, char *argv[]);
    /**
     * Initializes the plugin
     *
     * This initializes any background activities of the plugin.
     *
     * @return returns
     * - \bc true, if initializing was successful
     * - \bc false, otherwise
     */
    virtual bool Initialize(void);
    /**
     * Starts the plugin
     *
     * This starts the plugin. It starts additional threads, which are required
     * by the plugin.
     *
     * @return returns
     * - \bc true, if starting was successful
     * - \bc false, otherwise
     */
    virtual bool Start(void);
    /**
     * Stops the plugin
     *
     * This stops the plugin and all its components
     */
    virtual void Stop(void);
    /**
     * Message if still active
     *
     * This returns a message if the plugin is still active when a user attempts
     * to shut down the VDR.
     *
     * @return the message shown on the screen.
     */
    virtual cString Active(void);
    /**
     * Setup menu
     *
     * This creates a new instance of the setup menu, which is shown to the user
     * when he enters the VDR plugin setup menu
     *
     * @return the menu of the plugin
     */
    virtual cMenuSetupPage *SetupMenu(void);
    /*! @copydoc cUPnPConfig::parseSetup */
    virtual bool SetupParse(const char *Name, const char *Value);
    /**
     * Get the configuration directory
     *
     * This returns the directory, where configuration files are stored.
     *
     * @return the directory of the configuration files.
     */
    static const char* getConfigDirectory();
};

extern cCondWait DatabaseLocker;        ///< Locks the database to be loaded only if
                                        ///< the configuration file directory is set

#endif	/* _UPNP_H */

