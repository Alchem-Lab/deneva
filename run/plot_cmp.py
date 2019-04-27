#!/usr/bin/python
import matplotlib as mpl
mpl.use('pdf')
import matplotlib.pyplot as plt
import numpy as np
import re
import subprocess
import os.path
import sys

if len(sys.argv) < 2:
	out_path = 'out/'
else:
	out_path = 'out_' + str(sys.argv[1]) + '/';

if os.path.isdir(out_path) == False:
	print("%s does not exist.\n" % (out_path))
	exit()

series = ['TCP','RDMA']
workloads = ['PPS', 'TPCC', 'YCSB']
ccalgs = ['WAIT_DIE','NO_WAIT','TIMESTAMP','MVCC','CALVIN','MAAT','OCC']
servercnt = [2,4,8]

plt.clf()
plt.figure(figsize=(16,9))

cc_outs=[]
for wl in range(len(workloads)):	
	for sc_cnt in range(len(servercnt)):
		j = wl*len(servercnt)+sc_cnt
		ax = plt.subplot(3, 3, j+1)

		cc_outs.append([])
		for protocal in series:
			cc_outs[-1].append([])
			for inpt in ccalgs:
				fname = out_path + 'client_' + protocal + '_SERVERCNT' + str(servercnt[sc_cnt]) + '_' + workloads[wl] + '_' + inpt + '.out'
				res = ""
				if os.path.isfile(fname) == True:
					ps = subprocess.Popen(('awk', '-F,', '/\[summary\]/ {print $2}', fname), stdout=subprocess.PIPE)
					res = subprocess.check_output(('awk', '-F=', '{print $2}'), stdin=ps.stdout)
					ps.wait()
				if(res.strip() == ""):
					cc_outs[-1][-1].append(0)
				else: 
					print(res)
					cc_outs[-1][-1].append(int(float(res)/1000))

		width = 0.3
		colors=['b','g','r','c','m','y','k','grey']
		assert(len(colors) >= len(series))
		
		objs=[]
		for inpt in ccalgs:
				objs.append(str(inpt))
		y_pos = np.arange(len(objs))*1.5

		rects_list=[]
		for idx in range(len(series)):
			rects_list.append(plt.bar(y_pos+idx*width, cc_outs[j][idx], width, color=colors[idx], alpha=0.8, linewidth=0.1))
		
		if j % len(servercnt) == 0:
			plt.ylabel('Throughput (Trousand txns/s)', fontsize=8)
		plt.title(workloads[wl]+'_'+str(servercnt[sc_cnt]), fontsize=8, loc='right')
		#plt.xticks([], [])
		ax.get_xaxis().set_tick_params(direction='in', width=1, length=0)
		plt.xticks(y_pos+width*len(series)/2, objs, fontsize=6, rotation=0)
		ax.get_yaxis().set_tick_params(direction='in', width=0.5, length=2, pad=1)
		plt.yticks(fontsize=6)
		ax.yaxis.get_offset_text().set_size(2)
		ax.yaxis.set_ticks_position('left')

plt.legend((rects_list[0][0], rects_list[1][0]), series, fontsize=6, bbox_to_anchor=(-2.2, -0.2, 3, .06), loc=3, ncol=2, mode="expand", borderaxespad=0.)
if len(sys.argv) < 2:
	plt.savefig('tcp_cmp_rdma' + '.pdf', bbox_inches='tight')
else:
	plt.savefig('tcp_cmp_rdma_' + str(sys.argv[1]) + '.pdf', bbox_inches='tight')

