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
#include "helper.h"
#include "plock.h"
#include "mem_alloc.h"
#include "txn.h"
#include "remote_query.h"
#include "ycsb_query.h"
#include "tpcc_query.h"
#include "msg_queue.h"

/************************************************/
// per-partition Manager
/************************************************/
void PartMan::init(uint64_t node_id, uint64_t part_id) {
	uint64_t part_id_tmp = get_part_id(this);
  _part_id = part_id;
	_node_id = node_id; 
	waiter_cnt = 0;
	owner = NULL;
	waiters = (TxnManager **)
		mem_allocator.alloc(sizeof(TxnManager *) * g_node_cnt * g_inflight_max, part_id_tmp);
	pthread_mutex_init( &latch, NULL );
}

void PartMan::start_spec_ex() {
  pthread_mutex_lock( &latch );

  TxnManager * tmp_txn;
  BaseQuery * owner_qry;
  txn_table.get_txn(GET_NODE_ID(owner->get_pid()), owner->get_txn_id(),0,tmp_txn,owner_qry);
  for (UInt32 i = 0; i < waiter_cnt - 1; i++) {
    if(waiters[i]->spec)
      continue;
    if(waiters[i]->parts_locked > 1) 
      continue;
      //check for conflicts
    // TODO: Check for conflicts with all waiters earlier in queue, not just the plock owner
    BaseQuery * waiter_qry;
    txn_table.get_txn(GET_NODE_ID(waiters[i]->get_pid()), waiters[i]->get_txn_id(),0,tmp_txn,waiter_qry);
    if(waiters[i]->conflict(owner_qry,waiter_qry))
      continue;
    waiters[i]->spec = true;
    txn_table.restart_txn(waiters[i]->get_txn_id(),0);
  }
  

  pthread_mutex_unlock( &latch );
}

RC PartMan::lock(TxnManager * txn) {
  RC rc;

  pthread_mutex_lock( &latch );
  if (owner == NULL) {
    owner = txn;
#if DEBUG_TIMELINE
      printf("LOCK %ld -- %ld\n",owner->get_txn_id(),_part_id);
      //printf("LOCK %ld %ld\n",owner->get_txn_id(),get_sys_clock());
#endif
    assert(!owner->spec);
		// If not local, send remote response
		//if(GET_NODE_ID(owner->get_pid()) != _node_id) {
		if(GET_NODE_ID(owner->home_part) != _node_id) {
			remote_rsp(true,RCOK,owner);
    } else if(owner->home_part != owner->active_part) {
      // possibly restart txn, remove WAIT status

      // Model after RACK
      //printf("Local RACK 3 -- %ld\n",_part_id);
#if WORKLOAD == TPCC
	    BaseQuery * tmp_query = (TPCCQuery *) mem_allocator.alloc(sizeof(TPCCQuery), 0);
	      tmp_query = new TPCCQuery();
#elif WORKLOAD == YCSB
	    BaseQuery * tmp_query = (YCSBQuery *) mem_allocator.alloc(sizeof(YCSBQuery), 0);
        tmp_query = new YCSBQuery();
#endif
        tmp_query->set_txn_id(owner->get_txn_id());
        tmp_query->ts = owner->get_ts();
        tmp_query->home_part = owner->home_part;
        tmp_query->rtype = RACK;
        tmp_query->rc = RCOK;
        tmp_query->active_part = owner->home_part;
        work_queue.enqueue(txn->get_thd_id(),tmp_query,false);

    }
    rc = RCOK;
  } else if (owner->get_ts() < txn->get_ts()) {
    int i;
    // depends on txn in flight
		//assert(waiter_cnt < (g_thread_cnt * g_node_cnt -1 ));
    for (i = waiter_cnt; i > 0; i--) {
      assert(txn != waiters[i-1]);
      if (txn->get_ts() > waiters[i - 1]->get_ts()) {
        waiters[i] = txn;
        break;
      } else 
        waiters[i] = waiters[i - 1];
    }
    if (i == 0)
      waiters[i] = txn;
    waiter_cnt ++;
		//if(GET_NODE_ID(txn->get_pid()) == _node_id)
		if(txn->home_part == _part_id)
      ATOM_ADD(txn->ready_part, 1);
    rc = WAIT;
    txn->rc = rc;
    INC_STATS(0, cflt_cnt, 1);
    txn->wait_starttime = get_sys_clock();
  } else {
    rc = Abort;
    txn->rc = rc;
		// if we abort, need to send abort to remote node
		//if(GET_NODE_ID(txn->get_pid()) != _node_id) {
		if(GET_NODE_ID(txn->home_part) != _node_id) {
			remote_rsp(true,rc,txn);
    } else if(txn->home_part != txn->active_part) {
      // Abort this txn at this node, possible restarting txn

      // Model after RACK
      //printf("Local RACK 1 -- %ld\n",_part_id);
#if WORKLOAD == TPCC
	    BaseQuery * tmp_query = (TPCCQuery *) mem_allocator.alloc(sizeof(TPCCQuery), 0);
	      tmp_query = new TPCCQuery();
#elif WORKLOAD == YCSB
	    BaseQuery * tmp_query = (YCSBQuery *) mem_allocator.alloc(sizeof(YCSBQuery), 0);
        tmp_query = new YCSBQuery();
#endif
        tmp_query->set_txn_id(txn->get_txn_id());
        tmp_query->ts = txn->get_ts();
        tmp_query->home_part = txn->home_part;
        tmp_query->rtype = RACK;
        tmp_query->rc = rc;
        tmp_query->active_part = txn->home_part;
        work_queue.enqueue(txn->get_thd_id(),tmp_query,false);


    }
  }
  pthread_mutex_unlock( &latch );
  return rc;
}

void PartMan::unlock(TxnManager * txn) {
  pthread_mutex_lock( &latch );
  if (txn == owner) {   
#if DEBUG_TIMELINE
      printf("UNLOCK %ld -- %ld\n",owner->get_txn_id(),_part_id);
      //printf("UNLOCK %ld %ld\n",owner->get_txn_id(),get_sys_clock());
#endif
    if (waiter_cnt == 0) {
      owner = NULL;
    }
    else {
      do {
      owner = waiters[0];     
#if DEBUG_TIMELINE
      //printf("LOCK %ld %ld\n",owner->get_txn_id(),get_sys_clock());
      printf("LOCK %ld -- %ld\n",owner->get_txn_id(),_part_id);
#endif
      for (UInt32 i = 0; i < waiter_cnt - 1; i++) {
        assert( waiters[i]->get_ts() < waiters[i + 1]->get_ts() );
        waiters[i] = waiters[i + 1];
      }
      waiter_cnt --;
      } while(owner->spec && waiter_cnt > 0);

    if (waiter_cnt == 0 && owner->spec) {
      owner = NULL;
      goto final;
    }
      uint64_t t = get_sys_clock() - owner->wait_starttime;
      INC_STATS(txn->get_thd_id(),time_wait_lock,t);
      txn->txn_time_wait += t;
			//if(GET_NODE_ID(owner->get_pid()) == _node_id) {
			if(GET_NODE_ID(owner->home_part) == _node_id) {
        if(owner->home_part == _part_id) {
          ATOM_SUB(owner->ready_part, 1);
          // If local and ready_part is 0, restart txn
          if(owner->ready_part == 0 && owner->get_rsp_cnt() == 0) {
            owner->state = EXEC; 
            owner->rc = RCOK;
            txn_table.restart_txn(owner->get_txn_id(),0);
          }
        } else {
          // Model after RACK
      //printf("Local RACK 2 -- %ld\n",_part_id);
#if WORKLOAD == TPCC
	    BaseQuery * tmp_query = (TPCCQuery *) mem_allocator.alloc(sizeof(TPCCQuery), 0);
	      tmp_query = new TPCCQuery();
#elif WORKLOAD == YCSB
	    BaseQuery * tmp_query = (YCSBQuery *) mem_allocator.alloc(sizeof(YCSBQuery), 0);
        tmp_query = new YCSBQuery();
#endif
        tmp_query->set_txn_id(owner->get_txn_id());
        tmp_query->ts = owner->get_ts();
        tmp_query->home_part = owner->home_part;
        tmp_query->rtype = RACK;
        tmp_query->rc = RCOK;
        tmp_query->active_part = owner->home_part;
        work_queue.enqueue(txn->get_thd_id(),tmp_query,false);


        }

      }
      else {
        INC_STATS(txn->get_thd_id(),time_wait_lock_rem,get_sys_clock() - owner->wait_starttime);
        owner->rc = RCOK;
				remote_rsp(true,RCOK,owner);
			}
    } 
  } else {
    bool find = false;
    for (UInt32 i = 0; i < waiter_cnt; i++) {
      if (waiters[i] == txn) 
        find = true;
      if (find && i < waiter_cnt - 1) 
        waiters[i] = waiters[i + 1];
    }
    /*
		if(GET_NODE_ID(txn->get_pid()) == _node_id)
      ATOM_SUB(txn->ready_ulk, 1);
      */
		// We may not find a remote request among the waiters; ignore
    //assert(find);
		if(find)
      waiter_cnt --;
  }
	// Send response for rulk
  /*
  if(GET_NODE_ID(txn->get_pid()) != _node_id)
	  remote_rsp(false,RCOK,txn);
    */
  // If local, decr ready_ulk
//final:
	//if(GET_NODE_ID(txn->get_pid()) == _node_id)
final:
	if(txn->home_part == _part_id)
    ATOM_SUB(txn->ready_ulk, 1);

  pthread_mutex_unlock( &latch );
}


void PartMan::remote_rsp(bool l, RC rc, TxnManager * txn) {
  msg_queue.enqueue(txn->get_query(),RACK,GET_NODE_ID(txn->get_pid()));

}
/************************************************/
// Partition Lock
/************************************************/

void Plock::init(uint64_t node_id) {
	_node_id = node_id;
	ARR_PTR(PartMan, part_mans, g_part_cnt);
	for (UInt32 i = 0; i < g_part_cnt; i++)
		part_mans[i]->init(node_id,i);
}

void Plock::start_spec_ex(uint64_t * parts, uint64_t part_cnt) {
	for (uint64_t i = 0; i < part_cnt; i ++) {
		uint64_t part_id = parts[i];
		if(GET_NODE_ID(part_id) == get_node_id())  {
      part_mans[part_id]->start_spec_ex();
    }
  }
}

RC Plock::lock(uint64_t * parts, uint64_t part_cnt, TxnManager * txn) {
	uint64_t tid = txn->get_thd_id();
  // Part ID is at home node
	uint64_t nid = txn->get_node_id();
  txn->set_pid(GET_PART_ID(tid,nid));
  txn->parts_locked = 0;
	RC rc = RCOK;
  txn->rc = RCOK;
	ts_t starttime = get_sys_clock();
	UInt32 i;
	for (i = 0; i < part_cnt; i ++) {
		uint64_t part_id = parts[i];
    txn->parts_locked++;
		if(GET_NODE_ID(part_id) == get_node_id())  {
			rc = part_mans[part_id]->lock(txn);
		}
		else {
			// Increment txn->ready_part; Pass txn to remote thr somehow?
			// Have some Plock shared object and spin on that instead of txn object?
			ATOM_ADD(txn->ready_part,1);
			//remote_qry(true, part_id, txn);
      msg_queue.enqueue(txn->get_query(),RLK,GET_NODE_ID(part_id));
		}
		if (rc == Abort || txn->rc == Abort)
			break;
	}
	if (txn->ready_part > 0 && !(rc == Abort || txn->rc == Abort)) {
    rc = WAIT;
    txn->rc = rc;
    txn->wait_starttime = get_sys_clock();
    return rc;

	}
	// Abort and send unlock requests as necessary
	if (rc == Abort || txn->rc == Abort) {
    rc = Abort;
    txn->rc = rc;
    return rc;

   }
	assert(txn->ready_part == 0);
	INC_STATS(tid, time_lock_man, get_sys_clock() - starttime);
	return RCOK;
}

RC Plock::unlock(uint64_t * parts, uint64_t part_cnt, TxnManager * txn) {
	uint64_t tid = txn->get_thd_id();
	//uint64_t nid = txn->get_node_id();
	ts_t starttime = get_sys_clock();
  // TODO: Store # parts locked or sent locks in qry
  // TODO: Only send to each node once, regardless of # of parts 
	//for (UInt32 i = 0; i < part_cnt; i ++) {
	for (UInt32 i = 0; i < txn->parts_locked; i ++) {
		uint64_t part_id = parts[i];
		if(GET_NODE_ID(part_id) == get_node_id()) {
      ATOM_ADD(txn->ready_ulk,1);
			part_mans[part_id]->unlock(txn);
    }
		else {
      ATOM_ADD(txn->ready_ulk,1);
			//remote_qry(false,part_id,txn);
      msg_queue.enqueue(txn->get_query(),RULK,GET_NODE_ID(part_id));
    }
	}
  if(txn->ready_ulk > 0) {
    txn->wait_starttime = get_sys_clock();
    txn->rc = WAIT;
    return WAIT;

  }
	assert(txn->ready_ulk == 0);
	INC_STATS(tid, time_lock_man, get_sys_clock() - starttime);
  return RCOK;
}

void Plock::unpack_rsp(BaseQuery * query, void * d) {
	char * data = (char *) d;
	uint64_t ptr = HEADER_SIZE + sizeof(txnid_t) + sizeof(RemReqType);
	memcpy(&query->rc,&data[ptr],sizeof(RC));
	ptr += sizeof(query->rc);
	memcpy(&query->pid,&data[ptr],sizeof(query->pid));
	ptr += sizeof(query->pid);
	memcpy(&query->ts,&data[ptr],sizeof(query->ts));
	ptr += sizeof(query->ts);
}

void Plock::unpack(BaseQuery * query, char * data) {
	uint64_t ptr = HEADER_SIZE + sizeof(txnid_t) + sizeof(RemReqType);
	assert(query->rtype == RLK || query->rtype == RULK);
		
	memcpy(&query->pid,&data[ptr],sizeof(query->pid));
	ptr += sizeof(query->pid);
	memcpy(&query->ts,&data[ptr],sizeof(query->ts));
	ptr += sizeof(query->ts);
	memcpy(&query->part_cnt,&data[ptr],sizeof(query->part_cnt));
	ptr += sizeof(query->part_cnt);
	query->parts = new uint64_t[query->part_cnt];
	for (uint64_t i = 0; i < query->part_cnt; i++) {
		memcpy(&query->parts[i],&data[ptr],sizeof(query->parts[i]));
		ptr += sizeof(query->parts[i]);
	}
}


RC Plock::rem_lock(uint64_t * parts, uint64_t part_cnt, TxnManager * txn) {
  RC rc;
  assert(part_cnt >= 1);
	ts_t starttime = get_sys_clock();
	for (UInt32 i = 0; i < part_cnt; i ++) {
		uint64_t part_id = parts[i];
		assert(GET_NODE_ID(part_id) == get_node_id());
		rc = part_mans[part_id]->lock(txn);
	}
	INC_STATS(txn->get_thd_id(), time_lock_man, get_sys_clock() - starttime);
  return rc;
}

void Plock::rem_unlock(uint64_t * parts, uint64_t part_cnt, TxnManager * txn) {
	ts_t starttime = get_sys_clock();
	for (UInt32 i = 0; i < part_cnt; i ++) {
		uint64_t part_id = parts[i];
		assert(GET_NODE_ID(part_id) == get_node_id());
		part_mans[part_id]->unlock(txn);
	}
  INC_STATS(txn->get_thd_id(), time_lock_man, get_sys_clock() - starttime);
}

void Plock::rem_lock_rsp(RC rc, TxnManager * txn) {
	ts_t starttime = get_sys_clock();
	if(rc != RCOK) {
    assert(rc == Abort);
    txn->rc = rc;
  }
	ATOM_SUB(txn->ready_part, 1);
	INC_STATS(txn->get_thd_id(), time_lock_man, get_sys_clock() - starttime);
}

void Plock::rem_unlock_rsp(TxnManager * txn) {
	ts_t starttime = get_sys_clock();
	ATOM_SUB(txn->ready_ulk, 1);
	INC_STATS(txn->get_thd_id(), time_lock_man, get_sys_clock() - starttime);
}