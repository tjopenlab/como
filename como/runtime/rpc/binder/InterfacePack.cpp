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

#include "rpc/binder/InterfacePack.h"
#include <binder/Parcel.h>
#include <stdio.h>

namespace como {

const InterfaceID IID_IBinderInterfacePack =
        {{0x91180119,0x7044,0x4e55,0xa67f,{0xdf,0x58,0x59,0x92,0xb1,0x4e}}, &CID_COMORuntime};

COMO_INTERFACE_IMPL_LIGHT_3(InterfacePack, LightRefBase, IInterfacePack, IBinderInterfacePack, IParcelable);

InterfacePack::~InterfacePack()
{
    ReleaseCoclassID(mCid);
    ReleaseInterfaceID(mIid);
}

ECode InterfacePack:: GetCoclassID(
    /* [out] */ CoclassID& cid)
{
    cid = mCid;
    return NOERROR;
}

ECode InterfacePack::GetInterfaceID(
    /* [out] */ InterfaceID& iid)
{
    iid = mIid;
    return NOERROR;
}

ECode InterfacePack::IsParcelable(
    /* [out] */ Boolean& parcelable)
{
    parcelable = mIsParcelable;
    return NOERROR;
}

ECode InterfacePack::SetServerName(
    /* [in] */ const String& serverName)
{
    mServerName = serverName;
    return NOERROR;
}

ECode InterfacePack::GetServerName(
    /* [out] */ String& serverName)
{
    serverName = mServerName;
    return NOERROR;
}

ECode InterfacePack::SetProxyInfo(
    /* [in] */ Long proxyId)
{
    mProxyId = proxyId;
    return NOERROR;
}

ECode InterfacePack::GetProxyInfo(
    /* [out] */ Long& proxyId)
{
    proxyId = mProxyId;
    return NOERROR;
}

ECode InterfacePack::GetHashCode(
    /* [out] */ Long& hash)
{
    hash = reinterpret_cast<uintptr_t>(mBinder.get());
    return NOERROR;
}

ECode InterfacePack::GetServerObjectId(
    /* [out] */ Long& serverObjectId)
{
    serverObjectId = reinterpret_cast<uintptr_t>(mBinder.get());
    return NOERROR;
}

ECode InterfacePack::ReadFromParcel(
    /* [in] */ IParcel* source)
{
    HANDLE data;
    source->GetPayload(data);
    mBinder = reinterpret_cast<android::Parcel*>(data)->readStrongBinder();
    FAIL_RETURN(source->ReadCoclassID(mCid));
    FAIL_RETURN(source->ReadInterfaceID(mIid));
    FAIL_RETURN(source->ReadBoolean(mIsParcelable));
    FAIL_RETURN(source->ReadLong(mServerObjectId));
    FAIL_RETURN(source->ReadString(mServerName));
    return NOERROR;
}

ECode InterfacePack::WriteToParcel(
    /* [in] */ IParcel* dest)
{
    HANDLE data;
    dest->GetPayload(data);
    reinterpret_cast<android::Parcel*>(data)->writeStrongBinder(mBinder);
    FAIL_RETURN(dest->WriteCoclassID(mCid));
    FAIL_RETURN(dest->WriteInterfaceID(mIid));
    FAIL_RETURN(dest->WriteBoolean(mIsParcelable));
    FAIL_RETURN(dest->WriteLong(mServerObjectId));
    FAIL_RETURN(dest->WriteString(mServerName));
    return NOERROR;
}

ECode InterfacePack::GiveMeAhand(
    /* [in] */ const String& aHand)
{
    return NOERROR;
}

android::sp<android::IBinder> InterfacePack::GetAndroidBinder()
{
    return mBinder;
}

void InterfacePack::SetAndroidBinder(
    /* [in] */ android::sp<android::IBinder>& binder)
{
    mBinder = binder;
}

void InterfacePack::SetCoclassID(
    /* [in] */ const CoclassID& cid)
{
    mCid = CloneCoclassID(cid);
}

void InterfacePack::SetInterfaceID(
    /* [in] */ const InterfaceID& iid)
{
    mIid = CloneInterfaceID(iid);
}

void InterfacePack::SetParcelable(
    /* [in] */ Boolean parcelable)
{
    mIsParcelable = parcelable;
}

void InterfacePack::SetServerObjectId(
    /* [in] */ Long serverObjectId)
{
    mServerObjectId = serverObjectId;
}

} // namespace como
