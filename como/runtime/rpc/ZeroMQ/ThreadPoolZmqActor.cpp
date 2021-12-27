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

#include <assert.h>
#include <cerrno>
#include <csignal>
#include <pthread.h>
#include "util/comolog.h"
#include "ComoConfig.h"
#include "ThreadPoolZmqActor.h"

namespace como {

static struct timespec lastCheckConnExpireTime = {0,0};

//-------------------------------------------------------------------------
// TPZA : ThreadPoolZmqActor
TPZA_Executor::Worker::Worker(AutoPtr<CZMQChannel> channel, AutoPtr<IStub> stub)
    : mChannel(channel)
    , mStub(stub)
    , mWorkerStatus(WORKER_TASK_READY)
    , mCond(PTHREAD_COND_INITIALIZER)
{
    String serverName;
    channel->GetServerName(serverName);

    void *mSocket = CzmqFindSocket(serverName);
    if (nullptr == mSocket) {
        Logger::E("TPZA_Executor::Worker", "CzmqFindSocket: %s", serverName.string());
    }

    pthread_mutex_init(&mMutex, NULL);
    clock_gettime(CLOCK_REALTIME, &lastAccessTime);
}

ECode TPZA_Executor::Worker::Invoke()
{
    Integer eventCode;
    const char buf[4096];

    if (CzmqRecvBuf(eventCode, mSocket, buf, sizeof(buf), ZMQ_DONTWAIT) != 0) {
        clock_gettime(CLOCK_REALTIME, &lastAccessTime);
        switch (eventCode) {
            case ZmqFunCode::Method_Invoke:
                return mStub->Invoke(mInParcel, mOutParcel);

            case ZmqFunCode::GetComponentMetadata: {
                AutoPtr<IParcel> argParcel = new CDBusParcel();
                argParcel->SetData(reinterpret_cast<HANDLE>(data), size);
                CoclassID cid;
                argParcel->ReadCoclassID(cid);
                AutoPtr<IMetaComponent> mc;
                CoGetComponentMetadata(*cid.mCid, nullptr, mc);
                Array<Byte> metadata;
                ECode ec = mc->GetSerializedMetadata(metadata);
                ReleaseCoclassID(cid);
                numberOfBytes = zmq_send(mSocket, metadata.GetPayload(), metadata.GetLength(), 0);
                break;
            }

            default:
                Logger::E("TPZA_Executor::Worker::Invoke", "bad eventCode");
        }
    }
}

//-------------------------------------------------------------------------

AutoPtr<TPZA_Executor> TPZA_Executor::sInstance = nullptr;
Mutex TPZA_Executor::sInstanceLock;
AutoPtr<ThreadPoolZmqActor> TPZA_Executor::threadPool = nullptr;

AutoPtr<TPZA_Executor> TPZA_Executor::GetInstance()
{
    {
        Mutex::AutoLock lock(sInstanceLock);
        if (nullptr == sInstance) {
            sInstance = new TPZA_Executor();
            threadPool = new ThreadPoolZmqActor(ComoConfig::ThreadPoolZmqActor_MAX_THREAD_NUM);
        }
    }
    return sInstance;
}

int ThreadPoolExecutor::RunTask(
    /* [in] */ AutoPtr<TPZA_Executor::Worker> task)
{
    AutoPtr<Worker> w = new Worker(task, this);
    return threadPool->addTask(w);
}

int TPZA_Executor::CleanTask(int posWorkerList)
{
    return threadPool->cleanTask(posWorkerList);
}


void *ThreadPoolZmqActor::threadFunc(void *threadData)
{
    ECode ec;
    int i;

    while (true) {
        struct timespec currentTime;
        clock_gettime(CLOCK_REALTIME, &currentTime);

        // check for free time out connection
        if ((currentTime.tv_sec - lastCheckConnExpireTime.tv_sec) +
                    1000000000L * (lastCheckConnExpireTime.tv_nsec - currentTime.tv_nsec) >
                                    ComoConfig::DBUS_BUS_CHECK_EXPIRES_PERIOD) {
            clock_gettime(CLOCK_REALTIME, &lastCheckConnExpireTime);

            pthread_mutex_lock(&m_pthreadMutex);

            for (i = 0;  i < ((mWorkerList.size()) &&
                         (WORKER_TASK_READY != mWorkerList[i]->mWorkerStatus));  i++) {
                if ((currentTime.tv_sec - mWorkerList[i]->lastAccessTime.tv_sec) +
                            1000000000L * (currentTime.tv_nsec - mWorkerList[i]->lastAccessTime.tv_nsec) >
                                            ComoConfig::DBUS_BUS_SESSION_EXPIRES) {
                    delete mWorkerList[i];
                    mWorkerList[i]->mWorkerStatus = WORKER_IDLE;
                }
            }
            pthread_mutex_unlock(&m_pthreadMutex);
        }

        pthread_mutex_lock(&m_pthreadMutex);

        for (i = 0;  i < ((mWorkerList.size()) &&
                     (WORKER_TASK_READY != mWorkerList[i]->mWorkerStatus));  i++);

        while ((i >= mWorkerList.size()) && !shutdown) {
            pthread_mutex_unlock(&m_pthreadMutex);
            pthread_cond_wait(&m_pthreadCond, &m_pthreadMutex);
            pthread_mutex_lock(&m_pthreadMutex);

            for (i = 0;  i < ((mWorkerList.size()) &&
                         (WORKER_TASK_READY != mWorkerList[i]->mWorkerStatus));  i++);
        }

        if (shutdown) {
            pthread_mutex_unlock(&m_pthreadMutex);
            pthread_exit(nullptr);
            return nullptr;
        }

        AutoPtr<TPZA_Executor::Worker> w = mWorkerList[i];
        w->mWorkerStatus = WORKER_TASK_RUNNING;

        pthread_mutex_unlock(&m_pthreadMutex);

        w->ec = w->Invoke();
        w->mWorkerStatus = WORKER_TASK_FINISH;
        pthread_cond_signal(&(w->mCond));
    }

    return reinterpret_cast<void*>(ec);
}

//-------------------------------------------------------------------------
// ThreadPoolZmqActor
//

bool ThreadPoolZmqActor::shutdown = false;
std::vector<TPZA_Executor::Worker*> ThreadPoolZmqActor::mWorkerList;   // task list

pthread_mutex_t ThreadPoolZmqActor::m_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPoolZmqActor::m_pthreadCond = PTHREAD_COND_INITIALIZER;

ThreadPoolZmqActor::ThreadPoolZmqActor(int threadNum)
{
    mThreadNum = threadNum;
    create();
}

/*
 * return position in mWorkerList
 */
int ThreadPoolZmqActor::addTask(TPZA_Executor::Worker *task)
{
    int i;
    struct timespec currentTime;
    clock_gettime(CLOCK_REALTIME, &currentTime);

    pthread_mutex_lock(&m_pthreadMutex);

    for (i = 0;  i < mWorkerList.size();  i++) {
        if (nullptr == mWorkerList[i])
            break;

        // COMO Runtime could set mWorkerStatus as WORKER_IDLE
        if (WORKER_IDLE == mWorkerList[i]->mWorkerStatus)
            break;

        if (WORKER_TASK_RUNNING == mWorkerList[i]->mWorkerStatus)
            continue;

        if ((currentTime.tv_sec - mWorkerList[i]->lastAccessTime.tv_sec) +
                    1000000000L * (currentTime.tv_nsec - mWorkerList[i]->lastAccessTime.tv_nsec) >
                    ComoConfig::TPZA_TASK_EXPIRES) {
            break;
        }
    }
    if (i < mWorkerList.size()) {
        if (nullptr != mWorkerList[i])
            delete mWorkerList[i];
        mWorkerList[i] = task;
    }
    else {
        mWorkerList.push_back(task);
    }

    pthread_mutex_unlock(&m_pthreadMutex);
    pthread_cond_signal(&m_pthreadCond);

    return i;
}

int ThreadPoolZmqActor::cleanTask(int posWorkerList)
{
    if (posWorkerList < 0 || (posWorkerList >= mWorkerList.size()))
        return -1;

    pthread_mutex_lock(&m_pthreadMutex);
    mWorkerList[posWorkerList] = nullptr;
    pthread_mutex_unlock(&m_pthreadMutex);

    return posWorkerList;
}

/*
 * create the thread pool
 */
int ThreadPoolZmqActor::create(void)
{
    pthread_id = (pthread_t*)calloc(mThreadNum, sizeof(pthread_t));

    for (int i = 0;  i < mThreadNum;  i++) {
        pthread_attr_t threadAddr;
        pthread_attr_init(&threadAddr);
        pthread_attr_setdetachstate(&threadAddr, PTHREAD_CREATE_DETACHED);

        pthread_t thread;
        int ret = pthread_create(&pthread_id[i], nullptr, ThreadPoolZmqActor::threadFunc, nullptr);
        if (ret != 0) {
            return E_RUNTIME_EXCEPTION;
        }
    }
    return 0;
}

int ThreadPoolZmqActor::stopAll()
{
    if (shutdown) {
        return -1;
    }

    shutdown = true;
    pthread_cond_broadcast(&m_pthreadCond);

    for (int i = 0; i < mThreadNum; i++) {
        pthread_join(pthread_id[i], nullptr);
    }

    free(pthread_id);
    pthread_id = nullptr;

    pthread_mutex_destroy(&m_pthreadMutex);
    pthread_cond_destroy(&m_pthreadCond);

    return 0;
}

int ThreadPoolZmqActor::getTaskListSize()
{
    return mWorkerList.size();
}

} // namespace como
