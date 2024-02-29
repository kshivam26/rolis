#!/bin/bash
# sudo tc qdisc add dev eth0 root tbf rate 3gbit burst 15mb latency 1ms
echo "@@@0"
sudo cgdelete -g cpuset:/crpc-test-l
mkdir -p xxxx15
sudo cgcreate -g cpuset:/crpc-test-l
echo "@@@1"

trd=$1
version=$3
let yyml=trd+1
defvalue=1000
batch=${3:-$defvalue}

## uncomment
# if [ $trd -lt 8 ]; then
#   # let cpuset_cpus=yyml+2
#   let cpuset_cpus=yyml+1
# elif [ $trd -lt 16 ]; then
#   # let cpuset_cpus=yyml+4
#   let cpuset_cpus=yyml+2
# elif [ $trd -lt 24 ]; then
#   let cpuset_cpus=yyml+4
# else
#   let cpuset_cpus=yyml+8
# fi


# assign only one extra cpu (assigned using cgset)
let cpuset_cpus=(yyml)



sudo cgset -r cpuset.cpus=0-$cpuset_cpus crpc-test-l

echo $trd
echo $yyml
echo $cpuset_cpus
# echo $cpu_cfs_quota

# uncomment
sudo cgexec -g cpuset:crpc-test-l ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 180 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S $batch -q $2 # > ./xxxx15/leader-$trd-$batch.log 2>&1 &

# comment
# sudo ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 180 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S $batch -q $2 # > ./xxxx15/leader-$trd-$batch.log 2>&1 &

# sudo gdb -ex r -ex bt --args cgexec -g cpuset:crpc-test-l ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 180 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S $batch -q $2 # > ./xxxx15/leader-$trd-$batch.log 2>&1 &

# Uncomment the following line if you want to tail the log
# tail -f ./xxxx15/leader-$trd-$batch.log