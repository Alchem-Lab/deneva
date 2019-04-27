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
#include "manager.h"
#include "thread.h"
#include "transport.h"
#include "txn.h"
#include "wl.h"
#include "query.h"
#include "math.h"
#include "helper.h"
#include "msg_queue.h"
#include "message.h"

void Thread::heartbeat() {
  /*
#if TIME_ENABLE
  uint64_t now_time = get_sys_clock();
#else
  uint64_t now_time = get_wall_clock();
#endif
  if (now_time - heartbeat_time >= g_prog_timer) {
    printf("Heartbeat %ld %f\n",_thd_id,simulation->seconds_from_start(now_time));
    heartbeat_time = now_time;
  }
  */

}

void Thread::send_init_done_to_all_nodes() {
		for(uint64_t i = 0; i < g_total_node_cnt; i++) {
			if(i != g_node_id) {
        printf("Send INIT_DONE to %ld\n",i);
        msg_queue.enqueue(get_thd_id(),Message::create_message(INIT_DONE),i);
			}
		}
}

#if USE_RDMA == 1
void Thread::init(uint64_t node_id, Workload * workload) {
  _thd_id = worker_id_;
  _node_id = node_id;
  _wl = workload;
  rdm.init(_thd_id);
}
#else
void Thread::init(uint64_t thd_id, uint64_t node_id, Workload * workload) {
	_thd_id = thd_id;
	_node_id = node_id;
	_wl = workload;
	rdm.init(_thd_id);
}
#endif

uint64_t Thread::get_thd_id() { return _thd_id; }
uint64_t Thread::get_node_id() { return _node_id; }

#if USE_RDMA == 1
void Thread::communication_graph_global_sync() {
  if (get_thd_id() == 0) {
    for (uint64_t node = 0; node < g_total_node_cnt; ++node) {
      if (node == get_node_id()) continue;

      if (ISCLIENTN(node)) {
        for (uint64_t tid = 0; tid < g_total_client_thread_cnt; ++tid) {
          while(true) {
              if(cm_->sync_comm_graph(node, tid)) break;
              usleep(200000);
          }
        }
      } else {
        for (uint64_t tid = 0; tid < g_total_thread_cnt; ++tid) {
          while(true) {
              if(cm_->sync_comm_graph(node, tid)) break;
              usleep(200000);
          }
        }          
      }
    }
  }
}

void Thread::tRDMAsetup() {
  // BindToCore(worker_id_); // really specified to platforms

  init_routines(server_routine);
  DEBUG("%ld:%ld init_routines done.\n", _node_id, _thd_id);
  init_rdma();
  DEBUG("%ld:%ld init_rdma done.\n", _node_id, _thd_id);
  init_communication_graph();
  DEBUG("%ld:%ld init_comm graph done.\n", _node_id, _thd_id);
  pthread_barrier_wait( &warmup_bar );  // this barrier is a must
  communication_graph_global_sync();
  DEBUG("%ld:%ld comm graph global sync done.\n", _node_id, _thd_id);
  pthread_barrier_wait( &warmup_bar );  // this barrier is a must 
  create_qps_without_link_connect();
  DEBUG("%ld:%ld create_qps done.\n", _node_id, _thd_id);
  pthread_barrier_wait( &warmup_bar );
  connect_all_links();
  DEBUG("%ld:%ld connect all links done.\n", _node_id, _thd_id);
  pthread_barrier_wait( &warmup_bar );
  create_rdma_connections();
  DEBUG("%ld:%ld create_rdma_connections done.\n", _node_id, _thd_id);

  this->thread_local_init();   // application specific init
  register_callbacks();
}
#endif

void Thread::tsetup() {
	DEBUG("%ld:%ld Starting Setup\n",_node_id, _thd_id);
	pthread_barrier_wait( &warmup_bar );
#if USE_RDMA == 1
  tRDMAsetup();
  DEBUG("%ld:%ld RDMA setup done. \n",_node_id, _thd_id);
  pthread_barrier_wait( &warmup_bar );
#endif
  setup();
	DEBUG("%ld:%ld All setup done. Running.\n",_node_id, _thd_id);
	pthread_barrier_wait( &warmup_bar );
#if TIME_ENABLE
  run_starttime = get_sys_clock();
#else
  run_starttime = get_wall_clock();
#endif
  simulation->set_starttime(run_starttime);
  prog_time = run_starttime;
  heartbeat_time = run_starttime;
	pthread_barrier_wait( &warmup_bar );
}

void Thread::progress_stats() {
		if(get_thd_id() == 0) {
#if TIME_ENABLE
      uint64_t now_time = get_sys_clock();
#else
      uint64_t now_time = get_wall_clock();
#endif
      if (now_time - prog_time >= g_prog_timer) {
        prog_time = now_time;
        SET_STATS(get_thd_id(), total_runtime, prog_time - simulation->run_starttime); 

        if(ISCLIENT) {
          stats.print_client(true);
        } else {
          stats.print(true);
        }
      }
		}

}

#if USE_RDMA == 1
void Thread::run() {
  LOG(2) << "Starting running Thread.";
  // waiting for master to start workers
  this->inited = true;
  start_routine();
}
#endif
