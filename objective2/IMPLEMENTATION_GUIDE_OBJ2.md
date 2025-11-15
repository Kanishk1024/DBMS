# Implementation Guide - Objective 2: Slotted-Page Structure

## Overview
Implement a slotted-page mechanism for managing variable-length student records on fixed-size pages (4096 bytes).

## Student Record Structure

Based on `data/student.txt` format:
```
roll_no;name;batch;degree;dept;join_yr;categ;sex;father_name;birthdate;address;city;state;pincode
```

### Example Record:
```
B20CS001;Rahul Kumar;2020;B.Tech.;CSE;2020;GEN;M;Suresh Kumar;2002-05-15;123 Main St;Mumbai;Maharashtra;400001
```

### Variable-Length Concerns:
- `name`: 10-100 characters
- `father_name`: 10-100 characters  
- `address`: 20-200 characters
- `city`: 5-50 characters
- `state`: 5-50 characters

## Slotted Page Structure

```
+------------------------------------------+
|           Page Header (32 bytes)         |
|------------------------------------------|
|   Slot Directory (grows downward)        |
|   [ slot_0 | slot_1 | ... | slot_n ]     |
|                                          |
|            Free Space                    |
|                                          |
|   [ rec_n | ... | rec_1 | rec_0 ]       |
|   Records (grow upward)                  |
+------------------------------------------+
```

### Page Header Structure
```c
typedef struct {
    int page_id;           // Page number
    int num_slots;         // Number of slots currently used
    int free_space_offset; // Offset where free space starts
    int free_space_size;   // Amount of free space available
    int next_page;         // Next page in file (for sequential scan)
    int prev_page;         // Previous page in file
    char reserved[8];      // Reserved for future use
} SlottedPageHeader;  // Total: 32 bytes
```

### Slot Directory Entry
```c
typedef struct {
    short offset;    // Offset of record from start of page (0 if deleted)
    short length;    // Length of record in bytes (0 if deleted)
} SlotEntry;  // Total: 4 bytes per slot
```

## Student Record Format

### In-Memory Representation
```c
typedef struct {
    char roll_no[20];
    char name[100];
    int batch;
    char degree[20];
    char dept[10];
    int join_yr;
    char categ[10];
    char sex[2];
    char father_name[100];
    char birthdate[12];
    char address[200];
    char city[50];
    char state[50];
    char pincode[10];
} StudentRecord;
```

### On-Disk Representation (Variable-Length)
```c
// Record stored as: len1|field1|len2|field2|...|lenN|fieldN
// Each length is 2 bytes (short)

short write_student_record(char *buffer, StudentRecord *rec) {
    char *ptr = buffer;
    short len;
    
    // roll_no
    len = strlen(rec->roll_no);
    memcpy(ptr, &len, 2); ptr += 2;
    memcpy(ptr, rec->roll_no, len); ptr += len;
    
    // name
    len = strlen(rec->name);
    memcpy(ptr, &len, 2); ptr += 2;
    memcpy(ptr, rec->name, len); ptr += len;
    
    // batch (4 bytes integer)
    memcpy(ptr, &rec->batch, 4); ptr += 4;
    
    // ... continue for all fields
    
    return (short)(ptr - buffer);  // Total record length
}

int read_student_record(char *buffer, StudentRecord *rec) {
    char *ptr = buffer;
    short len;
    
    // roll_no
    memcpy(&len, ptr, 2); ptr += 2;
    memcpy(rec->roll_no, ptr, len); ptr += len;
    rec->roll_no[len] = '\0';
    
    // name
    memcpy(&len, ptr, 2); ptr += 2;
    memcpy(rec->name, ptr, len); ptr += len;
    rec->name[len] = '\0';
    
    // batch
    memcpy(&rec->batch, ptr, 4); ptr += 4;
    
    // ... continue for all fields
    
    return 0;
}
```

## Core Functions

### slotted_page.h
```c
#ifndef SLOTTED_PAGE_H
#define SLOTTED_PAGE_H

#include "pf.h"

#define SP_PAGE_SIZE 4096
#define SP_HEADER_SIZE 32
#define SP_SLOT_SIZE 4

// Error codes
#define SP_OK 0
#define SP_ERROR -1
#define SP_NO_SPACE -2
#define SP_INVALID_SLOT -3

// Initialize a new slotted page
int SP_InitPage(char *page_data);

// Insert a record into a page
int SP_InsertRecord(char *page_data, char *record, short rec_len, int *slot_num);

// Delete a record from a page
int SP_DeleteRecord(char *page_data, int slot_num);

// Get a record from a page
int SP_GetRecord(char *page_data, int slot_num, char *record, short *rec_len);

// Get free space in a page
int SP_GetFreeSpace(char *page_data);

// Compact a page (remove deleted records, reorganize)
int SP_CompactPage(char *page_data);

// Scan operations
typedef struct {
    int fd;            // File descriptor
    int curr_page;     // Current page number
    int curr_slot;     // Current slot number
} SP_ScanHandle;

int SP_OpenScan(SP_ScanHandle *handle, int fd);
int SP_GetNextRecord(SP_ScanHandle *handle, char *record, short *rec_len);
int SP_CloseScan(SP_ScanHandle *handle);

#endif
```

### slotted_page.c (Key Functions)

```c
int SP_InitPage(char *page_data) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    
    header->page_id = 0;
    header->num_slots = 0;
    header->free_space_offset = SP_PAGE_SIZE;  // Start from end
    header->free_space_size = SP_PAGE_SIZE - SP_HEADER_SIZE;
    header->next_page = -1;
    header->prev_page = -1;
    
    return SP_OK;
}

int SP_InsertRecord(char *page_data, char *record, short rec_len, int *slot_num) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    
    // Check if there's enough space for record + slot entry
    int space_needed = rec_len + SP_SLOT_SIZE;
    if (header->free_space_size < space_needed) {
        return SP_NO_SPACE;
    }
    
    // Find free slot or create new one
    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    int slot_idx = -1;
    
    for (int i = 0; i < header->num_slots; i++) {
        if (slots[i].offset == 0 && slots[i].length == 0) {
            slot_idx = i;  // Reuse deleted slot
            break;
        }
    }
    
    if (slot_idx == -1) {
        slot_idx = header->num_slots;
        header->num_slots++;
    }
    
    // Insert record at end of used space (grows upward from end)
    short new_offset = header->free_space_offset - rec_len;
    memcpy(page_data + new_offset, record, rec_len);
    
    // Update slot directory
    slots[slot_idx].offset = new_offset;
    slots[slot_idx].length = rec_len;
    
    // Update header
    header->free_space_offset = new_offset;
    header->free_space_size -= space_needed;
    
    *slot_num = slot_idx;
    return SP_OK;
}

int SP_DeleteRecord(char *page_data, int slot_num) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    
    if (slot_num >= header->num_slots) {
        return SP_INVALID_SLOT;
    }
    
    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    
    // Mark slot as deleted
    int rec_len = slots[slot_num].length;
    slots[slot_num].offset = 0;
    slots[slot_num].length = 0;
    
    // Free space is now available (but fragmented)
    header->free_space_size += rec_len;
    
    return SP_OK;
}

int SP_CompactPage(char *page_data) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    
    char *temp_page = malloc(SP_PAGE_SIZE);
    memcpy(temp_page, page_data, SP_PAGE_SIZE);
    
    // Reset page
    SP_InitPage(page_data);
    SlottedPageHeader *new_header = (SlottedPageHeader *)page_data;
    SlotEntry *new_slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    
    // Copy back valid records
    short new_offset = SP_PAGE_SIZE;
    int new_slot_count = 0;
    
    SlottedPageHeader *old_header = (SlottedPageHeader *)temp_page;
    SlotEntry *old_slots = (SlotEntry *)(temp_page + SP_HEADER_SIZE);
    
    for (int i = 0; i < old_header->num_slots; i++) {
        if (old_slots[i].offset != 0) {  // Valid record
            short rec_len = old_slots[i].length;
            new_offset -= rec_len;
            
            memcpy(page_data + new_offset, 
                   temp_page + old_slots[i].offset, 
                   rec_len);
            
            new_slots[new_slot_count].offset = new_offset;
            new_slots[new_slot_count].length = rec_len;
            new_slot_count++;
        }
    }
    
    new_header->num_slots = new_slot_count;
    new_header->free_space_offset = new_offset;
    new_header->free_space_size = new_offset - SP_HEADER_SIZE - 
                                   (new_slot_count * SP_SLOT_SIZE);
    
    free(temp_page);
    return SP_OK;
}
```

## File Management Layer

### student_file.h
```c
typedef struct {
    int fd;              // PF file descriptor
    int num_pages;       // Total pages in file
    int num_records;     // Total records
} StudentFile;

// Open/Create student file
int SF_CreateFile(char *filename);
int SF_OpenFile(char *filename, StudentFile *sf);
int SF_CloseFile(StudentFile *sf);

// Insert student record
int SF_InsertStudent(StudentFile *sf, StudentRecord *rec, RecordID *rid);

// Delete student record
int SF_DeleteStudent(StudentFile *sf, RecordID rid);

// Get student record
int SF_GetStudent(StudentFile *sf, RecordID rid, StudentRecord *rec);

// Sequential scan
typedef struct {
    StudentFile *sf;
    SP_ScanHandle sp_handle;
} SF_ScanHandle;

int SF_OpenScan(SF_ScanHandle *handle, StudentFile *sf);
int SF_GetNextStudent(SF_ScanHandle *handle, StudentRecord *rec);
int SF_CloseScan(SF_ScanHandle *handle);
```

## Test Program

### test_slotted_page.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "student_file.h"

void load_students_from_file(StudentFile *sf, char *data_file) {
    FILE *fp = fopen(data_file, "r");
    char line[1024];
    int count = 0;
    
    // Skip header
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        StudentRecord rec;
        // Parse line and populate rec
        parse_student_line(line, &rec);
        
        RecordID rid;
        if (SF_InsertStudent(sf, &rec, &rid) == SP_OK) {
            count++;
            if (count % 1000 == 0) {
                printf("Loaded %d records\n", count);
            }
        }
    }
    
    fclose(fp);
    printf("Total records loaded: %d\n", count);
}

void test_delete_and_space_utilization(StudentFile *sf) {
    // Delete 10% of records randomly
    // Measure space utilization before/after compaction
    // Report fragmentation statistics
}

void test_sequential_scan(StudentFile *sf) {
    SF_ScanHandle scan;
    StudentRecord rec;
    int count = 0;
    
    clock_t start = clock();
    SF_OpenScan(&scan, sf);
    
    while (SF_GetNextStudent(&scan, &rec) == SP_OK) {
        count++;
    }
    
    SF_CloseScan(&scan);
    clock_t end = clock();
    
    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Sequential scan: %d records in %.2f seconds\n", 
           count, time_taken);
}

int main() {
    StudentFile sf;
    
    // Create and open file
    SF_CreateFile("students.db");
    SF_OpenFile("students.db", &sf);
    
    // Load data
    printf("Loading students from data/student.txt...\n");
    load_students_from_file(&sf, "data/student.txt");
    
    // Collect statistics
    printf("\n=== Space Utilization ===\n");
    collect_space_stats(&sf);
    
    // Test deletions
    printf("\n=== Testing Deletions ===\n");
    test_delete_and_space_utilization(&sf);
    
    // Test sequential scan
    printf("\n=== Sequential Scan Performance ===\n");
    test_sequential_scan(&sf);
    
    SF_CloseFile(&sf);
    return 0;
}
```

## Performance Metrics

### Space Utilization
```c
typedef struct {
    int total_pages;
    int total_space;           // total_pages * 4096
    int used_space;            // Space occupied by records
    int slot_overhead;         // Space used by slot directory
    int header_overhead;       // Space used by headers
    int free_space;            // Available space
    int fragmented_space;      // Wasted space due to fragmentation
    double utilization_pct;    // (used_space / total_space) * 100
} SpaceStats;

void collect_space_stats(StudentFile *sf, SpaceStats *stats) {
    stats->total_pages = sf->num_pages;
    stats->total_space = sf->num_pages * SP_PAGE_SIZE;
    stats->used_space = 0;
    stats->slot_overhead = 0;
    stats->header_overhead = sf->num_pages * SP_HEADER_SIZE;
    stats->free_space = 0;
    stats->fragmented_space = 0;
    
    for (int page_num = 0; page_num < sf->num_pages; page_num++) {
        char *page_data;
        PF_GetThisPage(sf->fd, page_num, &page_data);
        
        SlottedPageHeader *header = (SlottedPageHeader *)page_data;
        SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
        
        stats->slot_overhead += header->num_slots * SP_SLOT_SIZE;
        stats->free_space += header->free_space_size;
        
        // Calculate actual used space
        for (int i = 0; i < header->num_slots; i++) {
            if (slots[i].offset != 0) {
                stats->used_space += slots[i].length;
            }
        }
        
        PF_UnpinPage(sf->fd, page_num);
    }
    
    stats->fragmented_space = stats->total_space - stats->used_space - 
                              stats->slot_overhead - stats->header_overhead - 
                              stats->free_space;
    
    stats->utilization_pct = (double)stats->used_space / stats->total_space * 100;
}
```

## Comparison with Static Records

Create a second implementation using fixed-size records (e.g., 512 bytes per record) and compare:

1. **Space efficiency**: Slotted pages should use less space for typical records
2. **Fragmentation**: Measure after deletions
3. **Access time**: Fixed-size may be slightly faster
4. **Records per page**: Slotted pages should fit more records

## Deliverables

1. Slotted page implementation (slotted_page.c/h)
2. Student file layer (student_file.c/h)
3. Test program (test_slotted_page.c)
4. Performance analysis document with:
   - Space utilization statistics
   - Average records per page
   - Fragmentation analysis
   - Comparison with fixed-size records
5. Graphs showing space efficiency vs record size distribution
