# Libfabric_Benchmark

## compile:

### makefile:
cd ".../CBM_Benchmark/"

### makefile options:

make, make remake, make clean

### Dependencies:
installed libfabric 1.6 or higher library - see https://ofiwg.github.io/libfabric/

numa library - https://linux.die.net/man/3/numa

pthread and openmp libraries are still included in makefile but are probably save to remove as they are not used anymore.

## configure:
### serverlist:
simple list of hostnames that act as servers/processing nodes. each line may hold one adress. '//' will tell the parser to ignore this line

current implementation assumes static client/server relationship and requires this file to find servers. Future implementation may run multi purpose nodes from central node. Removing the need for this file. 
### conf:
this Benchmark comes with various settings. the default settings should support a network of 10000x10000 nodes. [required] indicates that this field will most likely always need to be adjusted according to the network setup:

//port settings [required]

//client_port:[10000-19999], server_port:[20000-29999], controller_port=[30000-49999]

start_port=10000

controller_port=30000

server_offset=10000


//adress of the controller node [required]

controller_addr=10.1.103.102


//the size of each message. note this is not the MTU(payload) transmitted with each packet

//it is recommendet to keep this above the MTU 

msg_size=4096


//define number of nodes [required]

num_en=5

num_pn=5


//duration of the benchmark in seconds

max_duration=40

//time interval between each data snapshot in seconds

checkpoint_intervall=10

## run:
./Benchmark <cn/pn/en> <node_id> 
  
 Note: Benchmark assumes id format [0:n] port numbers are calculated based on node id.
 
## output:
results will be printet into file './[Weekday] [Month] [Day]' with format:
  
  en_id pn_id num_en num_pn delta_sec time_sec completions msg_size[byte] total[byte] total[Gbit] throughput[Gbit/s]
  
