#include "ring_imm_msg_v2.h"
#include "utils.h"
#include "ralloc.h"

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#define FORCE 0 //Whether to force poll completion
#define FORCE_REPLY 1

#define USE_SEND 0

namespace rdmaio {

  namespace ring_imm_msg_v2 {

#define SET_HEADER_TAILER(msgp,len) \
    *((uint64_t *)msgp) = len; \
    *((uint64_t *)(msgp + sizeof(uint64_t) + len)) = len;\
    len += sizeof(uint64_t) * 2;\

    RingMessage::RingMessage(uint64_t ring_size,uint64_t ring_padding,
                             int thread_id,RdmaCtrl *cm,char *base_ptr, msg_func_v2_t func)
      :ring_size_(ring_size),
       ring_padding_(ring_padding),
       thread_id_(thread_id),
       num_nodes_(cm->get_num_nodes()),
       total_buf_size_(ring_size + ring_padding),
       cm_(cm),
       max_recv_num_(MAX_RECV_SIZE),
       node_id_(cm->get_nodeid()),   
       callback_(func)
    {
#if FORCE == 1
      // warning!
#endif

      unsigned id = (unsigned)_COMPACT_ENCODE_ID(node_id_, thread_id_);
      assert((cm->comm_graph).find(id) != (cm->comm_graph).end());
      uint neighbors_cnt = (cm->comm_graph)[id].size();
      assert(neighbors_cnt <= MSG_MAX_DESTS_SUPPORTED);
      if (neighbors_cnt == 0) {
        fprintf(stderr, "%d:%d is not expected to communicate to other node:thread!\n");
      }
      //fprintf(stdout,"ring message @ %d init\n",thread_id);

      char *start_ptr = (char *)(cm_->conn_buf_);
      char *cur_ptr = base_ptr;
      uint i = 0;
      for (; i < id; i++) {
        base_offset_[i] = cur_ptr - start_ptr;
        if ((cm->comm_graph).find(i) != (cm->comm_graph).end() && (cm->comm_graph)[i].size() > 0) {
          cur_ptr += (total_buf_size_ + MSG_META_SZ) * (cm->comm_graph)[i].size();
        }
      }
      base_ptr_ = cur_ptr;
      for (; i < MSG_MAX_DESTS_SUPPORTED; i++) {
        base_offset_[i] = cur_ptr - start_ptr;
        if ((cm->comm_graph).find(i) != (cm->comm_graph).end() && (cm->comm_graph)[i].size() > 0) {
          cur_ptr += (total_buf_size_ + MSG_META_SZ) * (cm->comm_graph)[i].size();
        }
      }

      // base_ptr_ = (base_ptr + (total_buf_size_ + MSG_META_SZ) * neighbors_cnt * thread_id);


      for(uint i = 0;i < MSG_MAX_DESTS_SUPPORTED; ++i) {
        offsets_[i] = 0;
        headers_[i] = 0;
      }

      /* do we really need to clean the buffer here ? */
      //  memset(base_ptr_,0, (total_buf_size_ + MSG_META_SZ) * num_nodes_ );

      /* The baseoffsets must be synchronized at all machine */
      // base_offset_ = base_ptr_ - start_ptr;

      // calculate the recv_buf_size
      recv_buf_size_ = 0;
      while(recv_buf_size_ < MAX_PACKET_SIZE){
        recv_buf_size_ += MIN_STEP_SIZE;
      }

      std::vector<uint> neighbors = (cm->comm_graph)[id];
      for (uint i = 0; i < neighbors_cnt; i++) {
        fprintf(stderr, "[RingMessage]-%d:%d adding %d:%d to qp_vec_.\n", node_id_, thread_id_, _COMPACT_DECODE_MAC(neighbors[i]), _COMPACT_DECODE_THREAD(neighbors[i]));
        uint64_t qid = cm_->get_rc_qid_v2(thread_id_, _COMPACT_DECODE_MAC(neighbors[i]), _COMPACT_DECODE_THREAD(neighbors[i]), 0);
        Qp *qp = cm_->get_qp(qid);
        assert(qp != NULL);
        qp_vec_[neighbors[i]]=qp;
        init(neighbors[i]);
      }
    }

    Qp::IOStatus
    RingMessage::send_to(int node, int remote_thread, char *msgp,int len) {

      //SET_HEADER_TAILER(msgp,len);
      int ret = (int) Qp::IO_SUCC;

      // calculate offset
      //calculate inner thread offset
      unsigned offset_idx = (unsigned)_COMPACT_ENCODE_ID(node, remote_thread);
      assert(offset_idx < 256);
      uint64_t offset = base_offset_[offset_idx] + _COMPACT_ENCODE_ID(node_id_, thread_id_) * (total_buf_size_ + MSG_META_SZ) +
        (offsets_[offset_idx] % ring_size_) + MSG_META_SZ;
      printf("%d:%d send_to() base_offset: %lu send_offset: %lu to: %d, r_off:%lu\n",node_id_, thread_id_, base_offset_[offset_idx], _COMPACT_ENCODE_ID(node_id_, thread_id_) * (total_buf_size_ + MSG_META_SZ) +
         (offsets_[offset_idx] % ring_size_), node, offset);
      offsets_[offset_idx] += len;
      
      assert(len <= ring_padding_);

      // get qp
      uint64_t id = _COMPACT_ENCODE_ID(node, remote_thread);
      fprintf(stderr, "%d:%d sending to %d:%d\n", node_id_, thread_id_, node, remote_thread);
      assert(qp_vec_.find(id) != qp_vec_.end() && qp_vec_[id] != NULL);
      Qp *qp = qp_vec_[id];

      // calculate send flag
      int send_flag = (len < 64) ? (IBV_SEND_INLINE) : 0;

#if FORCE_REPLY == 1
      send_flag |= IBV_SEND_SIGNALED;
#else
      if(qp->first_send()) {
        send_flag |= IBV_SEND_SIGNALED;
      }

      if(qp->need_poll())
        ret |= qp->poll_completion();
#endif

      // post the request
      // printf("write to-> %p! total_size %u \n", qp->remote_attr_.buf + offset, len);
      ibv_wr_opcode op;
#if USE_SEND == 1
      op = IBV_WR_SEND_WITH_IMM;
#else
      op = IBV_WR_RDMA_WRITE_WITH_IMM;
#endif

      ImmMeta meta;
      meta.nid  = node_id_;
      meta.tid  = thread_id_;
      meta.size = len;
      meta.cid = 0;
      
      ret |= qp->rc_post_send(op,msgp,len, offset,send_flag,0,meta.content);

#if FORCE_REPLY == 1
      ret |= qp->poll_completion();
#endif
      assert(ret == Qp::IO_SUCC);
      return (Qp::IOStatus)ret;
    }

    Qp::IOStatus
    RingMessage::broadcast_to(int *nodeids, int num, char *msg,int len) {
      assert(false);
      int ret = (int)(Qp::IO_SUCC);
      return (Qp::IOStatus)ret;
    }

    void RingMessage::force_sync(int *node_ids, int num_of_node) {
      assert(false);
    }

    bool
    RingMessage::try_recv_from(int from_mac, char *buffer) {
      assert(false);
    }


    inline char *
    RingMessage::try_recv_from(int from_mac, int from_thread) {
      char *start_ptr = (char *)(cm_->conn_buf_);

      int offset_idx = _COMPACT_ENCODE_ID(from_mac, from_thread);
      assert(offset_idx < 256);
      uint64_t poll_offset = offset_idx * (total_buf_size_ + MSG_META_SZ) + headers_[offset_idx] % ring_size_;

      fprintf(stderr, "%d:%d try_recv_from address: base_offset_=%d, poll_offset=%lu\n", node_id_, thread_id_, base_ptr_ - start_ptr, poll_offset);
      return base_ptr_ + poll_offset + MSG_META_SZ;
    }

    void RingMessage::check() { }

    int RingMessage::poll_comps() {
      // assert(false);

      int poll_result = 0;
      assert(cm_->comm_graph.find(_COMPACT_ENCODE_ID(node_id_, thread_id_)) != cm_->comm_graph.end());
      auto nei = cm_->comm_graph[_COMPACT_ENCODE_ID(node_id_, thread_id_)];
      for(auto itr = nei.begin(); itr != nei.end(); ++itr) {
        const int send_node_id = _COMPACT_DECODE_MAC(*itr);
        const int send_thread_id = _COMPACT_DECODE_THREAD(*itr);
        // note that since recv_cqs are shared by all the qps created by the same thread,
        // qp_vec_[id]->recv_cq is basically equivalent to qp_vec_[0]->recv_cq
        uint64_t id = _COMPACT_ENCODE_ID(send_node_id, send_thread_id);
        poll_result = ibv_poll_cq(qp_vec_[id]->recv_cq, RC_MAX_SHARED_RECV_SIZE,wc_);

        // prepare for replies
        assert(poll_result >= 0); // FIXME: ignore error
        if (poll_result > 0) break;
      }

      for(uint i = 0;i < poll_result; ++i) {
        // msg_num: poll_result

        // if(wc_[i].status != IBV_WC_SUCCESS) assert(false); // FIXME!
        if (wc_[i].status != IBV_WC_SUCCESS) {
          fprintf (stderr,
               "got bad completion with status: 0x%x, vendor syndrome: 0x%x, with error %s\n",
               wc_[i].status, wc_[i].vendor_err,ibv_wc_status_str(wc_[i].status));
          assert(false);
        }

        ImmMeta meta;
        meta.content = wc_[i].imm_data;
        uint32_t nid = meta.nid;
        uint32_t tid = meta.tid;
        uint32_t len = meta.size;
        uint32_t ntid = _COMPACT_ENCODE_ID(nid, tid);
        
        char* msg;
#if USE_SEND == 1
        msg = (char*)wc_[i].wr_id;
#else
        msg = try_recv_from(nid, tid);
#endif
        callback_(msg, len, nid, tid);
#if USE_SEND == 0
        ack_msg(ntid, meta.size);
#endif
        idle_recv_nums_[ntid] += 1;
        if(idle_recv_nums_[ntid] > max_idle_recv_num_) {
          printf("--posted :%d\n",idle_recv_nums_[nid]);
          // fprintf(stderr, "nid = %d, mac from qid = %d\n", nid, _QP_DECODE_MAC(qid));
          // assert(nid == _QP_DECODE_MAC(qid));
          // assert(tid == _QP_DECODE_THREAD(qid));
          post_recvs(idle_recv_nums_[ntid], ntid);
          idle_recv_nums_[ntid] = 0;
        }
      }

      return poll_result;
    }


    void RingMessage::init(uint32_t ntid) {
      struct ibv_recv_wr* rr = rrs_[ntid];
      struct ibv_sge* sge = sges_[ntid];

      // uintptr_t ptr = (uintptr_t)Rmalloc(recv_buf_size_);

      // init recv relate data structures
      for(int i = 0; i < max_recv_num_; i++) {
#if USE_SEND == 1
        sge[i].length = recv_buf_size_;
        sge[i].lkey   = qp_vec_[ntid]->dev_->conn_buf_mr->lkey;
        sge[i].addr   = (uintptr_t)Rmalloc(recv_buf_size_);
        assert(sge[i].addr != 0);
#else
        sge[i].length = 0;
        sge[i].lkey   = qp_vec_[ntid]->dev_->conn_buf_mr->lkey;
        sge[i].addr   = (uintptr_t)NULL;
#endif
        rr[i].wr_id   = sge[i].addr;
        rr[i].sg_list = &sge[i];
        rr[i].num_sge = 1;

        rr[i].next    = (i < max_recv_num_ - 1) ?
          &rr[i + 1] : &rr[0];
      }

      idle_recv_nums_[ntid] = 0;
      recv_heads_[ntid] = 0;

      // post these recvs
      post_recvs(max_recv_num_, ntid);
    }

    inline void RingMessage::post_recvs(int recv_num, uint32_t ntid) {
      int tail   = recv_heads_[ntid] + recv_num - 1;
      if(tail >= max_recv_num_) {
        tail -= max_recv_num_;
      }
      ibv_recv_wr  *head_rr = rrs_[ntid] + recv_heads_[ntid];
      ibv_recv_wr  *tail_rr = rrs_[ntid] + tail;

      ibv_recv_wr  *temp = tail_rr->next;
      tail_rr->next = NULL;

      int rc = ibv_post_recv(qp_vec_[ntid]->qp,head_rr,&bad_rr_);
      CE_1(rc, "[RingMessage] qp: Failed to post_recvs, %s\n", strerror(errno));

      tail_rr->next = temp; // restore the recv chain
      recv_heads_[ntid] = (tail + 1) % max_recv_num_;

    }


    // end namespace msg
  }
};
