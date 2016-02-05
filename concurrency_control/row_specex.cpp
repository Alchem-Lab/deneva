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

#include "txn.h"
#include "row.h"
#include "row_specex.h"
#include "mem_alloc.h"

void 
Row_specex::init(row_t * row) {
	_row = row;
	_latch = (pthread_mutex_t *) 
		mem_allocator.alloc(sizeof(pthread_mutex_t));
	pthread_mutex_init( _latch, NULL );
	wts = 0;
	blatch = false;
}

RC
Row_specex::access(TxnManager * txn, TsType type) {
	RC rc = RCOK;
	pthread_mutex_lock( _latch );
	if (type == R_REQ) {
	  txn->cur_row->copy(_row);
		rc = RCOK;
	} else 
		assert(false);
	pthread_mutex_unlock( _latch );
	return rc;
}

void
Row_specex::latch() {
	pthread_mutex_lock( _latch );
}

bool
Row_specex::validate(uint64_t ts) {
	if (ts < wts) return false;
	else return true;
}

void
Row_specex::write(row_t * data, uint64_t ts) {
	_row->copy(data);
	if (PER_ROW_VALID) {
		assert(ts > wts);
		wts = ts;
	}
}

void
Row_specex::release() {
	pthread_mutex_unlock( _latch );
}
