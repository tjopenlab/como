//=========================================================================
// Copyright (C) 2021 The C++ Component Model(COMO) Open Source Project
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

//
// when building environment has $ENV{COMO_FUNCTION_SAFETY}, it will define
// -DCOMO_FUNCTION_SAFETY=$ENV{COMO_FUNCTION_SAFETY}

#ifdef COMO_FUNCTION_SAFETY
#endif

#include <time.h>
#include <vector>
#include <algorithm>
#include <pthread.h>
#include "comoobj.h"
#include "comoapi.h"
#include "comolog.h"
#include "mistring.h"
#include "ComoFunctionSafetyObject.h"

using namespace std;

namespace como {

static pthread_mutex_t funSafetyLock;

class SoelfComoFunctionSafetyObject
{
public:
    SoelfComoFunctionSafetyObject();
    ~SoelfComoFunctionSafetyObject();
};
static SoelfComoFunctionSafetyObject soelfComoFunctionSafetyObject;

// define an instance of CFSO_VECTOR, Class Function Safety Object management VECTOR
// All the objects for overdue management are in this variable
static CFSO_VECTOR objsLifeCycleExpires;

//
// class SoelfComoFunctionSafetyObject
//
SoelfComoFunctionSafetyObject::SoelfComoFunctionSafetyObject()
{
    pthread_mutex_init(&funSafetyLock, nullptr);
}

SoelfComoFunctionSafetyObject::~SoelfComoFunctionSafetyObject()
{
    pthread_mutex_destroy(&funSafetyLock);
}

//
// class ComoFunctionSafetyObject
//
ComoFunctionSafetyObject::ComoFunctionSafetyObject()
    : mIsValid(0)
{
    AutoPtr<IMetaCoclass> klass;
    IObject::Probe(this)->GetCoclass(klass);
    String funcSafetySetting;
    klass->GetFuncSafetySetting(funcSafetySetting);

    if (funcSafetySetting.IsEmpty()) {
        mExpires = CFSO_ExpireVALID;
    }
    else {
        // parse funcSafetySetting
        char *sfunc = strdup(funcSafetySetting.string());
        if (nullptr == sfunc) {
            mExpires = CFSO_ExpireVALID;
            Logger::E("ComoFunctionSafetyObject", "strdup error");
        }
        else {
            int seedsCapacity;
            char **seeds = MiString::SeperateStr(sfunc, ';', nullptr, seedsCapacity);
            if (nullptr == seeds) {
                mExpires = CFSO_ExpireVALID;
                free(sfunc);
                Logger::E("ComoFunctionSafetyObject", "MiString::SeperateStr error");
            }
            else {
                int num = 2;
                char *words[2];
                MiString::WordBreak(seeds[0], num, words, (char*)"=");
                if (num < 2) {
                    mExpires = CFSO_ExpireVALID;
                }
                else {
                    mExpires = atol(seeds[1]);
                    free(sfunc);
                    free(seeds);
                }
            }
        }
    }

    clock_gettime(CLOCK_REALTIME, &mLastModifiedTime);
    pthread_mutex_lock(&funSafetyLock);
    if (objsLifeCycleExpires.cfso_push(this) < 0) {
        Logger::E("ComoFunctionSafetyObject", "Construct Object error");
    }
    pthread_mutex_unlock(&funSafetyLock);
}

ComoFunctionSafetyObject::~ComoFunctionSafetyObject()
{
    if (mExpires > 0) {
        pthread_mutex_lock(&funSafetyLock);
        int index = objsLifeCycleExpires.cfso_find(this);
        if (index >= 0) {
            objsLifeCycleExpires.cfso_del(index);
        }
        else {
            Logger::E("ComoFunctionSafetyObject", "lost object: %lx", this);
        }
        pthread_mutex_unlock(&funSafetyLock);
    }
}

Long ComoFunctionSafetyObject::GetExpires()
{
    return mExpires;
}

ECode ComoFunctionSafetyObject::SetExpires(
    /* [out] */ Long expires)
{
    mExpires = expires;

    objsLifeCycleExpires.cfso_push(this);

    return NOERROR;
}

ECode ComoFunctionSafetyObject::SetLastModifiedInfo()
{
    clock_gettime(CLOCK_REALTIME, &mLastModifiedTime);

    return NOERROR;
}

/*
 * default function for checking ComoFunctionSafetyObject
 */
ECode ComoFunctionSafetyObject::IsValid(
        /* [out] */ Integer& isValid)
{
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);

    if ((mLastModifiedTime.tv_sec - time.tv_sec) +
                1.0e9*(mLastModifiedTime.tv_nsec - time.tv_nsec) > mExpires) {
        isValid = CFSO_ExpireTime;
        return NOERROR;
    }

    isValid = mIsValid;
    return NOERROR;
}

/*
 * default function for setting property isValid of ComoFunctionSafetyObject
 * if parameter isValid is 0, means set Object valid
 */
ECode ComoFunctionSafetyObject::InvalidObject(
        /* [out] */ Integer isValid)
{
    mIsValid = isValid;
    return NOERROR;
}


//
// class CFSO_VECTOR, Class Function Safety Object management VECTOR
//-------------------------------------------------------------------------

CFSO_VECTOR::CFSO_VECTOR()
{
    _size = 0;
    _extra = 0;
    extra = 100;
    numNullArray = 0;
    cfso_allocate();
}

CFSO_VECTOR::~CFSO_VECTOR()
{
    free(_data);
}

unsigned int CFSO_VECTOR::cfso_allocate()
{
    ComoFunctionSafetyObject *newData = (ComoFunctionSafetyObject *)realloc(_data,
                                            sizeof(ComoFunctionSafetyObject*) * (_size + extra));

    if (nullptr == newData) {
        _extra = 0;
        Logger::D("CFSO_VECTOR::cfso_allocate", "realloc error");
    }
    else {
        _extra = extra;
        _data = newData;
    }
    return _extra;
}

int CFSO_VECTOR::cfso_push(ComoFunctionSafetyObject *cfso) {
    if (numNullArray > 0) {
        int index = idxNullArray[numNullArray--];
        *(ComoFunctionSafetyObject**)(reinterpret_cast<HANDLE>(_data) + index *
                                                    sizeof(ComoFunctionSafetyObject*)) = cfso;
        return index;
    }

    if (_extra < 1) {
        if (cfso_allocate() <= 0)
            return -1;
    }

    *(ComoFunctionSafetyObject**)(reinterpret_cast<HANDLE>(_data) + _size *
                                                    sizeof(ComoFunctionSafetyObject*)) = cfso;
    _size++;
    _extra--;

    return 0;
}

ComoFunctionSafetyObject *CFSO_VECTOR::cfso_get(unsigned int index) {
    if (_size < 1 || _size <= index)
        return nullptr;
    return *(ComoFunctionSafetyObject**)(reinterpret_cast<HANDLE>(_data) +
                                                        index * sizeof(ComoFunctionSafetyObject*));
}

int CFSO_VECTOR::cfso_find(ComoFunctionSafetyObject *cfso)
{
    for (unsigned int i = 0;  i < _size;  i++) {
        if (cfso == *(ComoFunctionSafetyObject**)(reinterpret_cast<HANDLE>(_data) + i *
                                                            sizeof(ComoFunctionSafetyObject*))) {
            return i;
        }
    }

    return -1;
}

int CFSO_VECTOR::cfso_del(unsigned int index) {
    if (_size < 1 || _size <= index)
        return -1;

    if (numNullArray >= CFSO_VECTOR_SizeNullArray) {
        for ( ;  numNullArray >= 0;  numNullArray--) {
            ComoFunctionSafetyObject* cfso;
            for ( ;  _size > 0;  _size--) {
                cfso = *(ComoFunctionSafetyObject**)(reinterpret_cast<HANDLE>(_data) + (_size-1) *
                                                                    sizeof(ComoFunctionSafetyObject*));
                if (cfso != nullptr)
                    break;
            }
            if (cfso != nullptr) {
                *(ComoFunctionSafetyObject**)(reinterpret_cast<HANDLE>(_data) + numNullArray *
                                                                sizeof(ComoFunctionSafetyObject*)) = cfso;
            }
            else {
                numNullArray = 0;
                break;
            }
        }
    }

    idxNullArray[numNullArray++] = index;
    *(ComoFunctionSafetyObject**)(reinterpret_cast<HANDLE>(_data) +
                                                index * sizeof(ComoFunctionSafetyObject*)) = nullptr;
    return 0;
}

} // namespace como
