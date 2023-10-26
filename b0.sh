#!/bin/bash

# if [ $# -lt 2 ]; then
#   echo "Usage: $0 <num_threads> <cfs_quota_us>"
#   exit 1
# fi

num_threads=$1
# cfs_quota_us=$2

# if [ "$cfs_quota_us" -lt 0 ] || [ "$cfs_quota_us" -gt 100 ]; then
  # echo "cfs_quota_us must be between 0 and 100"
  # exit 1
# fi

# total_cores=$(nproc)
# lower_bound=0
# upper_bound=$(($total_cores/3))
# upper_bound=$(($upper_bound < ($num_threads + $lower_bound) ? $upper_bound : ($num_threads + $lower_bound)))

# cpuset_cpus="$lower_bound-$upper_bound"

# sudo cgdelete -g cpuset:/cpulimitl
mkdir -p xxxx15
# sudo cgcreate -t $USER:$USER -a $USER:$USER -g cpuset:/cpulimitl
let yyml=num_threads+1
# sudo cgset -r cpuset.mems=0 cpulimitl
# sudo cgset -r cpuset.cpus=$cpuset_cpus cpulimitl
# sudo cgset -r cpu.cfs_quota_us=$((cfs_quota_us * 1000)) cpulimitl
# sudo cgset -r cpu.shares=0 cpulimitl

sudo cgexec -g cpuset:cpulimitl ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S $batch # > ./xxxx15/leader-$trd-$batch.log 2>&1 &
./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S $batch # > ./xxxx15/leader-$trd-$batch.log 2>&1 &

#tail -f ./xxxx15/leader-$trd-$batch.log
