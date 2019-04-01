#!/usr/bin/python
import matplotlib as mpl
mpl.use('pdf')
import matplotlib.pyplot as plt
import numpy as np
import re
import subprocess
import os.path

out_path = 'out/'

workloads = ['PPS', 'TPCC', 'YCSB']
server_cnts = [2,4,8]
ccalgs = ['WAIT_DIE','NO_WAIT','TIMESTAMP','MVCC','CALVIN','MAAT','OCC']

plt.clf()
plt.figure(figsize=(10,2))

cc_outs=[]
for j in range(len(workloads)):
	ax = plt.subplot(1, 3, j+1)

	cc_outs.append([])
	for cc in ccalgs:
		cc_outs[-1].append([])
		for inpt in server_cnts:
			fname = out_path + 'client_' + 'RDMA' + '_SERVERCNT' + str(inpt) + '_' + workloads[j] + '_' + cc + '.out'
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

	width = 0.15
	colors=['b','g','r','c','m','y','k','grey']
	assert(len(colors) >= len(ccalgs))
	
	objs=[]
	for inpt in server_cnts:
			objs.append(str(inpt))
	y_pos = np.arange(len(objs))*1.5

	rects_list=[]
	for idx in range(len(ccalgs)):
		rects_list.append(plt.bar(y_pos+idx*width, cc_outs[j][idx], width, color=colors[idx], alpha=0.8, linewidth=0.1))
    
	if j == 0:
		plt.ylabel('Throughput (Trousand txns/s)', fontsize=5)
	plt.title(workloads[j], fontsize=5, loc='right')
	#plt.xticks([], [])
	ax.get_xaxis().set_tick_params(direction='in', width=1, length=0)
	plt.xticks(y_pos+width*len(ccalgs)/2, objs, fontsize=4, rotation=0)
	ax.get_yaxis().set_tick_params(direction='in', width=0.5, length=2, pad=1)
	plt.yticks(fontsize=4)
	ax.yaxis.get_offset_text().set_size(2)
	ax.yaxis.set_ticks_position('left')

plt.legend((rects_list[0][0], rects_list[1][0], rects_list[2][0], rects_list[3][0], rects_list[4][0], rects_list[5][0], rects_list[6][0]), ccalgs, fontsize=4, bbox_to_anchor=(-2.2, -0.17, 3, .06), loc=3, ncol=7, mode="expand", borderaxespad=0.)
plt.savefig('rdma_scalability' + '.pdf', bbox_inches='tight')
