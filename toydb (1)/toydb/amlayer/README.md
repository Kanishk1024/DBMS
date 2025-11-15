# toydb AM Layer (Access Method Layer)

## Overview

This directory contains the **B+ tree index implementation** for toydb database system.

## Contents

### Core Implementation
- **am.c** - B+ tree split operations
- **amfns.c** - Main AM functions (create, destroy, insert, delete)
- **amsearch.c** - B+ tree search operations
- **aminsert.c** - Leaf insertion logic
- **amscan.c** - Index scanning operations
- **amstack.c** - Stack for tree traversal
- **amglobals.c** - Global variables
- **amprint.c** - Debugging/print utilities

### Headers
- **am.h** - Data structures and constants
- **testam.h** - Testing utilities
- **pf.h** - PF layer interface

### Utilities
- **misc.c** - Wrapper functions (xAM_*, xPF_*)
- **main.c** - Original test program

### Test Programs
- **test1.c** - Basic insertion and scan tests
- **test2.c** - Additional AM operations
- **test3.c** - Scan with deletion tests
- **test_objective3.c** - Index building methods comparison ⭐

### Build System
- **makefile** - Compilation and linking rules
- **DOC** - Assignment guidelines

## Test Programs

### Original toydb Tests

```bash
make test1
./test1

make test2
./test2

make test3
./test3
```

### Objective 3: Index Building Methods ⭐

```bash
make test_objective3
./test_objective3 17815
```

**Purpose**: Compare three approaches for building B+ tree indexes on student data

**Documentation**: See `OBJECTIVE3_README.md` for full details

**Quick Start**: See `OBJECTIVE3_QUICK_START.md` for quick reference

## Dependencies

### Internal
- **PF Layer**: `../pflayer/` - Page file management

### External
- **Student Data**: `../../../data/student.txt` - Required for test_objective3

## Build Instructions

### Build All Tests
```bash
make test1 test2 test3 test_objective3
```

### Build Specific Test
```bash
make test_objective3
```

### Clean
```bash
make clean  # Removes *.o, executables, test files
```

## Key Functions

### Index Management
```c
AM_CreateIndex(fileName, indexNo, attrType, attrLength)
AM_DestroyIndex(fileName, indexNo)
```

### Index Operations
```c
AM_InsertEntry(fd, attrType, attrLength, value, recId)
AM_DeleteEntry(fd, attrType, attrLength, value, recId)
```

### Index Scanning
```c
AM_OpenIndexScan(fd, attrType, attrLength, op, value)
AM_FindNextEntry(scanDesc)
AM_CloseIndexScan(scanDesc)
```

### Wrapper Functions (misc.c)
These provide error checking and auto-exit:
```c
xAM_CreateIndex(...)
xAM_InsertEntry(...)
xAM_OpenIndexScan(...)
xPF_OpenFile(...)
xPF_CloseFile(...)
```

## File Naming Convention

Index files: `<filename>.<indexNo>`

Example:
- `student_method1.0` - Index 0 on student_method1
- `testrel.0` - Index 0 on testrel

## Notes

- **K&R C Style**: Original toydb code uses old C conventions
- **Warnings Expected**: Compiler warnings about K&R style are normal
- **Binary Files**: Index files are binary (B+ tree pages)

## Documentation

- **DOC** - Original toydb AM layer documentation
- **OBJECTIVE3_README.md** - Complete Objective 3 documentation
- **OBJECTIVE3_QUICK_START.md** - Quick reference for Objective 3

## Status

✅ All test programs working  
✅ Objective 3 implementation complete  
✅ Integrated with toydb build system  
✅ Fully documented
