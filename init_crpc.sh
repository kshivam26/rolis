#!/bin/bash
# sudo tc qdisc add dev eth0 root tbf rate 3gbit burst 15mb latency 1ms

echo "@@@1"
# bash install.sh
make paxos >& temp.txt
# make paxos
echo "@@@2"

bash multi.sh >& temp.txt
# bash multi.sh
echo "@@@3"