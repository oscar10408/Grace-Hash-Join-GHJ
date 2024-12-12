#include "Join.hpp"
#include "Join.hpp"
#include "Record.hpp"
#include "Mem.hpp"
#include "Disk.hpp"
#include "Bucket.hpp"
#include <vector>
#include <vector>
#include <iostream>

using namespace std;

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

