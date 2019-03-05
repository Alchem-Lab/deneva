/*
   Copyright 2016 Massachusetts Institute of Technology

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef _THREAD_H_
#define _THREAD_H_

#include "global.h"
#if USE_RDMA == 1
#include "core/rworker.h"
#include "rdmaio.h"
#include "util/util.h"
using namespace nocc::oltp;
using namespace nocc::util;
#endif

class Workload;

#if USE_RDMA == 1
class Thread : public RWorker {
public:
    Thread(int worker_id, rdmaio::RdmaCtrl *cm, int seed = 0) : RWorker(worker_id, cm, seed) {}

    virtual ~Thread() {}
    void send_init_done_to_all_nodes();
    void progress_stats();
    void heartbeat();
    uint64_t _thd_id;
    uint64_t _node_id;
    Workload * _wl;
    myrand rdm;
    uint64_t run_starttime;
    int server_routine = g_coroutine_cnt;

    uint64_t    get_thd_id();
    uint64_t    get_node_id();
    void        tsetup();
    void        tRDMAsetup();
    void        init(uint64_t node_id, Workload * workload);

    // the following function must be in the form void* (*)(void*)
    // to run with pthread.
    // conversion is done within the function.
    virtual void run() = 0; // run -> call worker routine
    virtual void setup() = 0;
    virtual void worker_routine(yield_func_t &yield) = 0;
    virtual void register_callbacks() = 0;     /*register read-only callback*/
    virtual void exit_handler() {}
    virtual void change_ctx(int cor_id) {}
    virtual void thread_local_init() {};
    virtual int choose_rnic_port() {
        int total_devices = cm_->query_devinfo();
        assert(total_devices > 0);
        use_port_ = 0;
        return use_port_;
    }
    virtual void init_communication_graph() {
        cm_->comm_graph_ready[_thd_id] = true;
    }
    void communication_graph_global_sync();
    virtual void create_rdma_connections() {}
private:
  uint64_t prog_time;
  uint64_t heartbeat_time;
};

#else

class Thread {
public:
    virtual ~Thread() {}
    void send_init_done_to_all_nodes();
    void progress_stats();
    void heartbeat();
    uint64_t _thd_id;
    uint64_t _node_id;
    Workload * _wl;
    myrand rdm;
    uint64_t run_starttime;

    uint64_t    get_thd_id();
    uint64_t    get_node_id();
    void tsetup();

    void        init(uint64_t thd_id, uint64_t node_id, Workload * workload);
    // the following function must be in the form void* (*)(void*)
    // to run with pthread.
    // conversion is done within the function.
    virtual void run() = 0;
    virtual void setup() = 0;

private:
  uint64_t prog_time;
  uint64_t heartbeat_time;
};
#endif

#endif
