#!/bin/bash

# List containers named container1, container2, or container3

stop_containers(){
existing_containers=$(sudo docker ps -a -q --filter "name=chain_l|chain_f1|chain_f2")

echo "$existing_containers"

# If any found, stop and remove them
if [[ -n "$existing_containers" ]]; then
  echo "Stopping and removing existing containers..."
  sudo docker stop $existing_containers
  sudo docker rm $existing_containers
fi

echo "1*****"
}

stop_containers

