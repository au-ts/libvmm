import numpy as np
import pandas as pd
import sys



# READ every line between thelines that say START_RESULTS and END_RESULTS
# and store them in a list
def get_data(f):
    lines = []
    for line in f:
        if line.strip() == "START_RESULTS":
            break

    for line in f:
        if line.strip() == "END_RESULTS":
            break
        lines.append(line.strip())
    return lines

def parse_data(data):
    parsed = []
    for l in data:
        l = l.split('|')
        l = l[1:-1]
        l = [x.strip() for x in l]
        l = [x.split(':')[1].strip() for x in l]
        l[-1] = int(l[-1])
        if l[2] == 0:
            continue
        parsed.append({'Fault': l[0], 'Event': l[1], 'Cycles': l[2]})
        
    return parsed

def segment_data(parsed):
    total_pd_data = []
    # Get VGICMaintenance Data
    vgic = [x for x in parsed if x['Fault'] == 'VGICMaintenance']
    # Every 2 data points is a full vgic call
    vgic_data = {'total': [], 'kernel': [], 'user': []}
    for i in range(0, len(vgic), 2):
        vgic_data['total'].append(vgic[i+1]['Cycles'])
        vgic_data['kernel'].append(vgic[i]['Cycles'])
        vgic_data['user'].append(vgic[i + 1]['Cycles'] - vgic[i]['Cycles'])
    
    total_pd_data.append(pd.DataFrame(vgic_data))
    
    # Get VPPIEvent Data
    vppi = [x for x in parsed if x['Fault'] == 'VPPIEvent']
    vppi_data = {'total': [], 'kernel': [], 'user': []}
    for i in range(0, len(vppi), 2):
        vppi_data['total'].append(vppi[i+1]['Cycles'])
        vppi_data['kernel'].append(vppi[i]['Cycles'])
        vppi_data['user'].append(vppi[i + 1]['Cycles'] - vppi[i]['Cycles'])
    total_pd_data.append(pd.DataFrame(vppi_data))

    # Get VMFault Data
    vmf = [x for x in parsed if x['Fault'] == 'VMFault']
    vmf_data = {'read': [], 'write': []}
    vmf_read = []
    vmf_write = []
    for i in range(len(vmf)):
        if vmf[i]['Event'] == 'TCB_ReadRegisters':
            vmf_read.append(vmf[i]['Cycles'])
        else:
            vmf_write.append(vmf[i]['Cycles'])
    total_pd_data.append(pd.DataFrame(vmf_read, columns=['read']))
    total_pd_data.append(pd.DataFrame(vmf_write, columns=['write']))

    vmf_total_data = {'total': [], 'read': [], 'write': [], 'user': []}
    for i in range(0, len(vmf), 3):
        if i + 2 >= len(vmf):
            break
        vmf_total_data['total'].append(vmf[i+2]['Cycles'])
        vmf_total_data['read'].append(vmf[i]['Cycles'])
        vmf_total_data['write'].append(vmf[i+1]['Cycles'])
        vmf_total_data['user'].append(vmf[i+2]['Cycles'] - vmf[i]['Cycles'] - vmf[i+1]['Cycles'])
    total_pd_data.append(pd.DataFrame(vmf_total_data))


    return total_pd_data

def save_data(data, data_names, output_file):
    # Save to excel file
    with pd.ExcelWriter(output_file) as writer:
        for i, d in enumerate(data):
            d.to_excel(writer, sheet_name=data_names[i], index=False)



def process_and_save_data(data, output_file):
    parsed = parse_data(data)
    final_data = segment_data(parsed)

    data_names = ['VGICMaintenance', 'VPPIEvent', 'VMFault_READ', 'VMFault_WRITE', 'VMFault_TOTAL']

    print("Saving data to excel file...")
    save_data(final_data, data_names, output_file)


def print_data(data):
    d = []

    for l in data:
        l = l.split('|')
        l = l[1:-1]
        l = [x.strip() for x in l]
        l = [x.split(':')[1].strip() for x in l]
        l[-1] = int(l[-1])
        d.append(l)

    d = d[:-1]

    # Convert the list of lists to a numpy array
    d = np.array(d)

    user_faults = ['VGICMaintenance', 'VPPIEvent']
    print("Average cycles for following user faults:")
    # Get the average cycle count for each fault where the second column is 'LibVMM_Total_Event'
    for fault in user_faults:
        check = ((d[:,1] == 'LibVMM_Total_Event') & (d[:,0] == fault))
        total_avg = np.mean(d[check,2].astype(int))

        check = ((d[:,1] != 'LibVMM_Total_Event') & (d[:,0] == fault))
        kernel_avg = np.mean(d[check,2].astype(int))

        print('\t', fault, 'Kernel Time:', f'{kernel_avg:.2f}', 'Total Time:', f'{total_avg:.2f}', 'User Time:', f'{total_avg - kernel_avg:.2f}')

        # sd and median of total time
        check = ((d[:,1] == 'LibVMM_Total_Event') & (d[:,0] == fault))
        total_std = np.std(d[check,2].astype(int))
        total_median = np.median(d[check,2].astype(int))

        print('\t\t', 'sd', f'{total_std:.2f}', 'Median', total_median)


    vmm_faults = ['VMFault', 'VCPUFault']
    print("Average cycles for following VMM faults:")
    for fault in vmm_faults:
        avg = np.mean(d[d[:,0] == fault,2].astype(int))

        #sd and median
        std = np.std(d[d[:,0] == fault,2].astype(int))
        median = np.median(d[d[:,0] == fault,2].astype(int))
        print('\t', fault, 'Mean', f'{avg:.2f}', 'sd', f'{std:.2f}', 'Median', median)

    # Get the unique values of the second column
    timed_events = np.unique(d[:,1])
    timed_events = np.delete(timed_events, np.argwhere(timed_events=='LibVMM_Total_Event'))
    # Get the average cycle count (value of third column) for each unique value
    print("Average cycles for following timed syscalls:")
    for event in timed_events:
        avg = np.mean(d[d[:,1] == event,2].astype(int))
        # sd and median
        std = np.std(d[d[:,1] == event,2].astype(int))
        median = np.median(d[d[:,1] == event,2].astype(int))
        print('\t', event, 'Mean', f'{avg:.2f}', 'sd', f'{std:.2f}', 'Median', median)



def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <logfile> <mode: print or excel (default print)> <output_file for excel>")
        sys.exit(1)

    file_name = sys.argv[1]
    mode = 'print'

    if len(sys.argv) > 2:
        mode = sys.argv[2]

    if mode not in ['print', 'excel']:
        print("Mode must be either 'print' or 'excel'")
        sys.exit(1)
    
    output_file = file_name

    if len(sys.argv) == 4:
        output_file = sys.argv[3]

    if not output_file.endswith(".xlsx"):
        output_file += ".xlsx"


    print("Reading data from", file_name)
    f = open(file_name, 'r')
    data = get_data(f)

    if mode == 'print':
        print_data(data)
    else:
        process_and_save_data(data, output_file)
        
main()