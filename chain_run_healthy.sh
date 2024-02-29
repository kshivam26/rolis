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

start_containers(){
# Start containers (replace with your container names/IDs and images)
sudo docker run -d --name chain_l --net my_network --ip 172.19.0.11 -it --cap-add=NET_ADMIN --cap-add=SYS_ADMIN -v /sys/fs/cgroup:/sys/fs/cgroup -v /home/users/kkumar/rolis_new/rolis:/root/rolis crpc_rolis
sudo docker run -d --name chain_f1 --net my_network --ip 172.19.0.12 -it --cap-add=NET_ADMIN --cap-add=SYS_ADMIN -v /sys/fs/cgroup:/sys/fs/cgroup -v /home/users/kkumar/rolis_new/rolis:/root/rolis crpc_rolis
sudo docker run -d --name chain_f2 --net my_network --ip 172.19.0.13 -it --cap-add=NET_ADMIN --cap-add=SYS_ADMIN -v /sys/fs/cgroup:/sys/fs/cgroup -v /home/users/kkumar/rolis_new/rolis:/root/rolis crpc_rolis
}

start_containers

build_code(){
echo "building the code"
sudo docker exec -i chain_l /root/rolis/init_crpc.sh
echo "now going to run the crpc system"
}

build_code

# Define arrays containing parameter values
num_threads=(12)   # Replace these values with your actual num_threads values
crpc_options=(0)         # Replace these values with your actual crpc_options values
bw_options=(3) # for bw 5, #threads=15, the bw for dynamic doesn't reach more than 3.1gbps and the leader cpu is fully utilized
MAX_RETRIES=3

echo "cp1_chain_run"
# evaluation_file="crpc_evaluation/evaluation.txt"
for bw_option in "${bw_options[@]}"; do
# Iterate through each combination of parameters
  for thread_value in "${num_threads[@]}"; do
      for option_value in "${crpc_options[@]}"; do        
        if [ "$bw_option" -eq 1 ] && [$thread_value > 6]; then
            # Certain conditions for option 1
            echo "Option 1: Continuing certain conditions"
            # Perform actions for option 1
            continue  # This continues to the next iteration of the loop
        elif [ "$bw_option" -eq 3 ] && [$thread_value > 12]; then
            # Certain conditions for option 2
            echo "Option 2: Checking conditions for continue"
            continue
            # Perform actions for option 2
        elif [ "$bw_option" -eq 5 ] && [$thread_value > 20]; then
            # Certain conditions for option 3
            echo "Option 3: Performing actions"
            continue
            # Perform actions for option 3
        else
            # Other conditions not covered above
            echo "Other option: $bw_option"
            # Perform actions for other options
        fi

        for ((i = 1; i <= 3; i++)); do
          throughput_value=""  # Initialize throughput value
          retries=0
          # Retry the iteration until throughput_value is obtained
          while [[ -z $throughput_value ]]; do
            sudo docker exec -i chain_l /root/rolis/init_crpc_bw.sh "$bw_option"
            echo "now executing the system with $thread_value and $option_value paramters"

            # Define the output file path inside the output directory based on thread_value and option_value
            output_file="crpc_evaluation/leader_output_${bw_option}_${thread_value}_${option_value}.txt"

            # Execute commands with the parameters
            timeout 4m sudo docker exec -i chain_l /root/rolis/b0.sh "$thread_value" "$option_value" >& $output_file &
            pid1=$!
            echo "$pid1"

            timeout 4m sudo docker exec -i chain_f1 /root/rolis/b1.sh "$thread_value" "$option_value" >&  crpc_evaluation/f1.txt &
            pid2=$!
            echo "$pid2"

            timeout 4m sudo docker exec -i chain_f2 /root/rolis/b2.sh "$thread_value" "$option_value" >& crpc_evaluation/f2.txt&
            pid3=$!
            echo "$pid3"

            # Wait for these processes to complete
            # wait $pid1 || wait $pid2 || wait $pid3
            # Wait for either pid1, pid2, or pid3 to finish/abort
            while : ; do
                if ! kill -0 $pid1 2>/dev/null; then
                    echo "pid1 ($pid1) finished"
                    break
                elif ! kill -0 $pid2 2>/dev/null; then
                    echo "pid2 ($pid2) finished"
                    break
                elif ! kill -0 $pid3 2>/dev/null; then
                    echo "pid3 ($pid3) finished"
                    break
                fi
                sleep 1
            done
            echo "leader process done"
            kill -- -$pid1
            kill -- -$pid2
            kill -- -$pid3
            echo "Killed processes with pid: $pid2 and $pid3"

            # Get throughput value from the output file
            throughput_value=$(grep -oP 'throughput without warmup and cool-down: \K[\d.]+' "$output_file")
          
            # If throughput value is not found, print a message and retry the iteration
            # Increment retries if throughput value is not found
            if [[ -z $throughput_value ]]; then
                ((retries++))
                if (( retries >= MAX_RETRIES )); then
                    echo "Max retries reached. Thread: $thread_value, Option: $option_value"
                    break
                else
                    echo "Throughput value not found. Retrying for thread: $thread_value, option: $option_value (Retry: $retries)"
                fi
            fi
            
            echo "throughput value: $throughput_value"
            stop_containers
            start_containers
            build_code
          done
        
          evaluation_file="crpc_evaluation/evaluation_healthy_${bw_option}_gbps.txt"
          # Write evaluation data to the evaluation file
          echo "${thread_value},${option_value},${throughput_value}" >> "$evaluation_file"
        done
        
      done
  done
done

