# Grace Hash Join (GHJ)

## Introduction
In Project 4, I implemented the Grace Hash Join (GHJ) algorithm using C++. This project simulates the data flow of records in disk and memory, and performs a join operation between two relations. The task was to implement two key phases of the GHJ algorithm: partitioning and probing.

## Starter Files
The provided starter files form the backbone of the project and include essential components like records, pages, disk, memory, buckets, and join functionality. My main task was to implement the partition and probe functions in the `Join.cpp` file.

### Key Files:
- `Record.hpp` / `Record.cpp`: Defines data records with two fields: key and data. These files also include important functions for partitioning (`partition_hash()`) and probing (`probe_hash()`).
- `Page.hpp` / `Page.cpp`: Defines the structure for emulated pages and includes functions for managing data records (e.g., `loadRecord()` and `loadPair()`).
- `Disk.hpp` / `Disk.cpp`: Provides disk functionality for reading data from text files into memory.
- `Mem.hpp` / `Mem.cpp`: Manages memory operations like loading pages from disk and flushing them back to disk.
- `Bucket.hpp` / `Bucket.cpp`: Stores the results of the partition phase, including relations mapped to specific buckets.
- `Join.hpp` / `Join.cpp`: Contains the two core functions, `partition()` and `probe()`, which implement the partitioning and probing phases of GHJ.

## Implementation Overview

### Partition Phase
The `partition()` function divides the records from the left and right relations into partitions based on a hash of their keys. These partitions are stored in buckets, which are further organized by disk page IDs.

### Probe Phase
The `probe()` function scans through the partitions and attempts to find matching records between the left and right relations based on the hash values. It then produces the final join result, which consists of the relevant disk page IDs.

## Constants
The project uses the following constants defined in `constants.hpp`:
- `RECORDS_PER_PAGE`: The maximum number of records in a page.
- `MEM_SIZE_IN_PAGE`: The size of memory in units of pages.
- `DISK_SIZE_IN_PAGE`: The size of disk in units of pages.

## Conclusion
This project helped me gain hands-on experience with implementing a fundamental database algorithm. It involved simulating how data is managed in both memory and disk, and how efficient partitioning and probing can optimize join operations. The project demonstrated the power of hash-based techniques in database management systems.

## Files Overview:
- **Record.hpp/Record.cpp**: Emulated data records.
- **Page.hpp/Page.cpp**: Management of data records on pages.
- **Disk.hpp/Disk.cpp**: Loading data from disk.
- **Mem.hpp/Mem.cpp**: Memory management and page operations.
- **Bucket.hpp/Bucket.cpp**: Storing partition results.
- **Join.hpp/Join.cpp**: Partition and probe functions.
- **constants.hpp**: Constant values used throughout the project.

## Key Reminders
 - Use the Record class’s member functions partition_hash() and probe_hash() for calculating the hash value of the record’s key in
 the partition and probe phases, respectively. 
 - When writing the memory page into disk, you do not need to consider which disk page you should write to. Instead, call the Mem
 class’s member function flushToDisk(Disk* d, unsigned int mem_page_id) , which will return the disk page id it writes to.
 - You can assume that any partition of the smaller relation will fit in the in-memory hash table. In other words, after applying the h2
 hash function, no bucket/partition will exceed one page. There is no need to perform a recursive hash. Here, “smaller relation” is
 defined as the relation with the fewer total number of records.
 - In the partition phase, do not store a record from the left relation and a record of the right relation in the same disk page. Do not
 store records for different buckets in the same disk page.
 - In the probe phase, for each page in the join result, fill in as many records as possible.
 - Do not make any optimizations even if one partition only involves the data from one relation.
 - Do not need to consider any parallel processing methods, including multithreading and multiprocessing, although one big
 advantage of GHJ is parallelism.
