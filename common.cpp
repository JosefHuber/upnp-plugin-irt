/*
 * File:   common.cpp
 * Author: savop
 *
 * Created on 17. April 2009, 20:53
 */

#include <stdarg.h>
#include "common.h"
#include "config.h"

#define MESSAGE_SIZE 256

void message(int Level, const char* , int , const char* Format, ...){
    if(Level && cUPnPConfig::verbosity >= Level){
        va_list ap;
        char Message[MESSAGE_SIZE];

        snprintf(Message, sizeof(Message), "[%d] %s", cThread::ThreadId(), Format);
        va_start(ap, Format);
        vsyslog(LOG_NOTICE, Message, ap);
        va_end(ap);
    }
}