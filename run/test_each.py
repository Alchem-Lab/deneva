#!/usr/bin/python

import os

use_rdma = 1
node_cnt = 2
workload = 'PPS'
cc_alg = 'WAIT_DIE'

os.system('sed -i \'s/^#define USE_RDMA.*$/#define USE_RDMA ' + str(use_rdma) + '/\' ../config.h')
os.system('sed -i \'s/^#define NODE_CNT.*$/#define NODE_CNT ' + str(node_cnt) + '/\' ../config.h')
os.system('sed -i \'s/^#define WORKLOAD.*$/#define WORKLOAD ' + workload + '/\' ../config.h')
os.system('sed -i \'s/^#define CC_ALG.*$/#define CC_ALG ' + cc_alg + '/\' ../config.h')
code = os.system('set -e && cd .. && ./build.sh') >> 8
if code == 0:
	os.system('./run_slurm.sh ' + str(node_cnt + 1) + ' ' +  str(node_cnt))
else:
	print('Compilation Failed.\n')
