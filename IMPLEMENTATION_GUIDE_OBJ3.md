etet # Implementation Guide - Objective 3: Index Building Comparison

## Overview
Build and compare three methods for creating a B+ tree index on the Student file using the AM (Access Method) layer.

## Index Structure

Build B+ tree index on `roll_no` field (primary key).

### Key Properties
- **Key Type**: String (char[20])
- **Key Comparison**: Lexicographic (strcmp)
- **Order**: Configurable (e.g., order = 50 means max 50 keys per node)

## AM Layer Functions

The existing AM layer provides:

```c
// Create/Open/Close index
int AM_CreateIndex(char *filename, int keytype, int keylen);
int AM_OpenIndex(char *filename);
int AM_CloseIndex(int indexId);

// Insert/Delete
int AM_InsertEntry(int indexId, char *key, RecordID rid);
int AM_DeleteEntry(int indexId, char *key, RecordID rid);

// Search
int AM_FindEntry(int indexId, char *key, RecordID *rid);
```

## Method 1: Incremental Inserts (Naive)

### Description
Insert records one-by-one in the order they appear in the file. This is the simplest approach but can lead to a poorly balanced tree with many page splits.

### Implementation

```c
// method1_incremental.c

#include "am.h"
#include "student_file.h"

int build_index_incremental(char *index_file, char *data_file) {
    int index_id;
    StudentFile sf;
    SF_ScanHandle scan;
    StudentRecord rec;
    RecordID rid;
    int count = 0;
    
    // Create index
    AM_CreateIndex(index_file, STRING_TYPE, 20);
    index_id = AM_OpenIndex(index_file);
    
    // Open student file
    SF_OpenFile(data_file, &sf);
    SF_OpenScan(&scan, &sf);
    
    // Statistics
    clock_t start = clock();
    long page_accesses_before = PF_GetPageAccesses();
    
    // Insert each record into index
    while (SF_GetNextStudent(&scan, &rec) == SP_OK) {
        // Get RecordID from current scan position
        rid.page_num = scan.sp_handle.curr_page;
        rid.slot_num = scan.sp_handle.curr_slot;
        
        // Insert into index
        AM_InsertEntry(index_id, rec.roll_no, rid);
        
        count++;
        if (count % 1000 == 0) {
            printf("Inserted %d keys\n", count);
        }
    }
    
    clock_t end = clock();
    long page_accesses_after = PF_GetPageAccesses();
    
    // Print statistics
    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;
    long total_accesses = page_accesses_after - page_accesses_before;
    
    printf("Method 1 - Incremental Inserts:\n");
    printf("  Records indexed: %d\n", count);
    printf("  Time taken: %.2f seconds\n", time_taken);
    printf("  Page accesses: %ld\n", total_accesses);
    printf("  Avg accesses per insert: %.2f\n", 
           (double)total_accesses / count);
    
    SF_CloseScan(&scan);
    SF_CloseFile(&sf);
    AM_CloseIndex(index_id);
    
    return 0;
}
```

### Expected Behavior
- Many page splits during insertion
- Tree may not be optimally balanced
- High number of page I/Os
- Tree height may be suboptimal

## Method 2: Bulk Creation (Sorted First)

### Description
1. Read all records and sort by roll_no
2. Insert sorted keys into B+ tree
3. This reduces splits and creates a more balanced tree

### Implementation

```c
// method2_bulk_sorted.c

typedef struct {
    char roll_no[20];
    RecordID rid;
} IndexEntry;

int compare_entries(const void *a, const void *b) {
    IndexEntry *ea = (IndexEntry *)a;
    IndexEntry *eb = (IndexEntry *)b;
    return strcmp(ea->roll_no, eb->roll_no);
}

int build_index_bulk_sorted(char *index_file, char *data_file) {
    StudentFile sf;
    SF_ScanHandle scan;
    StudentRecord rec;
    RecordID rid;
    
    // Step 1: Load all entries into memory
    printf("Loading all records...\n");
    
    // First pass: count records
    SF_OpenFile(data_file, &sf);
    int num_records = sf.num_records;
    
    // Allocate array
    IndexEntry *entries = malloc(num_records * sizeof(IndexEntry));
    int count = 0;
    
    // Load entries
    SF_OpenScan(&scan, &sf);
    while (SF_GetNextStudent(&scan, &rec) == SP_OK) {
        strcpy(entries[count].roll_no, rec.roll_no);
        entries[count].rid.page_num = scan.sp_handle.curr_page;
        entries[count].rid.slot_num = scan.sp_handle.curr_slot;
        count++;
    }
    SF_CloseScan(&scan);
    SF_CloseFile(&sf);
    
    printf("Loaded %d records\n", count);
    
    // Step 2: Sort entries
    printf("Sorting entries...\n");
    clock_t sort_start = clock();
    qsort(entries, count, sizeof(IndexEntry), compare_entries);
    clock_t sort_end = clock();
    double sort_time = (double)(sort_end - sort_start) / CLOCKS_PER_SEC;
    printf("Sorted in %.2f seconds\n", sort_time);
    
    // Step 3: Create index and insert sorted entries
    printf("Building index...\n");
    AM_CreateIndex(index_file, STRING_TYPE, 20);
    int index_id = AM_OpenIndex(index_file);
    
    clock_t insert_start = clock();
    long page_accesses_before = PF_GetPageAccesses();
    
    for (int i = 0; i < count; i++) {
        AM_InsertEntry(index_id, entries[i].roll_no, entries[i].rid);
        
        if ((i + 1) % 1000 == 0) {
            printf("Inserted %d/%d keys\n", i + 1, count);
        }
    }
    
    clock_t insert_end = clock();
    long page_accesses_after = PF_GetPageAccesses();
    
    // Print statistics
    double insert_time = (double)(insert_end - insert_start) / CLOCKS_PER_SEC;
    long total_accesses = page_accesses_after - page_accesses_before;
    
    printf("Method 2 - Bulk Sorted:\n");
    printf("  Records indexed: %d\n", count);
    printf("  Sort time: %.2f seconds\n", sort_time);
    printf("  Insert time: %.2f seconds\n", insert_time);
    printf("  Total time: %.2f seconds\n", sort_time + insert_time);
    printf("  Page accesses: %ld\n", total_accesses);
    printf("  Avg accesses per insert: %.2f\n", 
           (double)total_accesses / count);
    
    free(entries);
    AM_CloseIndex(index_id);
    
    return 0;
}
```

### Expected Behavior
- Fewer page splits (keys are sorted)
- Better space utilization
- More balanced tree
- Lower page I/O than Method 1

## Method 3: Bulk-Loading Algorithm

### Description
Optimized bulk-loading that builds the B+ tree bottom-up from sorted data. This is the most efficient method.

### Algorithm Overview

1. **Sort all keys** (like Method 2)
2. **Build leaf level** by packing keys into leaf nodes
3. **Build parent level** by extracting first key from each leaf
4. **Recursively build upper levels** until root is created

### Implementation

```c
// method3_bulk_load.c

typedef struct {
    int page_num;
    char first_key[20];
} PageInfo;

int build_index_bulk_load(char *index_file, char *data_file) {
    StudentFile sf;
    SF_ScanHandle scan;
    StudentRecord rec;
    
    // Step 1: Load and sort (same as Method 2)
    printf("Loading all records...\n");
    SF_OpenFile(data_file, &sf);
    int num_records = sf.num_records;
    
    IndexEntry *entries = malloc(num_records * sizeof(IndexEntry));
    int count = 0;
    
    SF_OpenScan(&scan, &sf);
    while (SF_GetNextStudent(&scan, &rec) == SP_OK) {
        strcpy(entries[count].roll_no, rec.roll_no);
        entries[count].rid.page_num = scan.sp_handle.curr_page;
        entries[count].rid.slot_num = scan.sp_handle.curr_slot;
        count++;
    }
    SF_CloseScan(&scan);
    SF_CloseFile(&sf);
    
    printf("Sorting %d entries...\n", count);
    qsort(entries, count, sizeof(IndexEntry), compare_entries);
    
    // Step 2: Bulk load
    printf("Bulk loading index...\n");
    
    clock_t start = clock();
    long page_accesses_before = PF_GetPageAccesses();
    
    // Create empty index file
    PF_CreateFile(index_file);
    int fd = PF_OpenFile(index_file);
    
    // Build leaf level
    int max_keys_per_leaf = 50;  // Adjust based on order
    int num_leaves = (count + max_keys_per_leaf - 1) / max_keys_per_leaf;
    PageInfo *leaf_info = malloc(num_leaves * sizeof(PageInfo));
    
    int leaf_count = 0;
    int entry_idx = 0;
    
    while (entry_idx < count) {
        // Allocate new leaf page
        int page_num;
        char *page_data;
        PF_AllocPage(fd, &page_num, &page_data);
        
        // Initialize leaf node
        init_leaf_node(page_data);
        
        // Fill leaf with keys
        int keys_in_leaf = 0;
        while (entry_idx < count && keys_in_leaf < max_keys_per_leaf) {
            insert_into_leaf(page_data, 
                           entries[entry_idx].roll_no,
                           entries[entry_idx].rid);
            keys_in_leaf++;
            entry_idx++;
        }
        
        // Mark page as dirty and unpin
        PF_MarkDirty(fd, page_num);
        PF_UnpinPage(fd, page_num);
        
        // Record leaf info
        strcpy(leaf_info[leaf_count].first_key, entries[entry_idx - keys_in_leaf].roll_no);
        leaf_info[leaf_count].page_num = page_num;
        leaf_count++;
    }
    
    printf("Created %d leaf nodes\n", leaf_count);
    
    // Build parent levels bottom-up
    PageInfo *current_level = leaf_info;
    int current_level_size = leaf_count;
    int max_keys_per_internal = 50;
    
    while (current_level_size > 1) {
        int parent_level_size = (current_level_size + max_keys_per_internal - 1) / 
                                max_keys_per_internal;
        PageInfo *parent_level = malloc(parent_level_size * sizeof(PageInfo));
        
        int parent_idx = 0;
        int child_idx = 0;
        
        while (child_idx < current_level_size) {
            // Allocate parent node
            int page_num;
            char *page_data;
            PF_AllocPage(fd, &page_num, &page_data);
            
            // Initialize internal node
            init_internal_node(page_data);
            
            // Add children to this parent
            int children_count = 0;
            while (child_idx < current_level_size && 
                   children_count < max_keys_per_internal) {
                insert_into_internal(page_data,
                                   current_level[child_idx].first_key,
                                   current_level[child_idx].page_num);
                children_count++;
                child_idx++;
            }
            
            PF_MarkDirty(fd, page_num);
            PF_UnpinPage(fd, page_num);
            
            strcpy(parent_level[parent_idx].first_key, 
                   current_level[child_idx - children_count].first_key);
            parent_level[parent_idx].page_num = page_num;
            parent_idx++;
        }
        
        free(current_level);
        current_level = parent_level;
        current_level_size = parent_level_size;
        
        printf("Created parent level with %d nodes\n", current_level_size);
    }
    
    // Current_level[0] is now the root
    int root_page = current_level[0].page_num;
    
    // Write root page number to header page
    write_index_header(fd, root_page, count);
    
    clock_t end = clock();
    long page_accesses_after = PF_GetPageAccesses();
    
    // Statistics
    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;
    long total_accesses = page_accesses_after - page_accesses_before;
    
    printf("Method 3 - Bulk Loading:\n");
    printf("  Records indexed: %d\n", count);
    printf("  Time taken: %.2f seconds\n", time_taken);
    printf("  Page accesses: %ld\n", total_accesses);
    printf("  Avg accesses per insert: %.2f\n", 
           (double)total_accesses / count);
    printf("  Tree height: %d\n", calculate_tree_height(num_leaves, max_keys_per_internal));
    
    free(entries);
    free(current_level);
    PF_CloseFile(fd);
    
    return 0;
}
```

### Expected Behavior
- Minimal page I/O (each page written once)
- Optimal tree balance
- Maximum space utilization (90-100%)
- Fastest construction time
- Predictable tree height

## Performance Comparison

### Test Program

```c
// test_index_building.c

int main() {
    char *data_file = "data/student.txt";
    
    printf("===== Index Building Comparison =====\n\n");
    
    // Method 1
    printf("Running Method 1: Incremental Inserts\n");
    build_index_incremental("index_method1.idx", data_file);
    printf("\n");
    
    // Method 2
    printf("Running Method 2: Bulk Sorted\n");
    build_index_bulk_sorted("index_method2.idx", data_file);
    printf("\n");
    
    // Method 3
    printf("Running Method 3: Bulk Loading\n");
    build_index_bulk_load("index_method3.idx", data_file);
    printf("\n");
    
    // Verify all indexes
    printf("Verifying indexes...\n");
    verify_index("index_method1.idx", data_file);
    verify_index("index_method2.idx", data_file);
    verify_index("index_method3.idx", data_file);
    
    // Analyze tree structures
    printf("\n===== Tree Structure Analysis =====\n");
    analyze_tree_structure("index_method1.idx");
    analyze_tree_structure("index_method2.idx");
    analyze_tree_structure("index_method3.idx");
    
    return 0;
}
```

### Tree Analysis Function

```c
typedef struct {
    int tree_height;
    int num_leaf_nodes;
    int num_internal_nodes;
    int total_keys;
    double avg_leaf_utilization;
    double avg_internal_utilization;
} TreeStats;

void analyze_tree_structure(char *index_file) {
    TreeStats stats = {0};
    
    int index_id = AM_OpenIndex(index_file);
    
    // Traverse tree and collect statistics
    traverse_and_analyze(index_id, &stats);
    
    printf("Index: %s\n", index_file);
    printf("  Height: %d\n", stats.tree_height);
    printf("  Leaf nodes: %d\n", stats.num_leaf_nodes);
    printf("  Internal nodes: %d\n", stats.num_internal_nodes);
    printf("  Total keys: %d\n", stats.total_keys);
    printf("  Avg leaf utilization: %.2f%%\n", stats.avg_leaf_utilization);
    printf("  Avg internal utilization: %.2f%%\n", stats.avg_internal_utilization);
    printf("\n");
    
    AM_CloseIndex(index_id);
}
```

## Expected Results

| Metric | Method 1 (Incremental) | Method 2 (Sorted) | Method 3 (Bulk) |
|--------|----------------------|-------------------|-----------------|
| Time | Slowest | Medium | Fastest |
| Page Accesses | Highest | Medium | Lowest |
| Tree Height | May be higher | Optimal | Optimal |
| Space Utilization | 60-70% | 80-90% | 90-100% |
| Page Splits | Many | Few | None |

## Graph Generation

```python
# scripts/compare_index_methods.py

import matplotlib.pyplot as plt
import numpy as np

methods = ['Method 1\nIncremental', 'Method 2\nSorted', 'Method 3\nBulk Load']

# Data from test output (example values)
build_times = [45.3, 28.7, 12.4]  # seconds
page_accesses = [125000, 45000, 18000]
space_util = [65, 85, 95]  # percentage

fig, axes = plt.subplots(1, 3, figsize=(15, 5))

# Graph 1: Build Time
axes[0].bar(methods, build_times, color=['red', 'orange', 'green'])
axes[0].set_ylabel('Time (seconds)')
axes[0].set_title('Index Build Time')
axes[0].grid(axis='y', alpha=0.3)

# Graph 2: Page Accesses
axes[1].bar(methods, page_accesses, color=['red', 'orange', 'green'])
axes[1].set_ylabel('Page Accesses')
axes[1].set_title('Total Page I/O Operations')
axes[1].grid(axis='y', alpha=0.3)

# Graph 3: Space Utilization
axes[2].bar(methods, space_util, color=['red', 'orange', 'green'])
axes[2].set_ylabel('Utilization (%)')
axes[2].set_title('Average Space Utilization')
axes[2].set_ylim([0, 100])
axes[2].grid(axis='y', alpha=0.3)

plt.tight_layout()
plt.savefig('index_comparison.png', dpi=300)
plt.show()
```

## Deliverables

1. Three index building implementations
2. Test program comparing all methods
3. Tree structure analysis tool
4. Performance data (CSV format)
5. Comparison graphs
6. Analysis document explaining:
   - Why bulk loading is faster
   - Trade-offs of each method
   - When to use each approach
   - Impact on query performance
