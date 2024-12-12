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
```cpp
/*
 * Input: Disk, Memory, Disk page ids for left relation, Disk page ids for right relation
 * Output: Vector of Buckets of size (MEM_SIZE_IN_PAGE - 1) after partition
 */


vector<Bucket> partition(Disk* disk, Mem* mem, pair<uint, uint> left_rel,
                         pair<uint, uint> right_rel) {
    // Step 1: Initialize the vector of Buckets
    vector<Bucket> partitions(MEM_SIZE_IN_PAGE - 1, Bucket(disk));
    
    // Step 2: Partition the left relation
    for (uint page_id = left_rel.first; page_id < left_rel.second; ++page_id) {
        // Load page from disk into memory at memory page 0
		// Memory page 0 is the input page, that is why B-1
        mem->mem_page(0)->reset();
        mem->loadFromDisk(disk, page_id, 0);
        Page* mem_page = mem->mem_page(0);

        // Process each record in the page
        for (uint i = 0; i < mem_page->size(); ++i) {
            Record record = mem_page->get_record(i);
            uint partition_index = record.partition_hash() % (MEM_SIZE_IN_PAGE - 1);
			
			// Add the record to the corresponding page in mem
			mem->mem_page(partition_index + 1)->loadRecord(record);

			// Check if the page is full, if so, flush it to disk and add new_page_id to bucket
			if (mem->mem_page(partition_index + 1)->full()) {
				uint new_page_id =  mem->flushToDisk(disk, partition_index + 1);
				partitions[partition_index].add_left_rel_page(new_page_id);
			}
        }
    }

    // Flush the remaining records in the memory page to disk
	for (uint i = 0; i < partitions.size(); ++i) {
		if (!mem->mem_page(i + 1)->empty()) {
			uint new_page_id =  mem->flushToDisk(disk, i + 1);
			partitions[i].add_left_rel_page(new_page_id);
            mem->mem_page(i + 1)->reset();
		}
	}

    // Step 3: Partition the right relation
    for (uint page_id = right_rel.first; page_id < right_rel.second; ++page_id) {
        // Load page from disk into memory at memory page 0
        mem->mem_page(0)->reset();
        mem->loadFromDisk(disk, page_id, 0);
        Page* mem_page = mem->mem_page(0);

        // Process each record in the page
        for (uint i = 0; i < mem_page->size(); ++i) {
            Record record = mem_page->get_record(i);
            uint partition_index = record.partition_hash() % (MEM_SIZE_IN_PAGE - 1);
			
			// Add the record to the corresponding page in mem
			mem->mem_page(partition_index + 1)->loadRecord(record);

			// Check if the page is full, if so, flush it to disk and add new_page_id to bucket
			if (mem->mem_page(partition_index + 1)->full()) {
				uint new_page_id =  mem->flushToDisk(disk, partition_index + 1);
				partitions[partition_index].add_right_rel_page(new_page_id);
			}
        }
    }

	// Flush the remaining records in the memory page to disk
	for (uint i = 0; i < partitions.size(); ++i) {
		if (!mem->mem_page(i + 1)->empty()) {
			uint new_page_id =  mem->flushToDisk(disk, i + 1);
			partitions[i].add_right_rel_page(new_page_id);
            mem->mem_page(i + 1)->reset();
		}
	}
    return partitions;
}
```

### Probe Phase
The `probe()` function scans through the partitions and attempts to find matching records between the left and right relations based on the hash values. It then produces the final join result, which consists of the relevant disk page IDs.
```cpp
/*
 * Input: Disk, Memory, Vector of Buckets after partition
 * Output: Vector of disk page ids for join result
 */
vector<uint> probe(Disk* disk, Mem* mem, vector<Bucket>& partitions) {
    vector<uint> disk_pages;

    Page* output_page = mem->mem_page(1);

    // Iterate through each bucket
    for (Bucket& bucket : partitions) {
        // Get left and right relation page IDs from the bucket
        vector<uint> left_pages = bucket.get_left_rel();
        vector<uint> right_pages = bucket.get_right_rel();
        
        if (left_pages.size() >= right_pages.size()){
            left_pages = bucket.get_right_rel();
            right_pages = bucket.get_left_rel();
        }

        for (uint i = 0; i < MEM_SIZE_IN_PAGE - 2; ++i) {
            mem->mem_page(i + 2)->reset();
        }

        // Hash all left relation records into memory (B-2 buckets)
        for (uint left_page_id : left_pages) {
            mem->mem_page(0)->reset();
            mem->loadFromDisk(disk, left_page_id, 0);  // Load left page into mem_page(0)
            Page* left_page = mem->mem_page(0);

            for (uint i = 0; i < left_page->size(); ++i) {
                Record left_record = left_page->get_record(i);
                uint hash_index = left_record.probe_hash() % (MEM_SIZE_IN_PAGE - 2);

                // Add the record to the corresponding hashed page in memory
                Page* hash_page = mem->mem_page(hash_index + 2);
                hash_page->loadRecord(left_record);
            }
        }
        
        // Iterate through right relation and find matches

        for (uint right_page_id : right_pages) {
            mem->mem_page(0)->reset();
            mem->loadFromDisk(disk, right_page_id, 0);  // Load right page into mem_page(0)
            Page* right_page = mem->mem_page(0);

            for (uint i = 0; i < right_page->size(); ++i) {
                Record right_record = right_page->get_record(i);
                uint hash_index = right_record.probe_hash() % (MEM_SIZE_IN_PAGE - 2);

                // Compare with hashed records in the corresponding memory page
                Page* hash_page = mem->mem_page(hash_index + 2);

                for (uint j = 0; j < hash_page->size(); ++j) {
                    Record left_record = hash_page->get_record(j);

                    if (left_record == right_record) {
                        // Flush output page to disk if full
                        if (output_page->full()) {
                            uint new_disk_page_id = mem->flushToDisk(disk, 1);
                            disk_pages.push_back(new_disk_page_id);
                            output_page->reset();
                        }

                        // If records match, add to output page (mem_page(1))
                        output_page->loadPair(left_record, right_record);

                    }
                }
            }
        }
    }
    
    // Flush any remaining records in the output page
    if (!output_page->empty()) {
        uint new_disk_page_id = mem->flushToDisk(disk, 1);
        disk_pages.push_back(new_disk_page_id);
        output_page->reset();
    }

    // Reset all hash pages in memory
    for (uint i = 2; i < MEM_SIZE_IN_PAGE; ++i) {
        mem->mem_page(i)->reset();
    }
    
    return disk_pages;
}
```

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
