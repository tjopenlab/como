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

#ifndef __COMO_CORE_CTHREADGROUP_H__
#define __COMO_CORE_CTHREADGROUP_H__

#include "como/core/ThreadGroup.h"
#include "_como_core_CThreadGroup.h"

namespace como {
namespace core {

Coclass(CThreadGroup)
    , public ThreadGroup
{
public:
    COMO_OBJECT_DECL();
};

} // namespace core
} // namespace como

#endif // __COMO_CORE_CTHREADGROUP_H__
