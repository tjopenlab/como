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

#include "RPCTestUnit.h"
#include <comoapi.h>
#include <comosp.h>
#include <ServiceManager.h>
#include <cstdio>
#include <unistd.h>
#include "ComoConfig.h"

using como::test::rpc::CService;
using como::test::rpc::IService;
using como::test::rpc::IID_IService;
using jing::ServiceManager;

int main(int argv, char** argc)
{
    AutoPtr<IService> srv;

    std::string ret = ComoConfig::AddZeroMQEndpoint(std::string("tongji"),  std::string(""));
    if (std::string("tongji") != ret) {
        printf("Failed to AddZeroMQEndpoint\n");
    }

    ECode ec = CService::New(IID_IService, (IInterface**)&srv);
    if (FAILED(ec)) {
        printf("Failed, CService::New\n");
        return 1;
    }

    ServiceManager::GetInstance()->AddRemoteService(
                                                String("127.0.0.1:80801"),
                                                String("127.0.0.1:80801"),
                                                String("rpcserviceZMQ"), srv);

    printf("==== rpc serviceZMQ wait for calling ====\n");
    while (true) {
        sleep(5);
    }

    return 0;
}
