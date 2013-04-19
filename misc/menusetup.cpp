/* 
 * File:   menusetup.cpp
 * Author: savop
 * 
 * Created on 19. April 2009, 16:50
 */

#include "config.h"
#include <vdr/osdbase.h>
#include "menusetup.h"
#include "../common.h"
#include "util.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <vdr/menuitems.h>


cMenuSetupUPnP::cMenuSetupUPnP(){
    // Get server acitve state
    MESSAGE(VERBOSE_CUSTOM_OUTPUT, "Creating menu");
    this->mCtrlBind = NULL;
    this->mCtrlAutoMode = NULL;
    this->mCtrlEnabled = NULL;
    this->mCtrlPort = NULL;
    this->mEnable = 0;
    this->mDetectPort = 0;
    this->mAutoSetup = 0;
    this->mPort = 0;
    this->mAddress = NULL;
    this->mInterfaceIndex = 0;
    this->Load();
    this->Update();
}

//cMenuSetupUPnP::~cMenuSetupUPnP() {
//    delete this->mCtrlAutoMode;
//    delete this->mCtrlEnabled;
//    delete this->mCtrlPort;
//    free(this->mAddress);
//}

void cMenuSetupUPnP::Load(void){
    cUPnPConfig* Config = cUPnPConfig::get();
    this->mEnable = Config->mEnable;
    this->mAutoSetup = Config->mAutoSetup;
    this->mInterfaceIndex = this->getInterfaceIndex(Config->mInterface);
    this->mAddress = strdup(Config->mAddress?Config->mAddress:"0.0.0.0");
    this->mPort = Config->mPort;

    if(Config->mPort==0) this->mDetectPort = 1;
}

const char* const* cMenuSetupUPnP::getInterfaceList(int* count){
    char** Ifaces = getNetworkInterfaces(count);
    char** IfaceList = new char*[++(*count)];
    IfaceList[0] = strdup(_("User defined"));
    for(int i=0; i < *count-1; i++){
        IfaceList[i+1] = strdup(Ifaces[i]);
    }
    delete [] Ifaces;
    return IfaceList;
}

int cMenuSetupUPnP::getInterfaceIndex(const char* Interface){
    MESSAGE(VERBOSE_CUSTOM_OUTPUT, "Getting Index of %s", Interface);
    if(!Interface) return 0;
    int count;
    int Index = 0;
    const char* const* Ifaces = this->getInterfaceList(&count);

    for(int i=1; i < count; i++){
        if(!strcmp(Interface, Ifaces[i])){
            Index = i;
            break;
        }
    }
    delete [] Ifaces;
    return Index;
}

const char* cMenuSetupUPnP::getInterface(int Index){
    int count;
    const char* const* Ifaces = this->getInterfaceList(&count);

    if(count < Index || Index < 1) return NULL;
    const char* Interface = strdup0(Ifaces[Index]);
    delete [] Ifaces;
    return Interface;
}

void cMenuSetupUPnP::Update(void){
    int Current = this->Current();
    this->Clear();
    // Add OSD menu item for enabling UPnP Server
    this->Add(mCtrlEnabled = new cMenuEditBoolItem(_("Enable UPnP Server"),&this->mEnable,_("disabled"),_("enabled")));
    if(this->mEnable){
        cMenuEditIntItem* editPortItem = NULL;
        this->Add(mCtrlAutoMode = new cMenuEditBoolItem(_("Auto detect settings"),&this->mAutoSetup,_("no"),_("yes")));
        // Add OSD menu item for IP address
        int Count;
        const char* const* Interfaces = this->getInterfaceList(&Count);
        this->Add(mCtrlBind = new cMenuEditStraItem(_("Bind to network interface"), &this->mInterfaceIndex, Count, Interfaces));
        
        cMenuEditIpItem* editIpItem;
        if(this->mInterfaceIndex){
            const sockaddr_in* addr = getIPFromInterface(this->getInterface(this->mInterfaceIndex));
            char* IP = strdup(inet_ntoa(addr->sin_addr));
            editIpItem = new cMenuEditIpItem(_("Current IP address"),IP);
            editIpItem->SetSelectable(false);
            free(IP);
        }
        else {
            editIpItem = new cMenuEditIpItem(_("Set IP address"),this->mAddress);
        }
        this->Add(editIpItem);
        this->Add(mCtrlPort = new cMenuEditBoolItem(_("Select port"), &this->mDetectPort, _("auto"), _("user definied")));
        if(this->mDetectPort){
            this->Add(editPortItem = new cMenuEditIntItem(_("User specified port"),
                                                   &this->mPort,
                                                   SERVER_MIN_PORT,
                                                   SERVER_MAX_PORT
                                                  ));
        }
        
        if(this->mAutoSetup){
            if(mCtrlPort) mCtrlPort->SetSelectable(false);
            if(mCtrlBind) mCtrlBind->SetSelectable(false);
            if(editPortItem) editPortItem->SetSelectable(false);
            if(editIpItem) editIpItem->SetSelectable(false);
        }
        else {
            if(mCtrlPort) mCtrlPort->SetSelectable(true);
            if(mCtrlBind) mCtrlBind->SetSelectable(true);
            if(editPortItem) editPortItem->SetSelectable(true);
            if(editIpItem && !this->mInterfaceIndex) editIpItem->SetSelectable(true);
        }
    }
    this->SetCurrent(this->Get(Current));
    this->Display();
}

eOSState cMenuSetupUPnP::ProcessKey(eKeys Key){

    cOsdItem *Item = this->Get(this->Current());

    eOSState State = cMenuSetupPage::ProcessKey(Key);

    Key = NORMALKEY(Key);

    if(Key != kRight && Key != kLeft){
        return State;
    }
    
    if(Item == this->mCtrlEnabled){
        if(this->mEnable){
            this->Update();
        }
        else if (!this->mEnable) {
            this->Update();
        }
    }
    else if (Item == this->mCtrlPort){
        if(this->mDetectPort){
            this->Update();
        }
        else if(!this->mDetectPort) {
            this->Update();
        }
    }
    else if (Item == this->mCtrlAutoMode){
        if(this->mAutoSetup){
            this->Update();
        }
        else if(!this->mAutoSetup) {
            this->Update();
        }
    }
    else if(Item == this->mCtrlBind){
//        if(!this->mInterfaceIndex){
//            this->Update();
//        }
//        else if(!this->mInterfaceIndex){
//            this->Update();
//        }
        this->Update();
    }
    return State;
}

void cMenuSetupUPnP::Store(void){
    cUPnPConfig* Config = cUPnPConfig::get();
    Config->mAddress = strdup0(this->mAddress);
    Config->mAutoSetup = this->mAutoSetup;
    Config->mEnable = this->mEnable;
    Config->mInterface = strdup0(this->getInterface(this->mInterfaceIndex));
    Config->mPort = (this->mDetectPort) ? 0 : this->mPort;

    this->SetupStore(SETUP_SERVER_AUTO, this->mAutoSetup);
    this->SetupStore(SETUP_SERVER_ENABLED, this->mEnable);
    this->SetupStore(SETUP_SERVER_INT, this->getInterface(this->mInterfaceIndex));
    this->SetupStore(SETUP_SERVER_ADDRESS, this->mAddress);
    this->SetupStore(SETUP_SERVER_PORT, this->mPort);
    
}
