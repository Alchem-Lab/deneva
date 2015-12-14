/*
   Copyright 2015 Rachael Harding

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
#include "ycsb.h"
#include "tpcc.h"
#include "thread.h"
#include "manager.h"
#include "math.h"
#include "mem_alloc.h"
#include "query.h"
#include "plock.h"
#include "occ.h"
#include "vll.h"
#include "transport.h"
#include "msg_queue.h"
#include "ycsb_query.h"
#include "sequencer.h"

void * f(void *);
void * g(void *);
void * worker(void *);
void * nn_worker(void *);
void * send_worker(void *);
void * abort_worker(void *);
void * calvin_lock_worker(void * id); 
void * calvin_seq_worker(void * id); 
void * calvin_worker(void * id); 
void network_test();
void network_test_recv();


Thread * m_thds;

// defined in parser.cpp
void parser(int argc, char * argv[]);

int main(int argc, char* argv[])
{
	// 0. initialize global data structure
	parser(argc, argv);
#if SEED != 0
  uint64_t seed = SEED + g_node_id;
#else
	uint64_t seed = get_sys_clock();
#endif
	srand(seed);
	printf("Random seed: %ld\n",seed);


	int64_t starttime;
	int64_t endtime;
  starttime = get_server_clock();
	// per-partition malloc
  printf("Initializing memory allocator... ");
  fflush(stdout);
	mem_allocator.init(g_part_cnt, MEM_SIZE / g_part_cnt); 
  printf("Done\n");
  printf("Initializing stats... ");
  fflush(stdout);
	stats.init();
  printf("Done\n");
  printf("Initializing global manager... ");
  fflush(stdout);
	glob_manager.init();
  printf("Done\n");
	Workload * m_wl;
	switch (WORKLOAD) {
		case YCSB :
			m_wl = new YCSBWorkload; break;
		case TPCC :
			m_wl = new TPCCWorkload; break;
		default:
			assert(false);
	}
	m_wl->init();
	printf("Workload initialized!\n");
  fflush(stdout);
#if NETWORK_TEST
	tport_man.init(g_node_id,m_wl);
	sleep(3);
	if(g_node_id == 0)
		network_test();
	else if(g_node_id == 1)
		network_test_recv();

	return 0;
#endif


  printf("Initializing remote query manager... ");
  fflush(stdout);
	rem_qry_man.init(g_node_id,m_wl);
  printf("Done\n");
  printf("Initializing transport manager... ");
  fflush(stdout);
	tport_man.init(g_node_id,m_wl);
  printf("Done\n");
  fflush(stdout);
  printf("Initializing work queue... ");
  work_queue.init(m_wl);
  printf("Done\n");
  printf("Initializing abort queue... ");
  abort_queue.init(m_wl);
  printf("Done\n");
  printf("Initializing message queue... ");
  msg_queue.init();
  printf("Done\n");
  printf("Initializing transaction pool... ");
  txn_pool.init(m_wl,g_inflight_max);
  printf("Done\n");
  printf("Initializing access pool... ");
  access_pool.init(m_wl,g_inflight_max * g_req_per_query);
  printf("Done\n");
  printf("Initializing txn node table pool... ");
  txn_table_pool.init(m_wl,g_inflight_max);
  printf("Done\n");
  printf("Initializing query pool... ");
  qry_pool.init(m_wl,g_inflight_max);
  printf("Done\n");
  printf("Initializing msg pool... ");
  msg_pool.init(m_wl,g_inflight_max);
  printf("Done\n");
  printf("Initializing transaction table... ");
  txn_table.init();
  printf("Done\n");
#if CC_ALG == CALVIN
  printf("Initializing sequencer... ");
  seq_man.init(m_wl);
  printf("Done\n");
#endif

	// 2. spawn multiple threads
	uint64_t thd_cnt = g_thread_cnt;
	uint64_t rthd_cnt = g_rem_thread_cnt;
	uint64_t sthd_cnt = g_send_thread_cnt;
  uint64_t all_thd_cnt = thd_cnt + rthd_cnt + sthd_cnt + 1;
#if CC_ALG == CALVIN
  assert(thd_cnt >= 3);
#endif
	
	pthread_t * p_thds = 
		(pthread_t *) malloc(sizeof(pthread_t) * (all_thd_cnt));
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t cpus;
	m_thds = new Thread[all_thd_cnt];
	// query_queue should be the last one to be initialized!!!
	// because it collects txn latency
	//if (WORKLOAD != TEST) {
	//	query_queue.init(m_wl);
	//}
	//pthread_barrier_init( &warmup_bar, NULL, g_thread_cnt );
#if CC_ALG == HSTORE || CC_ALG == HSTORE_SPEC
  printf("Initializing partition lock manager... ");
	part_lock_man.init(g_node_id);
  printf("Done\n");
#elif CC_ALG == OCC
  printf("Initializing occ lock manager... ");
	occ_man.init();
  printf("Done\n");
#elif CC_ALG == VLL
  printf("Initializing vll lock manager... ");
	vll_man.init();
  printf("Done\n");
#endif

  printf("Initializing threads... ");
  fflush(stdout);
	for (uint32_t i = 0; i < all_thd_cnt; i++) 
		m_thds[i].init(i, g_node_id, m_wl);
  printf("Done\n");
  fflush(stdout);

  endtime = get_server_clock();
  printf("Initialization Time = %ld\n", endtime - starttime);
  fflush(stdout);
	if (WARMUP > 0){
		printf("WARMUP start!\n");
		for (uint32_t i = 0; i < thd_cnt - 1; i++) {
			uint64_t vid = i;
			CPU_ZERO(&cpus);
      CPU_SET(i, &cpus);
      pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
			pthread_create(&p_thds[i], &attr, f, (void *)vid);
		}
		f((void *)(thd_cnt - 1));
		for (uint32_t i = 0; i < thd_cnt - 1; i++)
			pthread_join(p_thds[i], NULL);
		printf("WARMUP finished!\n");
	}
	warmup_finish = true;
	pthread_barrier_init( &warmup_bar, NULL, all_thd_cnt);

	uint64_t cpu_cnt = 0;
	// spawn and run txns again.
	starttime = get_server_clock();

  uint64_t i = 0;
#if CC_ALG == CALVIN
	for (i = 0; i < thd_cnt - 2; i++) {
#else
	for (i = 0; i < thd_cnt; i++) {
#endif
		uint64_t vid = i;
		CPU_ZERO(&cpus);
#if TPORT_TYPE_IPC
    CPU_SET((g_node_id * (g_thread_cnt) + cpu_cnt) % g_core_cnt, &cpus);
#elif !SET_AFFINITY
    CPU_SET(g_node_id * (g_thread_cnt) + cpu_cnt, &cpus);
#else
    CPU_SET(cpu_cnt, &cpus);
#endif
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
		cpu_cnt++;
#if CC_ALG == CALVIN
		pthread_create(&p_thds[i], &attr, calvin_worker, (void *)vid);
#else
		pthread_create(&p_thds[i], &attr, worker, (void *)vid);
#endif
  }

#if CC_ALG == CALVIN
  pthread_create(&p_thds[i], &attr, calvin_lock_worker, (void *)i);
  i++;
  pthread_create(&p_thds[i], &attr, calvin_seq_worker, (void *)i);
  i++;
#endif
  assert(i == thd_cnt);

	for (; i < thd_cnt + sthd_cnt; i++) {
		uint64_t vid = i;
		pthread_create(&p_thds[i], NULL, send_worker, (void *)vid);
		//pthread_create(&p_thds[i], &attr, send_worker, (void *)vid);
  }
	for (; i < thd_cnt + sthd_cnt + rthd_cnt ; i++) {
		uint64_t vid = i;
		pthread_create(&p_thds[i], NULL, nn_worker, (void *)vid);
		//pthread_create(&p_thds[i], &attr, nn_worker, (void *)vid);
  }

  abort_worker((void *)(i));

	for (i = 0; i < all_thd_cnt - 1; i++) 
		pthread_join(p_thds[i], NULL);

	endtime = get_server_clock();
	
  printf("PASS! SimTime = %f\n", (float)(endtime - starttime) / BILLION);
  if (STATS_ENABLE)
    stats.print(false);
  //malloc_stats_print(NULL, NULL, NULL);
  printf("\n");
  fflush(stdout);
  // Free things
	//tport_man.shutdown();

  /*
  txn_table.delete_all();
  txn_pool.free_all();
  access_pool.free_all();
  txn_table_pool.free_all();
  msg_pool.free_all();
  qry_pool.free_all();
  */
	return 0;
}

void * worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run();
	return NULL;
}

void * nn_worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run_remote();
	return NULL;
}

void * abort_worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run_abort();
	return NULL;
}


void * send_worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run_send();
	return NULL;
}

void * calvin_seq_worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run_calvin_seq();
	return NULL;
}

void * calvin_lock_worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run_calvin_lock();
	return NULL;
}

void * calvin_worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run_calvin();
	return NULL;
}

void network_test() {

	ts_t start;
	ts_t end;
	ts_t time;
	int bytes;
  float total = 0;
	//for(int i=4; i < 257; i+=4) {
	//	time = 0;
	//	for(int j=0;j < 1000; j++) {
	//		start = get_sys_clock();
	//		tport_man.simple_send_msg(i);
	//		while((bytes = tport_man.simple_recv_msg()) == 0) {}
	//		end = get_sys_clock();
	//		assert(bytes == i);
	//		time += end-start;
	//	}
	//	time = time/1000;
	//	printf("Network Bytes: %d, s: %f\n",i,time/BILLION);
	//}
	for (int i = 0; i < 4; ++i) {
		time = 0;
		int num_bytes = (int) pow(10,i);
		printf("Network Bytes: %d\nns: ", num_bytes);
		for(int j = 0;j < 1000; j++) {
			start = get_sys_clock();
			tport_man.simple_send_msg(num_bytes);
			while((bytes = tport_man.simple_recv_msg()) == 0) {}
			end = get_sys_clock();
			assert(bytes == num_bytes);
			time = end-start;
      total += time;
			//printf("%lu\n",time);
		}
		printf("Avg(s): %f\n",total/BILLION/1000);
    fflush(stdout);
		//time = time/1000;
		//printf("Network Bytes: %d, s: %f\n",i,time/BILLION);
		//printf("Network Bytes: %d, ns: %.3f\n",i,time);
		
	}

}

void network_test_recv() {
	int bytes;
	while(1) {
		if( (bytes = tport_man.simple_recv_msg()) > 0)
			tport_man.simple_send_msg(bytes);
	}
}