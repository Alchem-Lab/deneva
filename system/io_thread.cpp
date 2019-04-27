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

#if USE_RDMA == 1
#include "ring_imm_msg_v2.h"
#include "ud_msg_v2.h"
#endif

void InputThread::setup() {
#if USE_RDMA == 1
  assert(tport_man.rdmaCtrl != NULL && tport_man.rdma_buffer != NULL && msg_handler_ != NULL);
  Transport::msg_handler = msg_handler_;
#endif

  std::vector<Message*> * msgs;
  while(!simulation->is_setup_done()) {

#if USE_RDMA == 1
    msgs = tport_man.recv_msg_rdma(get_thd_id());
#else
    msgs = tport_man.recv_msg(get_thd_id());
#endif

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

void InputThread::run() {
  tsetup();
  printf("Running InputThread %ld\n",_thd_id);
  fflush(stdout);
  
  if(ISCLIENT) {
    client_recv_loop();
  } else {
    server_recv_loop();
  }

  return;
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

#if USE_RDMA == 1
    msgs = tport_man.recv_msg_rdma(get_thd_id());
#else
    msgs = tport_man.recv_msg(get_thd_id());
#endif

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
      DEBUG_TXN("Recv %ld from %ld, %ld -- %f\n",((ClientResponseMessage*)msg)->txn_id,msg->return_node_id,inf,float(timespan)/BILLION);
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

#if USE_RDMA == 1
    msgs = tport_man.recv_msg_rdma(get_thd_id());
#else
    msgs = tport_man.recv_msg(get_thd_id());
#endif

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
#if USE_RDMA == 1
  assert(tport_man.rdmaCtrl != NULL && tport_man.rdma_buffer != NULL && msg_handler_ != NULL);
  Transport::msg_handler = msg_handler_;
#endif

  DEBUG_M("OutputThread::setup MessageThread alloc\n");
  messager = (MessageThread *) mem_allocator.alloc(sizeof(MessageThread));
  messager->init(_thd_id);
	while (!simulation->is_setup_done() || !msg_queue.allQEmpty()) {
    messager->run();
  }
}

void OutputThread::run() {

  tsetup();
  printf("Running OutputThread %ld\n",_thd_id);
  fflush(stdout);

  while (!simulation->is_done()) {
    heartbeat();
    messager->run();
  }

  printf("FINISH %ld:%ld\n",_node_id,_thd_id);
  fflush(stdout);

  messager->fini();
  return;
}

#if USE_RDMA == 1

void InputThread::init_communication_graph() {
  unsigned my_id = _COMPACT_ENCODE_ID(_node_id, _thd_id);
  cm_->comm_graph[my_id].clear();
  for (uint i = 0; i < g_total_node_cnt; i++) {
    int output_thread_id = 2;
    if (i != _node_id) {
      uint the_other_id = _COMPACT_ENCODE_ID(i, output_thread_id);
      cm_->comm_graph[my_id].push_back(the_other_id);
      /**
         If that thread is located on another machine, 
         the reverse edge is added since there won't be 
         any communication for that thread to add its 
         adj list to the comm_graph on this machine.
      **/
      // cm_->comm_graph[the_other_id].push_back(my_id);
    }
  }
  cm_->comm_graph_ready[_thd_id] = true;
}

#if RAW_RDMA == 1
// override the create_rdma_connections() function only when in RAW_RDMA mode.
void InputThread::create_rdma_connections() {
#if USE_UD_MSG == 1
  int total_connections = 1;
  create_rdma_ud_raw_connections(total_connections);
#elif USE_RC_MSG == 1
  create_rdma_rc_raw_connections(tport_man.rdma_buffer + HUGE_PAGE_SZ,
                             tport_man.total_ring_sz,tport_man.ring_padding);
#endif
}

static uint32_t checksum(char* buf,unsigned len) {
	uint32_t res = 0;
	for (unsigned i = 0; i < len; i++)
		res ^= (uint32_t)buf[i];
	return res;
}

bool InputThread::poll_comp_callback(char *msg, int from_nid,int from_tid) {
  int32_t len = *((int32_t*)(msg + 4*sizeof(int32_t)));
  uint32_t check = checksum(msg + 4*sizeof(uint32_t), len-sizeof(uint32_t)*4);
  if (check != *((uint32_t*)(msg + 3*sizeof(int32_t)))) {
	  pthread_mutex_lock(&g_lock);
	  fprintf(stderr, "InputThread: received msg of length %u from %d:%d\n", len, from_nid, from_tid);
	  for (int i = 0; i < len; i++) {
	    fprintf(stderr, "0x%x ", (unsigned char)msg[i]);
	  }
	  fprintf(stderr, "Checksum Failed.\n");
	  pthread_mutex_unlock(&g_lock);
	  assert(false);
  }
  assert((uint64_t)(*((int32_t*)msg)) == g_node_id);
  assert((uint64_t)(*((int32_t*)(msg + sizeof(int32_t)))) != g_node_id);

  char* buf = ((char*)mem_allocator.alloc(len+sizeof(uint32_t)));
  assert(buf != NULL);
  *((uint32_t *)buf) = (uint32_t)len;
  memcpy(buf+sizeof(uint32_t), msg, len);
  assert(Transport::recv_buffers != NULL);
  tport_man.recv_buffers->push(buf);
  return true;
}

void InputThread::create_rdma_rc_raw_connections(char *start_buffer, uint64_t total_ring_sz,uint64_t total_ring_padding) {
  assert(msg_handler_ == NULL);
  using namespace rdmaio::ring_imm_msg_v2;
  msg_handler_ = new RingMessage(total_ring_sz,total_ring_padding,_thd_id,cm_,start_buffer, \
                                 std::bind(&InputThread::poll_comp_callback, this,       \
                                           std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
}

void InputThread::create_rdma_ud_raw_connections(int total_connections) {
  int dev_id = cm_->get_active_dev(use_port_);
  int port_idx = cm_->get_active_port(use_port_);

  assert(msg_handler_ == NULL); 
  using namespace rdmaio::udmsg_v2;
  msg_handler_ = new UDMsg(cm_, _thd_id, total_connections,
                           2048, // max concurrent msg received
                           std::bind(&InputThread::poll_comp_callback,this,
                                     std::placeholders::_1,std::placeholders::_2,std::placeholders::_3),
                           dev_id,port_idx,1);
}

#endif

void OutputThread::init_communication_graph() {
  uint my_id = _COMPACT_ENCODE_ID(_node_id, _thd_id);
  cm_->comm_graph[my_id].clear();
  for (uint i = 0; i < g_total_node_cnt; i++) {
    const int input_thread_id = 1;
    if (i != _node_id) {
      uint the_other_id = _COMPACT_ENCODE_ID(i, input_thread_id);
      cm_->comm_graph[my_id].push_back(the_other_id);
      // If that thread is located on another machine, the reverse edge is added since
      // there won't be any communication for that thread to add its adj list to the comm_graph on this machine.
      // cm_->comm_graph[the_other_id].push_back(my_id);
    }
  }

  cm_->comm_graph_ready[_thd_id] = true;
}

#if RAW_RDMA == 1
// override the create_rdma_connections() function only when in RAW_RDMA mode.
void OutputThread::create_rdma_connections() {
#if USE_UD_MSG == 1
  int total_connections = 1;
  create_rdma_ud_raw_connections(total_connections);
#elif USE_RC_MSG == 1
  create_rdma_rc_raw_connections(tport_man.rdma_buffer + HUGE_PAGE_SZ,
                             tport_man.total_ring_sz,tport_man.ring_padding);
#endif
}

// OutputThread will never receive messages.
bool OutputThread::poll_comp_callback(char *msg, int from_nid,int from_tid) {
  DEBUG_COMM("OutputThread: msg = %s from %d:%d", msg, from_nid, from_tid);
  assert(false);
  return true;
}

void OutputThread::create_rdma_rc_raw_connections(char *start_buffer, uint64_t total_ring_sz,uint64_t total_ring_padding) {
  assert(msg_handler_ == NULL);
  using namespace rdmaio::ring_imm_msg_v2;
  msg_handler_ = new RingMessage(total_ring_sz,total_ring_padding,_thd_id,cm_,start_buffer, \
                                 std::bind(&OutputThread::poll_comp_callback, this,       \
                                           std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
}

void OutputThread::create_rdma_ud_raw_connections(int total_connections) {
  int dev_id = cm_->get_active_dev(use_port_);
  int port_idx = cm_->get_active_port(use_port_);

  assert(msg_handler_ == NULL); 
  using namespace rdmaio::udmsg_v2;
  msg_handler_ = new UDMsg(cm_, _thd_id, total_connections,
                           2048, // max concurrent msg received
                           std::bind(&OutputThread::poll_comp_callback,this,
                                     std::placeholders::_1,std::placeholders::_2,std::placeholders::_3),
                           dev_id,port_idx,1);
}

#endif // RAW_RDMA
#endif // USE_RDMA

