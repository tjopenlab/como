//=========================================================================
// Copyright (C) 2018 The C++ Component Model(COMO) Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

#include "comolog.h"
#include <cstdarg>
#if defined(__android__)
#include <android/log.h>
#elif defined(__linux__)
#include <cstdio>
#endif
#include <time.h>
#include <string.h>
#include <sys/time.h>

namespace como {

int Logger::sLevel = DEBUG;

static void GetLocalTimeWithMs(char *currentTime, size_t maxChars);

void Logger::D(
    /* [in] */ const char* tag,
    /* [in] */ const char* format, ...)
{
    if (DEBUG < sLevel) {
        return;
    }

    va_list argList;

#if defined(__android__)
    va_start(argList, format);
    __android_log_vprint(ANDROID_LOG_DEBUG, tag, format, argList);
    va_end(argList);
#elif defined(__linux__)
    char currentTime[64];
    GetLocalTimeWithMs(currentTime, 64);

    printf("[%s DEBUG %s]: ", currentTime, tag);
    va_start(argList, format);
    vprintf(format, argList);
    va_end(argList);
    printf("\n");
#endif
}

void Logger::E(
    /* [in] */ const char* tag,
    /* [in] */ const char* format, ...)
{
    if (ERROR < sLevel) {
        return;
    }

    va_list argList;

#if defined(__android__)
    va_start(argList, format);
    __android_log_vprint(ANDROID_LOG_ERROR, tag, format, argList);
    va_end(argList);
#elif defined(__linux__)
    char currentTime[64];
    GetLocalTimeWithMs(currentTime, 64);

    printf("[%s ERROR %s]: ", currentTime, tag);
    va_start(argList, format);
    vprintf(format, argList);
    va_end(argList);
    printf("\n");
#endif
}

void Logger::V(
    /* [in] */ const char* tag,
    /* [in] */ const char* format, ...)
{
    if (VERBOSE < sLevel) {
        return;
    }

    va_list argList;

#if defined(__android__)
    va_start(argList, format);
    __android_log_vprint(ANDROID_LOG_VERBOSE, tag, format, argList);
    va_end(argList);
#elif defined(__linux__)
    char currentTime[64];
    GetLocalTimeWithMs(currentTime, 64);

    printf("[%s VERBOSE %s]: ", currentTime, tag);
    va_start(argList, format);
    vprintf(format, argList);
    va_end(argList);
    printf("\n");
#endif
}

void Logger::W(
    /* [in] */ const char* tag,
    /* [in] */ const char* format, ...)
{
    if (WARNING < sLevel) {
        return;
    }

    va_list argList;

#if defined(__android__)
    va_start(argList, format);
    __android_log_vprint(ANDROID_LOG_WARN, tag, format, argList);
    va_end(argList);
#elif defined(__linux__)
    char currentTime[64];
    GetLocalTimeWithMs(currentTime, 64);

    printf("[%s WARNING %s]: ", currentTime, tag);
    va_start(argList, format);
    vprintf(format, argList);
    va_end(argList);
    printf("\n");
#endif
}

#if defined(__android__)
static int ToAndroidLogPriority(
    /* [in] */ int level)
{
    switch(level) {
        case Logger::VERBOSE:
            return ANDROID_LOG_VERBOSE;
        case Logger::DEBUG:
            return ANDROID_LOG_DEBUG;
        case Logger::WARNING:
            return ANDROID_LOG_WARN;
        case Logger::ERROR:
            return ANDROID_LOG_ERROR;
        default:
            return ANDROID_LOG_DEFAULT;
    }
}
#endif

void Logger::Log(
    /* [in] */ int level,
    /* [in] */ const char* tag,
    /* [in] */ const char* format, ...)
{
    if (level < sLevel) {
        return;
    }

    va_list argList;

#if defined(__android__)
    va_start(argList, format);
    __android_log_vprint(ToAndroidLogPriority(level), tag, format, argList);
    va_end(argList);
#elif defined(__linux__)
    char currentTime[64];
    GetLocalTimeWithMs(currentTime, 64);

    printf("[%s LOG %s]: ", currentTime, tag);
    va_start(argList, format);
    vprintf(format, argList);
    va_end(argList);
    printf("\n");
#endif
}

void Logger::SetLevel(
    /* [in] */ int level)
{
    sLevel = level;
}

/*
 * get current time in this format: "2021-07-09 11:02:58.361 +0800"
 */
static void GetLocalTimeWithMs(char *currentTime, size_t maxChars)
{
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    int milli = curTime.tv_usec / 1000;

    // 2021-11-30 12:34:56
    char ymdhms[20];
    // The +hhmm or -hhmm numeric timezone
    char timezone[8];
    struct tm nowTime;
    localtime_r(&curTime.tv_sec, &nowTime);
    strftime(ymdhms, sizeof(ymdhms), "%Y-%m-%d %H:%M:%S", &nowTime);
    strftime(timezone, sizeof(timezone), "%z", &nowTime);

    snprintf(currentTime, maxChars, "%s.%03d %s", ymdhms, milli, timezone);
}

} // namespace como
