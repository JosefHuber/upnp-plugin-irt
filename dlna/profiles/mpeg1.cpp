/* 
 * File:   profiles_mpeg1.cpp
 * Author: savop
 * 
 * Created on 7. Dezember 2009, 13:34
 */

#include "profiles/mpeg1.h"

DLNAProfile DLNA_PROFILE_MPEG1 = { "MPEG1", "video/mpeg" };

DLNAVideoMapping MPEG1_VIDEO_MAP[] =
{
    { &DLNA_PROFILE_MPEG1, DLNA_VCP_MPEG1, DLNA_VPP_MPEG1, DLNA_APP_MPEG1_L2 }
};