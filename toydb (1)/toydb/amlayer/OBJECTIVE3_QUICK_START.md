# Objective 3 - Quick Reference

## 🚀 Run the Test

```bash
cd "toydb (1)/toydb/amlayer"
make test_objective3
./test_objective3 17815
```

## 📁 Files

- **test_objective3.c** - Main implementation (551 lines)
- **OBJECTIVE3_README.md** - Full documentation
- **makefile** - Build system

## 🎯 What It Does

Compares 3 methods for building B+ tree indexes:

1. **Bulk Creation** - Index existing unsorted file
2. **Incremental** - Build index one record at a time
3. **Bulk-Loading** - Pre-sort data, then build index

## 📊 Expected Output

```
Method 1: Bulk Creation    0.016s  1,116,228 rec/s  (baseline)
Method 2: Incremental      0.016s  1,121,216 rec/s  (~same)
Method 3: Bulk-Loading     0.019s    942,692 rec/s  (slower due to sort)
```

## 🔧 Dependencies

- AM Layer (B+ tree) - `amlayer/*.c`
- PF Layer (pages) - `../pflayer/*.c`
- Data file - `../../../data/student.txt`

## ✅ Quick Verify

```bash
# Build
make test_objective3

# Test with 1000 records
./test_objective3 1000

# Check output files
ls -lh student_method*.0

# Clean
make clean
```

## 📚 Full Documentation

See **OBJECTIVE3_README.md** for complete details.
