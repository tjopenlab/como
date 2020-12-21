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

#ifndef __COMO_UTIL_CALENDARFACTORY_H__
#define __COMO_UTIL_CALENDARFACTORY_H__

#include "como.util.ICalendar.h"
#include "como.util.ILocale.h"
#include "como.util.ITimeZone.h"

namespace como {
namespace util {

class COM_PUBLIC CalendarFactory
{
public:
    static ECode GetInstance(
        /* [out] */ ICalendar** cal);

    static ECode GetInstance(
        /* [in] */ ITimeZone* zone,
        /* [out] */ ICalendar** cal);

    static ECode GetInstance(
        /* [in] */ ILocale* aLocale,
        /* [out] */ ICalendar** cal);

    static ECode GetInstance(
        /* [in] */ ITimeZone* zone,
        /* [in] */ ILocale* aLocale,
        /* [out] */ ICalendar** cal);

    static ECode GetJapaneseImperialInstance(
        /* [in] */ ITimeZone* zone,
        /* [in] */ ILocale* aLocale,
        /* [out] */ ICalendar** cal);

private:
    CalendarFactory();
};

}
}

#endif // __COMO_UTIL_CALENDARFACTORY_H__
