#ifndef RDMA_RING_IMM_V2_MSG_
#define RDMA_RING_IMM_V2_MSG_

#include "rdmaio.h"
#include "msg_handler.h"

/*
 * Layout of message buffer in RDMA registered region
 * | meta data | ring buffer | overflow padding |
 */

namespace rdmaio {
  
  namespace ring_imm_msg_v2 {

    const int MAX_RECV_SIZE = RC_MAX_RECV_SIZE;
    const int RC_MAX_DOORBELL_SIZE = 16;

    // constants
    const uint8_t MAX_BROADCAST_SERVERS = 32;
    const uint8_t MSG_META_SZ = sizeof(uint64_t);
    const uint8_t  MSG_MAX_DESTS_SUPPORTED = 255;

    const uint32_t MSG_DEFAULT_PADDING = (16 * 1024);
    const uint32_t MSG_DEFAULT_SZ = (4 * 1024 * 1024 - MSG_META_SZ - MSG_DEFAULT_PADDING);


    union ImmMeta {
      struct {
          uint32_t nid  : 4;
          uint32_t tid  : 4;
          uint32_t size : 10;
          uint32_t seq  : 14;
      };
      uint32_t content;
    };


    class RingMessage : public MsgHandler {

    public:
      /*
       * ringSz:      The buffer for receiving messages.
       * ringPadding: The overflow buffer for one message. msgsize must <= ringPadding
       * basePtr:     The start pointer of the total message buffer used at one server
       * callback:    The callback function after receive a message
       */
      RingMessage(uint64_t ring_size,uint64_t ring_padding,int thread_id,RdmaCtrl *cm,char *base_ptr, msg_func_v2_t callback);

      Qp::IOStatus send_to(int node_id,char *msg,int len) {
        return send_to(node_id, thread_id_, msg, len);
      }

      Qp::IOStatus send_to(int node_id,int tid,char *msg,int len);

      Qp::IOStatus broadcast_to(int *nodeids, int num, char *msg,int len);
      
      Qp::IOStatus post_pending(int node_id,char *msg,int len) {
        return send_to(node_id,thread_id_, msg,len);
      }

      Qp::IOStatus post_pending(int node_id,int tid,char *msg,int len) {
        return send_to(node_id,tid, msg,len);
      }


      Qp::IOStatus flush_pending() {
        return Qp::IO_SUCC;
      }

      // force a sync among all current in-flight messages, return when all these msgs are ready
      void force_sync(int *node_id,int num_of_node);

      // Return true if one message is received
      bool  try_recv_from(int from_mac, char *buffer);
      char *try_recv_from(int from_mac, int from_thread); // return: NULL no msg found, otherwise a pointer to the msg

      int  poll_comps();

      // if we receive one
      void inline __attribute__((always_inline))
        ack_msg(int from_mac_thread, int size) {
        headers_[from_mac_thread] += size;
      }

      int   get_num_nodes() { return num_nodes_; }
      int   get_thread_id() { return thread_id_; }

      virtual void check();

    private:
      std::unordered_map<int, Qp *> qp_vec_;
      std::unordered_map<uint, std::vector<uint> >* comm_graph;

      // The ring buffer size
      const uint64_t ring_size_;
      const uint64_t ring_padding_;
      const uint64_t total_buf_size_;

      // The Remote base offsets used for locating remote threads when sending messages
      uint64_t base_offset_[MSG_MAX_DESTS_SUPPORTED];

      // num nodes in total
      int num_nodes_;
    public:
      // my node id
      int node_id_;

      /* Local base offset */
      char *base_ptr_;
      RdmaCtrl *cm_;

      /* Local offsets used for polling */
      uint64_t offsets_[MSG_MAX_DESTS_SUPPORTED];

      /* Remote offsets used for sending messages */
      uint64_t headers_[MSG_MAX_DESTS_SUPPORTED];

      /* the sequence number for sending messages */
      uint64_t seq_[MSG_MAX_DESTS_SUPPORTED];

      /* the expected sequence number when polling */
      uint64_t exp_seq_[MSG_MAX_DESTS_SUPPORTED];

      /* The thread id */
      int thread_id_;

    private:
      //-------------------------------------
      // related to qp infos

      int recv_heads_[MSG_MAX_DESTS_SUPPORTED];
      int idle_recv_nums_[MSG_MAX_DESTS_SUPPORTED];
      int max_idle_recv_num_ = 1;
      int max_recv_num_ = 0;
      int recv_buf_size_; // calculated during init

      msg_func_v2_t callback_;

      // recv data structures
      struct ibv_recv_wr rrs_[MSG_MAX_DESTS_SUPPORTED][MAX_RECV_SIZE];
      struct ibv_sge sges_[MSG_MAX_DESTS_SUPPORTED][MAX_RECV_SIZE];
      struct ibv_recv_wr *bad_rr_;
      std::unordered_map<uint32_t, struct ibv_wc> wc_maps[MSG_MAX_DESTS_SUPPORTED];

      // private helper functions
      void init(uint32_t nid);
      void post_recvs(int recv_num,uint32_t nid);

    };

  }
};

#endif
