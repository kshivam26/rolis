#!/bin/bash
# sudo tc qdisc add dev eth0 root tbf rate 3gbit burst 15mb latency 1ms
sudo cgdelete -g cpuset:/crpc-test-f1
mkdir -p xxxx15
sudo cgcreate -g cpuset:/crpc-test-f1

trd=$1
version=$2
let yyml=trd+1
defvalue=1000
batch=${3:-$defvalue}

# if [ $trd -lt 5 ]; then
#   let cpuset_cpus=yyml
# #   let cpu_cfs_quota=50000*yyml
# else
#   let cpuset_cpus=yyml+2
# #   let cpu_cfs_quota=50000*yyml
# fi

# if [ $trd -lt 8 ]; then
#   let cpuset_cpus=yyml+1
# elif [ $trd -lt 16 ]; then
#   let cpuset_cpus=yyml+2
# elif [ $trd -lt 24 ]; then
#   let cpuset_cpus=yyml+3
# else
#   let cpuset_cpus=yyml+4
# fi

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

let cpuset_cpus=(yyml)

# uncomment
sudo cgset -r cpuset.cpus=$((2*yyml))-$((2*yyml+cpuset_cpus)) crpc-test-f1




echo $trd
echo $yyml
echo $cpuset_cpus
# echo $cpu_cfs_quota

# uncomment
sudo cgexec -g cpuset:crpc-test-f1 ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 180 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p1 # > ./xxxx15/follower-$trd.log 2>&1 &

# comment
# sudo ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 180 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p1 # > ./xxxx15/follower-$trd.log 2>&1 &
# Uncomment the following line if you want to tail the log
# tail -f ./xxxx15/leader-$trd-$batch.log