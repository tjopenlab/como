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

#ifndef __COMO_TEXT_CATTRIBUTEDCHARACTERITERATORATTRIBUTE_H__
#define __COMO_TEXT_CATTRIBUTEDCHARACTERITERATORATTRIBUTE_H__

#include "como/text/AttributedCharacterIteratorAttribute.h"
#include "_como_text_CAttributedCharacterIteratorAttribute.h"

namespace como {
namespace text {

Coclass(CAttributedCharacterIteratorAttribute)
    , public AttributedCharacterIteratorAttribute
{
public:
    COMO_OBJECT_DECL();

    static ECode New(
        /* [in] */ const String& name,
        /* [in] */ const InterfaceID& iid,
        /* [out] */ IInterface** object);
};

}
}

#endif // __COMO_TEXT_CATTRIBUTEDCHARACTERITERATORATTRIBUTE_H__
