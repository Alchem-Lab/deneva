#!/usr/bin/python

import os
out_path = 'out/'
os.system('mkdir -p ' + out_path)
TCPorRDMA = ['TCP','RDMA']
servercnt = ['','SERVERCNT2','SERVERCNT4','SERVERCNT8']
workload = ['YCSB','TPCC','PPS']
ccalg = ['WAIT_DIE','NO_WAIT','TIMESTAMP','MVCC','CALVIN','MAAT','OCC']
for tr in range(0,1):
	for sc in range(1,2):
		for wl in range(2,3):
			for ca in range(4,5):
				#for j in range (0,2):
					os.system('sed -i \'s/^#define USE_RDMA.*$/#define USE_RDMA ' + str(tr) + '/\' ../config.h')
					os.system('sed -i \'s/^#define NODE_CNT.*$/#define NODE_CNT ' + str(2**sc) + '/\' ../config.h')
					os.system('sed -i \'s/^#define WORKLOAD.*$/#define WORKLOAD ' + workload[wl] + '/\' ../config.h')
					os.system('sed -i \'s/^#define CC_ALG.*$/#define CC_ALG ' + ccalg[ca] + '/\' ../config.h')
					os.system('cd .. && ./build.sh')
					os.system('./run_slurm.sh ' + str(2**sc + 1) + ' ' +  str(2**sc))
					os.system('mv ../' + str(2**sc) +'.out ' + out_path + 'client_' + TCPorRDMA[tr] + '_' + servercnt[sc] + '_' + workload[wl] + '_' + ccalg[ca] + '.out')

