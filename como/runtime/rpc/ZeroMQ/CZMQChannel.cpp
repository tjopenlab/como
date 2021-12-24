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

#include <vector>
#include <unistd.h>
#include <time.h>
#include "comorpc.h"
#include "CZMQChannel.h"
#include "CZMQParcel.h"
#include "InterfacePack.h"
#include "util/comolog.h"
#include "ComoConfig.h"

namespace como {

typedef struct tagDBusConnectionContainer {
    DBusConnection* conn;
    void* user_data;
    struct timespec lastAccessTime;
} DBusConnectionContainer;

static std::vector<DBusConnectionContainer*> conns;
static int num_DBUS_DISPATCHER = 0;
struct timespec lastCheckConnExpireTime = {0,0};

CZMQChannel::ServiceRunnable::ServiceRunnable(
    /* [in] */ CZMQChannel* owner,
    /* [in] */ IStub* target)
    : mOwner(owner)
    , mTarget(target)
    , mRequestToQuit(false)
{}

ECode CZMQChannel::ServiceRunnable::Run()
{
    DBusError err;

    dbus_error_init(&err);

    DBusConnection* conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        Logger::E("CZMQChannel", "Connect to bus daemon failed, error is \"%s\".", err.message);
        dbus_error_free(&err);
        return E_RUNTIME_EXCEPTION;
    }

    const char* name = dbus_bus_get_unique_name(conn);
    if (nullptr == name) {
        Logger::E("CZMQChannel", "Get unique name failed.");
        if (nullptr != conn) {
            dbus_connection_close(conn);
            dbus_connection_unref(conn);
        }
        dbus_error_free(&err);
        return E_RUNTIME_EXCEPTION;
    }

    mOwner->mName = name;

    DBusObjectPathVTable opVTable;

    opVTable.unregister_function = nullptr;
    opVTable.message_function = CZMQChannel::ServiceRunnable::HandleMessage;

    dbus_connection_register_object_path(conn,
                                         STUB_OBJECT_PATH, &opVTable, static_cast<void*>(this));

    {
        Mutex::AutoLock lock(mOwner->mLock);
        mOwner->mStarted = true;
    }
    mOwner->mCond.Signal();

    DBusConnectionContainer *conn_;
    Mutex connsLock;
    {
        Mutex::AutoLock lock(connsLock);
        conn_ = (DBusConnectionContainer*)malloc(sizeof(DBusConnectionContainer));
        if (nullptr == conn_) {
            Logger::E("CZMQChannel", "malloc failed.");
            if (nullptr != conn) {
                dbus_connection_close(conn);
                dbus_connection_unref(conn);
            }
            dbus_error_free(&err);
            return E_RUNTIME_EXCEPTION;
        }

        conn_->conn = conn;
        conn_->user_data = static_cast<void*>(this);
        clock_gettime(CLOCK_REALTIME, &conn_->lastAccessTime);
        conns.push_back(conn_);
    }

    if (num_DBUS_DISPATCHER < ComoConfig::ThreadPool_MAX_DBUS_DISPATCHER) {
        num_DBUS_DISPATCHER++;

        while (true) {
            DBusDispatchStatus status;
            struct timespec currentTime;
            DBusConnection *conn_dbus;

            clock_gettime(CLOCK_REALTIME, &currentTime);

            {
                Mutex::AutoLock lock(connsLock);

                // check for free time out connection
                if ((currentTime.tv_sec - lastCheckConnExpireTime.tv_sec) +
                            1000000000L * (lastCheckConnExpireTime.tv_nsec - currentTime.tv_nsec) >
                                            ComoConfig::DBUS_BUS_CHECK_EXPIRES_PERIOD) {
                    clock_gettime(CLOCK_REALTIME, &lastCheckConnExpireTime);

                    Mutex::AutoLock lock(connsLock);

                    for(std::vector<DBusConnectionContainer*>::iterator it = conns.begin();
                                                                    it != conns.end(); ) {
                        if ((currentTime.tv_sec - (*it)->lastAccessTime.tv_sec) +
                                    1000000000L * (currentTime.tv_nsec - (*it)->lastAccessTime.tv_nsec) >
                                                    ComoConfig::DBUS_BUS_SESSION_EXPIRES) {

                            IInterface* intf = reinterpret_cast<IInterface*>((*it)->user_data);
                            REFCOUNT_RELEASE(intf);

                            conn_dbus = (*it)->conn;
                            dbus_connection_close(conn_dbus);
                            dbus_connection_unref(conn_dbus);

                            free(*it);
                            conns.erase(it);
                        }
                        else {
                            it++;
                        }
                    }
                }
            }

            for (size_t i = 0;  i < conns.size();  i++) {
                {
                    Mutex::AutoLock lock(connsLock);
                    conn_dbus = conns[i]->conn;
                }

                do {
                    // dbus_connection_read_write_dispatch() return TRUE if the
                    // disconnect message has not been processed
                    if (! dbus_connection_read_write_dispatch(conn_dbus, 0)) {
                        // release the DBusConnection
                        conns[i]->lastAccessTime.tv_sec = 0;
                    }

                    status = dbus_connection_get_dispatch_status(conn_dbus);
                    if (DBUS_DISPATCH_DATA_REMAINS == status) {
                        clock_gettime(CLOCK_REALTIME, &(conns[i]->lastAccessTime));
                    }
                    else
                        break;
                } while (!mRequestToQuit);

                if (status == DBUS_DISPATCH_NEED_MEMORY) {
                    Logger::E("CZMQChannel", "DBus dispatching needs more memory.");
                    break;
                }
            }

            // usleep - suspend execution for microsecond intervals
            usleep(20);
        }
    }

    /* the conn should not be closed!
    dbus_connection_close(conn);
    dbus_connection_unref(conn);
    */

    return NOERROR;
}

DBusHandlerResult CZMQChannel::ServiceRunnable::HandleMessage(
    /* [in] */ DBusConnection* conn,
    /* [in] */ DBusMessage* msg,
    /* [in] */ void* arg)
{
    CZMQChannel::ServiceRunnable* thisObj = static_cast<CZMQChannel::ServiceRunnable*>(arg);

    if (dbus_message_is_method_call(msg, STUB_INTERFACE_PATH, "GetComponentMetadata")) {
        if (CZMQChannel::DEBUG) {
            Logger::D("CZMQChannel", "Handle \"GetComponentMetadata\" message.");
        }

        DBusMessageIter args;
        DBusMessageIter subArg;
        void* data = nullptr;
        Integer size = 0;

        if (!dbus_message_iter_init(msg, &args)) {
            Logger::E("CZMQChannel", "\"GetComponentMetadata\" message has no arguments.");
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&args)) {
            Logger::E("CZMQChannel", "\"GetComponentMetadata\" message has no array arguments.");
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        dbus_message_iter_recurse(&args, &subArg);
        dbus_message_iter_get_fixed_array(&subArg, &data, (int*)&size);

        AutoPtr<IParcel> argParcel = new CDBusParcel();
        argParcel->SetData(reinterpret_cast<HANDLE>(data), size);
        CoclassID cid;
        argParcel->ReadCoclassID(cid);
        AutoPtr<IMetaComponent> mc;
        CoGetComponentMetadata(*cid.mCid, nullptr, mc);
        Array<Byte> metadata;
        ECode ec = mc->GetSerializedMetadata(metadata);
        ReleaseCoclassID(cid);

        DBusMessage* reply = dbus_message_new_method_return(msg);

        dbus_message_iter_init_append(reply, &args);
        dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &ec);
        HANDLE resData = reinterpret_cast<HANDLE>(metadata.GetPayload());
        Long resSize = metadata.GetLength();
        dbus_message_iter_open_container(&args,
                DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &subArg);
        dbus_message_iter_append_fixed_array(&subArg,
                DBUS_TYPE_BYTE, &resData, resSize);
        dbus_message_iter_close_container(&args, &subArg);

        dbus_uint32_t serial = 0;
        if (!dbus_connection_send(conn, reply, &serial)) {
            Logger::E("CZMQChannel", "Send reply message failed.");
        }
        dbus_connection_flush(conn);

        dbus_message_unref(reply);
    }
    else if (dbus_message_is_method_call(msg,
            STUB_INTERFACE_PATH, "Invoke")) {
        if (CZMQChannel::DEBUG) {
            Logger::D("CZMQChannel", "Handle \"Invoke\" message.");
        }

        DBusMessageIter args;
        DBusMessageIter subArg;
        void* data = nullptr;
        Integer size = 0;

        if (!dbus_message_iter_init(msg, &args)) {
            Logger::E("CZMQChannel", "\"Invoke\" message has no arguments.");
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&args)) {
            Logger::E("CZMQChannel", "\"Invoke\" message has no array arguments.");
            return DBUS_HANDLER_RESULT_HANDLED;
        }
        dbus_message_iter_recurse(&args, &subArg);
        dbus_message_iter_get_fixed_array(&subArg, &data, (int*)&size);

        AutoPtr<IParcel> argParcel = new CDBusParcel();
        argParcel->SetData(reinterpret_cast<HANDLE>(data), size);
        AutoPtr<IParcel> resParcel = new CDBusParcel();
        ECode ec = thisObj->mTarget->Invoke(argParcel, resParcel);

        DBusMessage* reply = dbus_message_new_method_return(msg);

        dbus_message_iter_init_append(reply, &args);
        dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &ec);
        HANDLE resData;
        Long resSize;
        dbus_message_iter_open_container(&args,
                DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &subArg);
        resParcel->GetData(resData);
        resParcel->GetDataSize(resSize);
        dbus_message_iter_append_fixed_array(&subArg,
                DBUS_TYPE_BYTE, &resData, resSize);
        dbus_message_iter_close_container(&args, &subArg);

        dbus_uint32_t serial = 0;
        if (!dbus_connection_send(conn, reply, &serial)) {
            Logger::E("CZMQChannel", "Send reply message failed.");
        }
        dbus_connection_flush(conn);

        dbus_message_unref(reply);
    }
    else if (dbus_message_is_method_call(msg,
            STUB_INTERFACE_PATH, "IsPeerAlive")) {
        if (CZMQChannel::DEBUG) {
            Logger::D("CZMQChannel", "Handle \"IsPeerAlive\" message.");
        }
        DBusMessageIter args;

        DBusMessage* reply = dbus_message_new_method_return(msg);

        ECode ec = NOERROR;
        dbus_bool_t val = TRUE;

        dbus_message_iter_init_append(reply, &args);
        dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &ec);
        dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &val);

        dbus_uint32_t serial = 0;
        if (!dbus_connection_send(conn, reply, &serial)) {
            Logger::E("CZMQChannel", "Send reply message failed.");
        }
        dbus_connection_flush(conn);

        dbus_message_unref(reply);
    }
    else if (dbus_message_is_method_call(msg,
            STUB_INTERFACE_PATH, "Release")) {
        if (CZMQChannel::DEBUG) {
            Logger::D("CZMQChannel", "Handle \"Release\" message.");
        }

        thisObj->mTarget->Release();
        thisObj->mRequestToQuit = true;
    }
    else {
        const char* name = dbus_message_get_member(msg);
        if (name != nullptr && CZMQChannel::DEBUG) {
            Logger::D("CZMQChannel",
                    "The message which name is \"%s\" does not be handled.", name);
        }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

//-------------------------------------------------------------------------------

const CoclassID CID_CDBusChannel =
        {{0x8efc6167,0xe82e,0x4c7d,0x89aa,{0x66,0x8f,0x39,0x7b,0x23,0xcc}}, &CID_COMORuntime};

COMO_INTERFACE_IMPL_1(CZMQChannel, Object, IRPCChannel);

COMO_OBJECT_IMPL(CZMQChannel);

CZMQChannel::CZMQChannel(
    /* [in] */ RPCType type,
    /* [in] */ RPCPeer peer)
    : mType(type)
    , mPeer(peer)
    , mStarted(false)
    , mCond(mLock)
{}

ECode CZMQChannel::Apply(
    /* [in] */ IInterfacePack* ipack)
{
    mName = InterfacePack::From(ipack)->GetDBusName();
    return NOERROR;
}

ECode CZMQChannel::GetRPCType(
    /* [out] */ RPCType& type)
{
    type = mType;
    return NOERROR;
}

ECode CZMQChannel::GetServerAddress(
    /* [out] */ String& value)
{
    value = nullptr;    // the same machine, ServerAddress is nullptr
    return NOERROR;
}

ECode CZMQChannel::IsPeerAlive(
    /* [out] */ Boolean& alive)
{
    ECode ec = NOERROR;
    DBusError err;
    DBusConnection* conn = nullptr;
    DBusMessage* msg = nullptr;
    DBusMessage* reply = nullptr;
    DBusMessageIter args;
    alive = false;

    dbus_error_init(&err);

    conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);

    if (dbus_error_is_set(&err)) {
        Logger::E("CZMQChannel", "Connect to bus daemon failed, error is \"%s\".",
                err.message);
        ec = E_RUNTIME_EXCEPTION;
        goto Exit;
    }

    msg = dbus_message_new_method_call(
            mName, STUB_OBJECT_PATH, STUB_INTERFACE_PATH, "IsPeerAlive");
    if (msg == nullptr) {
        Logger::E("CZMQChannel", "Fail to create dbus message.");
        ec = E_RUNTIME_EXCEPTION;
        goto Exit;
    }

    if (DEBUG) {
        Logger::D("CZMQChannel", "Send message.");
    }

    reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    if (dbus_error_is_set(&err)) {
        Logger::E("CZMQChannel.IsPeerAlive()", "Fail to send message, error is \"%s\"", err.message);
        ec = E_REMOTE_EXCEPTION;
        goto Exit;
    }

    if (!dbus_message_iter_init(reply, &args)) {
        Logger::E("CZMQChannel", "Reply has no results.");
        ec = E_REMOTE_EXCEPTION;
        goto Exit;
    }

    if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) {
        Logger::E("CZMQChannel", "The first result is not Integer.");
        ec = E_REMOTE_EXCEPTION;
        goto Exit;
    }

    dbus_message_iter_get_basic(&args, &ec);

    if (SUCCEEDED(ec)) {
        if (!dbus_message_iter_next(&args)) {
            Logger::E("CZMQChannel", "Reply has no out arguments.");
            ec = E_REMOTE_EXCEPTION;
            goto Exit;
        }
        if (DBUS_TYPE_BOOLEAN != dbus_message_iter_get_arg_type(&args)) {
            Logger::E("CZMQChannel", "Reply arguments is not array.");
            ec = E_REMOTE_EXCEPTION;
            goto Exit;
        }

        dbus_bool_t val;
        dbus_message_iter_get_basic(&args, &val);
        alive = val == TRUE ? true : false;
    }
    else {
        if (DEBUG) {
            Logger::D("CZMQChannel", "Remote call failed with ec = 0x%x.", ec);
        }
    }

Exit:
    if (msg != nullptr) {
        dbus_message_unref(msg);
    }
    if (reply != nullptr) {
        dbus_message_unref(reply);
    }
    if (conn != nullptr) {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
    }

    dbus_error_free(&err);

    return ec;
}

ECode CZMQChannel::LinkToDeath(
    /* [in] */ IProxy* proxy,
    /* [in] */ IDeathRecipient* recipient,
    /* [in] */ HANDLE cookie,
    /* [in] */ Integer flags)
{
    return E_UNSUPPORTED_OPERATION_EXCEPTION;
}

ECode CZMQChannel::UnlinkToDeath(
    /* [in] */ IProxy* proxy,
    /* [in] */ IDeathRecipient* recipient,
    /* [in] */ HANDLE cookie,
    /* [in] */ Integer flags,
    /* [out] */ AutoPtr<IDeathRecipient>* outRecipient)
{
    return E_UNSUPPORTED_OPERATION_EXCEPTION;
}

ECode CZMQChannel::GetComponentMetadata(
    /* [in] */ const CoclassID& cid,
    /* [out, callee] */ Array<Byte>& metadata)
{
    ECode ec = NOERROR;
    DBusError err;
    DBusConnection* conn = nullptr;
    DBusMessage* msg = nullptr;
    DBusMessage* reply = nullptr;
    DBusMessageIter args, subArg;
    HANDLE data;
    Long size;

    AutoPtr<IParcel> parcel;
    CoCreateParcel(RPCType::Local, parcel);
    parcel->WriteCoclassID(cid);

    dbus_error_init(&err);

    conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);

    if (dbus_error_is_set(&err)) {
        Logger::E("CZMQChannel", "Connect to bus daemon failed, error is \"%s\".",
                err.message);
        ec = E_RUNTIME_EXCEPTION;
        goto Exit;
    }

    msg = dbus_message_new_method_call(
            mName, STUB_OBJECT_PATH, STUB_INTERFACE_PATH, "GetComponentMetadata");
    if (msg == nullptr) {
        Logger::E("CZMQChannel", "Fail to create dbus message.");
        ec = E_RUNTIME_EXCEPTION;
        goto Exit;
    }

    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_open_container(&args,
            DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE_AS_STRING, &subArg);
    parcel->GetData(data);
    parcel->GetDataSize(size);
    dbus_message_iter_append_fixed_array(&subArg,
            DBUS_TYPE_BYTE, &data, size);
    dbus_message_iter_close_container(&args, &subArg);

    if (DEBUG) {
        Logger::D("CZMQChannel", "Send message.");
    }

    reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    if (dbus_error_is_set(&err)) {
        Logger::E("CZMQChannel.GetComponentMetadata()", "Fail to send message, error is \"%s\"", err.message);
        ec = E_REMOTE_EXCEPTION;
        goto Exit;
    }

    if (!dbus_message_iter_init(reply, &args)) {
        Logger::E("CZMQChannel", "Reply has no results.");
        ec = E_REMOTE_EXCEPTION;
        goto Exit;
    }

    if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args)) {
        Logger::E("CZMQChannel", "The first result is not Integer.");
        ec = E_REMOTE_EXCEPTION;
        goto Exit;
    }

    dbus_message_iter_get_basic(&args, &ec);

    if (SUCCEEDED(ec)) {
        if (!dbus_message_iter_next(&args)) {
            Logger::E("CZMQChannel", "Reply has no out arguments.");
            ec = E_REMOTE_EXCEPTION;
            goto Exit;
        }
        if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&args)) {
            Logger::E("CZMQChannel", "Reply arguments is not array.");
            ec = E_REMOTE_EXCEPTION;
            goto Exit;
        }

        void* replyData = nullptr;
        Integer replySize;
        dbus_message_iter_recurse(&args, &subArg);
        dbus_message_iter_get_fixed_array(&subArg,
                &replyData, &replySize);

        metadata = Array<Byte>::Allocate(replySize);
        if (metadata.IsNull()) {
            Logger::E("CZMQChannel", "Malloc %d size metadata failed.", replySize);
            ec = E_OUT_OF_MEMORY_ERROR;
            goto Exit;
        }
        memcpy(metadata.GetPayload(), replyData, replySize);
    }
    else {
        if (DEBUG) {
            Logger::D("CZMQChannel", "Remote call failed with ec = 0x%x.", ec);
        }
    }

Exit:
    if (msg != nullptr) {
        dbus_message_unref(msg);
    }
    if (reply != nullptr) {
        dbus_message_unref(reply);
    }
    if (conn != nullptr) {
        dbus_connection_close(conn);
        dbus_connection_unref(conn);
    }

    dbus_error_free(&err);

    return ec;
}

ECode CZMQChannel::Invoke(
    /* [in] */ IMetaMethod* method,
    /* [in] */ IParcel* argParcel,
    /* [out] */ AutoPtr<IParcel>& resParcel)
{
    ECode ec = NOERROR;
    HANDLE data;
    Long size;



    argParcel->GetData(data);
    argParcel->GetDataSize(size);

    //TODO
    // send request through ZMQ
    reply = zmq_SendWithReplyAndBlock(conn, msg, -1, &err);

    if (SUCCEEDED(ec)) {
        resParcel = new CDBusParcel();

        Integer hasOutArgs;
        method->GetOutArgumentsNumber(hasOutArgs);
        if (hasOutArgs) {
            if (!dbus_message_iter_next(&args)) {
                Logger::E("CZMQChannel", "Reply has no out arguments.");
                ec = E_REMOTE_EXCEPTION;
                goto Exit;
            }
            if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&args)) {
                Logger::E("CZMQChannel", "Reply arguments is not array.");
                ec = E_REMOTE_EXCEPTION;
                goto Exit;
            }

            void* replyData = nullptr;
            Integer replySize;
            dbus_message_iter_recurse(&args, &subArg);
            dbus_message_iter_get_fixed_array(&subArg,
                    &replyData, &replySize);
            if (replyData != nullptr) {
                resParcel->SetData(reinterpret_cast<HANDLE>(replyData), replySize);
            }
        }
    }
    else {
        if (DEBUG) {
            Logger::D("CZMQChannel", "Remote call failed with ec = 0x%x.", ec);
        }
    }

Exit:

    return ec;
}

ECode CZMQChannel::StartListening(
    /* [in] */ IStub* stub)
{
    int ret = 0;

    if (mPeer == RPCPeer::Stub) {
        AutoPtr<ThreadPoolExecutor::Runnable> r = new ServiceRunnable(this, stub);
        ret = ThreadPoolExecutor::GetInstance()->RunTask(r);
    }

    if (0 == ret) {
        Mutex::AutoLock lock(mLock);
        while (!mStarted) {
            mCond.Wait();
        }
    }
    return NOERROR;
}

ECode CZMQChannel::Match(
    /* [in] */ IInterfacePack* ipack,
    /* [out] */ Boolean& matched)
{
    IDBusInterfacePack* idpack = IDBusInterfacePack::Probe(ipack);
    if (idpack != nullptr) {
        InterfacePack* pack = (InterfacePack*)idpack;
        if (pack->GetDBusName().Equals(mName)) {
            matched = true;
            return NOERROR;
        }
    }
    matched = false;
    return NOERROR;
}

} // namespace como