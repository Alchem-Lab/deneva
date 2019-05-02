#!/usr/bin/python

import os
import subprocess

use_rdma = 1
node_cnt = 4
client_cnt = 4
workload = 'PPS'
cc_alg = 'NO_WAIT'

os.system('sed -i \'s/^#define USE_RDMA.*$/#define USE_RDMA ' + str(use_rdma) + '/\' ../config.h')
os.system('sed -i \'s/^#define NODE_CNT.*$/#define NODE_CNT ' + str(node_cnt) + '/\' ../config.h')
os.system('sed -i \'s/^#define CLIENT_NODE_CNT.*$/#define CLIENT_NODE_CNT ' + str(client_cnt) + '/\' ../config.h')
os.system('sed -i \'s/^#define WORKLOAD.*$/#define WORKLOAD ' + workload + '/\' ../config.h')
os.system('sed -i \'s/^#define CC_ALG.*$/#define CC_ALG ' + cc_alg + '/\' ../config.h')
code = os.system('set -e && cd .. && ./build.sh') >> 8
if code != 0:
	print('Compilation Failed.\n')
	exit()

os.system('./run_slurm.sh ' + str(node_cnt + client_cnt) + ' ' +  str(node_cnt))

tput_sum = 0
for client in range(client_cnt):
	ps = subprocess.Popen(('awk', '-F,', '/\[summary\]/ {print $2}', '../' + str(node_cnt + client) + '.out'), stdout=subprocess.PIPE)
	res = subprocess.check_output(('awk', '-F=', '{print $2}'), stdin=ps.stdout)
	ps.wait()
	if res.strip() != "":
		tput = int(float(res.strip())/1000)
		tput_sum += tput
		print("Client %d tput=%d\n" % (node_cnt + client, tput))
	else:
		print("Error. Client %d No tput.\n", node_cnt + client)
print("Total tput=%d\n" % tput_sum)
