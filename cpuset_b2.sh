#!/bin/bash
# sudo tc qdisc add dev eth0 root tbf rate 3gbit burst 15mb latency 1ms
sudo cgdelete -g cpuset:/crpc-test-f2-50
echo "@1"
sudo cgdelete -g cpu:/crpc-test-f2-50
echo "@2"
mkdir -p xxxx15
# echo "user is: $USER"
sudo cgcreate -g cpuset:/crpc-test-f2-50
echo "@3"
sudo cgcreate -g cpu:/crpc-test-f2-50
echo "@4"

trd=$1
version=$2
let yyml=trd+1
defvalue=1000
batch=${2:-$defvalue}

# if [ $trd -lt 8 ]; then
#   let cpuset_cpus=yyml+1
# elif [ $trd -lt 16 ]; then
#   let cpuset_cpus=yyml+2
# elif [ $trd -lt 24 ]; then
#   let cpuset_cpus=yyml+3
# else
#   let cpuset_cpus=yyml+4
# fi

if [ $trd -lt 8 ]; then
  let cpuset_cpus=yyml+2
elif [ $trd -lt 16 ]; then
  let cpuset_cpus=yyml+4
elif [ $trd -lt 24 ]; then
  let cpuset_cpus=yyml+6
else
  let cpuset_cpus=yyml+8
fi


# if [ $trd -lt 5 ]; then
#   let cpuset_cpus=yyml
#   let cpu_cfs_quota=50000*yyml
# else
#   let cpuset_cpus=yyml+2
#   let cpu_cfs_quota=50000*cpuset_cpus
# fi
let cpu_cfs_quota=50000*cpuset_cpus

sudo cgset -r cpuset.cpus=$((4*yyml))-$((4*yyml+cpuset_cpus)) crpc-test-f2-50
echo "@5"
sudo cgset -r cpu.cfs_quota_us=$cpu_cfs_quota crpc-test-f2-50
echo "The cpu.cfs_quota_us value is: $(sudo cat /sys/fs/cgroup/cpu/crpc-test-f2-50/cpu.cfs_quota_us)"

echo $trd
echo $yyml
echo $cpuset_cpus
echo $cpu_cfs_quota

sudo cgexec -g cpu:crpc-test-f2-50 -g cpuset:crpc-test-f2-50 ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 180 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p2 # > ./xxxx15/follower-$trd.log 2>&1 &


# Uncomment the following line if you want to tail the log
# tail -f ./xxxx15/leader-$trd-$batch.log