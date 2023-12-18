import re
import sys
import numpy as np

def parse_iozone_results(input_files, output_file):
    aggregated_results = {}

    # Process each file
    for file_path in input_files:
        with open(file_path, 'r') as file:
            iozone_results = file.read()

        matches = re.findall(r"\b\d+\s+(\d+)\s+(\d+)\s+\d+\s+(\d+)", iozone_results)

        for reclen, write, read in matches:
            if reclen not in aggregated_results:
                aggregated_results[reclen] = {"write": [], "read": []}
            
            aggregated_results[reclen]["write"].append(int(write))
            aggregated_results[reclen]["read"].append(int(read))

    # Writing results with raw data and statistics
    with open(output_file, 'w') as file:
        file.write("Record Length, Write: num1, num2, ..., Avg, Std, Read: num1, num2, ..., Avg, Std\n")
        for reclen, data in sorted(aggregated_results.items(), key=lambda x: int(x[0])):
            write_values = data["write"]
            read_values = data["read"]

            write_avg = np.mean(write_values)
            write_std = np.std(write_values)
            read_avg = np.mean(read_values)
            read_std = np.std(read_values)

            write_values_str = ', '.join(map(str, write_values))
            read_values_str = ', '.join(map(str, read_values))

            line = f"{reclen}, Write: {write_values_str}, {write_avg}, {write_std}, Read: {read_values_str}, {read_avg}, {read_std}\n"
            file.write(line)

if len(sys.argv) < 3:
    print("Usage: <output_file> <input_file1> <input_file2> ...")
    sys.exit(1)

output_file = sys.argv[1]
input_files = sys.argv[2:]

parse_iozone_results(input_files, output_file)
