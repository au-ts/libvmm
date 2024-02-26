#!/usr/bin/env python3
import numpy as np

patch_f = open("logfile_patch", 'r')
no_patch_f = open("logfile_no_patch", 'r')

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

patch_d = get_data(patch_f)
no_patch_d = get_data(no_patch_f)


def compile_data(data):
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

print("\nWith Ivan's patch:")
print('-' * 100)
compile_data(patch_d)
print("\nWithout Ivan's patch:")
print('-' * 100)
compile_data(no_patch_d)
