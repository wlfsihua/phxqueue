/*
Tencent is pleased to support the open source community by making PhxQueue available.
Copyright (C) 2017 THL A29 Limited, a Tencent company. All rights reserved.
Licensed under the BSD 3-Clause License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

<https://opensource.org/licenses/BSD-3-Clause>

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/



/* store_client.cpp

 Generated by phxrpc_pb2client from store.proto

*/

#include "store_client.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>

#include "phxrpc_store_stub.h"

#include "phxqueue/comm.h"


using namespace std;


static phxrpc::ClientConfig global_storeclient_config_;
static phxrpc::ClientMonitorPtr global_storeclient_monitor_;


bool StoreClient::Init(const char *config_file) {
    return global_storeclient_config_.Read(config_file);
}

const char *StoreClient::GetPackageName() {
    const char *ret = global_storeclient_config_.GetPackageName();
    if (strlen(ret) == 0) {
        ret = "phxqueue_phxrpc.store";
    }
    return ret;
}

StoreClient::StoreClient() {
    static mutex monitor_mutex;
    if (!global_storeclient_monitor_.get()) {
        monitor_mutex.lock();
        if (!global_storeclient_monitor_.get()) {
            global_storeclient_monitor_ = phxrpc::MonitorFactory::GetFactory()
                ->CreateClientMonitor(GetPackageName());
        }
        global_storeclient_config_.SetClientMonitor(global_storeclient_monitor_);
        monitor_mutex.unlock();
    }
}

StoreClient::~StoreClient() {}

int StoreClient::PhxEcho(const google::protobuf::StringValue &req,
                         google::protobuf::StringValue *resp) {
    const phxrpc::Endpoint_t *ep = global_storeclient_config_.GetRandom();

    if (ep != nullptr) {
        phxrpc::BlockTcpStream socket;
        bool open_ret = phxrpc::PhxrpcTcpUtils::Open(&socket, ep->ip, ep->port,
                    global_storeclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                    *(global_storeclient_monitor_.get()));
        if (open_ret) {
            socket.SetTimeout(global_storeclient_config_.GetSocketTimeoutMS());

            StoreStub stub(socket, *(global_storeclient_monitor_.get()));
            return stub.PhxEcho(req, resp);
        }
    }

    return -1;
}

int StoreClient::PhxBatchEcho(const google::protobuf::StringValue &req,
                              google::protobuf::StringValue *resp) {
    int ret = -1;
    size_t echo_server_count = 2;
    uthread_begin;
    for (size_t i = 0; i < echo_server_count; i++) {
        uthread_t [=, &uthread_s, &ret](void *) {
            const phxrpc::Endpoint_t *ep = global_storeclient_config_.GetByIndex(i);
            if (ep != nullptr) {
                phxrpc::UThreadTcpStream socket;
                if (phxrpc::PhxrpcTcpUtils::Open(&uthread_s, &socket, ep->ip, ep->port,
                            global_storeclient_config_.GetConnectTimeoutMS(), *(global_storeclient_monitor_.get()))) {
                    socket.SetTimeout(global_storeclient_config_.GetSocketTimeoutMS());
                    StoreStub stub(socket, *(global_storeclient_monitor_.get()));
                    int this_ret = stub.PhxEcho(req, resp);
                    if (this_ret == 0) {
                        ret = this_ret;
                        uthread_s.Close();
                    }
                }
            }
        };
    }
    uthread_end;
    return ret;
}

int StoreClient::Add(const phxqueue::comm::proto::AddRequest &req,
                     phxqueue::comm::proto::AddResponse *resp) {
    const phxrpc::Endpoint_t *ep = global_storeclient_config_.GetRandom();

    if (ep != nullptr) {
        phxrpc::BlockTcpStream socket;
        bool open_ret = phxrpc::PhxrpcTcpUtils::Open(&socket, ep->ip, ep->port,
                    global_storeclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                    *(global_storeclient_monitor_.get()));
        if (open_ret) {
            socket.SetTimeout(global_storeclient_config_.GetSocketTimeoutMS());

            StoreStub stub(socket, *(global_storeclient_monitor_.get()));
            return stub.Add(req, resp);
        }
    }

    return -1;
}

int StoreClient::Get(const phxqueue::comm::proto::GetRequest &req,
                     phxqueue::comm::proto::GetResponse *resp) {
    const phxrpc::Endpoint_t *ep = global_storeclient_config_.GetRandom();

    if (ep != nullptr) {
        phxrpc::BlockTcpStream socket;
        bool open_ret = phxrpc::PhxrpcTcpUtils::Open(&socket, ep->ip, ep->port,
                    global_storeclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                    *(global_storeclient_monitor_.get()));
        if (open_ret) {
            socket.SetTimeout(global_storeclient_config_.GetSocketTimeoutMS());

            StoreStub stub(socket, *(global_storeclient_monitor_.get()));
            return stub.Get(req, resp);
        }
    }

    return -1;
}

phxqueue::comm::RetCode
StoreClient::ProtoAdd(const phxqueue::comm::proto::AddRequest &req,
                      phxqueue::comm::proto::AddResponse &resp) {
    const char *ip{req.master_addr().ip().c_str()};
    const int port{req.master_addr().port()};

    auto &&socket_pool = phxqueue::comm::ResourcePool<uint64_t, phxrpc::BlockTcpStream>::GetInstance();
    auto &&key = phxqueue::comm::utils::EncodeAddr(req.master_addr());
    auto socket = move(socket_pool->Get(key));

    if (nullptr == socket.get()) {
        socket.reset(new phxrpc::BlockTcpStream());

        bool open_ret{phxrpc::PhxrpcTcpUtils::Open(socket.get(), ip, port,
                                                   global_storeclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                                                   *(global_storeclient_monitor_.get()))};
        if (!open_ret) {
            QLErr("phxrpc Open err. ip %s port %d", ip, port);

            return phxqueue::comm::RetCode::RET_ERR_SYS;
        }
        socket->SetTimeout(global_storeclient_config_.GetSocketTimeoutMS());
    }

    StoreStub stub(*(socket.get()), *(global_storeclient_monitor_.get()));
    stub.SetKeepAlive(true);
    int ret{stub.Add(req, &resp)};
    if (0 > ret) {
        QLErr("Add err %d", ret);
    }
    if (-1 != ret && -202 != ret) {
        socket_pool->Put(key, socket);
    }
    return static_cast<phxqueue::comm::RetCode>(ret);
}

phxqueue::comm::RetCode
StoreClient::ProtoGet(const phxqueue::comm::proto::GetRequest &req,
                      phxqueue::comm::proto::GetResponse &resp) {
    const char *ip{req.master_addr().ip().c_str()};
    const int port{req.master_addr().port()};

    auto &&socket_pool = phxqueue::comm::ResourcePool<uint64_t, phxrpc::BlockTcpStream>::GetInstance();
    auto &&key = phxqueue::comm::utils::EncodeAddr(req.master_addr());
    auto socket = move(socket_pool->Get(key));

    if (nullptr == socket.get()) {
        socket.reset(new phxrpc::BlockTcpStream());

        bool open_ret{phxrpc::PhxrpcTcpUtils::Open(socket.get(), ip, port,
                                                   global_storeclient_config_.GetConnectTimeoutMS(), nullptr, 0,
                                                   *(global_storeclient_monitor_.get()))};
        if (!open_ret) {
            QLErr("phxrpc Open err. ip %s port %d", ip, port);

            return phxqueue::comm::RetCode::RET_ERR_SYS;
        }
        socket->SetTimeout(global_storeclient_config_.GetSocketTimeoutMS());
    }

    StoreStub stub(*(socket.get()), *(global_storeclient_monitor_.get()));
    stub.SetKeepAlive(true);
    int ret{stub.Get(req, &resp)};
    if (0 > ret) {
        QLErr("Get err %d", ret);
    }
    if (-1 != ret && -202 != ret) {
        socket_pool->Put(key, socket);
    }
    return static_cast<phxqueue::comm::RetCode>(ret);
}

