#!/bin/python

avg_execute=0
avg_comm=0
abort_elapsed=0
txn = {}
with open('txn_timing.txt') as f:
    for line in f:
        fields = line.strip().split('\t')
        txn[int(fields[0])] = [float(x) for x in fields[1:]]
        if txn[int(fields[0])][2] > 0:
            abort_elapsed += txn[int(fields[0])][2] - txn[int(fields[0])][1]

with open('txn_elapsed.txt') as f:
    for line in f:
        fields = line.strip().split('\t')
        txn[int(fields[0])].append(float(fields[1])-txn[int(fields[0])][-1])

for key, value in sorted(txn.iteritems()):
	avg_execute += value[-2]
	avg_comm += value[-1]
#	print("%s %s" % (str(key), ' '.join([str(x) for x in value])))

print("The following result is only correct for single-partition transactions.")
print("total txns=%d" % len(txn))
print("total_execute=%f" % avg_execute)
print("total_comm=%f" % avg_comm)

avg_execute /= len(txn)
avg_comm /= len(txn)

print("avg_execute=%f" % avg_execute)
print("avg_comm=%f" % avg_comm)
print("abort_elapsed=%f" % abort_elapsed)
