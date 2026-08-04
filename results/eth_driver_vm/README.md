## Logs are sorted as follows
- Boardname_description_log - Most recent log
- Boardname_description_log_suffix - A log that should be saved with the reason to remember

Run `results`
- Boardname_description_results.csv - Throughput, results from the iq runner

Run `process`
- Boardname_description_util_summary.csv - Summary of utilisation

Run `post_process`
- Boardname_description_packets.csv - Per packet measurements

- Timing.txt 4 microbenchmarks using the timer to estimate cycles around 4 areas of interest (recv, sendto, rx copy, tx copy)