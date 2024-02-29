import csv
import math
import os
import sys

def bucketize_values(average_values):
    buckets = {f"{i}-{i+1}": 0 for i in range(0, 100)}

    # Handle values greater than or equal to 100
    buckets["100-101"] = 0

    for value in average_values:
        if math.isnan(value):
            continue  # Skip NaN values, if any
        elif value >= 100:
            buckets["100-101"] += 1
        else:
            bucket = f"{int(value)}-{int(value)+1}"
            buckets[bucket] += 1

    # print ("buckets for cpu_util: ", buckets)
    return buckets

def find_highest_count_bucket(buckets):
    max_bucket = max(buckets, key=buckets.get)
    return max_bucket

def main(input_folder):
    file_path = os.path.join(input_folder, "output_cpu_util.csv")

    average_values = []

    # Read average values from the output.csv file
    with open(file_path, newline='') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            # Assuming the average total CPU usage is in the first column
            average_values.append(float(row[0]))

    # Bucketize the values
    buckets = bucketize_values(average_values)

    # Find the bucket with the highest count
    max_bucket = find_highest_count_bucket(buckets)

    # Calculate and print the midpoint of the highest count bucket
    start, end = map(int, max_bucket.split('-'))
    midpoint = (start + end) / 2
    if (midpoint > 100):
        midpoint = 100
    print(midpoint)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python analyze_output.py <input_folder>")
        sys.exit(1)

    input_folder = sys.argv[1]
    main(input_folder)
