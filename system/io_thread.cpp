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

#include "global.h"
#include "helper.h"
#include "manager.h"
#include "thread.h"
#include "io_thread.h"
#include "query.h"
#include "ycsb_query.h"
#include "tpcc_query.h"
#include "mem_alloc.h"
#include "transport.h"
#include "math.h"
#include "msg_thread.h"
#include "msg_queue.h"
#include "message.h"
#include "client_txn.h"
#include "work_queue.h"

// default the rmda device port to use as 1.
int use_port_ = 1;
int QP_NUMS = 1
#define USE_UD_MSG 0

void InputThread::setup() {
#if USE_RDMA
  assert(tport_man.rdmaCtrl != NULL && tport_man.rdma_buffer != NULL);
  setup_rdma(tport_man.rdmaCtrl);
  create_qps(tport_man.rdmaCtrl);
#endif

  std::vector<Message*> * msgs;
  while(!simulation->is_setup_done()) {
    msgs = tport_man.recv_msg(get_thd_id());
    if(msgs == NULL)
      continue;
    while(!msgs->empty()) {
      Message * msg = msgs->front();
      if(msg->rtype == INIT_DONE) {
        printf("Received INIT_DONE from node %ld\n",msg->return_node_id);
        fflush(stdout);
        simulation->process_setup_msg();
      } else {
        assert(ISSERVER || ISREPLICA);
        //printf("Received Msg %d from node %ld\n",msg->rtype,msg->return_node_id);
#if CC_ALG == CALVIN
      if(msg->rtype == CALVIN_ACK ||(msg->rtype == CL_QRY && ISCLIENTN(msg->get_return_id()))) {
        work_queue.sequencer_enqueue(get_thd_id(),msg);
        msgs->erase(msgs->begin());
        continue;
      }
      if( msg->rtype == RDONE || msg->rtype == CL_QRY) {
        assert(ISSERVERN(msg->get_return_id()));
        work_queue.sched_enqueue(get_thd_id(),msg);
        msgs->erase(msgs->begin());
        continue;
      }
#endif
        work_queue.enqueue(get_thd_id(),msg,false);
      }
      msgs->erase(msgs->begin());
    }
    delete msgs;
  }
}

#if USE_RDMA
void InputThread::setup_rdma(rdmaio::RdmaCtrl* cm_) {
  cm_->thread_local_init();
  // get the device id and port id used on the nic.

  int dev_id = cm_->get_active_dev(use_port_);
  int port_idx = cm_->get_active_port(use_port_);
  ASSERT(port_idx > 0) << "worker " << _thd_id
                       << " get port idx " << port_idx;

  // open the specific RNIC handler, and register its memory
  cm_->open_device(dev_id);
  cm_->register_connect_mr(dev_id); // register memory on the specific device  
}

void InputThread::create_qps(rdmaio::RdmaCtrl* cm_) {
  LOG(1) << "using RDMA device: " << use_port_ << " to create qps @" << _thd_id;
  assert(use_port_ >= 0); // check if init_rdma has been called

  int dev_id = cm_->get_active_dev(use_port_);
  int port_idx = cm_->get_active_port(use_port_);

  for(uint i = 0; i < QP_NUMS; i++){
    cm_->link_connect_qps(_thd_id, dev_id, port_idx, i, IBV_QPT_RC);
  }
  // note, link_connect_qps correctly handles duplicates creations
#if USE_UD_MSG == 0 // use RC QP, thus create its QP
  cm_->link_connect_qps(_thd_id, dev_id, port_idx, 0, IBV_QPT_RC);
#endif // USE_UD_MSG  
}
#endif

RC InputThread::run() {
  tsetup();
  printf("Running InputThread %ld\n",_thd_id);

  if(ISCLIENT) {
    client_recv_loop();
  } else {
    server_recv_loop();
  }

  return FINISH;

}

RC InputThread::client_recv_loop() {
	int rsp_cnts[g_servers_per_client];
	memset(rsp_cnts, 0, g_servers_per_client * sizeof(int));

	run_starttime = get_sys_clock();
  uint64_t return_node_offset;
  uint64_t inf;

  std::vector<Message*> * msgs;

	while (!simulation->is_done()) {
    heartbeat();
    uint64_t starttime = get_sys_clock();
		msgs = tport_man.recv_msg(get_thd_id());
    INC_STATS(_thd_id,mtx[28], get_sys_clock() - starttime);
    starttime = get_sys_clock();
    //while((m_query = work_queue.get_next_query(get_thd_id())) != NULL) {
    //Message * msg = work_queue.dequeue();
    if(msgs == NULL)
      continue;
    while(!msgs->empty()) {
      Message * msg = msgs->front();
			assert(msg->rtype == CL_RSP);
      return_node_offset = msg->return_node_id - g_server_start_node;
      assert(return_node_offset < g_servers_per_client);
      rsp_cnts[return_node_offset]++;
      INC_STATS(get_thd_id(),txn_cnt,1);
      uint64_t timespan = get_sys_clock() - ((ClientResponseMessage*)msg)->client_startts; 
      INC_STATS(get_thd_id(),txn_run_time, timespan);
      if (warmup_done) {
        INC_STATS_ARR(get_thd_id(),client_client_latency, timespan);
      }
      //INC_STATS_ARR(get_thd_id(),all_lat,timespan);
      inf = client_man.dec_inflight(return_node_offset);
      DEBUG("Recv %ld from %ld, %ld -- %f\n",((ClientResponseMessage*)msg)->txn_id,msg->return_node_id,inf,float(timespan)/BILLION);
      assert(inf >=0);
      // delete message here
      msgs->erase(msgs->begin());
    }
    delete msgs;
    INC_STATS(_thd_id,mtx[29], get_sys_clock() - starttime);

	}

  printf("FINISH %ld:%ld\n",_node_id,_thd_id);
  fflush(stdout);
  return FINISH;
}

RC InputThread::server_recv_loop() {

	myrand rdm;
	rdm.init(get_thd_id());
	RC rc = RCOK;
	assert (rc == RCOK);
  uint64_t starttime;

  std::vector<Message*> * msgs;
	while (!simulation->is_done()) {
    heartbeat();
    starttime = get_sys_clock();

		msgs = tport_man.recv_msg(get_thd_id());

    INC_STATS(_thd_id,mtx[28], get_sys_clock() - starttime);
    starttime = get_sys_clock();

    if(msgs == NULL)
      continue;
    while(!msgs->empty()) {
      Message * msg = msgs->front();
      if(msg->rtype == INIT_DONE) {
        msgs->erase(msgs->begin());
        continue;
      }
#if CC_ALG == CALVIN
      if(msg->rtype == CALVIN_ACK ||(msg->rtype == CL_QRY && ISCLIENTN(msg->get_return_id()))) {
        work_queue.sequencer_enqueue(get_thd_id(),msg);
        msgs->erase(msgs->begin());
        continue;
      }
      if( msg->rtype == RDONE || msg->rtype == CL_QRY) {
        assert(ISSERVERN(msg->get_return_id()));
        work_queue.sched_enqueue(get_thd_id(),msg);
        msgs->erase(msgs->begin());
        continue;
      }
#endif
      work_queue.enqueue(get_thd_id(),msg,false);
      msgs->erase(msgs->begin());
    }
    delete msgs;
    INC_STATS(_thd_id,mtx[29], get_sys_clock() - starttime);

	}
  printf("FINISH %ld:%ld\n",_node_id,_thd_id);
  fflush(stdout);
  return FINISH;
}

void OutputThread::setup() {
#if USE_RDMA
  assert(tport_man.rdmaCtrl != NULL && tport_man.rdma_buffer != NULL);
  setup_rdma(tport_man.rdmaCtrl);
  create_qps(tport_man.rdmaCtrl);
  create_rdma_rc_connections();
#endif

  DEBUG_M("OutputThread::setup MessageThread alloc\n");
  messager = (MessageThread *) mem_allocator.alloc(sizeof(MessageThread));
  messager->init(_thd_id);
	while (!simulation->is_setup_done()) {
    messager->run();
  }
}

#if USE_RDMA
void OutputThread::setup_rdma(rdmaio::RdmaCtrl* cm_) {
  cm_->thread_local_init();

  // get the device id and port id used on the nic.

  int dev_id = cm_->get_active_dev(use_port_);
  int port_idx = cm_->get_active_port(use_port_);
  ASSERT(port_idx > 0) << "worker " << _thd_id
                       << " get port idx " << port_idx;

  // open the specific RNIC handler, and register its memory
  cm_->open_device(dev_id);
  cm_->register_connect_mr(dev_id); // register memory on the specific device  
}

void OutputThread::create_qps(rdmaio::RdmaCtrl* cm_) {
  LOG(1) << "using RDMA device: " << use_port_ << " to create qps @" << _thd_id;
  assert(use_port_ >= 0); // check if init_rdma has been called

  int dev_id = cm_->get_active_dev(use_port_);
  int port_idx = cm_->get_active_port(use_port_);

  for(uint i = 0; i < QP_NUMS; i++){
    cm_->link_connect_qps(_thd_id, dev_id, port_idx, i, IBV_QPT_RC);
  }
  // note, link_connect_qps correctly handles duplicates creations
#if USE_UD_MSG == 0 // use RC QP, thus create its QP
  cm_->link_connect_qps(_thd_id, dev_id, port_idx, 0, IBV_QPT_RC);
#endif // USE_UD_MSG  
}

void OutputThread::create_rdma_rc_connections(char *start_buffer, uint64_t total_ring_sz,uint64_t total_ring_padding) {
  ASSERT(msg_handler_ == NULL);
  msg_handler_ = new RingMessage(total_ring_sz,total_ring_padding,_thd_id,cm_,start_buffer,
                                 std::bind(&OutputThread::poll_comp_callback, this,
                                           std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
}

#endif

RC OutputThread::run() {

  tsetup();
  printf("Running OutputThread %ld\n",_thd_id);

	while (!simulation->is_done()) {
    heartbeat();
    messager->run();
  }

  printf("FINISH %ld:%ld\n",_node_id,_thd_id);
  fflush(stdout);
  return FINISH;
}


