/*
 * File:   config.cpp
 * Author: savop
 * author: J.Huber, IRT GmbH
 * Created on 15. August 2009, 13:03
 * Last modification: February 7, 2013
 */

#include <stdio.h>
#include <vdr/tools.h>
#include <getopt.h>
#include "config.h"
#include "../common.h"
#include "../upnp.h"

cUPnPConfig::cUPnPConfig(){
    this->mDatabaseFolder = NULL;
    this->mHTTPFolder = NULL;
    this->mParsedArgs = NULL;
    this->mInterface = NULL;
    this->mAddress = NULL;
    this->mAutoSetup = 0;
    this->mEnable = 0;
    this->mPort = 0;
    this->mTitleAppend = false;
    this->mDurationZeroChange = false;
    this->mEpgShow = false;
    this->mRadioShow = false;
	this->mOpressTimers = false;
	this->mWithoutCA = false;
	this->mChangeRadioClass = false;
	this->mEpgPreviewDays = 7;            // default value
	this->mFirstChannelsAmount = 0;       // take all channels
	this->mEpgFile = strdup("epg.data");  // see also DEFAULTEPGDATAFILENAME in vdr.c
}

cUPnPConfig::~cUPnPConfig(){}

cUPnPConfig* cUPnPConfig::get(){
    if(cUPnPConfig::mInstance == NULL)
        cUPnPConfig::mInstance = new cUPnPConfig();

    if(cUPnPConfig::mInstance)
        return cUPnPConfig::mInstance;
    else return NULL;
}

cUPnPConfig* cUPnPConfig::mInstance = NULL;
int cUPnPConfig::verbosity = 0;

bool cUPnPConfig::processArgs(int argc, char* argv[]){
  // Implement command line argument processing here if applicable.
    static struct option long_options[] = {
        {"int",     required_argument, NULL, 'i'},
        {"address", required_argument, NULL, 'a'},
        {"port",    required_argument, NULL, 'p'},
        {"autodetect", no_argument,    NULL, 'd'},
		{"broadcastprepend", no_argument, NULL, 'B'},
		{"changeradioclass", no_argument, NULL, 'C'},
        {"duration", no_argument,      NULL, 'D'},
        {"epg",     required_argument, NULL, 'E'},
		{"first_channels", required_argument, NULL, 'F'},
		{"oppress_timers",no_argument, NULL, 'O'},
		{"preview_epg", required_argument, NULL, 'P'}, 
        {"radio",   no_argument,       NULL, 'R'},
		{"title_append", no_argument,  NULL, 'T'},
        {"verbose",    no_argument,    NULL, 'v'},
		{"without_ca", no_argument,    NULL, 'W'},
        {"httpdir", required_argument, NULL, 0},
        {"dbdir",   required_argument, NULL, 0},
        {0, 0, 0, 0}
    };

    int c = 0; int index = -1;
    struct option* opt = NULL;

    // Check if anything went wrong by setting 'success' to false
    // As there are multiple tests you may get a faulty behavior
    // if the current value is not considered in latter tests.
    // Asume that: Success = true;
    // success && false = false; --> new value of success is false
    // success && true = true; --> still true.
    // So, in the case of true and only true, success contains the
    // desired value.
    bool success = true;
    bool ifaceExcistent = false;
    bool addExcistent = false;
    static int verbose = 0;

    while((c = getopt_long(argc, argv, "i:a:BCDE:F:OP:p:dRTvW",long_options, &index)) != -1){
        switch(c){
            case 'i':
                if(addExcistent) { ERROR("Address given but must be absent!"); return false; }
                success = this->parseSetup(SETUP_SERVER_INT, optarg) && success;
                success = this->parseSetup(SETUP_SERVER_ADDRESS, NULL) && success;
                success = this->parseSetup(SETUP_SERVER_AUTO, "0") && success;
                ifaceExcistent = true;
                break;
            case 'a':
                if(ifaceExcistent) { ERROR("Interface given but must be absent!"); return false; }
                success = this->parseSetup(SETUP_SERVER_ADDRESS, optarg) && success;
                success = this->parseSetup(SETUP_SERVER_INT, NULL) && success;
                success = this->parseSetup(SETUP_SERVER_AUTO, "0") && success;
                addExcistent = true;
                break;
            case 'p':
                success = this->parseSetup(SETUP_SERVER_PORT, optarg) && success;
                success = this->parseSetup(SETUP_SERVER_AUTO, "0") && success;
                break;
            case 'd':
                success = this->parseSetup(SETUP_SERVER_AUTO, optarg) && success;
                break;
            case 'v':
                cUPnPConfig::verbosity++;
                verbose++;
                break;
			case 'B':
				fprintf(stdout, "Got the upnp plugin runtime parameter B: prepend the broadcast event title with channel number and name\n");
				this->mBroadcastPrepend = true;
				break;
			case 'C':
				fprintf(stdout, "Got the upnp plugin runtime parameter C: Change the UPnP class audiobroadcast to videobroadcast\n");
				this->mChangeRadioClass = true;
				break;
			case 'D':
				fprintf(stdout, "Got the upnp plugin runtime parameter D: Set a 0 sec duration to a 50 min duration\n");
				this->mDurationZeroChange = true;
				break;
			case 'E':
				success = this->parseSetup(SETUP_EPG_DATA_FILE, optarg) && success;
				fprintf(stdout, "Got the upnp plugin runtime parameter E, epg data file: %s\n", this->mEpgFile);
				this->mEpgShow = true;
				break;
			case 'F':
				success = this->parseSetup(SETUP_AMOUNT_CHANNELS, optarg) && success;
				if (success){
					if (this->mFirstChannelsAmount == 0){
						fprintf(stdout, "Take all channels from channels.conf\n");
					}
					else {
					    fprintf(stdout, "Take the first %i channels from channels.conf\n", this->mFirstChannelsAmount);
					}
				}
				break;		
			case 'O':
				fprintf(stdout, "Got the upnp plugin runtime parameter O: Oppress the record timer objects with browsing\n");
				this->mOpressTimers = true;
				break;
			case 'P': // note: between -P and the amount of days there must be a space to avoid segmentation fault
				success = this->parseSetup(SETUP_PREVIEW_EPG_DAYS, optarg) && success;
				if (success){
					fprintf(stdout, "Got the upnp plugin runtime parameter P: %i EPG preview days\n", this->mEpgPreviewDays);
				}
				break; 
			case 'R':
				fprintf(stdout, "Got the upnp plugin runtime parameter R: show radio\n");
				this->mRadioShow = true;
				break;
			case 'T':
				fprintf(stdout, "Got the upnp plugin runtime parameter T: Append the channel name to the recording title\n");
				this->mTitleAppend = true;
				break;
			case 'W':
				fprintf(stdout, "Got the upnp plugin runtime parameter W: without scrambled channels\n");
				this->mWithoutCA = true;
				break;
            case 0:
                opt = &long_options[index];
                if(!strcasecmp("httpdir", opt->name)){
                    success = this->parseSetup(SETUP_WEBSERVER_DIR, optarg) && success;
                }
                else if(!strcasecmp("dbdir", opt->name)){
                    success = this->parseSetup(SETUP_DATABASE_DIR, optarg) && success;
                }
                break;
            default:
                return false;
        }
    }
    return success;
}

bool cUPnPConfig::parseSetup(const char *Name, const char *Value)
{
    const char* ptr;
    if(*this->mParsedArgs && (ptr = strstr(this->mParsedArgs,Name))!=NULL){
        MESSAGE(VERBOSE_SDK, "Skipping %s=%s, was overridden in command line.", Name, Value);
        return true;
    }

    MESSAGE(VERBOSE_SDK, "VARIABLE %s has value %s", Name, Value);
    // Parse your own setup parameters and store their values.
    if (!strcasecmp(Name, SETUP_SERVER_ENABLED)){
		this->mEnable = atoi(Value);
	}
    else if (!strcasecmp(Name, SETUP_SERVER_AUTO)){
		this->mAutoSetup = atoi(Value);
	}
    else if (!strcasecmp(Name, SETUP_SERVER_INT)){
		this->mInterface = strdup0(Value); // (Value) ? strn0cpy(this->mInterface, Value, strlen(this->mInterface)) : NULL;
	}
    else if (!strcasecmp(Name, SETUP_SERVER_ADDRESS)){
		this->mAddress = strdup0(Value); //(Value) ? strn0cpy(this->mAddress, Value, strlen(this->mAddress)) : NULL;  
	}
    else if (!strcasecmp(Name, SETUP_SERVER_PORT)){
		this->mPort = atoi(Value);
	}
    else if (!strcasecmp(Name, SETUP_WEBSERVER_DIR)){
		this->mHTTPFolder = strdup0(Value);
	}
    else if (!strcasecmp(Name, SETUP_DATABASE_DIR)){
		this->mDatabaseFolder = strdup0(Value);
	}
	else if (!strcasecmp(Name, SETUP_PREVIEW_EPG_DAYS)){
		this->mEpgPreviewDays = atoi(Value);
	}
	else if (!strcasecmp(Name, SETUP_AMOUNT_CHANNELS)){
		this->mFirstChannelsAmount = atoi(Value);
	}
	else if (!strcasecmp(Name, SETUP_EPG_DATA_FILE)){
		this->mEpgFile = strdup0(Value);
	}
    else{
		return false;
	}

    this->mParsedArgs = cString::sprintf("%s%s", *this->mParsedArgs, Name);

    return true;
}