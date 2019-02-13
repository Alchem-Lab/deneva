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

#ifndef _CALVINTHREAD_H_
#define _CALVINTHREAD_H_

#include "global.h"

class Workload;

/*
class CalvinThread : public Thread {
public:
	RC 			run();
  void setup();
  uint64_t txn_starttime;
private:
	TxnManager * m_txn;
};
*/

#if USE_RDMA == 1

#include "rdmaio.h"
class CalvinLockThread : public Thread {
public:
    CalvinLockThread(int worker_id, rdmaio::RdmaCtrl *cm, int seed = 0) : Thread(worker_id, cm, seed) {}
    void run();
    void setup();
    void register_callbacks() {}
    void worker_routine(yield_func_t &yield) {}
private:
    TxnManager * m_txn;
};

class CalvinSequencerThread : public Thread {
public:
    CalvinSequencerThread(int worker_id, rdmaio::RdmaCtrl *cm, int seed = 0) : Thread(worker_id, cm, seed) {}
    void run();
    void setup();
    void register_callbacks() {}
    void worker_routine(yield_func_t &yield) {}
private:
    bool is_batch_ready();
  uint64_t last_batchtime;
};

#else

class CalvinLockThread : public Thread {
public:
    void run();
    void setup();
private:
    TxnManager * m_txn;
};

class CalvinSequencerThread : public Thread {
public:
    void run();
    void setup();
private:
    bool is_batch_ready();
	uint64_t last_batchtime;
};

#endif

#endif
