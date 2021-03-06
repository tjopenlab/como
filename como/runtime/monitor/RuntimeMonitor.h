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

#ifndef __COMO_RUNTIMEMONITOR_H__
#define __COMO_RUNTIMEMONITOR_H__

#include <queue>
#include <circular_buffer.h>
#ifdef RPC_OVER_ZeroMQ_SUPPORT
#include "zmq.h"
#endif
#include "mutex.h"
#include "comodef.h"
#include "comotypes.h"

namespace como {

// align to Short type
enum RTM_CommandType {
    CMD_BY_STRING                      = 0x0001,

    CMD_Server_Activate_InvokeMethod   = 0x0101,
    CMD_Server_Deactivate_InvokeMethod = 0x0102,
    CMD_Server_InvokeMethod            = 0x0103,
    CMD_Server_Dump_InvokeMethod       = 0x0104,

    CMD_Client_Activate_InvokeMethod   = 0x0201,
    CMD_Client_Deactivate_InvokeMethod = 0x0202,
    CMD_Client_InvokeMethod            = 0x0203,
    CMD_Client_Dump_InvokeMethod       = 0x0204,

    CMD_Server_CpuMemoryStatus         = 0x0301,
};

// align to Short type
enum RTM_ParamTransDirection {
    CALL_METHOD                        = 0x0001,
    RETURN_FROM_METHOD                 = 0x0002,
};

enum RTM_WhichQueue {
    InvokeMethodClientQueue            = 0x1,
    InvokeMethodServerQueue            = 0x2,
};

#pragma pack(4)
typedef struct tagRTM_InvokeMethod {
    Long            length;             // total length of this struct
    Long            uuid64;
    Long            time;               // Unit: microseconds, from January 1, 1970.
    CoclassID       coclassID;
    UUID            interfaceID_mUuid;
    Long            serverObjectId;
    Integer         methodIndexPlus4;
    RTM_ParamTransDirection in_out:8;
    Byte            reserved[3];
    Byte            parcel[0];          // from here, Byte *parcel;
} RTM_InvokeMethod;

typedef struct tagRTM_Command {
    Long            length;             // total length of this struct
    RTM_CommandType command:16;
    Short           param;
    Byte            parcel[0];          // from here, Byte *;
} RTM_Command;

typedef struct tagRTM_CpuMemoryStatus {
    Float           cpuUsagePercent;    // CPU usage percentage
    Long            totalAllocdSpace;   // Total allocated space (uordblks)
    Long            totalFreeSpace;     // Total free space (fordblks)
} RTM_CpuMemoryStatus;

#pragma pack()

class COM_PUBLIC RuntimeMonitor
{
public:
    RuntimeMonitor();

    ECode StartRuntimeMonitor();

#ifdef RPC_OVER_ZeroMQ_SUPPORT
    static ECode RuntimeMonitorMsgProcessor(zmq_msg_t& msg, Array<Byte>& resBuffer);
#endif

    static constexpr int RM_LOG_BUFFER_SIZE = 4096;
    static constexpr int RTM_INVOKEMETHOD_QUEUE_SIZE = 512;

    static ECode GetMethodFromRtmInvokeMethod(RTM_InvokeMethod *rtm_InvokeMethod,
                                 IInterface *intf, AutoPtr<IMetaMethod>& method);

    static ECode WriteRtmInvokeMethod(Long uuid64,
                                      Long serverObjectId, CoclassID& clsId,
                                      InterfaceID iid, RTM_ParamTransDirection in_out,
                                      Integer methodIndexPlus4, IParcel *parcel,
                                      RTM_WhichQueue whichQueue);

    static RTM_InvokeMethod* DeserializeRtmInvokeMethod(
                                            RTM_InvokeMethod *rtm_InvokeMethod);

    static ECode DumpRtmInvokeMethod(RTM_InvokeMethod *rtm_InvokeMethod,
                                                             String& strBuffer);

    static int WriteLog(const char *log, size_t strLen);

    static RTM_Command* GenRtmCommand(RTM_CommandType command, Short param,
                                            const Byte *buffer, int bufferSize);

    static RTM_Command* GenRtmCommand(RTM_CommandType command, Short param,
                                                              const char *cstr);

    static ECode GenRtmCommand(RTM_CommandType command, Short param,
                                            const Byte *buffer, int bufferSize,
                                            Array<Byte>& arrayRtmCommand);

    static ECode GenRtmCommand(RTM_CommandType command, Short param,
                                const char *cstr, Array<Byte>& arrayRtmCommand);

    static CircularBuffer<RTM_InvokeMethod*> rtmInvokeMethodServerQueue;
    static CircularBuffer<RTM_InvokeMethod*> rtmInvokeMethodClientQueue;
    static Mutex rtmInvokeMethodServerQueue_Lock;
    static Mutex rtmInvokeMethodClientQueue_Lock;

};

#define RuntimeMonitor_Log()  \
           RuntimeMonitor_Log_ __runtimeMonitor_Log_((const char*)__FUNCTION__);

class COM_PUBLIC RuntimeMonitor_Log_
{
public:
    // define at the first line of a Function
    RuntimeMonitor_Log_(const char *functionName);

    ~RuntimeMonitor_Log_();
};

} // namespace como

#endif // __COMO_RUNTIMEMONITOR_H__
