import subprocess
import json

# Create Docker network with the specified subnet
# create_network_command = "sudo docker network create my_network --subnet=172.17.2.0/24"
# subprocess.run(create_network_command, shell=True)

# Docker network and IP information
network_name = "my_network"
ip_addresses = ["172.19.0.11", "172.19.0.12", "172.19.0.13"]  # Replace with the IP addresses you want to stop

# Get container IDs and IPs within the network
inspect_command = f"sudo docker network inspect {network_name}"
output = subprocess.check_output(inspect_command, shell=True)
network_info = json.loads(output.decode("utf-8"))

# Find and stop containers with specified IPs
for ip in ip_addresses:
    for container in network_info[0]['Containers'].values():
        if container['IPv4Address'].startswith(ip):
            container_id = container['Name']
            stop_command = f"docker stop {container_id}"
            subprocess.run(stop_command, shell=True)
    
# Define the Docker commands for each script
docker_commands = [
    "sudo docker run -t -d --net my_network --ip 172.19.0.11 -it --cap-add=NET_ADMIN -v /home/users/kkumar/rolis_new/rolis:/root/rolis rolis_ks /root/rolis/cpuset_b0.sh 4 1",
    "sudo docker run -d --net my_network --ip 172.19.0.12 -it --cap-add=NET_ADMIN -v /home/users/kkumar/rolis_new/rolis:/root/rolis rolis_ks /root/rolis/cpuset_b1.sh 4",
    "sudo docker run -d --net my_network --ip 172.19.0.13 -it --cap-add=NET_ADMIN -v /home/users/kkumar/rolis_new/rolis:/root/rolis rolis_ks /root/rolis/cpuset_b2.sh 4"
]

# Run Docker containers for each script concurrently
processes = []
log_files = []  # To store the names of log files

for idx, command in enumerate(docker_commands):
    stdout_log = f"command_{idx + 1}_stdout.log"
    stderr_log = f"command_{idx + 1}_stderr.log"
    log_files.extend([stdout_log, stderr_log])  # Store the log file names for later reference
    
    stdout_file = open(stdout_log, "w")
    stderr_file = open(stderr_log, "w")
    
    process = subprocess.Popen(command, shell=True, stdout=stdout_file, stderr=stderr_file, text=True)
    processes.append(process)

# Wait for all processes to finish
for process in processes:
    process.wait()

# # Close log files
# for log_file in log_files:
#     log_file.close()

# Print log file names for reference
print("Log files created:")
for log_file in log_files:
    print(log_file)