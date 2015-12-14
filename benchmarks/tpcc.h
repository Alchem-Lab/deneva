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

#ifndef _TPCC_H_
#define _TPCC_H_

#include "wl.h"
#include "txn.h"
#include "remote_query.h"
#include "query.h"
#include "row.h"

class table_t;
class INDEX;
class TPCCQuery;

class TPCCWorkload : public Workload {
public:
	RC init();
	RC init_table();
	RC init_schema(const char * schema_file);
	RC get_txn_man(TxnManager *& txn_manager);
	table_t * 		t_warehouse;
	table_t * 		t_district;
	table_t * 		t_customer;
	table_t *		t_history;
	table_t *		t_neworder;
	table_t *		t_order;
	table_t *		t_orderline;
	table_t *		t_item;
	table_t *		t_stock;

	INDEX * 	i_item;
	INDEX * 	i_warehouse;
	INDEX * 	i_district;
	INDEX * 	i_customer_id;
	INDEX * 	i_customer_last;
	INDEX * 	i_stock;
	INDEX * 	i_order; // key = (w_id, d_id, o_id)
//	INDEX * 	i_order_wdo; // key = (w_id, d_id, o_id)
//	INDEX * 	i_order_wdc; // key = (w_id, d_id, c_id)
	INDEX * 	i_orderline; // key = (w_id, d_id, o_id)
	INDEX * 	i_orderline_wd; // key = (w_id, d_id). 
	
	// XXX HACK
	// For delivary. Only one txn can be delivering a warehouse at a time.
	// *_delivering[warehouse_id] -> the warehouse is delivering.
	bool ** delivering;
//	bool volatile ** delivering;

private:
	uint64_t num_wh;
	void init_tab_item(int id);
	void init_tab_wh();
	void init_tab_dist(uint64_t w_id);
	void init_tab_stock(int id,uint64_t w_id);
	// init_tab_cust initializes both tab_cust and tab_hist.
	void init_tab_cust(int id, uint64_t d_id, uint64_t w_id);
	void init_tab_hist(uint64_t c_id, uint64_t d_id, uint64_t w_id);
	void init_tab_order(int id,uint64_t d_id, uint64_t w_id);
	
	UInt32 perm_count;
	uint64_t * perm_c_id;
	void init_permutation();
	uint64_t get_permutation();

	static void * threadInitItem(void * This);
	static void * threadInitWh(void * This);
	static void * threadInitDist(void * This);
	static void * threadInitStock(void * This);
	static void * threadInitCust(void * This);
	static void * threadInitHist(void * This);
	static void * threadInitOrder(void * This);
};

  struct thr_args{
    TPCCWorkload * wl;
    UInt32 id;
    UInt32 tot;
  };

class TPCCTxnManager : public TxnManager
{
public:
	void init( Workload * h_wl);
  bool conflict(BaseQuery * query1,BaseQuery * query2);
  void read_keys(BaseQuery * query){ assert(false);}; 
  RC acquire_locks(BaseQuery * query); 
	RC run_txn(BaseQuery * query);
	RC run_calvin_txn(BaseQuery * query); 
	RC run_rem_txn(BaseQuery * query);
  RC run_tpcc_phase2(BaseQuery * query); 
  RC run_tpcc_phase5(BaseQuery * query); 
	void rem_txn_rsp(BaseQuery * rtn_query,BaseQuery * loc_query); 
	void rem_txn_rsp(BaseQuery * query); 
	//void pack(r_query * query, void ** data, int * sizes, int * num);
	//void unpack(r_query * query, char * data);
private:
	TPCCWorkload * _wl;
	volatile RC _rc;
	TPCCQuery * _qry;
  row_t * row;
  bool rem_done;
  bool fin;

void merge_txn_rsp(BaseQuery * query1, BaseQuery * query2);
//RC run_txn(BaseQuery * query, bool rem);
void rtn_tpcc_state(BaseQuery * query);
void next_tpcc_state(BaseQuery * query);
RC run_txn_state(BaseQuery * query);

	RC run_payment_0(uint64_t w_id, uint64_t d_id, uint64_t d_w_id, double h_amount, row_t *& r_wh_local);
	RC run_payment_1(uint64_t w_id, uint64_t d_id, uint64_t d_w_id, double h_amount, row_t * r_wh_local);
	RC run_payment_2(uint64_t w_id, uint64_t d_id, uint64_t d_w_id, double h_amount, row_t *& r_dist_local);
	RC run_payment_3(uint64_t w_id, uint64_t d_id, uint64_t d_w_id, double h_amount, row_t * r_dist_local);
	RC run_payment_4(uint64_t w_id, uint64_t d_id,uint64_t c_id,uint64_t c_w_id, uint64_t c_d_id, char * c_last, double h_amount, bool by_last_name, row_t *& r_cust_local); 
	RC run_payment_5(uint64_t w_id, uint64_t d_id,uint64_t c_id,uint64_t c_w_id, uint64_t c_d_id, char * c_last, double h_amount, bool by_last_name, row_t * r_cust_local); 
	RC new_order_0(uint64_t w_id, uint64_t d_id, uint64_t c_id, bool remote, uint64_t  ol_cnt,uint64_t  o_entry_d, uint64_t * o_id, row_t *& r_wh_local);
	RC new_order_1(uint64_t w_id, uint64_t d_id, uint64_t c_id, bool remote, uint64_t  ol_cnt,uint64_t  o_entry_d, uint64_t * o_id, row_t * r_wh_local);
	RC new_order_2(uint64_t w_id, uint64_t d_id, uint64_t c_id, bool remote, uint64_t  ol_cnt,uint64_t  o_entry_d, uint64_t * o_id, row_t *& r_cust_local);
	RC new_order_3(uint64_t w_id, uint64_t d_id, uint64_t c_id, bool remote, uint64_t  ol_cnt,uint64_t  o_entry_d, uint64_t * o_id, row_t * r_cust_local);
	RC new_order_4(uint64_t w_id, uint64_t d_id, uint64_t c_id, bool remote, uint64_t  ol_cnt,uint64_t  o_entry_d, uint64_t * o_id, row_t *& r_dist_local);
	RC new_order_5(uint64_t w_id, uint64_t d_id, uint64_t c_id, bool remote, uint64_t  ol_cnt,uint64_t  o_entry_d, uint64_t * o_id, row_t * r_dist_local);
	RC new_order_6(uint64_t ol_i_id, row_t *& r_item_local);
	RC new_order_7(uint64_t ol_i_id, row_t * r_item_local);
	RC new_order_8(uint64_t w_id,uint64_t  d_id,bool remote, uint64_t ol_i_id, uint64_t ol_supply_w_id, uint64_t ol_quantity,uint64_t  ol_number,uint64_t  o_id, row_t *& r_stock_local);
	RC new_order_9(uint64_t w_id,uint64_t  d_id,bool remote, uint64_t ol_i_id, uint64_t ol_supply_w_id, uint64_t ol_quantity,uint64_t  ol_number,uint64_t ol_amount, uint64_t  o_id, row_t * r_stock_local);
	RC run_order_status(TPCCQuery * query);
	RC run_delivery(TPCCQuery * query);
	RC run_stock_level(TPCCQuery * query);
};

#endif