import matplotlib.pyplot as plt
import numpy as np
import sys

def read_speeds_from_file(filename):
    with open(filename, 'r') as file:
        speeds = [int(line.strip()) for line in file]
    return speeds

def save_bar_graph(record_sizes, read_speeds, output_filename):
    y_pos = np.arange(len(record_sizes))

    bars = plt.bar(y_pos, read_speeds, align='center', alpha=0.7, color='b')
    
    for bar in bars:
        yval = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2.0, yval, int(yval), va='bottom', ha='center')  # Add text on top of the bars
    
    plt.xticks(y_pos, record_sizes)
    plt.ylabel('Read Speed (kB/s)')
    plt.xlabel('Record Size')
    plt.title('Block Device Benchmark')

    plt.savefig(output_filename)
    print(f"Graph saved as {output_filename}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <data_file> <output_graph>")
        sys.exit(1)

    data_file = sys.argv[1]
    output_file = sys.argv[2]

    record_sizes = ['4k', '16k', '512k', '1024k', '16384k']
    read_speeds = read_speeds_from_file(data_file)

    if len(read_speeds) != len(record_sizes):
        print("Error: The number of read speeds does not match the number of record sizes.")
        sys.exit(1)

    save_bar_graph(record_sizes, read_speeds, output_file)
