#!/bin/bash
# sudo tc qdisc add dev eth0 root tbf rate 3gbit burst 15mb latency 1ms
sudo cgdelete -g cpuset:/crpc-test-f2-50
echo "@1"
sudo cgdelete -g cpu:/crpc-test-f2-50
echo "@2"
sudo cgdelete -g memory:/mem_crpc-test-f2-50
mkdir -p xxxx15
# echo "user is: $USER"
sudo cgcreate -g cpuset:/crpc-test-f2-50
echo "@3"
sudo cgcreate -g cpu:/crpc-test-f2-50
echo "@4"
sudo cgcreate -g memory:/mem_crpc-test-f2-50

trd=$1
version=$2
let yyml=trd+1
defvalue=1000
batch=${3:-$defvalue}
echo "$batch"
echo "@cp1"
s_version=1
slow_version=${4:-$s_version} # 1: slow cpu, 2: slow network, 3: memory contention
echo "@cp2"
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
echo "@cp3"
echo "$slow_version"
sudo cgset -r cpuset.cpus=$((5*yyml))-$((5*yyml+cpuset_cpus)) crpc-test-f2-50
echo "@5"

# uncomment
if [ $slow_version -eq 1 ]; then
    echo "50% slow version"
    let cpu_cfs_quota=$((50000 * (cpuset_cpus + 1))) # test with 25% slowdown
    # uncomment
    
    # uncomment
    sudo cgset -r cpu.cfs_quota_us=$cpu_cfs_quota crpc-test-f2-50
    echo "The cpu.cfs_quota_us value is: $(sudo cat /sys/fs/cgroup/cpu/crpc-test-f2-50/cpu.cfs_quota_us)"
elif [ $slow_version -eq 3 ]; then
    echo "Memory contention"
    sudo cgset -r memory.limit_in_bytes=500000000 mem_crpc-test-f2-50 # test with 50MB 50000000 memory
    echo "The memory.limit_in_bytes value is: $(sudo cat /sys/fs/cgroup/memory/mem_crpc-test-f2-50/memory.limit_in_bytes)"
fi
echo $trd
echo $yyml
echo $cpuset_cpus
# echo $cpu_cfs_quota

# uncomment
sudo cgexec -g cpu:crpc-test-f2-50 -g cpuset:crpc-test-f2-50 -g memory:mem_crpc-test-f2-50 ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 180 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p2 # > ./xxxx15/follower-$trd.log 2>&1 &

# sudo ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 180 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p2 # > ./xxxx15/follower-$trd.log 2>&1 &

# Uncomment the following line if you want to tail the log
# tail -f ./xxxx15/leader-$trd-$batch.log