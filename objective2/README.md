# Objective 2: Slotted-Page Structure for Variable-Length Records

This folder contains the implementation of a slotted-page mechanism for managing variable-length student records on fixed-size pages (4096 bytes).

## Overview

The slotted-page structure efficiently stores variable-length records by:
- Using a page header (32 bytes) to track page metadata
- Maintaining a slot directory that grows downward from the header
- Storing records that grow upward from the end of the page
- Supporting record insertion, deletion, and sequential scanning

## Files

### Core Implementation
- **slotted_page.h/c**: Core slotted page implementation with page initialization, record insertion/deletion, and page compaction
- **student_file.h/c**: File management layer for student records with serialization/deserialization

### Test Programs
- **test_objective2_final.c**: Comprehensive performance analysis comparing slotted pages vs static records with detailed statistics

### Build System
- **Makefile**: Build and test automation
- **run_objective2_tests.sh**: Script to run multiple test configurations

### Documentation
- **IMPLEMENTATION_GUIDE_OBJ2.md**: Detailed implementation guide with code examples

## Data Source

The tests use student records from `../data/student.txt` (17,816 records with variable-length fields).

## Building and Running

### Build everything:
```bash
make
```

### Run comprehensive tests (10,000 records):
```bash
make test
```

### Run quick test (1,000 records):
```bash
make test-quick
```

### Run with custom record count:
```bash
./test_objective2_final 5000
```

### Clean operations:
```bash
make clean        # Remove build artifacts only
make clean-db     # Remove test database files only
make clean-all    # Remove everything
```

## Test Output

The tests compare four storage methods:
1. **Slotted Page** (variable-length records)
2. **Static 256B** (fixed 256-byte records)
3. **Static 512B** (fixed 512-byte records)
4. **Static 1024B** (fixed 1024-byte records)

### Generated Files
- `test_slotted.db`: Variable-length slotted page database
- `test_static_256.db`: Fixed 256-byte records
- `test_static_512.db`: Fixed 512-byte records
- `test_static_1024.db`: Fixed 1024-byte records

## Performance Metrics

The tests measure and compare:
- **Space Utilization**: Percentage of space used for actual data
- **File Size**: Total storage required
- **Insert Rate**: Records inserted per second
- **Scan Rate**: Records scanned per second
- **Records per Page**: Average number of records fitting in a 4KB page
- **Space Savings**: Comparison between variable-length and fixed-size approaches

## Key Results (10,000 records)

| Storage Method | File Size | Utilization | Avg Record Size | Space Savings |
|----------------|-----------|-------------|-----------------|---------------|
| Slotted Page   | ~1 MB     | 94%         | 97 bytes        | Baseline      |
| Static 256B    | ~2.5 MB   | 100%        | 256 bytes       | -60%          |
| Static 512B    | ~5 MB     | 100%        | 512 bytes       | -80%          |
| Static 1024B   | ~10 MB    | 100%        | 1024 bytes      | -90%          |

**Conclusion**: Slotted pages provide ~90% space savings compared to 1024-byte fixed records and ~60% savings compared to 256-byte fixed records, with minimal performance overhead.

## Features Implemented

✅ Variable-length record storage
✅ Efficient space utilization (>94%)
✅ Record insertion and deletion
✅ Sequential scanning
✅ Page compaction to eliminate fragmentation
✅ Comprehensive performance comparison
✅ Space utilization statistics
✅ Support for actual student data

## Usage Example

```c
// Create and open student file
StudentFile sf;
SF_CreateFile("students.db");
SF_OpenFile("students.db", &sf);

// Insert a student record
StudentRecord rec;
strcpy(rec.roll_no, "B20CS001");
strcpy(rec.name, "John Doe");
// ... populate other fields ...

RecordID rid;
SF_InsertStudent(&sf, &rec, &rid);

// Sequential scan
SF_ScanHandle scan;
SF_OpenScan(&scan, &sf);
while (SF_GetNextStudent(&scan, &rec) == SP_OK) {
    // Process record
}
SF_CloseScan(&scan);

// Get space statistics
SpaceStats stats;
SF_GetSpaceStats(&sf, &stats);
printf("Space utilization: %.2f%%\n", stats.utilization_pct);

SF_CloseFile(&sf);
```
