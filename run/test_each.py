#!/usr/bin/python

import os
import subprocess

use_rdma = 0
node_cnt = 4
workload = 'PPS'
cc_alg = 'NO_WAIT'

os.system('sed -i \'s/^#define USE_RDMA.*$/#define USE_RDMA ' + str(use_rdma) + '/\' ../config.h')
os.system('sed -i \'s/^#define NODE_CNT.*$/#define NODE_CNT ' + str(node_cnt) + '/\' ../config.h')
os.system('sed -i \'s/^#define WORKLOAD.*$/#define WORKLOAD ' + workload + '/\' ../config.h')
os.system('sed -i \'s/^#define CC_ALG.*$/#define CC_ALG ' + cc_alg + '/\' ../config.h')
code = os.system('set -e && cd .. && ./build.sh') >> 8
if code != 0:
	print('Compilation Failed.\n')
	exit()

os.system('./run_slurm.sh ' + str(node_cnt + 1) + ' ' +  str(node_cnt))
ps = subprocess.Popen(('awk', '-F,', '/\[summary\]/ {print $2}', '../' + str(node_cnt) + '.out'), stdout=subprocess.PIPE)
res = subprocess.check_output(('awk', '-F=', '{print $2}'), stdin=ps.stdout)
ps.wait()
if res.strip() != "":
	print("tput=%d\n" % (int(float(res.strip())/1000)))
else:
	print("Error. No tput.\n")