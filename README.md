### fcp - fast cp, tool for benchmarking read/write operations on a file system


## options
- help (h) -> print help message
- input (i) -> path to input file
- output (o) -> path to output file
- raw (r) -> output only the amount of nanoseconds that copying took 
- threads (t) -> number of threads
- queue_depth (q) -> use async copying, specify number of chunks that will be independently enqueued
- legacy (l) -> when doing async copying, use libaio instead of liburing