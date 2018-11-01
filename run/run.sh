#!/bin/bash

#ssh nerv1 "cd ~/git_repos/deneva/ && ./rundb -nid0 > 1.out" > /dev/null & 
#ssh nerv2 "cd ~/git_repos/deneva/ && ./rundb -nid1 > 2.out" > /dev/null &
#ssh nerv3 "cd ~/git_repos/deneva/ && ./runcl -nid2 > 3.out" > /dev/null &
#ssh nerv4 "cd ~/git_repos/deneva/ && ./runcl -nid3 > 4.out" > /dev/null &
USERNAME=chao
HOSTS="$1"
NODE_CNT="$2"
count=0

for HOSTNAME in ${HOSTS}; do
  if [ $count -ge $NODE_CNT ]; then
	  SCRIPT="cd ~/git_repos/deneva/ && timeout -k 5m 5m ./runcl -nid${count} > ${count}.out 2>&1"
    echo "nerv${HOSTNAME}: runcl -nid${count}"
  else
	  SCRIPT="cd ~/git_repos/deneva/ && timeout -k 5m 5m ./rundb -nid${count} > ${count}.out"
    echo "nerv${HOSTNAME}: rundb -nid${count}"
  fi
	ssh -n -o BatchMode=yes -o StrictHostKeyChecking=no -l ${USERNAME} nerv${HOSTNAME} "${SCRIPT}" &
	count=`expr $count + 1`
done

while [ $count -gt 0 ]; do
	wait $pids
	count=`expr $count - 1`
done
