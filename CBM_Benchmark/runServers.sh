#!/bin/bash

#path=/home/berger/Libfabric_Benchmark/CBM_Benchmark
cd /home/berger/Libfabric_Benchmark/CBM_Benchmark
#server
SERVERS=5

## declare an array variable
#declare -a nodes=(pn16 pn17 pn18 pn19 pn21 pn22 pn23 pn28 pn29 pn30 pn31 pn32 pn33 pn34 pn37 pn38) #pn30 reserved?
# bad servers: ? pn17 pn33 pn34 pn38

declare -a nodes=(pn17 pn18 pn19 pn21 pn22 pn23 pn28 pn29 pn31 pn32 pn37)
#declare -a nodes=(pn22 pn23 pn28 pn29 pn31 pn32)
# get length of an array
arraylength=${#nodes[@]}
echo ${SERVERS} " / " ${arraylength}

# use for loop to read all values and indexes
for (( i=0; i<${SERVERS}; i++ ));
do
  srun --nodelist=${nodes[$i]} -l ./Benchmark pn ${i} &
done


# for (( i=0; i<${SERVERS}; i++ ));
# do
#   srun --nodelist=${nodes[$i]} -l ib_write_bw -a &
# done


# m=16
# i=0
# while [ ${i} -lt ${SERVERS} ]
# do
# NODE=`expr $m + $i`
# echo $NODE
# srun --nodelist=pn${NODE} -l ./Benchmark pn ${i} & i=`expr 1 + $i`
# done


