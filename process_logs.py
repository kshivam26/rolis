import re
from datetime import datetime

pattern = re.compile(r"I \[.*\] (\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+) \| \+\+\+\+ par_id: (\d+), temp_dir_1_lat: (\d+\.\d+); temp_dir_2_lat: (\d+\.\d+); temp_dir_1_lat ([><]) temp_dir_2_lat")

# Dictionary to store lines for each par_id
par_id_lines = {}

# Open the log file
with open('result_leader.txt', 'r') as file:
    for line in file:
        # Check if the line matches the pattern
        match = pattern.search(line)
        if match:
            timestamp_str = match.group(1)
            timestamp = datetime.strptime(timestamp_str, "%Y-%m-%d %H:%M:%S.%f")
            timestamp_formatted = timestamp.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]  # Strip off extra zeros
            par_id = match.group(2)
            temp_dir_1_lat = round(float(match.group(3)))
            temp_dir_2_lat = round(float(match.group(4)))
            comparison_sign = match.group(5)

            # Calculate the difference based on the comparison sign and round it
            if comparison_sign == '>':
                difference = round(temp_dir_1_lat / temp_dir_2_lat)
            else:
                difference = round(-(temp_dir_2_lat / temp_dir_1_lat))  # Adding negative sign for < comparison
            
            # Look for direction_match pattern in the next line
            next_line = next(file, None)
            direction_match = re.search(r"par_id: (\d+), current direction_prob is: (\d+\.\d+)", next_line) if next_line else None
            # if
            
            # If direction_match found, extract direction_prob and append to the output line
            if direction_match:
                direction_prob = direction_match.group(2)
                output_line = f"timestamp: {timestamp_formatted}, temp_dir_1_lat: {temp_dir_1_lat}, temp_dir_2_lat: {temp_dir_2_lat}, difference: {difference}, direction_prob: {direction_prob}"
            else:
                output_line = f"timestamp: {timestamp_formatted}, temp_dir_1_lat: {temp_dir_1_lat}, temp_dir_2_lat: {temp_dir_2_lat}, difference: {difference}"

            # Create or append to the list of lines for each par_id
            if par_id in par_id_lines:
                par_id_lines[par_id].append(output_line)
            else:
                par_id_lines[par_id] = [output_line]
print ("cp")
# Output the calculated differences for each par_id
for par_id, lines in par_id_lines.items():
    print(f"par_id: {par_id}")
    for line in lines:
        print(line)
    print()

