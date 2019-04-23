#!/bin/bash

IFCONFIG_PATH="../ifconfig.txt"
USERNAME=$USER
HOSTS=`cat $IFCONFIG_PATH`
NODE_CNT="$1"
count=0

#run the distributed DBMS
for HOSTNAME in ${HOSTS}; do
  if [ "$count" -ge "$NODE_CNT" ]; then
	  SCRIPT="cd ~/git_repos/deneva/ && ./run/set_hugepage.sh && ./run/cat_hugepage.sh && timeout -k 5m 5m ./runcl -nid${count} > ${count}.out 2>&1"
    echo "${HOSTNAME}: runcl -nid${count}"
  else
	  SCRIPT="cd ~/git_repos/deneva/ && ./run/set_hugepage.sh && ./run/cat_hugepage.sh && timeout -k 5m 5m ./rundb -nid${count} > ${count}.out 2>&1"
    echo "${HOSTNAME}: rundb -nid${count}"
  fi
	ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} ${HOSTNAME} "${SCRIPT}" &
	count=`expr $count + 1`
done

while [ $count -gt 0 ]; do
	wait $pids
	count=`expr $count - 1`
done

scancel -u $USERNAME
