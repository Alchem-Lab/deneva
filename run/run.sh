#!/bin/bash

srun hostname -s | sort -n > slurm.hosts
USERNAME=chao
HOSTS=`cat slurm.hosts`
NODE_CNT="$1"
count=0

#generate ifconfig.txt
IFCONFIG_PATH="../ifconfig.txt"
if [ -f ${IFCONFIG_PATH} ]; then
  rm -fr ${IFCONFIG_PATH}
fi
for HOSTNAME in ${HOSTS}; do
  grep "\b${HOSTNAME}\b" /etc/hosts | awk '{print $1}' >> ${IFCONFIG_PATH}
done

#run the distributed DBMS
for HOSTNAME in ${HOSTS}; do
  if [ "$count" -ge "$NODE_CNT" ]; then
	  SCRIPT="cd ~/git_repos/deneva/ && timeout -k 5m 5m ./runcl -nid${count} > ${count}.out 2>&1"
    echo "${HOSTNAME}: runcl -nid${count}"
  else
	  SCRIPT="cd ~/git_repos/deneva/ && timeout -k 5m 5m ./rundb -nid${count} > ${count}.out"
    echo "${HOSTNAME}: rundb -nid${count}"
  fi
	ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "${SCRIPT}" &
	# ssh -n -l ${USERNAME} ${HOSTNAME} "echo ${HOSTNAME}" &
	count=`expr $count + 1`
done

while [ $count -gt 0 ]; do
	wait $pids
	count=`expr $count - 1`
done

scancel -u chao
