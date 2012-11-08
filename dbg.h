/*
* Copyright(c) 2007  Ravikumar.T All rights reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
* Authors:
* Ravikumar.T <naidu.trk@gmail.com>
* Steven dake <sdake@montavista.com> { Time printing routine. }
*
*/
#include<stdio.h>
#include<stdarg.h>
#include<stdbool.h>
#include<errno.h>
#include<string.h>
#include<time.h>
#include<stdlib.h>
#include<syslog.h>

#ifndef __DBG_H__
#define __DBG_H__
#ifdef __cplusplus 
extern "C"{
#endif 
/*
   NOTE
   -----
   This file uses compiler specific features for variadic macros. Better,
   compile code with latest gcc compiler. 
   we donot strive to comply with multiple compilers existing, we only comply
   with gcc these macros defined below can be ported to other compilers but
   with a great level of pain and effort.Those who use different compilers are
   on their own :-) .
   */
/*
   Debug levels defined. 
   */
#define PANIC  0x0 
#define FATAL  0x1
#define ERROR  0x2
#define WARN   0x3 
#define INFO   0x4 
#define SYSERR 0x5 

/*
   Module id is a bit in the bitmap moddbg.
   */
#define NULL_MOD_ID (0x0)
#define MEM_POOL_ID (0x1<<0)
#define MSG_POOL_ID (0x1<<1)
#define SOCK_LIB_ID (0x1<<2)
#define PMON_ID     (0x1<<3)
#define WILD_CARD   (0xffffffff)

/*
   logging options whether to log to file or console or both.
   code in daemon mode is not supposed to log to console. 
   */
#define WRITE_TO_CONSOLE (0x1<<1)
#define WRITE_TO_SYSLOG  (0x1<<2)

/*
   courtesy:steven dake
   fills the given string with the current time including microseconds and
   returns the pointer to the same string.
   shamelessly lifted from steven dakes code ;-). 
   */
static inline char *
current_time( char *cur_time )
{
    struct timeval tv = {0x0};
    char time[256];

    gettimeofday(&tv,NULL);
    strftime(time,sizeof(time),"%b:%e %k:%M:%S",localtime(&tv.tv_sec));
    sprintf(cur_time,"%s:%06ld",time,(long)tv.tv_usec);
    return cur_time;
}

/*
   initialize log interface of the module.
   signature is a string attached to every log message logged in the
   /var/log/messages file. only applicable when syslog is in use. 
   */
static inline void 
loginit(char *signature)
{
    openlog(signature, LOG_CONS|LOG_NDELAY, LOG_DAEMON);
    return;
}

static void inline 
dprintf( 
        const int modId, /*private to each module statically initialized in
                           the module file. */
        const int dbgLevel, const char *file, 
        const int line, const char *fmt, 
        ... )
{
    extern long long dbgbmap;
    static char *levels[] = {"pan","fat","err","war","inf","sys"};
    va_list  ap;
    char time[512]={'\0'};
    char logstring[512];

    if( dbgbmap & modId )
    {
        va_start(ap,fmt);
        sprintf(logstring,"\n[%3s] %s %s:%d",levels[dbgLevel],current_time(time),file,line);
        vsprintf(logstring+strlen(logstring),fmt,ap);
        va_end(ap);

        int priority = 0x0;
        switch(dbgLevel) 
        {
            case INFO:
                priority = LOG_INFO;
                break;
            case WARN:
                priority = LOG_WARNING;
                break;
            case ERROR:
                priority = LOG_ERR;
                break;
            case SYSERR:
                {
                    extern int errno;
                    char reason[256];
                    strerror_r(errno,reason,256);
                    strcat(logstring,reason); 
                    errno = 0x0;
                    priority = LOG_ERR;
                }
                break;
            case PANIC:
                fprintf(stderr,"%s",logstring);
                syslog( LOG_EMERG,"%s",logstring );
                abort();
            case FATAL:
                fprintf(stderr,"%s",logstring);
                syslog( LOG_CRIT,"%s",logstring );
                exit(-1); 
        }

        extern int logoptions;
        if( logoptions & WRITE_TO_SYSLOG )
            syslog(priority,"%s",logstring );
        else if(logoptions & WRITE_TO_CONSOLE )
            fprintf(stderr,"%s",logstring);
    }

    return;
}

static inline void
_aprint(bool _eval, ... )
{
    va_list  ap1;
    char *fmt=NULL;
    va_start(ap1,_eval);
    fmt=va_arg(ap1,char*);
    vfprintf(stderr,fmt,ap1);
    vsyslog( LOG_EMERG , fmt , ap1 );
    va_end(ap1);
}

/*
   Debug macros for use.
   */
#ifdef __GNUC__

#define fatal(fmt,...) dprintf(modId,FATAL,__FILE__,__LINE__,fmt,##__VA_ARGS__)
#define panic(fmt,...) dprintf(modId,PANIC,__FILE__,__LINE__,fmt,##__VA_ARGS__)
#define error(fmt,...) dprintf(modId,ERROR,__FILE__,__LINE__,fmt,##__VA_ARGS__)
#define warn(fmt,...)  dprintf(modId,WARN ,__FILE__,__LINE__,fmt,##__VA_ARGS__)
#define info(fmt,...)  dprintf(modId,INFO ,__FILE__,__LINE__,fmt,##__VA_ARGS__)
#define syserr(fmt,...) dprintf(modId,SYSERR,__FILE__,__LINE__,fmt,##__VA_ARGS__)

/* 
   cudnt work out a one argument macro we have to give atleast 2 arguments
   for this to work. :-( can any body try it out ? 
   */
#define sm_assert(exp,...){                              \
    bool _eval = (exp);                                  \
    if(_eval == false )                                  \
    {                                                    \
        error("Assertion Failure:(%s) is false :",#exp); \
        _aprint(_eval,##__VA_ARGS__);                    \
        abort();                                         \
    }                                                    \
}

#endif  /* ifdef __GNUC__ */

#ifdef __cplusplus 
}
#endif 
#endif 
