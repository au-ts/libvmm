import numpy as np
import pandas as pd
import sys

if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} <logfile>")
    sys.exit(1)

file_name = sys.argv[1]

f = open(file_name, 'r')

def get_data(f):
    lines = []
    for line in f:
        if line.strip() == "START_DATA":
            break

    for line in f:
        if line.strip() == "END_DATA":
            break
        lines.append(line.strip())
    return lines
def parse_data(data):
    d = {'No-Op Cycles': []}
    for l in data:
        d['No-Op Cycles'].append(int(l.split(':')[1]))
    return pd.DataFrame(d)

print("Reading data from", file_name)
data = get_data(f)
parsed = parse_data(data)


with pd.ExcelWriter(f'{"_".join(file_name.split("_")[1:])}.xlsx') as writer:
    parsed.to_excel(writer, index=False)