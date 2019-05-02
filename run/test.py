#!/usr/bin/python

import os
import subprocess
import os.path

# save the previous raw data and pdfs.
out_path = 'out'
d = subprocess.check_output(('date', '-Iminutes'))
if os.path.isdir(out_path):
	os.system('mv ' + out_path + ' ' + out_path + '_' + ''.join(d.split(':')))
if os.path.isfile('rdma_scalability.pdf'):
	os.system('mv rdma_scalability.pdf rdma_scalability' + '_' + ''.join(d.strip().split(':')) + '.pdf')
if os.path.isfile('tcp_scalability.pdf'):
	os.system('mv tcp_scalability.pdf tcp_scalability' + '_' + ''.join(d.strip().split(':')) + '.pdf')
if os.path.isfile('tcp_cmp_rdma.pdf'):
	os.system('mv tcp_cmp_rdma.pdf tcp_cmp_rdma' + '_' + ''.join(d.strip().split(':')) + '.pdf')
os.system('mkdir -p ' + out_path)

TCPorRDMA = ['TCP','RDMA']
servercnt = [2,4,8]
clientcnt = [2,4,8]
workload = ['YCSB','TPCC','PPS']
ccalg = ['WAIT_DIE','NO_WAIT','TIMESTAMP','MVCC','CALVIN','MAAT','OCC']
for tr in range(0,2):
	for index, sc in enumerate(servercnt):
		for wl in range(0,3):
			for ca in range(0,7):
				fname = out_path + '/client_' + TCPorRDMA[tr] + '_' + 'SERVERCNT' + str(sc) + '_' + workload[wl] + '_' + ccalg[ca]
				if os.path.isfile(fname) == True and subprocess.check_output(['awk', '-F,', '/\[summary\]/ {print $2}', fname]).strip() != "":
					print(fname)
					continue
				os.system('sed -i \'s/^#define USE_RDMA.*$/#define USE_RDMA ' + str(tr) + '/\' ../config.h')
				os.system('sed -i \'s/^#define NODE_CNT.*$/#define NODE_CNT ' + str(sc) + '/\' ../config.h')
				os.system('sed -i \'s/^#define CLIENT_NODE_CNT.*$/#define CLIENT_NODE_CNT ' + str(clientcnt[index]) + '/\' ../config.h')
				os.system('sed -i \'s/^#define WORKLOAD.*$/#define WORKLOAD ' + workload[wl] + '/\' ../config.h')
				os.system('sed -i \'s/^#define CC_ALG.*$/#define CC_ALG ' + ccalg[ca] + '/\' ../config.h')
				code = os.system('set -e && cd .. && ./build.sh') >> 8
				if code == 0:
					os.system('./run_slurm.sh ' + str(sc+clientcnt[index]) + ' ' +  str(sc))
					for o in range(clientcnt[index]):
						os.system('mv ../' + str(sc+o) +'.out ' + fname + '_CLIENT' + str(sc+o) + '.out')
				else:
					print('Compilation Failed.\n')

				
				

