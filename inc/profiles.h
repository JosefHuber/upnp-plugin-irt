/* 
 * File:   profiles.h
 * Author: savop
 *
 * Created on 8. Dezember 2009, 12:45
 */

#ifndef _PROFILES_H
#define	_PROFILES_H

#include "../common.h"

#include "profiles/aac.h"
#include "profiles/ac3.h"
#include "profiles/amr.h"
#include "profiles/atrac3plus.h"
#include "profiles/jpeg.h"
#include "profiles/lpcm.h"
#include "profiles/mpa.h"
#include "profiles/mpeg1.h"
#include "profiles/mpeg2.h"
#include "profiles/mpeg4_p2.h"
#include "profiles/mpeg4_p10.h"
#include "profiles/png.h"

#ifdef WITH_WINDOWS_MEDIA
    #include "profiles/wma.h"
    #include "profiles/wmv9.h"
#endif

#endif	/* _PROFILES_H */

