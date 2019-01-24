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
#if USE_RDMA
#include "rdmaio.h"
#include "msg_handler.h"
#endif

class Workload;
class MessageThread;

#ifdef USE_RDMA
class RDMAController : public Thread {
protected:
  RDMAController(rdmaio::RdmaCtrl* ctrl) {cm_ = ctrl;}
  /* used by InputThread */
  bool poll_comp_callback(char *msg,int nid,int tid);
  /* used by both InputThread and OutputThread */
  void setup_rdma();
  void create_qps();
  void create_rdma_rc_connections(char *start_buffer, uint64_t total_ring_sz,uint64_t total_ring_padding);
  void create_rdma_ud_connections(int total_connections);
protected:
  rdmaio::MsgHandler * msg_handler_ = NULL;
  rdmaio::RdmaCtrl* cm_ = NULL;
};

class InputThread : public RDMAController {
public:
  InputThread(rdmaio::RdmaCtrl* ctrl) : RDMAController(ctrl) {}
	RC 			run();
  RC  client_recv_loop();
  RC  server_recv_loop();
  void  check_for_init_done();
  void setup();
};

class OutputThread : public RDMAController {
public:
  OutputThread(rdmaio::RdmaCtrl* ctrl) : RDMAController(ctrl) {} 
	RC 			run();
  void setup();
  MessageThread * messager;
};

#else

class InputThread : public Thread {
public:
  RC      run();
  RC  client_recv_loop();
  RC  server_recv_loop();
  void  check_for_init_done();
  void setup();
};

class OutputThread : public Thread {
public:
  RC      run();
  void setup();
  MessageThread * messager;
};
#endif

#endif
