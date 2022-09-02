//
// Copyright (C) 2009-2018 Oleg Polivets. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#pragma once

#ifdef __linux__
#define PAUSE() system("bash -c \"read -rsp $'Press any key to continue...\n' -n 1 key\"");
#define PATH_SEPARATOR "/"
#else
#define PAUSE() system("pause");
#define PATH_SEPARATOR "\\"
#endif

//#define OPDEBUG_CONSOLE_LOG 1
#ifdef _DEBUG
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 1
#endif
#endif

#if (DEBUG_ENABLED != 1 && OPDEBUG_CONSOLE_LOG != 1)

#define LOG(m, ...)     ((void)0)
#define JUSTLOG(m, ...) ((void)0)
#define LOGINIT()       ((void)0)
#define LOGPARAMS(n,l,m)((void)0)
#define LOGUNINIT()     ((void)0)
#define LOG(m, ...)     ((void)0)
#define WARN(m, ...)    ((void)0)
#define HEXDUMP(m,b,s)  ((void)0)
#define LOGCTX

#else

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#include <string>
#include <map>

#ifndef WIN32
#include <unistd.h> // getcwd
#endif

#include "thread.hpp"

#if (OPDEBUG_CONSOLE_LOG==1)

#define LOG(m, ...)     printf(m"\n", ##__VA_ARGS__)
#define JUSTLOG(m, ...) printf(m"\n", ##__VA_ARGS__)
#define WARN(m, ...)    printf("WARNING: " m "\n", ##__VA_ARGS__)
#define HEXDUMP(m,b,s)  ((void)0)
#ifdef WIN32
#define LOGINIT()       (AllocConsole(), freopen("CONOUT$", "w", stdout))
#else
#define LOGINIT()       ((void)0)
#endif
#define LOGPARAMS(n,l,m)((void)0)
#define LOGUNINIT()     ((void)0)
#define LOGCTX

#else

#define LOG(m, ...)     op::Logger::log(0, m"\n", ##__VA_ARGS__)
#define WARN(m, ...)    op::Logger::log(1, "WARNING: " m"\n", ##__VA_ARGS__)
#define JUSTLOG(m, ...) op::Logger::log(255, m"\n", ##__VA_ARGS__)
#define HEXDUMP(m,b,s)  op::Logger::hexdump(m, (void*) b, s)
#define LOGINIT()       op::Logger::init(); atexit(OPLOG______clnp___)
#define LOGPARAMS(n,l,m)op::Logger::set_params(n, l, m)
#define LOGUNINIT()     op::Logger::uninit()
#define LOGCTX                        \
    char op::Logger::g_logPath[256];  \
    char op::Logger::g_path[256];     \
    char* op::Logger::g_buffer = 0;   \
    char* op::Logger::g_buffcurr = 0; \
    char* op::Logger::g_buffend = 0;  \
    int op::Logger::g_level = 0;      \
    op::mutex op::Logger::g_sunc;     \
    time_t op::Logger::gTsNext;       \
    unsigned op::Logger::g_maxLogs=0; \
    void OPLOG______clnp___(void) { LOG("atexit"); LOGUNINIT(); }

#define OPDEBUG_MAX_BUFFER 4096
#define OPDEBUG_APPEND_TIME 1
#define OPDEBUG_APPEND_TID  0

#ifndef OPDEBUG_INTO_DEBUGOUTPUT
#define OPDEBUG_INTO_DEBUGOUTPUT 0
#endif
#ifndef OPDEBUG_INTO_FILE
#define OPDEBUG_INTO_FILE 1
#endif
#ifndef OPDEBUG_INTO_DIR
#define OPDEBUG_INTO_DIR 0
#endif

namespace op {

class Logger {

private:
    template<class _Ty>
    struct Greater
        : public std::binary_function<_Ty, _Ty, bool>
    {    // functor for operator<
    bool operator()(const _Ty& _Left, const _Ty& _Right) const
        {    // apply operator> to operands
        return (_Left > _Right);
        }
    };
    
    template<typename Key, typename T>
    static void flipMap(const std::map< Key, T > & source,
                 std::multimap< T, Key, Greater<T> > & result) {
        typedef std::map< Key, T > source_type;
        typedef typename source_type::const_iterator source_iterator;
        for (source_iterator i = source.begin(), end = source.end(); i != end; ++i) {
            result.insert(std::make_pair((*i).second, (*i).first));
        }
    }

    class DateTimeMagic {
        static bool IsLeapYear(int year) {
            if (year %   4 != 0) return false;
            if (year % 400 == 0) return true;
            if (year % 100 == 0) return false;
            return true;
        }

        static int GetDaysInMonth(int year, int month) {
            int daysInMonths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            int days = daysInMonths[month];
            if (month == 1 && IsLeapYear(year)) // February of a leap year
                days += 1;
            return days;
        }

    public:
        static time_t FirstSecsOfNextDay(struct tm & d) {
            time_t now = time(0);
            d = *localtime(&now);

            d.tm_yday += 1;
            d.tm_mday += 1;
            d.tm_wday += 1;
            int days = GetDaysInMonth(d.tm_year, d.tm_mon);
            if (d.tm_mday >= days) {
                d.tm_mday = 0;
                d.tm_mon += 1;
                if (d.tm_mon > 11) {
                    d.tm_year += 1;
                }
            }

            d.tm_hour = 0;
            d.tm_sec  = 0;
            d.tm_min  = 0;

            time_t t = mktime(&d);
            d = *localtime(&now);
            return t;
        }

    };

public:
    static void init() {
        if (g_buffer == NULL) {
            g_buffer = new char[OPDEBUG_MAX_BUFFER];
            g_buffcurr = g_buffer;
            g_buffend  = g_buffer + OPDEBUG_MAX_BUFFER;
        }
        //set_params("debug", 0);
    }
    static void regen_name() {
        struct tm d;
        time_t t = DateTimeMagic::FirstSecsOfNextDay(d);
        sprintf(g_logPath, "%s%s%04d-%02d-%02d.log", g_path, PATH_SEPARATOR, d.tm_year+1900, d.tm_mon+1, d.tm_mday);
        gTsNext = t;
#if (1)
        if (g_maxLogs == 0)
            return;

        DIR *theFolder = ::opendir(g_path);
        if (theFolder == NULL)
            return;

        struct dirent *next_file;
        char filepath[260];
        std::multimap< time_t, std::string, Greater<time_t> > logs_sorted; {
        std::map< std::string, time_t > logs;
        while ((next_file = ::readdir(theFolder)) != NULL) {
            size_t len = ::strlen(next_file->d_name);
            const char * ext = &next_file->d_name[len-4];
            if (len <= 4 || ::strcmp(ext, ".log")) continue;
            // build the path for each file in the folder
            ::sprintf(filepath, "%s%s%s", g_path, PATH_SEPARATOR, next_file->d_name);
            struct stat info;
            if (::stat(filepath, &info) == 0) {
                logs[std::string(filepath)] =
#if !defined(WIN32)
                    info.st_mtim.tv_sec
#else
                    info.st_mtime
#endif
                ;
//              std::cout << next_file->d_name << " " << info.st_mtim.tv_sec << std::endl;
            }
        }
        ::closedir(theFolder);
        flipMap(logs, logs_sorted);
        }

        if (logs_sorted.size() <= g_maxLogs)
            return;

        unsigned cnt = 0;
		std::multimap<time_t, std::string, Greater<time_t> >::iterator i = logs_sorted.begin();
        for (; i != logs_sorted.end(); ++i, ++cnt) {
//          std::cout << (*i).second << " " << (*i).first << std::endl;
            if (cnt < g_maxLogs) continue;
            ::remove((*i).second.c_str());
        }
#endif
    }
    static void set_params(const char * mod_path, int level, unsigned maxLogs) {
        *g_path = '\0';
        g_level = level;
        g_maxLogs = maxLogs;
#if (OPDEBUG_INTO_DIR != 1)
        sprintf(g_logPath, "%s.log", mod_path);
#else
        // is relative path?
    #if defined(WIN32)
        if (mod_path[1] != ':') {
            ::GetCurrentDirectoryA(sizeof(g_path), g_path);
            strcat(g_path, PATH_SEPARATOR);
//            strcat(g_path, "LOGS-");
        }
    #else
        if (mod_path[0] != '/') {
            ::getcwd(g_path, sizeof(g_path));
            strcat(g_path, PATH_SEPARATOR);
//            strcat(g_path, "LOGS-");
        }
    #endif // win32
        strcat(g_path, mod_path);
        // create directory
    #if defined(WIN32)
        CreateDirectoryA(g_path, NULL);
    #else
        mkdir(g_path, 0777); // notice that 777 is different than 0777
    #endif // win32
        regen_name();
#endif // OPDEBUG_INTO_DIR
    }
    static void uninit() {
        if (g_buffer) {
            tracer_flush(0, true);
            delete[] g_buffer;
            g_buffer = NULL;
        }
    }
    static void log(int level, const char * fmt, ...) {
        va_list argptr;
        va_start(argptr, fmt);
        tracer_lev(level, fmt, argptr);
        va_end(argptr);
    }
    static void hexdump(const char * msg, void * buff, unsigned int size) {
        if (g_level > 0) return;
        char format[OPDEBUG_MAX_BUFFER];
        make_preambule(format, "%s\n", 0);

        g_sunc.lock();

        tracer_flush(sprintf(g_buffcurr, format, msg));
        char * ptr = g_buffcurr;
        unsigned char * p = (unsigned char*) buff;
        unsigned i = 0;
        for (; i < size; ++i) {
            if (i % 32 == 0 && i > 0) {
                *ptr = '\n';
                ++ptr;
                if (g_buffend - ptr <= 100) {
                    tracer_flush(ptr - g_buffcurr);
                    ptr = g_buffcurr;
                }
            }
            ptr += (sprintf(ptr, "%02x", p[i]));
        }
        if (*ptr != '\n') {
            *ptr = '\n';
            ++ptr;
            *ptr = '\0';
        }
        tracer_flush(ptr - g_buffcurr);
        g_sunc.unlock();
    }

private:
    static char g_logPath[256];
    static char g_path[256];
    static char* g_buffer;
    static char* g_buffcurr;
    static char* g_buffend;
    static op::mutex g_sunc;
    static int g_level;
    static time_t gTsNext;
    static unsigned g_maxLogs;
    
    static void make_preambule(char * buff, const char * fmt, int level) {
#if (OPDEBUG_APPEND_TIME == 1)
        {
    #ifdef WIN32
            SYSTEMTIME tm;
            GetSystemTime(&tm);
            sprintf(buff, "%02d:%02d:%02d.%03d ", tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);
    #else
            time_t timer;
            struct tm* tm_info;
            time(&timer);
            tm_info = localtime(&timer);
            strftime(buff, 25, "%H:%M:%S ", tm_info);
    #endif
        }
#endif
#if (OPDEBUG_APPEND_TID == 1)
        {
            sprintf(buff + strlen(buff), "[0x%x] ", op::thread::self_id());
        }
#endif
        strcat(buff, fmt);
    }
    
    static void tracer_lev(int level, const char *fmt, va_list argptr) {
        if (level < g_level) return;
        char format[OPDEBUG_MAX_BUFFER];
        make_preambule(format, fmt, level);
        g_sunc.lock();
        tracer_flush(vsprintf(g_buffcurr, format, argptr));
        g_sunc.unlock();
    }

    static void tracer_flush(unsigned bytes, bool force = false) {
        char * last = g_buffcurr;
#if (OPDEBUG_INTO_DEBUGOUTPUT == 1)
        g_buffcurr += bytes;
        if (bytes > 0) {
            *g_buffcurr = '\0';
        }
#else
        g_buffcurr += bytes;
#endif

#if (OPDEBUG_INTO_DEBUGOUTPUT == 1)
        {
    #ifdef WIN32
            OutputDebugStringA(last);
    #else
            printf(last);
    #endif
        }
#endif
        if ((g_buffend - g_buffcurr) > 1024 && !force) {
            return;
        }

#if (OPDEBUG_INTO_FILE == 1)
        {
    #if (OPDEBUG_INTO_DIR == 1)
            if (time(0) >= gTsNext) {
                regen_name();
            }
    #endif // #if (OPDEBUG_INTO_DIR == 1)
            FILE * fp = fopen(g_logPath, "a");
            if (fp) {
                fwrite(g_buffer, sizeof(char), (g_buffcurr - g_buffer), fp);
                fclose(fp);
            }
        }
#endif // #if (OPDEBUG_INTO_FILE == 1)

        g_buffcurr = g_buffer;
    }

}; // class Logger

} // namespace op 

#endif // else OPDEBUG_CONSOLE_LOG

#endif // else DEBUG_ENABLED
 
