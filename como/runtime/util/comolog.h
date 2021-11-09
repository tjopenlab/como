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

#ifndef __COMO_LOGGER_H__
#define __COMO_LOGGER_H__

#include "comodef.h"

namespace como {

class COM_PUBLIC Logger
{
public:
    Logger();
    ~Logger();

    static void D(
        /* [in] */ const char* tag, ...);

    static void E(
        /* [in] */ const char* tag, ...);

    static void V(
        /* [in] */ const char* tag, ...);

    static void W(
        /* [in] */ const char* tag, ...);

    static void Log(
        /* [in] */ int level,
        /* [in] */ const char* tag, ...);

    static void SetLevel(
        /* [in] */ int level);

    static void SetSamplingTag(
        /* [in] */ const char *szSamplingTag_);

public:
    static constexpr int VERBOSE = 0;
    static constexpr int DEBUG = 1;
    static constexpr int WARNING = 2;
    static constexpr int ERROR = 3;

private:
    COM_LOCAL static int sLevel;
    COM_LOCAL static char szSamplingTag[32];
};

} // namespace como

/*
statement:
    #define Logger::D   do () while(0);
cause:
    warning: ISO C++11 requires whitespace after the macro name

so, we define Logger_D and call it to avoid this warning when Logger::D is empty
*/
#ifdef DISABLE_LOGGER
#define Logger_D(format, ...)
#define Logger_E(format, ...)
#define Logger_V(format, ...)
#define Logger_W(format, ...)
#define Logger_Log(format, ...)
#else
#define Logger_D Logger::D
#define Logger_E Logger::E
#define Logger_V Logger::V
#define Logger_W Logger::W
#define Logger_Log Logger::Log
#endif

#endif // __COMO_LOGGER_H__
