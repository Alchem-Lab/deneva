#ifndef R_MEM_ALLOCATOR
#define R_MEM_ALLOCATOR

#include "ralloc.h"
#include "config.h"
#include <stdint.h>
#include <queue>

namespace nocc {

  namespace oltp {

    class RMemAllocator {
    public:

      RMemAllocator() {
        RThreadLocalInit();
        for(int i = 0;i < MAX_INFLIGHT_REQS;++i) {

          buf_pools_[i] = (char *)Rmalloc(MSG_SIZE_MAX);
          // check for alignment
          assert( ((uint64_t)(buf_pools_[i])) % 8 == 0);
          if(buf_pools_[i] != NULL)
            memset(buf_pools_[i],0,MSG_SIZE_MAX);
          else
            assert(false);
        }
        current_buf_slot_ = 0;
      }

      ~RMemAllocator() {
        for(int i = 0;i < MAX_INFLIGHT_REQS;++i) {
          Rfree(buf_pools_[i]);
        }
      }

      inline char * operator[] (int id) const{
        return buf_pools_[id];
      }

      inline char * get_req_buf() {

        uint16_t buf_idx = (current_buf_slot_++) % MAX_INFLIGHT_REQS;
        assert(buf_idx >= 0 && buf_idx < MAX_INFLIGHT_REQS);
        // fetch the buf
        char *res = buf_pools_[buf_idx];
        return res;
        // return (char *)Rmalloc(MSG_SIZE_MAX);
      }

    private:
      char *buf_pools_[MAX_INFLIGHT_REQS];
      uint64_t current_buf_slot_;
    };
  } // namespace db
};  // namespace nocc

#endif
