/* 
 * File:   profiles_mpeg4_p10.cpp
 * Author: savop
 * Author: J.Huber, IRT GmbH
 * Created on 7. Dezember 2009, 13:38
 * Last modification: March 22, 2013
 */

#include "profiles/mpeg4_p10.h"
#include "profiles/container.h"
#include "util.h"
#include "profiles/ac3.h"
#include "avdetector.h"

AcceptedBitrates	DLNA_VIDEOBITRATES_MPEG4_TS_EU      = { true, {1, Mbps(30)}};
AcceptedResolution  DLNA_RESOLUTIONS_MPEG4_PAL_HD[]		= { 
	{ 1920, 1080, 50, 1}, 
	{ 1920, 1080, 25, 1}, 
	{ 1440, 1080, 50, 1}, 
	{ 1440, 1080, 25, 1}, 
	{ 1280, 1080, 50, 1}, 
	{ 1280, 1080, 25, 1}, 
	{ 960, 1080, 50, 1}, 
	{ 960, 1080, 25, 1}, 
	{ 1280, 720, 50, 1}, 
	{ 1280, 720, 25, 1}, 
	{ 960, 720, 50, 1}, 
	{ 960, 720, 25, 1}, 
	{ 640, 720, 50, 1}, 
	{ 640, 720, 25, 1},
	{ 720, 576, 50, 1}, 
	{ 720, 576, 25, 1}, 
	{ 544, 576, 50, 1},
	{ 544, 576, 25, 1},
	{ 480, 576, 50, 1},
	{ 480, 576, 25, 1},
	{ 352, 576, 50, 1},
	{ 352, 576, 25, 1},
	{ 352, 288, 50, 1},
	{ 352, 288, 25, 1},
};

DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_MULT5                     = {"AVC_TS_MP_SD_AAC_MULT5", ""}; ///< AVC main profile AAC 5.1
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_MULT5_T                   = {"AVC_TS_MP_SD_AAC_MULT5_T", ""};   ///< AVC main profile AAC 5.1 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_MULT5_ISO                 = {"AVC_TS_MP_SD_AAC_MULT5_ISO", ""}; ///< AVC main profile AAC 5.1 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_HEAAC_L2                      = {"AVC_TS_MP_SD_HEAAC_L2", ""};  ///< AVC main profile HEAAC L2
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_HEAAC_L2_T                    = {"AVC_TS_MP_SD_HEAAC_L2_T", ""};    ///< AVC main profile HEAAC L2 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_HEAAC_L2_ISO                  = {"AVC_TS_MP_SD_HEAAC_L2_ISO", ""};  ///< AVC main profile HEAAC L2 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_MPEG1_L3                      = {"AVC_TS_MP_SD_MPEG1_L3", ""};  ///< AVC main profile MP3
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_MPEG1_L3_T                    = {"AVC_TS_MP_SD_MPEG1_L3_T", ""};    ///< AVC main profile MP3 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_MPEG1_L3_ISO                  = {"AVC_TS_MP_SD_MPEG1_L3_ISO", ""};  ///< AVC main profile MP3 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AC3                           = {"AVC_TS_MP_SD_AC3", ""};       ///< AVC main profile AC3
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AC3_T                         = {"AVC_TS_MP_SD_AC3_T", ""};     ///< AVC main profile AC3 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AC3_ISO                       = {"AVC_TS_MP_SD_AC3_ISO", ""};   ///< AVC main profile AC3 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_LTP                       = {"AVC_TS_MP_SD_AAC_LTP", ""};       ///< AVC main profile AAC LTP
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_LTP_T                     = {"AVC_TS_MP_SD_AAC_LTP_T", ""};     ///< AVC main profile AAC LTP with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_LTP_ISO                   = {"AVC_TS_MP_SD_AAC_LTP_ISO", ""};   ///< AVC main profile AAC LTP without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_LTP_MULT5                 = {"AVC_TS_MP_SD_AAC_LTP_MULT5", ""};       ///< AVC main profile AAC LTP 5.1
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_LTP_MULT5_T               = {"AVC_TS_MP_SD_AAC_LTP_MULT5_T", ""};     ///< AVC main profile AAC LTP 5.1 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_LTP_MULT5_ISO             = {"AVC_TS_MP_SD_AAC_LTP_MULT5_ISO", ""};   ///< AVC main profile AAC LTP 5.1 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_LTP_MULT7                 = {"AVC_TS_MP_SD_AAC_LTP_MULT7", ""};       ///< AVC main profile AAC LTP 7.1
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_LTP_MULT7_T               = {"AVC_TS_MP_SD_AAC_LTP_MULT7_T", ""};     ///< AVC main profile AAC LTP 7.1 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_AAC_LTP_MULT7_ISO             = {"AVC_TS_MP_SD_AAC_LTP_MULT7_ISO", ""};   ///< AVC main profile AAC LTP 7.1 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_BSAC                          = {"AVC_TS_MP_SD_BSAC", ""};      ///< AVC main profile BSAC
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_BSAC_T                        = {"AVC_TS_MP_SD_BSAC_T", ""};    ///< AVC main profile BSAC with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_SD_BSAC_ISO                      = {"AVC_TS_MP_SD_BSAC_ISO", ""};  ///< AVC main profile BSAC without time stamp

DLNAProfile DLNA_PROFILE_AVC_MP4_MP_SD_AAC_MULT5                    = {"AVC_MP4_MP_SD_AAC_MULT5", ""};    ///< AVC main profile MP4 AAC 5.1
DLNAProfile DLNA_PROFILE_AVC_MP4_MP_SD_HEAAC_L2                     = {"AVC_MP4_MP_SD_HEAAC_L2", ""}; ///< AVC main profile MP4 HEAAC L2
DLNAProfile DLNA_PROFILE_AVC_MP4_MP_SD_MPEG1_L3                     = {"AVC_MP4_MP_SD_MPEG1_L3", ""}; ///< AVC main profile MP4 MP3
DLNAProfile DLNA_PROFILE_AVC_MP4_MP_SD_AC3                          = {"AVC_MP4_MP_SD_AC3", ""};      ///< AVC main profile MP4 AC3
DLNAProfile DLNA_PROFILE_AVC_MP4_MP_SD_AAC_LTP                      = {"AVC_MP4_MP_SD_AAC_LTP", ""};  ///< AVC main profile MP4 AAC LTP
DLNAProfile DLNA_PROFILE_AVC_MP4_MP_SD_AAC_LTP_MULT5                = {"AVC_MP4_MP_SD_AAC_LTP_MULT5", ""};    ///< AVC main profile MP4 AAC LTP 5.1
DLNAProfile DLNA_PROFILE_AVC_MP4_MP_SD_AAC_LTP_MULT7                = {"AVC_MP4_MP_SD_AAC_LTP_MULT7", ""};    ///< AVC main profile MP4 AAC LTP 7.1
DLNAProfile DLNA_PROFILE_AVC_MP4_MP_SD_ATRAC3plus                   = {"AVC_MP4_MP_SD_ATRAC3plus", ""};   ///< AVC main profile MP4 ATRAC3+
DLNAProfile DLNA_PROFILE_AVC_MP4_MP_SD_BSAC                         = {"AVC_MP4_MP_SD_BSAC", ""};     ///< AVC main profile MP4 BSAC

DLNAProfile DLNA_PROFILE_AVC_MP4_BP_L3L_SD_AAC                      = {"AVC_MP4_BP_L3L_SD_AAC", ""};  ///< AVC baseline profile MP4 AAC
DLNAProfile DLNA_PROFILE_AVC_MP4_BP_L3L_SD_HEAAC                    = {"AVC_MP4_BP_L3L_SD_HEAAC", ""};    ///< AVC baseline profile MP4 HEAAC

DLNAProfile DLNA_PROFILE_AVC_MP4_BP_L3_SD_AAC                       = {"AVC_MP4_BP_L3_SD_AAC", ""};   ///< AVC baseline profile standard MP4 AAC

DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_MULT5                  = {"AVC_TS_BL_CIF30_AAC_MULT5", ""};  ///< AVC CIF30 baseline profile AAC 5.1
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_MULT5_T                = {"AVC_TS_BL_CIF30_AAC_MULT5_T", ""};    ///< AVC CIF30 baseline profile AAC 5.1 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_MULT5_ISO              = {"AVC_TS_BL_CIF30_AAC_MULT5_ISO", ""};  ///< AVC CIF30 baseline profile AAC 5.1 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_HEAAC_L2                   = {"AVC_TS_BL_CIF30_HEAAC_L2", ""};   ///< AVC CIF30 baseline profile HEAAC L2
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_HEAAC_L2_T                 = {"AVC_TS_BL_CIF30_HEAAC_L2_T", ""}; ///< AVC CIF30 baseline profile HEAAC L2 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_HEAAC_L2_ISO               = {"AVC_TS_BL_CIF30_HEAAC_L2_ISO", ""};   ///< AVC CIF30 baseline profile HEAAC L2 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_MPEG1_L3                   = {"AVC_TS_BL_CIF30_MPEG1_L3", ""};   ///< AVC CIF30 baseline profile MP3
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_MPEG1_L3_T                 = {"AVC_TS_BL_CIF30_MPEG1_L3_T", ""}; ///< AVC CIF30 baseline profile MP3 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_MPEG1_L3_ISO               = {"AVC_TS_BL_CIF30_MPEG1_L3_ISO", ""};   ///< AVC CIF30 baseline profile MP3 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AC3                        = {"AVC_TS_BL_CIF30_AC3", ""};    ///< AVC CIF30 baseline profile AC3
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AC3_T                      = {"AVC_TS_BL_CIF30_AC3_T", ""};  ///< AVC CIF30 baseline profile AC3 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AC3_ISO                    = {"AVC_TS_BL_CIF30_AC3_ISO", ""};    ///< AVC CIF30 baseline profile AC3 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_LTP                    = {"AVC_TS_BL_CIF30_AAC_LTP", ""};    ///< AVC CIF30 baseline profile AAC LTP
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_LTP_T                  = {"AVC_TS_BL_CIF30_AAC_LTP_T", ""};  ///< AVC CIF30 baseline profile AAC LTP with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_LTP_ISO                = {"AVC_TS_BL_CIF30_AAC_LTP_ISO", ""};    ///< AVC CIF30 baseline profile AAC LTP without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_LTP_MULT5              = {"AVC_TS_BL_CIF30_AAC_LTP_MULT5", ""};  ///< AVC CIF30 baseline profile AAC LTP 5.1
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_LTP_MULT5_T            = {"AVC_TS_BL_CIF30_AAC_LTP_MULT5_T", ""};    ///< AVC CIF30 baseline profile AAC LTP 5.1 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_LTP_MULT5_ISO          = {"AVC_TS_BL_CIF30_AAC_LTP_MULT5_ISO", ""};  ///< AVC CIF30 baseline profile AAC LTP 5.1 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_940                    = {"AVC_TS_BL_CIF30_AAC_940", ""};    ///< AVC CIF30 baseline profile AAC 940
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_940_T                  = {"AVC_TS_BL_CIF30_AAC_940_T", ""};  ///< AVC CIF30 baseline profile AAC 940 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF30_AAC_940_ISO                = {"AVC_TS_BL_CIF30_AAC_940_ISO", ""};    ///< AVC CIF30 baseline profile AAC 940 without time stamp

DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF30_AAC_MULT5                 = {"AVC_MP4_BL_CIF30_AAC_MULT5", ""}; ///< AVC CIF30 baseline profile MP4 AAC 5.1
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF30_HEAAC_L2                  = {"AVC_MP4_BL_CIF30_HEAAC_L2", ""};  ///< AVC CIF30 baseline profile MP4 HEAAC L2
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF30_MPEG1_L3                  = {"AVC_MP4_BL_CIF30_MPEG1_L3", ""};  ///< AVC CIF30 baseline profile MP4 MP3
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF30_AC3                       = {"AVC_MP4_BL_CIF30_AC3", ""};   ///< AVC CIF30 baseline profile MP4 AC3
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF30_AAC_LTP                   = {"AVC_MP4_BL_CIF30_AAC_LTP", ""};   ///< AVC CIF30 baseline profile MP4 AAC LTP
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF30_AAC_LTP_MULT5             = {"AVC_MP4_BL_CIF30_AAC_LTP_MULT5", ""}; ///< AVC CIF30 baseline profile MP4 AAC LTP 5.1
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF30_BSAC                      = {"AVC_MP4_BL_CIF30_BSAC", ""};  ///< AVC CIF30 baseline profile BSAC
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF30_BSAC_MULT5                = {"AVC_MP4_BL_CIF30_BSAC_MULT5", ""};    ///< AVC CIF30 baseline profile BSAC 5.1

DLNAProfile DLNA_PROFILE_AVC_MP4_BL_L2_CIF30_AAC                    = {"AVC_MP4_BL_L2_CIF30_AAC", ""};    ///< AVC CIF30 baseline profile L2 AAC

DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF15_HEAAC                     = {"AVC_MP4_BL_CIF15_HEAAC", ""}; ///< AVC CIF15 baseline profile HEAAC
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF15_AMR                       = {"AVC_MP4_BL_CIF15_AMR", ""};   ///< AVC CIF15 baseline profile AMR

DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_MULT5                     = {"AVC_TS_MP_HD_AAC_MULT5", ""};    ///< AVC main profile AAC 5.1
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_MULT5_T                   = {"AVC_TS_MP_HD_AAC_MULT5_T", ""};  ///< AVC main profile AAC 5.1 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_MULT5_ISO                 = {"AVC_TS_MP_HD_AAC_MULT5_ISO", ""};    ///< AVC main profile AAC 5.1 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_HEAAC_L2                      = {"AVC_TS_MP_HD_HEAAC_L2", ""};     ///< AVC main profile HEAAC L2
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_HEAAC_L2_T                    = {"AVC_TS_MP_HD_HEAAC_L2_T", ""};   ///< AVC main profile HEAAC L2 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_HEAAC_L2_ISO                  = {"AVC_TS_MP_HD_HEAAC_L2_ISO", ""}; ///< AVC main profile HEAAC L2 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_MPEG1_L3                      = {"AVC_TS_MP_HD_MPEG1_L3", ""};  ///< AVC main profile MP3
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_MPEG1_L3_T                    = {"AVC_TS_MP_HD_MPEG1_L3_T", ""};    ///< AVC main profile MP3 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_MPEG1_L3_ISO                  = {"AVC_TS_MP_HD_MPEG1_L3_ISO", ""};  ///< AVC main profile MP3 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AC3                           = {"AVC_TS_MP_HD_AC3", ""};       ///< AVC main profile AC3
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AC3_T                         = {"AVC_TS_MP_HD_AC3_T", ""};     ///< AVC main profile AC3 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AC3_ISO                       = {"AVC_TS_MP_HD_AC3_ISO", ""};   ///< AVC main profile AC3 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC                           = {"AVC_TS_MP_HD_AAC", ""};       ///< AVC main profile AAC
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_T                         = {"AVC_TS_MP_HD_AAC_T", ""};     ///< AVC main profile AAC with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_ISO                       = {"AVC_TS_MP_HD_AAC_ISO", ""};   ///< AVC main profile AAC without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_LTP                       = {"AVC_TS_MP_HD_AAC_LTP", ""};   ///< AVC main profile AAC LTP
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_LTP_T                     = {"AVC_TS_MP_HD_AAC_LTP_T", ""}; ///< AVC main profile AAC LTP with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_LTP_ISO                   = {"AVC_TS_MP_HD_AAC_LTP_ISO", ""};   ///< AVC main profile AAC LTP without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_LTP_MULT5                 = {"AVC_TS_MP_HD_AAC_LTP_MULT5", ""}; ///< AVC main profile AAC LTP 5.1
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_LTP_MULT5_T               = {"AVC_TS_MP_HD_AAC_LTP_MULT5_T", ""};   ///< AVC main profile AAC LTP 5.1 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_LTP_MULT5_ISO             = {"AVC_TS_MP_HD_AAC_LTP_MULT5_ISO", ""}; ///< AVC main prpfile AAC LTP 5.1 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_LTP_MULT7                 = {"AVC_TS_MP_HD_AAC_LTP_MULT7", ""}; ///< AVC main profile AAC LTP 7.1
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_LTP_MULT7_T               = {"AVC_TS_MP_HD_AAC_LTP_MULT7_T", ""};   ///< AVC main profile AAC LTP 7.1 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_MP_HD_AAC_LTP_MULT7_ISO             = {"AVC_TS_MP_HD_AAC_LTP_MULT7_ISO", ""}; ///< AVC main prpfile AAC LTP 7.1 without time stamp

DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_AAC                        = {"AVC_TS_BL_CIF15_AAC", ""};    ///< AVC baseline profile AAC
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_AAC_T                      = {"AVC_TS_BL_CIF15_AAC_T", ""};  ///< AVC baseline profile AAC with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_AAC_ISO                    = {"AVC_TS_BL_CIF15_AAC_ISO", ""};    ///< AVC baseline profile AAC without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_AAC_540                    = {"AVC_TS_BL_CIF15_AAC_540", ""};    ///< AVC baseline profile AAC 540
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_AAC_540_T                  = {"AVC_TS_BL_CIF15_AAC_540_T", ""};  ///< AVC baseline profile AAC 540 with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_AAC_540_ISO                = {"AVC_TS_BL_CIF15_AAC_540_ISO", ""};    ///< AVC baseline profile AAC 540 without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_AAC_LTP                    = {"AVC_TS_BL_CIF15_AAC_LTP", ""};    ///< AVC baseline profile AAC LTP
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_AAC_LTP_T                  = {"AVC_TS_BL_CIF15_AAC_LTP_T", ""};  ///< AVC baseline profile AAC LTP with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_AAC_LTP_IS0                = {"AVC_TS_BL_CIF15_AAC_LTP_IS0", ""};    ///< AVC baseline profile AAC LTP without time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_BSAC                       = {"AVC_TS_BL_CIF15_BSAC", ""};   ///< AVC baseline profile BSAC
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_BSAC_T                     = {"AVC_TS_BL_CIF15_BSAC_T", ""}; ///< AVC baseline profile BSAC with time stamp
DLNAProfile DLNA_PROFILE_AVC_TS_BL_CIF15_BSAC_ISO                   = {"AVC_TS_BL_CIF15_BSAC_ISO", ""};   ///< AVC baseline profile BSAC without time stamp

DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF15_AAC                       = {"AVC_MP4_BL_CIF15_AAC", ""};   ///< AVC baseline profile AAC
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF15_AAC_520                   = {"AVC_MP4_BL_CIF15_AAC_520", ""};   ///< AVC baseline profile AAC 520
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF15_AAC_LTP                   = {"AVC_MP4_BL_CIF15_AAC_LTP", ""};   ///< AVC baseline profile AAC LTP
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF15_AAC_LTP_520               = {"AVC_MP4_BL_CIF15_AAC_LTP_520", ""};   ///< AVC baseline profile AAC LTP 520
DLNAProfile DLNA_PROFILE_AVC_MP4_BL_CIF15_BSAC                      = {"AVC_MP4_BL_CIF15_BSAC", ""};  ///< AVC baseline profile BSAC

DLNAProfile DLNA_PROFILE_AVC_MP4_BL_L12_CIF15_HEAAC                 = {"AVC_MP4_BL_L12_CIF15_HEAAC", ""}; ///< AVC baseline profile HEAAC

DLNAProfile DLNA_PROFILE_AVC_MP4_BL_L1B_QCIF15_HEAAC                = {"AVC_MP4_BL_L1B_QCIF15_HEAAC", ""};    ///< AVC baseline profile QCIF15

DLNAProfile DLNA_PROFILE_AVC_3GPP_BL_CIF30_AMR_WBplus               = {"AVC_3GPP_BL_CIF30_AMR_WBplus", ""};   ///< AVC 3GPP baseline profile CIF30 AMR WB+
DLNAProfile DLNA_PROFILE_AVC_3GPP_BL_CIF15_AMR_WBplus               = {"AVC_3GPP_BL_CIF15_AMR_WBplus", ""};   ///< AVC 3GPP baseline profile CIF15 AMR WB+

DLNAProfile DLNA_PROFILE_AVC_3GPP_BL_QCIF15_AAC                     = {"AVC_3GPP_BL_QCIF15_AAC", ""}; ///< AVC 3GPP baseline profile QCIF15 AAC
DLNAProfile DLNA_PROFILE_AVC_3GPP_BL_QCIF15_AAC_LTP                 = {"AVC_3GPP_BL_QCIF15_AAC_LTP", ""}; ///< AVC 3GPP baseline profile QCIF15 AAC LTP
DLNAProfile DLNA_PROFILE_AVC_3GPP_BL_QCIF15_HEAAC                   = {"AVC_3GPP_BL_QCIF15_HEAAC", ""};   ///< AVC 3GPP baseline profile QCIF15 HEAAC
DLNAProfile DLNA_PROFILE_AVC_3GPP_BL_QCIF15_AMR_WBplus              = {"AVC_3GPP_BL_QCIF15_AMR_WBplus", ""};  ///< AVC 3GPP baseline profile QCIF15 AMR WB+
DLNAProfile DLNA_PROFILE_AVC_3GPP_BL_QCIF15_AMR                     = {"AVC_3GPP_BL_QCIF15_AMR", ""}; ///< AVC 3GPP baseline profile QCIF15 AMR

DLNAProfile DLNA_PROFILE_AVC_TS_HD_EU                               = {"AVC_TS_HD_EU", "video/vnd.dlna.mpeg-tts"};           ///< DLNA Profile for HD DVB Television broadcasts
DLNAProfile DLNA_PROFILE_AVC_TS_HD_EU_T                             = {"AVC_TS_HD_EU_T", "video/vnd.dlna.mpeg-tts"};
DLNAProfile DLNA_PROFILE_AVC_TS_HD_EU_ISO                           = {"AVC_TS_HD_EU_ISO", "video/mpeg"};       ///< DLNA Profile for HD DVB Television broadcasts without timestamp

DLNAVideoMapping MPEG4_P10_VIDEO_MAP[] = {
	{ &DLNA_PROFILE_AVC_TS_HD_EU_ISO,	DLNA_VCP_MPEG2_TS_ISO,	DLNA_VPP_MPEG2_PAL_HD,	DLNA_APP_MPEG1_L2},
	{ &DLNA_PROFILE_AVC_TS_HD_EU,		DLNA_VCP_MPEG2_TS,		DLNA_VPP_MPEG2_PAL_HD,	DLNA_APP_MPEG1_L2},
};

/**
  * Probe the DLNA profile for a MPEG4 part 10 codec (H.264)
  * Note: At present the 'probeContainerProfile' function can deliver a different video container profile value when it is called repeatedly.
  *       So if a previously called MPEG2 profiler has returned a valid video container profile this value is used to determine the DLNA profile.
  */
DLNAProfile* cMPEG4_P10Profiler::probeDLNAProfile(AVFormatContext* FormatCtx, VideoContainerProfile* VideoContProf){
	VideoContainerProfile VCP = *VideoContProf;
	if (*VideoContProf == DLNA_VCP_UNKNOWN){
		VCP = MPEG2Profiler.probeContainerProfile(FormatCtx);
	}   
    VideoPortionProfile     VPP = MPEG4_P10Profiler.probeVideoProfile(FormatCtx);
    AudioPortionProfile     APP = MPEG2Profiler.probeAudioProfile(FormatCtx);

    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: VCP: %d, VPP: %d, APP: %d", VCP, VPP, APP);
	if (APP == DLNA_APP_MPEG2_L2){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_MPEG2_L2");
	}
	else if (APP == DLNA_APP_MPEG1_L1){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_MPEG1_L1");
	}
	else if (APP == DLNA_APP_MPEG1_L2){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_MPEG1_L2");
	}
	else if (APP == DLNA_APP_MPEG1_L3){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_MPEG1_L3");
	}
	else if (APP == DLNA_APP_LPCM){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_LPCM");
	}
	else if (APP == DLNA_APP_AC3){
		MESSAGE(VERBOSE_METADATA, "The audio profile is DLNA_APP_AC3");
	}
	else {
		MESSAGE(VERBOSE_METADATA, "Not dealt audio profile is %d", APP);
	}

	if (VCP == DLNA_VCP_MPEG2_TS){
		MESSAGE(VERBOSE_METADATA, "The video container profile is DLNA_VCP_MPEG2_TS");
	}
	else if (VCP == DLNA_VCP_MP4){
		MESSAGE(VERBOSE_METADATA, "The video container profile is DLNA_VCP_MP4");
	}
	else if (VCP == DLNA_VCP_MPEG2_TS_T){
		MESSAGE(VERBOSE_METADATA, "The video container profile is DLNA_VCP_MPEG2_TS_T");
	}
	else if (VCP == DLNA_VCP_MPEG2_TS_ISO){
		MESSAGE(VERBOSE_METADATA, "The video container profile is DLNA_VCP_MPEG2_TS_ISO");
	}

    for(int i=0; i < (int) (sizeof(MPEG4_P10_VIDEO_MAP)/sizeof(DLNAVideoMapping)); i++){
        if(     MPEG4_P10_VIDEO_MAP[i].VideoContainer == VCP &&
                MPEG4_P10_VIDEO_MAP[i].VideoProfile   == VPP &&
                MPEG4_P10_VIDEO_MAP[i].AudioProfile   == APP){
					MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: A profile was returned successfully");
            return MPEG4_P10_VIDEO_MAP[i].Profile;
        }
    }
	MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: No profile found!");
    return NULL;
}

VideoPortionProfile cMPEG4_P10Profiler::probeVideoProfile(AVFormatContext* FormatCtx){
#ifndef AVCODEC53
    AVCodecContext* VideoCodec = cCodecToolKit::getFirstCodecContext(FormatCtx, CODEC_TYPE_VIDEO);
    AVStream* VideoStream = cCodecToolKit::getFirstStream(FormatCtx, CODEC_TYPE_VIDEO);
#endif
#ifdef AVCODEC53
    AVCodecContext* VideoCodec = cCodecToolKit::getFirstCodecContext(FormatCtx, AVMEDIA_TYPE_VIDEO);
    AVStream* VideoStream = cCodecToolKit::getFirstStream(FormatCtx, AVMEDIA_TYPE_VIDEO);
#endif

    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Codec-ID:             %d", VideoCodec->codec_id);
    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Codec-Name:           %s", VideoCodec->codec_name);
    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Codec Bitrate:        %d", VideoCodec->bit_rate);
    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Codec width:          %d", VideoCodec->coded_width);
    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Codec height:         %d", VideoCodec->coded_height);
    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Codec Profile:        %d", VideoCodec->profile);
    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Codec Level:          %d", VideoCodec->level);
    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Codec Chroma:         %d", VideoCodec->pix_fmt);
    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Stream aspect ratio   %d:%d", VideoStream->sample_aspect_ratio.num, VideoStream->sample_aspect_ratio.den);
    MESSAGE(VERBOSE_METADATA, "MPEG4_P10Profiler: Stream fps            %2.3f", av_q2d(VideoStream->r_frame_rate));


	if(VideoCodec->codec_id == CODEC_ID_H264){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_H264");
		if(cCodecToolKit::matchesAcceptedResolutions(DLNA_RESOLUTIONS_MPEG4_PAL_HD,
                (int) (sizeof(DLNA_RESOLUTIONS_MPEG4_PAL_HD)/sizeof(AcceptedResolution)), VideoStream) &&
                cCodecToolKit::matchesAcceptedSystemBitrate(DLNA_VIDEOBITRATES_MPEG4_TS_EU, FormatCtx)) {
            return DLNA_VPP_MPEG2_PAL_HD;
        }
	}
	else if (VideoCodec->codec_id == CODEC_ID_MPEG4){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_MPEG4");
	}
	else if (VideoCodec->codec_id == CODEC_ID_AAC){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_AAC");
	}
	else if (VideoCodec->codec_id == CODEC_ID_AC3){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_AC3");
	}
	else if (VideoCodec->codec_id == CODEC_ID_NONE){
		MESSAGE(VERBOSE_METADATA, "VideoCodec->codec_id is CODEC_ID_NONE");
	}
	else {
		MESSAGE(VERBOSE_METADATA, "cMPEG4_P10Profiler: VideoCodec->codec_id has the unexpected value %i", (int)VideoCodec->codec_id);
	}

    return DLNA_VPP_UNKNOWN;
}

cMPEG4_P10Profiler MPEG4_P10Profiler;