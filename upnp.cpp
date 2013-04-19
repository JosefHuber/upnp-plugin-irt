/*
 * upnp.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 * Author:
 * Author: J.Huber, IRT GmbH
 * Last modification: April 15, 2013
 * $Id$
 */

#include <stdio.h>
#include "upnp.h"
#include "menusetup.h"
#include "config.h"

cCondWait DatabaseLocker;

static const char *VERSION        = "0.0.2";
static const char *DESCRIPTION    = PLUGIN_DESCRIPTION;

const char* cPluginUpnp::mConfigDirectory = NULL;

cPluginUpnp::cPluginUpnp(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    this->mUpnpServer = new cUPnPServer();
}

cPluginUpnp::~cPluginUpnp()
{
  // Clean up after yourself!
    delete this->mUpnpServer;
}

const char* cPluginUpnp::getConfigDirectory(){
    return cPluginUpnp::mConfigDirectory;
}

const char *cPluginUpnp::Version(void){
    return VERSION;
}

const char *cPluginUpnp::Description(void) {
    return DESCRIPTION;
}

const char *cPluginUpnp::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    cString cmdHelp;

    cmdHelp = cString::sprintf(_(
            "  The server can automatically detect both IP address and an\n"
            "  appropriate port, assuming that the first network interface\n"
            "  is connected to the internal network. However, it is possible\n"
            "  to specify alternative settings with the following options:\n\n"
            "  -i <interface>  --int=<interface>     The server network\n"
            "                                        interface\n"
            "                                        e.g: eth0, wlan1 etc.\n"
            "                                        If given option '-a' must\n"
            "                                        be absent.\n"
            "  -a <address>    --address=<address>   The server IPv4 address.\n"
            "                                        If given option '-i' must\n"
            "                                        be absent.\n"
            "  -p <port>       --port=<port>         The server port\n"
            "                                        Supported ports:\n"
            "                                              %5d (auto detect)\n"
            "                                        %5d-%5d (user defined)\n"
            "  -d              --autodetect          Force auto detection\n"
            "                                        Use this option to\n"
            "                                        overwrite the setup menu\n"
            "                                        options.\n"
            "  -B              --broadcastprepend    Prepend the broadcast title with\n"
            "                                        the channel name and channel number.\n"
            "  -C              --changeradioclass    Change the UPnP Class ..audioitem.audioBroadcast\n"
            "                                        to object.item.videoItem.videoBroadcast\n"
            "  -D              --duration            Change a resource duration of '0:00:00'\n"
            "                                        to '0:50:00'. A workaround for some\n"
            "                                        live-stream players.\n"
            "  -E <file>       --epg=<file>          Prepare EPG containers and items and\n"
            "                                        show them with contentdirectory::browse.\n"
            "                                        The 'file' specifies the epg data file in use. \n"
			"  -F <amount>     --first_channels      Take only the first n channels\n"
            "                                        from channels.conf\n"
			"  -O              --oppress_timers      When the parameter is given, with the\n"
			"                                        contentdirectory::browse the folders with\n"
			"                                        the record timers are oppressed.\n"
			"  -P <days>       --preview_epg         The number of days the EPG events are\n"
            "                                        gathered in advance. The -P parameter\n"
            "                                        only takes effect when combined with -E\n"
            "  -R              --radio               Prepare radio containers and items and\n"
            "                                        show them with contentdirectory::browse.\n"
            "  -T              --title_append        Append the channel name to the recording\n"
            "										 title.\n"
            "  -v              --verbose             Increase verbosity level\n"
            "                                        The more v options the\n"
            "                                        higher the output level\n"
            "  -W              --without_ca          Only free to air channels are selected\n"
            "                                        from channels.conf.\n"
            "                  --dbdir=<directory>   The directory in which the\n"
            "                                        metadata database is stored\n"
            "                  --httpdir=<directory> The directory where the\n"
            "                                        http documents are located\n"),
            0,
            SERVER_MIN_PORT,
            SERVER_MAX_PORT
            );
    return cmdHelp;
}

bool cPluginUpnp::ProcessArgs(int argc, char *argv[])
{
    return cUPnPConfig::get()->processArgs(argc, argv);
}

bool cPluginUpnp::Initialize(void)
{
     // Initialize any background activities the plugin shall perform.
    MESSAGE(VERBOSE_SDK, "######### LETS GET READY TO RUMBLE #########");

    cPluginUpnp::mConfigDirectory = strdup(cPlugin::ConfigDirectory(this->Name()));
    if(!cPluginUpnp::getConfigDirectory()){
        ERROR("Cannot set configuration directory");
        return false;
    }
    MESSAGE(VERBOSE_SDK, "Configuration directory: %s", cPluginUpnp::getConfigDirectory());
    DatabaseLocker.Signal();
    return this->mUpnpServer->init();
}

bool cPluginUpnp::Start(void)
{
    MESSAGE(VERBOSE_SDK, "Call plugin START");
    // Start any background activities the plugin shall perform.
    return this->mUpnpServer->start();
    //return true;
}

void cPluginUpnp::Stop(void)
{
    MESSAGE(VERBOSE_SDK, "Call plugin STOP");
    // Stop any background activities the plugin is performing.
    this->mUpnpServer->stop();
}

cString cPluginUpnp::Active(void)
{
    // Return a message string if shutdown should be postponed
    return this->mUpnpServer->isRunning() ? _("The UPnP server is still running."): NULL;
}

cMenuSetupPage *cPluginUpnp::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
    return new cMenuSetupUPnP();
}

bool cPluginUpnp::SetupParse(const char *Name, const char *Value)
{
    return cUPnPConfig::get()->parseSetup(Name, Value);
}

VDRPLUGINCREATOR(cPluginUpnp); // Don't touch this!

