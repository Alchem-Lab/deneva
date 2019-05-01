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

#ifndef _IOTHREAD_H_
#define _IOTHREAD_H_

#include "global.h"
#if USE_RDMA == 1
#include "rdmaio.h"
#include "msg_handler.h"
#include "transport.h"
#endif

class Workload;
class MessageThread;

#if USE_RDMA == 1
class InputThread : public Thread {
public:
  InputThread(int worker_id, rdmaio::RdmaCtrl *cm, int seed = 0) : Thread(worker_id, cm, seed) {}
	void run();
  void setup();
  void register_callbacks() {}
  void worker_routine(yield_func_t &yield) {}
  void init_communication_graph();

  RC  client_recv_loop();
  RC  server_recv_loop();
  void  check_for_init_done();

#if RAW_RDMA == 1
  void create_rdma_connections();
  void create_rdma_rc_raw_connections(char *start_buffer, uint64_t total_ring_sz,uint64_t total_ring_padding);
  void create_rdma_ud_raw_connections(int total_connections);
  bool poll_comp_callback(char *msg, int from_nid,int from_tid);
#endif
};

class OutputThread : public Thread {
public:
  OutputThread(int worker_id, rdmaio::RdmaCtrl *cm, int seed = 0) : Thread(worker_id, cm, seed) {}
	void run();
  void setup();
  void register_callbacks() {}
  void worker_routine(yield_func_t &yield) {}
  void init_communication_graph();
  
  MessageThread * messager;

#if RAW_RDMA == 1
  void create_rdma_connections();
  void create_rdma_rc_raw_connections(char *start_buffer, uint64_t total_ring_sz,uint64_t total_ring_padding);
  void create_rdma_ud_raw_connections(int total_connections);
  bool poll_comp_callback(char *msg, int from_nid,int from_tid);
#endif
};

#else

class InputThread : public Thread {
public:
  void run();
  void setup();
  RC  client_recv_loop();
  RC  server_recv_loop();
  void  check_for_init_done();
};

class OutputThread : public Thread {
public:
  void run();
  void setup();
  MessageThread * messager;
};
#endif

#endif
