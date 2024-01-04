#!/bin/bash
# sudo tc qdisc add dev eth0 root tbf rate 3gbit burst 15mb latency 1ms

#!/bin/bash

# Check if there is at least one argument
if [ $# -lt 1 ]; then
    echo "Usage: $0 <argument>"
    exit 1
fi

# Get the first argument
arg=$1

# Perform different actions based on the argument
if [ $arg = 1 ]; then
    # Command for condition 1
    echo "Executing command for condition 1"
    sudo tc qdisc add dev eth0 root tbf rate 1gbit burst 15mb latency 1ms
    # Add your command here
elif [ $arg = 3 ]; then
    # Command for condition 2
    echo "Executing command for condition 2"
    # Add your command here
    sudo tc qdisc add dev eth0 root tbf rate 3gbit burst 15mb latency 1ms
elif [ $arg = 5 ]; then
    # Command for condition 3
    echo "Executing command for condition 3"
    # Add your command here
    sudo tc qdisc add dev eth0 root tbf rate 5gbit burst 15mb latency 1ms
else
    # Default action if the argument doesn't match any condition
    echo "Unknown argument: $arg"
    sudo tc qdisc add dev eth0 root tbf rate 10gbit burst 15mb latency 1ms
fi