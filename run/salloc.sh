#！/bin/bash

HOST_CNT=1
if [ x$1 != "x" ];then
	HOST_CNT=$1
fi

IFCONFIG_PATH="../ifconfig.txt"
if [ -f ${IFCONFIG_PATH} ]; then
  rm -fr ${IFCONFIG_PATH}
fi

TIME_ALLOC='00:5:00'
if [ x$2 != "x" ];then
	echo $2 | sed 's/,/\n/g' >> ${IFCONFIG_PATH}
	salloc -N $1 -t $TIME_ALLOC --nodelist=$2
else
	LAST_HOST=16
	FIRST_HOST=`echo $LAST_HOST - $HOST_CNT + 1 | bc`
	HOSTS_CSV=`seq -s, -f"nerv%.0f" $LAST_HOST -1 $FIRST_HOST`
	HOSTS=`seq -f"nerv%.0f" $LAST_HOST -1 $FIRST_HOST`

	for HOSTNAME in ${HOSTS}; do
	#  grep "\b${HOSTNAME}\b" /etc/hosts | awk '{print $1}' >> ${IFCONFIG_PATH}
	  echo ${HOSTNAME}  >> ${IFCONFIG_PATH}
	done
	salloc -N $1 -t $TIME_ALLOC --nodelist=$HOSTS_CSV
fi
