# Implementation Guide - Objective 1: Page Buffering

## Overview
Implement an enhanced buffer pool manager for the PF layer with LRU/MRU replacement strategies.

## Current PF Layer Architecture

The existing PF layer in `toydb/pflayer/` provides:
- `PF_Init()`: Initialize PF layer
- `PF_CreateFile()`: Create a new file
- `PF_OpenFile()`: Open an existing file
- `PF_CloseFile()`: Close a file
- `PF_GetThisPage()`: Read a specific page
- `PF_AllocPage()`: Allocate a new page
- `PF_DisposePage()`: Free a page

## Required Enhancements

### 1. Buffer Pool Structure

Create enhanced buffer pool in `pflayer/buf_enhanced.h`:

```c
typedef enum {
    REPLACE_LRU,  // Least Recently Used
    REPLACE_MRU   // Most Recently Used
} ReplacementStrategy;

typedef struct buffer_frame {
    int fd;                // File descriptor
    int page_num;          // Page number
    char *data;            // Page data (PF_PAGE_SIZE bytes)
    int pin_count;         // Pin count
    int dirty;             // Dirty flag (0 or 1)
    int last_used;         // Timestamp for LRU
    struct buffer_frame *next;  // For LRU/MRU list
    struct buffer_frame *prev;
} BufferFrame;

typedef struct buffer_pool {
    BufferFrame *frames;   // Array of buffer frames
    int pool_size;         // Number of frames
    ReplacementStrategy strategy;
    int clock_hand;        // For clock algorithm (optional)
    
    // Statistics
    long reads;            // Logical reads
    long writes;           // Logical writes
    long physical_reads;   // Physical reads from disk
    long physical_writes;  // Physical writes to disk
    long hits;             // Buffer hits
    long misses;           // Buffer misses
} BufferPool;
```

### 2. Required Functions

#### Initialization
```c
int BUF_Init(int pool_size, ReplacementStrategy strategy);
// Initialize buffer pool with specified size and strategy
```

#### Page Access
```c
int BUF_GetPage(int fd, int page_num, char **page_data);
// Get page from buffer (load from disk if not in buffer)

int BUF_MarkDirty(int fd, int page_num);
// Mark a page as modified

int BUF_UnpinPage(int fd, int page_num);
// Unpin a page (decrease pin count)

int BUF_FlushPage(int fd, int page_num);
// Write a specific dirty page to disk

int BUF_FlushAll();
// Write all dirty pages to disk
```

#### Statistics
```c
void BUF_GetStatistics(BufferStats *stats);
// Get current buffer statistics

void BUF_ResetStatistics();
// Reset statistics counters

void BUF_PrintStatistics();
// Print statistics to stdout
```

### 3. LRU Implementation

```c
/* LRU uses doubly-linked list with most recent at head */

BufferFrame* find_lru_victim(BufferPool *pool) {
    BufferFrame *victim = NULL;
    int oldest_time = INT_MAX;
    
    for (int i = 0; i < pool->pool_size; i++) {
        if (pool->frames[i].pin_count == 0) {
            if (pool->frames[i].last_used < oldest_time) {
                oldest_time = pool->frames[i].last_used;
                victim = &pool->frames[i];
            }
        }
    }
    return victim;
}
```

### 4. MRU Implementation

```c
/* MRU evicts most recently used unpinned page */
/* Useful for sequential scans where we won't revisit recent pages */

BufferFrame* find_mru_victim(BufferPool *pool) {
    BufferFrame *victim = NULL;
    int newest_time = -1;
    
    for (int i = 0; i < pool->pool_size; i++) {
        if (pool->frames[i].pin_count == 0) {
            if (pool->frames[i].last_used > newest_time) {
                newest_time = pool->frames[i].last_used;
                victim = &pool->frames[i];
            }
        }
    }
    return victim;
}
```

### 5. Dirty Flag Management

```c
int BUF_MarkDirty(int fd, int page_num) {
    BufferFrame *frame = find_frame(fd, page_num);
    if (frame == NULL) {
        return BUF_ERROR;
    }
    frame->dirty = 1;
    return BUF_OK;
}

// When evicting a dirty page:
if (victim->dirty) {
    // Write to disk
    write_page_to_disk(victim->fd, victim->page_num, victim->data);
    pool->physical_writes++;
    victim->dirty = 0;
}
```

### 6. Statistics Collection

```c
typedef struct {
    long logical_reads;
    long logical_writes;
    long physical_reads;
    long physical_writes;
    long buffer_hits;
    long buffer_misses;
    double hit_ratio;
} BufferStats;

void BUF_GetStatistics(BufferStats *stats) {
    stats->logical_reads = global_pool->reads;
    stats->logical_writes = global_pool->writes;
    stats->physical_reads = global_pool->physical_reads;
    stats->physical_writes = global_pool->physical_writes;
    stats->buffer_hits = global_pool->hits;
    stats->buffer_misses = global_pool->misses;
    
    long total_accesses = stats->buffer_hits + stats->buffer_misses;
    stats->hit_ratio = (total_accesses > 0) ? 
        (double)stats->buffer_hits / total_accesses : 0.0;
}
```

## Test Program Structure

### test_buffering.c

```c
#include <stdio.h>
#include <stdlib.h>
#include "pf.h"
#include "buf_enhanced.h"

void run_read_write_test(int read_pct, int write_pct, int num_ops) {
    // Open test file
    // Execute mix of read/write operations
    // Collect statistics
    // Print results
}

int main(int argc, char *argv[]) {
    int buffer_size = atoi(argv[1]);  // e.g., 10, 20, 50
    char *strategy = argv[2];          // "LRU" or "MRU"
    
    ReplacementStrategy strat = (strcmp(strategy, "LRU") == 0) ? 
        REPLACE_LRU : REPLACE_MRU;
    
    BUF_Init(buffer_size, strat);
    
    // Test different read/write mixtures
    int mixtures[][2] = {
        {100, 0},   // 100% read
        {90, 10},   // 90% read, 10% write
        {70, 30},   // 70% read, 30% write
        {50, 50},   // 50% read, 50% write
        {30, 70},   // 30% read, 70% write
        {10, 90},   // 10% read, 90% write
        {0, 100}    // 100% write
    };
    
    printf("Buffer Size: %d, Strategy: %s\n", buffer_size, strategy);
    printf("Read%%\tWrite%%\tHits\tMisses\tPhysRead\tPhysWrite\tHitRatio\n");
    
    for (int i = 0; i < 7; i++) {
        BUF_ResetStatistics();
        run_read_write_test(mixtures[i][0], mixtures[i][1], 10000);
        
        BufferStats stats;
        BUF_GetStatistics(&stats);
        
        printf("%d\t%d\t%ld\t%ld\t%ld\t%ld\t%.2f\n",
            mixtures[i][0], mixtures[i][1],
            stats.buffer_hits, stats.buffer_misses,
            stats.physical_reads, stats.physical_writes,
            stats.hit_ratio);
    }
    
    return 0;
}
```

## Graph Generation

### scripts/generate_io_graphs.py

```python
import matplotlib.pyplot as plt
import numpy as np

# Data from test_buffering output
read_pct = [100, 90, 70, 50, 30, 10, 0]
lru_phys_reads = []    # Fill with your data
lru_phys_writes = []   # Fill with your data
mru_phys_reads = []    # Fill with your data
mru_phys_writes = []   # Fill with your data

# Graph 1: Physical I/O vs Read/Write Mixture
plt.figure(figsize=(12, 6))

plt.subplot(1, 2, 1)
plt.plot(read_pct, lru_phys_reads, 'b-o', label='LRU Reads')
plt.plot(read_pct, lru_phys_writes, 'b--s', label='LRU Writes')
plt.xlabel('Read Percentage')
plt.ylabel('Physical I/O Operations')
plt.title('LRU Strategy: Physical I/O vs Read/Write Mix')
plt.legend()
plt.grid(True)

plt.subplot(1, 2, 2)
plt.plot(read_pct, mru_phys_reads, 'r-o', label='MRU Reads')
plt.plot(read_pct, mru_phys_writes, 'r--s', label='MRU Writes')
plt.xlabel('Read Percentage')
plt.ylabel('Physical I/O Operations')
plt.title('MRU Strategy: Physical I/O vs Read/Write Mix')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.savefig('io_comparison.png')
plt.show()

# Graph 2: Hit Ratio Comparison
plt.figure(figsize=(10, 6))
plt.plot(read_pct, lru_hit_ratio, 'b-o', label='LRU')
plt.plot(read_pct, mru_hit_ratio, 'r-s', label='MRU')
plt.xlabel('Read Percentage')
plt.ylabel('Buffer Hit Ratio')
plt.title('Buffer Hit Ratio: LRU vs MRU')
plt.legend()
plt.grid(True)
plt.savefig('hit_ratio_comparison.png')
plt.show()
```

## Implementation Steps

1. **Study existing buffer code** in `buf.c` and `hash.c`
2. **Create buf_enhanced.h** with new structures
3. **Implement buf_enhanced.c** with LRU/MRU
4. **Modify PF layer** to use new buffer manager
5. **Create test_buffering.c** 
6. **Compile and test** with various parameters
7. **Collect data** and generate graphs
8. **Analyze results** and document findings

## Expected Results

- LRU should perform better for random access patterns
- MRU should perform better for sequential scans
- Larger buffer pools should have higher hit ratios
- Write-heavy workloads should have more physical I/O

## Deliverables

1. Modified PF layer code with buffer pool
2. test_buffering program
3. Performance data (CSV format)
4. Graphs showing I/O statistics
5. Analysis document comparing LRU vs MRU
