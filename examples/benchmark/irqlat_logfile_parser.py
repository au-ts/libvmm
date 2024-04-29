
import numpy as np
import pandas as pd
import sys



# READ every line between thelines that say START DATA and END DATA
# and store them in a list
def get_data(f):
    lines = []
    for line in f:
        if line.strip() == "START DATA":
            break

    for line in f:
        if line.strip() == "END DATA":
            break
        lines.append(line.strip())
    lines = lines[1:]
    dividers = []
    # Find where all the -------------------------------- are
    for i in range(len(lines)):
        if lines[i] == "--------------------------------":
            dividers.append(i)
    data = []
    # data.append(lines[0:dividers[0]])
    data.append(lines[dividers[0]+1:dividers[1]])
    data.append(lines[dividers[1]+1:dividers[2]])
    data.append(lines[dividers[2]+1:dividers[3]])
    data.append(lines[dividers[3]+1:])

    full_pd_data = []
    data_i = 0
    for d in data:
        curr_dict_data = {}
        headings = d[0].split()
        for h in headings:
            curr_dict_data[h] = []
        for i in range(1, len(d)):
            for j, h in enumerate(headings):
                curr_dict_data[h].append(d[i].split()[j])
        pd_data = pd.DataFrame(curr_dict_data)
        full_pd_data.append(pd_data)
        data_i += 1
    return full_pd_data  

        

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <logfile> <output_excel name (optional)>")
        sys.exit(1)


    file_name = sys.argv[1]
    output_excel = file_name

    if len(sys.argv) == 3:
        output_excel = sys.argv[2]

    if not output_excel.endswith(".xlsx"):
        output_excel += ".xlsx"

    try:
        f = open(file_name, 'r')
    except: 
        print("Could not open file", file_name)
        sys.exit(1)

    data_types = ["Cache", "TLB", "Bus", "Final"]



    print("Reading data from", file_name)
    data = get_data(f)

    print("Saving data to excel file:", output_excel)
    with pd.ExcelWriter(output_excel) as writer:
        for i, d in enumerate(data):
            d.to_excel(writer, sheet_name=data_types[i], index=False)

main()