#!/bin/bash

# Check if an input file is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <input_file>"
    exit 1
fi

input_file="$1"

# Check if the input file exists
if [ ! -f "$input_file" ]; then
    echo "Error: Input file '$input_file' not found."
    exit 1
fi

# Extract latency values and calculate percentiles
latency_values=($(grep -oP 'latency: \K\d+ microseconds' "$input_file" | sed 's/ microseconds//'))
num_values=${#latency_values[@]}

# Check if there are any latency values
if [ $num_values -eq 0 ]; then
    echo "No latency values found in the input file."
    exit 1
fi

# Calculate mean
sum=0
for latency in "${latency_values[@]}"; do
    sum=$((sum + latency))
done
mean=$((sum / num_values))

# Sort the array
sorted_latencies=($(for latency in "${latency_values[@]}"; do echo "$latency"; done | sort -n))

# Save sorted latencies to a file
printf "%s\n" "${sorted_latencies[@]}" > sorted_latencies.txt

# Calculate percentiles with linear interpolation
p50="${sorted_latencies[num_values / 2]}"
p90="${sorted_latencies[num_values * 9 / 10]}"
p95="${sorted_latencies[num_values * 95 / 100]}"
p99_index=$((num_values * 99 / 100))
p99_lower="${sorted_latencies[p99_index]}"
p99_upper="${sorted_latencies[p99_index + 1]}"
p99=$((p99_lower + (p99_upper - p99_lower) / 2))
p999="${sorted_latencies[num_values * 999 / 1000]}"

# Display results
echo "Mean: $mean microseconds"
echo "Median (P50): $p50 microseconds"
echo "P90: $p90 microseconds"
echo "P95: $p95 microseconds"
echo "P99: $p99 microseconds"
echo "P99.9: $p999 microseconds"