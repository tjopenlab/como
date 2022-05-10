//=========================================================================
// Copyright (C) 2022 The C++ Component Model(COMO) Open Source Project
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

#include <time.h>
#include "ini.h"
#include "comolog.h"
#include "reflHelpers.h"
#include "registry.h"
#include "CircleBuffer.h"
#include "RuntimeMonitor.h"

namespace como {

CircleBuffer<char> *logCircleBuf = nullptr;
CircleBuffer<char> *loggerOutputCircleBuf = nullptr;
std::deque<RTM_InvokeMethod*> RuntimeMonitor::rtmInvokeMethodServerQueue(
                                    RuntimeMonitor::RTM_INVOKEMETHOD_QUEUE_SIZE);
std::deque<RTM_InvokeMethod*> RuntimeMonitor::rtmInvokeMethodClientQueue(
                                    RuntimeMonitor::RTM_INVOKEMETHOD_QUEUE_SIZE);

RuntimeMonitor::RuntimeMonitor() {
    //
}

ECode RuntimeMonitor::StartRuntimeMonitor()
{
    logCircleBuf = new CircleBuffer<char>(RM_LOG_BUFFER_SIZE);
    if (nullptr != logCircleBuf) {
        ECode ec = ECode_CircleBuffer(logCircleBuf);
        if (FAILED(ec)) {
            return ec;
        }
    }
    else
        return E_OUT_OF_MEMORY_ERROR;

    loggerOutputCircleBuf = new CircleBuffer<char>(RM_LOG_BUFFER_SIZE);
    if (nullptr != loggerOutputCircleBuf) {
        ECode ec = ECode_CircleBuffer(loggerOutputCircleBuf);
        if (FAILED(ec)) {
            return ec;
        }
    }
    else
        return E_OUT_OF_MEMORY_ERROR;

    // Collect the logs generated during COMO running for monitoring by RuntimeMonitor
    Logger::SetLoggerWriteLog(RuntimeMonitor::WriteLog);

    return NOERROR;
}

static int handler(void* user, const char* section, const char* name, const char* value)
{
    if (/*strcmp(section, "") == 0 && */strcmp(name, "ExportObject") == 0) {
        WalkExportObject(RPCType::Remote, *(String*)user);
    }
    else if (/*strcmp(section, "") == 0 && */strcmp(name, "ImportObject") == 0) {
        WalkImportObject(RPCType::Remote, *(String*)user);
    }
    else if (/*strcmp(section, "") == 0 && */strcmp(name, "RtmLog") == 0) {
        String string;
        logCircleBuf->ReadString(string);
        *(String*)user += string;
    }
    else if (/*strcmp(section, "") == 0 && */strcmp(name, "ComoLogger") == 0) {
        String string;
        loggerOutputCircleBuf->ReadString(string);
        *(String*)user += string;
    }

    return 1;
}

/**
 * RTM_CommandType::COMMAND_BY_STRING:
 *      RtmLog:
 *          RuntimeMonitor_Log(), RuntimeMonitor_Log_()
 *      ComoLogger:
 *          Logger::E(), Logger::D(), Logger::V()
 *      ImportObject:
 *          registry.sLocalImportRegistry, registry.sRemoteImportRegistry
 * RTM_CommandType::COMMAND_InvokeMethod
 *      RuntimeMonitor::WriteRtmInvokeMethod()
 */
#ifdef RPC_OVER_ZeroMQ_SUPPORT
ECode RuntimeMonitor::RuntimeMonitorMsgProcessor(zmq_msg_t& msg, Array<Byte>& resBuffer)
{
    String string;
    RTM_Command *rtmCommand = (RTM_Command *)zmq_msg_data(&msg);
    switch (rtmCommand->command) {
        case (Short)RTM_CommandType::CMD_BY_STRING: {
            String string;

            // parse monitor commands
            if (ini_parse_string((const char*)rtmCommand->parcel, handler, &string) < 0) {
                return E_ILLEGAL_ARGUMENT_EXCEPTION;
            }

            Integer len = string.GetByteLength() + 1;
            resBuffer = Array<Byte>(len);
            if (! resBuffer.IsNull()) {
                // resBuffer.Copy works very slowly
                memcpy(resBuffer.GetPayload(), (const char*)string, len);
            }

            break;
        }
        case (Short)RTM_CommandType::CMD_Server_InvokeMethod: {
            if (! rtmInvokeMethodClientQueue.empty()) {
                RTM_InvokeMethod *rtmMethod = rtmInvokeMethodClientQueue.front();

                resBuffer = Array<Byte>(rtmMethod->length);
                if (! resBuffer.IsNull()) {
                    // resBuffer.Copy works very slowly
                    memcpy(resBuffer.GetPayload(), (const char*)string, rtmMethod->length);

                    rtmInvokeMethodClientQueue.pop_front();
                }
            }
            break;
        }
    }

    return NOERROR;
}
#endif

RuntimeMonitor_Log_::RuntimeMonitor_Log_(const char *functionName)
{
    String strClassInfo, strInterfaceInfo, methodSignature;

    reflHelpers::intfGetObjectInfo((IInterface*)this, functionName, strClassInfo,
                                              strInterfaceInfo, methodSignature);
    char buffer[256];
    Logger::Monitor(buffer, sizeof(buffer), "tag");
    String string = buffer;
    string += "coclass:" + strClassInfo + " interface:" + strInterfaceInfo +
              " method:" + functionName + " signature:" + methodSignature  + "\n";
    logCircleBuf->Write(string);
}

RuntimeMonitor_Log_::~RuntimeMonitor_Log_()
{}

ECode RuntimeMonitor::GetMethodFromRtmInvokeMethod(RTM_InvokeMethod *rtm_InvokeMethod,
                                       IInterface *intf, AutoPtr<IMetaMethod>& method)
{
    InterfaceID iid;
    intf->GetInterfaceID(intf, iid);

    AutoPtr<IMetaCoclass> klass;
    IObject::Probe(intf)->GetCoclass(klass);
    AutoPtr<IMetaInterface> imintf;
    klass->GetInterface(iid, imintf);
    imintf->GetMethod(rtm_InvokeMethod->methodIndexPlus4, method);

    return NOERROR;
}

int RuntimeMonitor::WriteLog(const char *log, size_t strLen)
{
    return loggerOutputCircleBuf->Write(log, strLen+1);
}

/**
 * in_out: 0, in; 1, out
 * whichQueue: 0, rtmInvokeMethodClientQueue; 1, rtmInvokeMethodServerQueue
 */
ECode RuntimeMonitor::WriteRtmInvokeMethod(Long serverObjectId, CoclassID& cid,
                                      InterfaceID iid, Integer in_out,
                                      Integer methodIndexPlus4, IParcel *parcel,
                                      Integer whichQueue)
{
    Long sizeParcel;
    parcel->GetDataSize(sizeParcel);

    Long len = sizeof(RTM_InvokeMethod) + sizeParcel;
    RTM_InvokeMethod *rtm_InvokeMethod = (RTM_InvokeMethod*)malloc(len);
    if (nullptr == rtm_InvokeMethod)
        return E_OUT_OF_MEMORY_ERROR;

    clock_gettime(CLOCK_REALTIME, &(rtm_InvokeMethod->time));
    rtm_InvokeMethod->length = len;
    rtm_InvokeMethod->serverObjectId = serverObjectId;
    rtm_InvokeMethod->coclassID_mUuid = cid.mUuid;
    rtm_InvokeMethod->interfaceID_mUuid = iid.mUuid;
    rtm_InvokeMethod->in_out = in_out;
    rtm_InvokeMethod->methodIndexPlus4 = methodIndexPlus4;

    HANDLE parcelData;
    Byte *p = (Byte*)rtm_InvokeMethod + sizeof(RTM_InvokeMethod);
    parcel->GetData(parcelData);
    memcpy(p, (Byte*)parcelData, sizeParcel);

    if (0 == whichQueue) {
        if (rtmInvokeMethodClientQueue.size() >= RTM_INVOKEMETHOD_QUEUE_SIZE) {
            RTM_InvokeMethod *rtmMethod = rtmInvokeMethodClientQueue.front();
            free(rtmMethod);
            rtmInvokeMethodClientQueue.pop_front();
        }
        rtmInvokeMethodClientQueue.push_back(rtm_InvokeMethod);
    }
    else {
        if (rtmInvokeMethodServerQueue.size() >= RTM_INVOKEMETHOD_QUEUE_SIZE) {
            RTM_InvokeMethod *rtmMethod = rtmInvokeMethodServerQueue.front();
            free(rtmMethod);
            rtmInvokeMethodServerQueue.pop_front();
        }
        rtmInvokeMethodServerQueue.push_back(rtm_InvokeMethod);
    }

    return NOERROR;
}

RTM_Command* RuntimeMonitor::GenRtmCommand(RTM_CommandType command, Short param,
                                                               const char *cstr)
{
    Integer len = strlen(cstr) + 1;
    RTM_Command *rtmCommand = (RTM_Command*)malloc(sizeof(RTM_Command) + len);
    if (nullptr != rtmCommand) {
        rtmCommand->command = (Short)command;
        rtmCommand->param = param;
        memcpy(rtmCommand->parcel, cstr, len);
    }
    return rtmCommand;
}

} // namespace como
