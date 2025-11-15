# Enhanced Buffer Management - PF Layer Integration

## Overview
This directory contains the enhanced buffer management implementation integrated into toydb's Page File (PF) layer.

## What Was Modified

### Core Files (Enhanced)
- **`buf.c`** - Added LRU/MRU replacement strategies and statistics tracking
- **`pf.h`** - Added `ReplacementStrategy` enum and `BufferStats` structure
- **`Makefile`** - Added build rules for `test_realdata`

### New Test Program
- **`test_realdata.c`** - Validates buffer management with real database files

## Build & Run

### Using toydb's Native Makefile

```bash
# Clean previous builds
make clean

# Build the test program
make test_realdata

# Run the test
./test_realdata
```

### Run Original ToyDB Tests
```bash
# Build original tests
make original_tests

# Run them
./testpf
./testhash
```

## What test_realdata Does

1. **Loads Real Data** from `../../../data/` directory:
   - student.txt (17,814 records)
   - courses.txt (4,269 records)
   - department.txt (31 records)
   - program.txt (13 records)
   - studemail.txt (5,314 records)

2. **Tests Three Workload Patterns**:
   - **Sequential**: Linear page access (like table scans)
   - **Random**: Uniform random access (like index lookups)
   - **Hotspot**: 80-20 rule (80% accesses to 20% of pages)

3. **Compares Strategies**:
   - LRU (Least Recently Used)
   - MRU (Most Recently Used)

4. **Reports Statistics**:
   - Buffer hits/misses
   - Physical I/O operations
   - Hit ratios

## Key Results

### Small Datasets (< 20 pages)
- Both LRU/MRU: ~99% hit ratio
- Entire dataset fits in buffer

### Medium Datasets (20-200 pages)
- LRU hotspot: ~60% hit ratio
- MRU hotspot: ~22% hit ratio
- **LRU is 2.7x better** for realistic workloads

### Large Datasets (> 200 pages)
- LRU hotspot: ~16% hit ratio
- MRU hotspot: ~5% hit ratio
- **LRU significantly better**

## Buffer Manager Enhancements

### Statistics Tracked
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
```

### New API Functions
```c
void BUF_SetStrategy(ReplacementStrategy strategy);
void BUF_GetStatistics(BufferStats *stats);
void BUF_ResetStatistics(void);
void BUF_PrintStatistics(void);
```

### Replacement Strategies
```c
typedef enum {
    REPLACE_LRU,    // Least Recently Used
    REPLACE_MRU     // Most Recently Used
} ReplacementStrategy;
```

## Architecture

```
test_realdata.c
     ↓
pflayer.o (combined object file)
     ↓
  ┌──┴──┐
  ↓     ↓     ↓
pf.o  buf.o  hash.o
  ↓     ↓     ↓
pf.c  buf.c  hash.c  ← Enhanced with LRU/MRU & stats
```

## File Organization

```
pflayer/
├── pf.h           ← Enhanced: Added BufferStats, ReplacementStrategy
├── pf.c           ← Original: File operations
├── buf.c          ← Enhanced: Added LRU/MRU and statistics
├── hash.c         ← Original: Hash table for page lookup
├── pftypes.h      ← Original: Type definitions
├── Makefile       ← Enhanced: Added test_realdata target
├── test_realdata.c ← New: Real data validation
├── testpf.c       ← Original: PF layer tests
├── testhash.c     ← Original: Hash table tests
└── README.md      ← This file
```

## Buffer Pool Configuration

- **Size**: 20 pages (defined by `PF_MAX_BUFS` in `pftypes.h`)
- **Page Size**: 4096 bytes (`PF_PAGE_SIZE`)
- **Total Buffer Memory**: 80 KB

## Clean Up

```bash
# Remove all build artifacts and test databases
make clean
```

This removes:
- Object files (*.o)
- Executables (testpf, testhash, test_realdata)
- Test databases (*.db)
- Original test files (file1, file2)

## Integration Status

✅ LRU/MRU strategies implemented  
✅ Statistics tracking functional  
✅ Real data validation complete  
✅ Compatible with original toydb tests  
✅ No breaking changes to existing API  

## Next Steps

1. Run `make test_realdata` to see performance analysis
2. Compare results with different buffer sizes
3. Test with your own database files
4. Integrate with AM (Access Method) layer for complete DBMS
