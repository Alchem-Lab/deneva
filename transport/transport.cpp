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
#include "transport.h"
#include "nn.hpp"
#include "tpcc_query.h"
#include "query.h"
#include "message.h"

#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>

#if USE_RDMA == 1
#include "ring_imm_msg.h"
#include "ralloc.h"
#include "util/util.h"
#endif

#define MAX_IFADDR_LEN 20 // max # of characters in name of address

#if USE_RDMA == 1
__thread rdmaio::MsgHandler* Transport::msg_handler = NULL;
__thread std::vector<Message*>* Transport::all_msgs = NULL;
#endif

std::string Transport::host_to_ip(const std::string &host) {
    std::string res = "";

    struct addrinfo hints, *infoptr;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // AF_INET means IPv4 only addresses

    int result = getaddrinfo(host.c_str(), NULL, &hints, &infoptr);
    if (result) {
      fprintf(stderr, "getaddrinfo: %s at %s\n", gai_strerror(result),host.c_str());
      return "";
    }
    char ip[64]; memset(ip,0,sizeof(ip));

    for(struct addrinfo *p = infoptr; p != NULL; p = p->ai_next) {
      getnameinfo(p->ai_addr, p->ai_addrlen, ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);
    }

    return string(ip);
}

void Transport::read_ifconfig(const char * ifaddr_file) {
	ifaddr = new char *[g_total_node_cnt];
  hosts = new char *[g_total_node_cnt];

	uint64_t cnt = 0;
  printf("Reading ifconfig file: %s\n",ifaddr_file);
	ifstream fin(ifaddr_file);
	string line;
  while (getline(fin, line)) {
    hosts[cnt] = new char[line.length()+1];
    strcpy(hosts[cnt], line.c_str());
    std::string ip = host_to_ip(line);
		ifaddr[cnt] = new char[ip.length()+1];
    strcpy(ifaddr[cnt],ip.c_str());
		printf("%ld: %s\n",cnt,ifaddr[cnt]);
		cnt++;
	}
  assert(cnt == g_total_node_cnt);
}

uint64_t Transport::get_socket_count() {
  uint64_t sock_cnt = 0;
  if(ISCLIENT)
    sock_cnt = (g_total_node_cnt)*2 + g_client_send_thread_cnt * g_servers_per_client;
  else
    sock_cnt = (g_total_node_cnt)*2 + g_client_send_thread_cnt;
  return sock_cnt;
}

string Transport::get_path() {
	string path;
#if SHMEM_ENV
  path = "/dev/shm/";
#else
	char * cpath;
  cpath = getenv("SCHEMA_PATH");
	if(cpath == NULL)
		path = "./";
	else
		path = string(cpath);
#endif
	path += "ifconfig.txt";
  return path;

}

Socket * Transport::get_socket() {
  //Socket * socket = new Socket;
  Socket * socket = (Socket*) mem_allocator.align_alloc(sizeof(Socket));
  new(socket) Socket();
	int timeo = 1000; // timeout in ms
	int stimeo = 1000; // timeout in ms
  int opt = 0;
  socket->sock.setsockopt(NN_SOL_SOCKET,NN_RCVTIMEO,&timeo,sizeof(timeo));
  socket->sock.setsockopt(NN_SOL_SOCKET,NN_SNDTIMEO,&stimeo,sizeof(stimeo));
  // NN_TCP_NODELAY doesn't cause TCP_NODELAY to be set -- nanomsg issue #118
  socket->sock.setsockopt(NN_SOL_SOCKET,NN_TCP_NODELAY,&opt,sizeof(opt));
  return socket;
}

uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id) {
  uint64_t port_id = TPORT_PORT;
  port_id += g_total_node_cnt * dest_node_id;
  port_id += src_node_id;
  DEBUG_COMM("Port ID:  %ld -> %ld : %ld\n",src_node_id,dest_node_id,port_id);
  return port_id;
}

#if NETWORK_DELAY_TEST || !ENVIRONMENT_EC2
uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id, uint64_t send_thread_id) {
  uint64_t port_id = 0;
  DEBUG_COMM("Calc port id %ld %ld %ld\n",src_node_id,dest_node_id,send_thread_id);
  port_id += g_total_node_cnt * dest_node_id;
  DEBUG_COMM("%ld\n",port_id);
  port_id += src_node_id;
  DEBUG_COMM("%ld\n",port_id);
//  uint64_t max_send_thread_cnt = g_send_thread_cnt > g_client_send_thread_cnt ? g_send_thread_cnt : g_client_send_thread_cnt;
//  port_id *= max_send_thread_cnt;
  port_id += send_thread_id * g_total_node_cnt * g_total_node_cnt;
  DEBUG_COMM("%ld\n",port_id);
  port_id += TPORT_PORT;
  DEBUG_COMM("%ld\n",port_id);
  DEBUG_COMM("Port ID:  %ld, %ld -> %ld : %ld\n",send_thread_id,src_node_id,dest_node_id,port_id);
  return port_id;
}
#else

uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id, uint64_t send_thread_id) {
  uint64_t port_id = 0;
  DEBUG_COMM("Calc port id %ld %ld %ld\n",src_node_id,dest_node_id,send_thread_id);
  port_id += dest_node_id + src_node_id;
  DEBUG_COMM("%ld\n",port_id);
  port_id += send_thread_id * g_total_node_cnt * 2;
  DEBUG_COMM("%ld\n",port_id);
  port_id += TPORT_PORT;
  DEBUG_COMM("%ld\n",port_id);
  DEBUG_COMM("Port ID:  %ld, %ld -> %ld : %ld\n",send_thread_id,src_node_id,dest_node_id,port_id);
  return port_id;
}
#endif



Socket * Transport::bind(uint64_t port_id) {
  Socket * socket = get_socket();
  char socket_name[MAX_TPORT_NAME];
#if TPORT_TYPE == IPC
  sprintf(socket_name,"ipc://node_%ld.ipc",port_id);
#else
#if ENVIRONMENT_EC2
  sprintf(socket_name,"tcp://eth0:%ld",port_id);
#else
  sprintf(socket_name,"tcp://%s:%ld",ifaddr[g_node_id],port_id);
#endif
#endif
  DEBUG_COMM("Sock Binding to %s %d\n",socket_name,g_node_id);
  int rc = socket->sock.bind(socket_name);
  if(rc < 0) {
    printf("Bind Error: %d %s\n",errno,strerror(errno));
    assert(false);
  }
  return socket;
}

Socket * Transport::connect(uint64_t dest_id,uint64_t port_id) {
  Socket * socket = get_socket();
  char socket_name[MAX_TPORT_NAME];
#if TPORT_TYPE == IPC
  sprintf(socket_name,"ipc://node_%ld.ipc",port_id);
#else
#if ENVIRONMENT_EC2
  sprintf(socket_name,"tcp://eth0;%s:%ld",ifaddr[dest_id],port_id);
#else
  sprintf(socket_name,"tcp://%s;%s:%ld",ifaddr[g_node_id],ifaddr[dest_id],port_id);
#endif
#endif
  DEBUG_COMM("Sock Connecting to %s %d -> %ld\n",socket_name,g_node_id,dest_id);
  int rc = socket->sock.connect(socket_name);
  if(rc < 0) {
    printf("Connect Error: %d %s\n",errno,strerror(errno));
    assert(false);
  }
  return socket;
}

void Transport::init() {
  _sock_cnt = get_socket_count();

  rr = 0;
	printf("Tport Init %d: %ld\n",g_node_id,_sock_cnt);

  string path = get_path();
	read_ifconfig(path.c_str());

  for(uint64_t node_id = 0; node_id < g_total_node_cnt; node_id++) {
    if(node_id == g_node_id)
      continue;
    // Listening ports
    if(ISCLIENTN(node_id)) {
      for(uint64_t client_thread_id = g_client_thread_cnt + g_client_rem_thread_cnt; client_thread_id < g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt; client_thread_id++) {
        uint64_t port_id = get_port_id(node_id,g_node_id,client_thread_id % g_client_send_thread_cnt);
        Socket * sock = bind(port_id);
        recv_sockets.push_back(sock);
        DEBUG_COMM("Socket insert: {%ld}: %ld\n",node_id,(uint64_t)sock);
      }
    } else {
      for(uint64_t server_thread_id = g_thread_cnt + g_rem_thread_cnt; server_thread_id < g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt; server_thread_id++) {
        uint64_t port_id = get_port_id(node_id,g_node_id,server_thread_id % g_send_thread_cnt);
        Socket * sock = bind(port_id);
        recv_sockets.push_back(sock);
        DEBUG_COMM("Socket insert: {%ld}: %ld\n",node_id,(uint64_t)sock);
      }
    }
    // Sending ports
    if(ISCLIENTN(g_node_id)) {
      for(uint64_t client_thread_id = g_client_thread_cnt + g_client_rem_thread_cnt; client_thread_id < g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt; client_thread_id++) {
        uint64_t port_id = get_port_id(g_node_id,node_id,client_thread_id % g_client_send_thread_cnt);
        std::pair<uint64_t,uint64_t> sender = std::make_pair(node_id,client_thread_id);
        Socket * sock = connect(node_id,port_id);
        send_sockets.insert(std::make_pair(sender,sock));
        DEBUG_COMM("Socket insert: {%ld,%ld}: %ld\n",node_id,client_thread_id,(uint64_t)sock);
      }
    } else {
      for(uint64_t server_thread_id = g_thread_cnt + g_rem_thread_cnt; server_thread_id < g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt; server_thread_id++) {
        uint64_t port_id = get_port_id(g_node_id,node_id,server_thread_id % g_send_thread_cnt);
        std::pair<uint64_t,uint64_t> sender = std::make_pair(node_id,server_thread_id);
        Socket * sock = connect(node_id,port_id);
        send_sockets.insert(std::make_pair(sender,sock));
        DEBUG_COMM("Socket insert: {%ld,%ld}: %ld\n",node_id,server_thread_id,(uint64_t)sock);
      }
    }
  }

	fflush(stdout);
#if USE_RDMA == 1
  printf("Tport RDMA Init %d.\n",g_node_id);
  initRDMA();
#endif 
}

void Transport::shutdown() {
#if USE_RDMA == 1
  shutdownRDMA();
#endif
}

#if USE_RDMA == 1
void Transport::initRDMA() {
  uint64_t M = 1024*1024;
  uint64_t r_buffer_size = M*RBUF_SIZE_M;

  std::vector<std::string> net_def_;
  for (unsigned node = 0; node < g_total_node_cnt; node++)
    net_def_.push_back(std::string(hosts[node]));

  int tcp_port = 8888;
  rdma_buffer = (char *)nocc::util::malloc_huge_pages(r_buffer_size, HUGE_PAGE_SZ, HUGE_PAGE);
  assert(rdma_buffer != NULL);

  // start creating RDMA
  rdmaCtrl = new rdmaio::RdmaCtrl(g_node_id, net_def_, tcp_port, false);
  rdmaCtrl->set_connect_mr(rdma_buffer, r_buffer_size); // register the buffer
  rdmaCtrl->query_devinfo();
  memset(rdma_buffer,0,r_buffer_size);

  uint64_t M2 = HUGE_PAGE_SZ;
  uint64_t total_sz = 0;

  // Calculating message size
  uint64_t ring_area_sz = 0;
#if USE_RC_MSG == 1
  using namespace rdmaio::ring_imm_msg;
  ring_padding = MAX_MSG_SIZE;
  total_ring_sz = g_coroutine_cnt * (2 * MAX_MSG_SIZE + 2 * 4096)  + ring_padding + MSG_META_SZ; // used for applications
  assert(total_ring_sz < r_buffer_size);

  fprintf(stderr, "[Mem] Ring size for each thread: %lu\n", total_ring_sz);

  // ringsz = total_ring_sz - ring_padding - MSG_META_SZ;

  ring_area_sz = total_ring_sz * net_def_.size() * (g_this_total_thread_cnt + 1);
  DEBUG("[Mem] Total msg buf area: %lfG.\n", nocc::util::get_memory_size_g(ring_area_sz));
#elif USE_UD_MSG == 1
#elif USE_TCP_MSG == 1
#else
  assert(false);
#endif
  total_sz += ring_area_sz + M2;
  assert(r_buffer_size > total_sz);

  // Init rmalloc
  free_buffer = rdma_buffer + total_sz; // use the free buffer as the local RDMA heap
  uint64_t real_alloced = RInit(free_buffer, r_buffer_size - total_sz);
  assert(real_alloced != 0);
  DEBUG("[Mem] RDMA heap size: %lfG.\n", nocc::util::get_memory_size_g(real_alloced));

  RThreadLocalInit();

  rdmaCtrl->start_server(); // listening server for receive QP connection requests
}

void Transport::shutdownRDMA() {
  rdmaCtrl->end_server();
}
#endif

// rename sid to send thread id
void Transport::send_msg(uint64_t send_thread_id, uint64_t dest_node_id, void * sbuf,int size) {
  uint64_t starttime = get_sys_clock();

  Socket * socket = send_sockets.find(std::make_pair(dest_node_id,send_thread_id))->second;
  // Copy messages to nanomsg buffer
	void * buf = nn_allocmsg(size,0);
	memcpy(buf,sbuf,size);
  DEBUG_COMM("%ld Sending batch of %d bytes to node %ld on socket %ld\n",send_thread_id,size,dest_node_id,(uint64_t)socket);

  int rc = -1;
  while(rc < 0 && (!simulation->is_setup_done() || (simulation->is_setup_done() && !simulation->is_done()))) {
    rc= socket->sock.send(&buf,NN_MSG,NN_DONTWAIT);
  }
  //nn_freemsg(sbuf);
  DEBUG_COMM("%ld Batch of %d bytes sent to node %ld\n",send_thread_id,size,dest_node_id);

  INC_STATS(send_thread_id,msg_send_time,get_sys_clock() - starttime);
  INC_STATS(send_thread_id,msg_send_cnt,1);
}

#if USE_RDMA == 1
// send msg through RDMA connection
void Transport::send_msg_rdma(uint64_t send_thread_id, uint64_t dest_node_id, void * sbuf,int size) {
  uint64_t starttime = get_sys_clock();
  assert(msg_handler != NULL);
  const int dest_thread_id = 1;
  msg_handler->send_to(dest_node_id, dest_thread_id, (char*)sbuf, size);
  DEBUG_COMM("%ld Batch of %d bytes sent to node %ld, thread %d\n",send_thread_id,size,dest_node_id, dest_thread_id);
  INC_STATS(send_thread_id,msg_send_time,get_sys_clock() - starttime);
  INC_STATS(send_thread_id,msg_send_cnt,1);
}

#if DEBUG_CHECKSUM
static uint32_t checksum(char* buf,unsigned len) {
	uint32_t res = 0;
	for (unsigned i = 0; i < len; i++)
		res ^= (uint32_t)buf[i];
	return res;
}
#endif

// send msg through RDMA connection
void Transport::send_msg_to_thread_rdma(uint64_t send_thread_id, uint64_t dest_node_id, uint64_t dest_thread_id, void * sbuf,int size) {
  uint64_t starttime = get_sys_clock();
#if DEBUG_ASSERT
  assert(msg_handler != NULL);
  assert(size == (*((int32_t*)((char*)sbuf + 4*sizeof(int32_t)))));
#endif

#if DEBUG_CHECKSUM
  uint32_t check = checksum((char*)sbuf + sizeof(uint32_t)*4, (unsigned)size-sizeof(uint32_t)*4);
  // pthread_mutex_lock(&g_lock);
  // fprintf(stderr, "%ld: Sending %d bytes to node %ld, thread %ld\n", send_thread_id,size,dest_node_id, dest_thread_id);
  // for (int i = 0; i < size; i++) {
  //  fprintf(stderr, "0x%x ", ((unsigned char*)sbuf)[i]);
  // }
  // fprintf(stderr, "checksum=%u\n", check);
  // pthread_mutex_unlock(&g_lock);

  assert(check == (*((uint32_t*)((char*)sbuf + 3*sizeof(int32_t)))));
#endif

#if DEBUG_ASSERT
  assert((uint64_t)(*((int32_t*)sbuf)) == dest_node_id);
  assert((uint64_t)(*((int32_t*)((char*)sbuf + sizeof(int32_t)))) == g_node_id);
#endif
  
  msg_handler->send_to(dest_node_id, dest_thread_id, (char*)sbuf, size);
  DEBUG_COMM("%ld Batch of %d bytes sent to node %ld, thread %ld\n",send_thread_id,size,dest_node_id, dest_thread_id);
  INC_STATS(send_thread_id,msg_send_time,get_sys_clock() - starttime);
  INC_STATS(send_thread_id,msg_send_cnt,1);
}
#endif

// Listens to sockets for messages from other nodes
std::vector<Message*> * Transport::recv_msg(uint64_t thd_id) {
	int bytes = 0;
	void * buf;
  uint64_t starttime = get_sys_clock();
  std::vector<Message*> * msgs = NULL;
  //uint64_t ctr = starttime % recv_sockets.size();
  uint64_t rand = (starttime % recv_sockets.size()) / g_this_rem_thread_cnt;
  //uint64_t ctr = ((thd_id % g_this_rem_thread_cnt) % recv_sockets.size()) + rand * g_this_rem_thread_cnt;
  uint64_t ctr = thd_id % g_this_rem_thread_cnt;
  if(ctr >= recv_sockets.size())
    return msgs;
  if(g_this_rem_thread_cnt < g_total_node_cnt) {
    ctr += rand * g_this_rem_thread_cnt;
    while(ctr >= recv_sockets.size()) {
      ctr -= g_this_rem_thread_cnt;
    }
  }
  assert(ctr < recv_sockets.size());
  uint64_t start_ctr = ctr;
  
	
  while(bytes <= 0 && (!simulation->is_setup_done() || (simulation->is_setup_done() && !simulation->is_done()))) {
    Socket * socket = recv_sockets[ctr];
		bytes = socket->sock.recv(&buf, NN_MSG, NN_DONTWAIT);

    //++ctr;
    ctr = (ctr + g_this_rem_thread_cnt);

    if(ctr >= recv_sockets.size())
      ctr = (thd_id % g_this_rem_thread_cnt) % recv_sockets.size();
    if(ctr == start_ctr)
      break;

		if(bytes <= 0 && errno != 11) {
		  printf("Recv Error %d %s\n",errno,strerror(errno));
			nn::freemsg(buf);	
		}

		if(bytes>0)
			break;
	}

	if(bytes <= 0 ) {
    INC_STATS(thd_id,msg_recv_idle_time, get_sys_clock() - starttime);
    return msgs;
	}

  INC_STATS(thd_id,msg_recv_time, get_sys_clock() - starttime);
	INC_STATS(thd_id,msg_recv_cnt,1);

	starttime = get_sys_clock();

  msgs = Message::create_messages((char*)buf);
  DEBUG_COMM("Batch of %d bytes recv from node %ld; Time: %f\n",bytes,msgs->front()->return_node_id,simulation->seconds_from_start(get_sys_clock()));

	nn::freemsg(buf);	

	INC_STATS(thd_id,msg_unpack_time,get_sys_clock()-starttime);
  return msgs;
}

#if USE_RDMA == 1
std::vector<Message*> * Transport::recv_msg_rdma(uint64_t thd_id) {
  uint64_t starttime = get_sys_clock();
  assert(msg_handler != NULL);
  all_msgs = new std::vector<Message*>();
  
  if(msg_handler != NULL)
    msg_handler->poll_comps(); // poll rpcs requests/replies

  if (all_msgs->empty()) {
    INC_STATS(thd_id,msg_recv_idle_time, get_sys_clock() - starttime);
    return all_msgs;
  }

  INC_STATS(thd_id,msg_recv_time, get_sys_clock() - starttime);
  INC_STATS(thd_id,msg_recv_cnt, all_msgs->size());
  return all_msgs;
}

#endif
/*
void Transport::simple_send_msg(int size) {
	void * sbuf = nn_allocmsg(size,0);

	ts_t time = get_sys_clock();
	memcpy(&((char*)sbuf)[0],&time,sizeof(ts_t));

  int rc = -1;
  while(rc < 0 ) {
  if(g_node_id == 0)
    rc = socket->sock.send(&sbuf,NN_MSG,0);
  else
    rc = socket->sock.send(&sbuf,NN_MSG,0);
	}
}

uint64_t Transport::simple_recv_msg() {
	int bytes;
	void * buf;

  if(g_node_id == 0)
		bytes = socket->sock.recv(&buf, NN_MSG, NN_DONTWAIT);
  else
		bytes = socket->sock.recv(&buf, NN_MSG, NN_DONTWAIT);
    if(bytes <= 0 ) {
      if(errno != 11)
        nn::freemsg(buf);	
      return 0;
    }

	ts_t time;
	memcpy(&time,&((char*)buf)[0],sizeof(ts_t));
	//printf("%d bytes, %f s\n",bytes,((float)(get_sys_clock()-time)) / BILLION);

	nn::freemsg(buf);	
	return bytes;
}
*/

