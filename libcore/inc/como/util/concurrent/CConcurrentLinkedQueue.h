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

#ifndef __COMO_UTIL_CONCURRENT_CCONCURRENTLINKEDQUEUE_H__
#define __COMO_UTIL_CONCURRENT_CCONCURRENTLINKEDQUEUE_H__

#include "como/util/concurrent/ConcurrentLinkedQueue.h"
#include "_como_util_concurrent_CConcurrentLinkedQueue.h"

namespace como {
namespace util {
namespace concurrent {

Coclass(CConcurrentLinkedQueue)
    , public ConcurrentLinkedQueue
{
public:
    COMO_OBJECT_DECL();
};

} // namespace concurrent
} // namespace util
} // namespace como

#endif // __COMO_UTIL_CONCURRENT_CCONCURRENTLINKEDQUEUE_H__
