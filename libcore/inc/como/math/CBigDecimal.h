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

#ifndef __COMO_MATH_CBIGDECIMAL_H__
#define __COMO_MATH_CBIGDECIMAL_H__

#include "como/math/BigDecimal.h"
#include "_como_math_CBigDecimal.h"

namespace como {
namespace math {

Coclass(CBigDecimal)
    , public BigDecimal
{
public:
    COMO_OBJECT_DECL();

    static ECode New(
        /* [in] */ Long smallValue,
        /* [in] */ Integer scale,
        /* [in] */ const InterfaceID& iid,
        /* [out] */ IInterface** object);

    static ECode New(
        /* [in] */ Integer smallValue,
        /* [in] */ Integer scale,
        /* [in] */ const InterfaceID& iid,
        /* [out] */ IInterface** object);

    using _CBigDecimal::New;
};

}
}

#endif // __COMO_MATH_CBIGDECIMAL_H__
