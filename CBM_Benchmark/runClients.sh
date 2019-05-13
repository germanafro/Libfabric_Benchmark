#!/bin/bash

#path=/home/berger/Libfabric_Benchmark/CBM_Benchmark
cd /home/berger/Libfabric_Benchmark/CBM_Benchmark
#settings
CLIENTS=5


## declare an array variable
#declare -a nodes=(pn02 pn04 pn05 pn06 pn07 pn08 pn10 pn11 pn12 pn13 pn15 pn20 pn24 pn25 pn26 pn27 pn35 pn36 pn39 pn40) 
# bad clients? pn20 pn24
declare -a nodes=(pn04 pn05 pn06 pn07 pn08 pn10 pn11 pn12 pn13 pn15 pn25 pn26 pn27 pn35 pn36 pn39 pn40) #pn02 is controller

# get length of an array
arraylength=${#nodes[@]}
echo ${CLIENTS} " / " ${arraylength}

# use for loop to read all values and indexes
for (( i=0; i<${CLIENTS}; i++ ));
do
  srun --nodelist=${nodes[$i]} -l ./Benchmark en ${i} &
done


# for (( i=0; i<${CLIENTS}; i++ ));
# do
#   srun --nodelist=${nodes[$i]} -l ib_write_bw -a ${servers[$i]}ib0 &
# done

#m=31
#i=0
#while [ ${i} -lt ${CLIENTS} ]
#do
#NODE=`expr $m + $i`
#echo $NODE
#srun --nodelist=pn${NODE} -l ./Benchmark en ${i} & i=`expr 1 + $i`
#done






#srun --nodelist=pn32 -l ./Benchmark ${SERVERS} 10240 1 pn15ib0 6667 & 
#srun --nodelist=pn33 -l ./Benchmark ${SERVERS} 10240 1 pn15ib0 6668 &  
#srun --nodelist=pn34 -l ./Benchmark ${SERVERS} 10240 1 pn15ib0 6669 & 
#srun --nodelist=pn35 -l ./Benchmark ${SERVERS} 10240 1 pn15ib0 6670 & 
#srun --nodelist=pn36 -l ./Benchmark ${SERVERS} 10240 1 pn15ib0 6671 & 
#srun --nodelist=pn37 -l ./Benchmark ${SERVERS} 10240 1 pn15ib0 6672 & 
#srun --nodelist=pn38 -l ./Benchmark 1 10240 1 pn15ib0 6673 & 
#srun --nodelist=pn39 -l ./Benchmark 1 10240 1 pn15ib0 6674 & 
#srun --nodelist=pn40 -l ./Benchmark 1 10240 1 pn15ib0 6675