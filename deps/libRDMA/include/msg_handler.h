#ifndef RDMA_MSG_
#define RDMA_MSG_

// an abstraction of RDMA message passing interface

#include "rdmaio.h"

namespace rdmaio {

  typedef std::function<void(char *,int,int)> msg_func_t;
  typedef std::function<void(char *,int,int,int)> msg_func_v2_t;

  class MsgHandler {
  public:

    // additional padding used in the handler
    virtual int msg_padding() {
      return 0;
    }

    // send methods
    virtual Qp::IOStatus send_to(int node_id,char *msg,int len) = 0;
    virtual Qp::IOStatus send_to(int node_id,int tid,char *msg,int len) {
      return send_to(node_id,msg,len);
    }
    virtual Qp::IOStatus broadcast_to(int *node_ids, int num_of_node, char *msg,int len) = 0;

    virtual Qp::IOStatus broadcast_to(const std::set<int> &server_set, char *msg,int len) {
      prepare_pending();
      Qp::IOStatus ret;
      for(auto it = server_set.begin();it != server_set.end();++it) {
        ret = post_pending(*it,msg,len);
        if (ret != Qp::IO_SUCC)
          return ret;
      }
      return flush_pending();
    }

    // delayed send methods; the message shall be sent after flush_pending
    virtual Qp::IOStatus prepare_pending() {
      return Qp::IO_SUCC;
    }

    virtual Qp::IOStatus post_pending(int node_id,char *msg,int len) {
      return send_to(node_id,msg,len);
    }

    virtual Qp::IOStatus post_pending(int node_id,int tid,char *msg,int len) {
      return send_to(node_id,tid,msg,len);
    }

    virtual Qp::IOStatus flush_pending() {
      return Qp::IO_SUCC;
    }

    virtual void force_sync(int *node_id,int num_of_node) {

    }

    // poll all pending messages, return the number of completions polled.
    virtual int  poll_comps() = 0;

    virtual int get_num_nodes() = 0;
    virtual int get_thread_id() = 0;

    // print debug msg
    virtual void check() = 0;
    virtual void report() { } // report running statistics


  };

} // namespace rdmaio

#endif
