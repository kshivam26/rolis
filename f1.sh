#!/bin/bash
sudo cgdelete -g cpuset:/cpulimitf
mkdir -p xxxx16
sudo cgcreate -t $USER:$USER -a $USER:$USER  -g cpuset:/cpulimitf
trd=$1
let yyml=trd+1
sudo cgset -r cpuset.mems=0 cpulimitf
#sudo cgset -r cpuset.cpus=0-$trd cpulimitf
sudo cgset -r cpuset.cpus=0-$trd cpulimitf
sudo cgexec -g cpuset:cpulimitf ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_4follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p1 > ./xxxx16/follower-$trd.log 2>&1 &
#tail -f ./xxxx16/follower-$trd.log
