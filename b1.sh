#!/bin/bash

if [ $# -lt 2 ]; then
  echo "Usage: $0 <num_threads> <cfs_quota_us>"
  exit 1
fi

num_threads=$1
cfs_quota_us=$2

if [ "$cfs_quota_us" -lt 0 ] || [ "$cfs_quota_us" -gt 100 ]; then
  echo "cfs_quota_us must be between 0 and 100"
  exit 1
fi

total_cores=$(nproc)
cpuset_cpus="$(echo "scale=0; $total_cores/3 + 1" | bc)-$(echo "scale=0; $total_cores*2/3" | bc)"

sudo cgdelete -g cpuset:/cpulimitf
mkdir -p xxxx15
sudo cgcreate -t $USER:$USER -a $USER:$USER -g cpuset:/cpulimitf
let yyml=num_threads+1
# sudo cgset -r cpuset.mems=0 cpulimitf
sudo cgset -r cpuset.cpus=0-$num_threads cpulimitf
sudo cgset -r cpu.cfs_quota_us=$((cfs_quota_us * 1000)) cpulimitf
sudo cgset -r cpu.shares=0 cpulimitf

sudo cgexec -g cpuset:cpulimitf ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p2 # > ./xxxx15/follower-$trd.log 2>&1 &
# ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p2 # > ./xxxx15/follower-$trd.log 2>&1 &

#tail -f ./xxxx15/follower-$trd.log
