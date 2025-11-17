📘 **DBMS Assignment**
*A complete guide to PF Layer Buffering, Slotted Pages, and Index Construction in ToyDB*

---

# 🏛️ **Introduction**

This project extends the **ToyDB** system—an educational database that consists of:

1. **PF Layer (Paged File Layer)**  
   Manages fixed-size disk pages, buffering, and low-level storage.

2. **AM Layer (Access Method Layer)**  
   Implements B+ tree–based indexing on top of PF.

3. **Higher-level modules**  
   Including a **slotted-page structure** to store variable-length student records.

The assignment required implementing three major objectives:

- **Objective 1:**  
  PF Layer page buffering with LRU/MRU replacement, I/O statistics, and graph generation for real database workloads.

- **Objective 2:**  
  A variable-length record **slotted-page** storage engine and its performance evaluation.

- **Objective 3:**  
  Comparison of three different **B+ tree index construction methods** in the AM Layer.

This README explains all implementations, how to run them, and the complete performance results.

---

# 🔥 **Objective 1 – PF Layer Buffering, LRU/MRU & Real-Data Graphs**

## 🎯 Goal
Implement a full buffer manager inside the PF layer with:

- **Two replacement strategies:**  
  - **LRU (default)**  
  - **MRU (useful for sequential workloads)**

- **Configurable buffer pool size**
- **Dirty-bit management**
- **Logical vs physical I/O counters**
- **Support for 11 read/write mixtures (100/0 → 0/100)**
- **Graph generation using real database files**
- **CSV statistics export**

All results must visualize:
> **X-axis → read/write mixture**  
> **Y-axis → hit ratio / buffer stats / physical I/O**

---

## 📁 Relevant Files

pflayer/
├── test_realdata.c # Generates statistics & CSV output
├── generate_graphs.py # Creates professional graphs
├── Makefile # Automates tests & graph creation
├── realdata_lru.csv # Generated statistics
├── realdata_mru.csv # Generated statistics
└── graphs/ # Output graphs

---

## 🧪 Running the Tests

### Generate all results + graphs
```bash
make graphs
```

Clean only generated results
```bash
make clean-results
```

Full rebuild
```bash
make clean
make tests
make graphs
```

📊 Real Dataset Used

| File           | Approx. Pages | Description           |
|----------------|--------------|----------------------|
| student.txt    | 446 pages    | Large dataset (~17K rows) |
| courses.txt    | 107 pages    | Medium dataset       |
| studemail.txt  | 133 pages    | Medium dataset       |
| department.txt | 10 pages     | Very small dataset   |
| program.txt    | 10 pages     | Very small dataset   |

Each workload runs 5000 operations with random page accesses.

📈 Generated CSV Files

- realdata_lru.csv
- realdata_mru.csv

Format:

Dataset,ReadPct,WritePct,NumPages,  
LogicalReads,LogicalWrites,  
PhysicalReads,PhysicalWrites,  
BufferHits,BufferMisses,HitRatio

🖼️ Graphs Generated

- hit_ratio_vs_mixture.png  
  Primary graph: Shows LRU vs MRU for each workload

- physical_io_vs_mixture.png  
  Tracks actual disk I/O

- strategy_comparison.png  
  Side-by-side summary: hit ratios + total I/O

- buffer_performance.png  
  Bar chart of hits/misses for each mixture

All graphs are placed in graphs/.

📌 Key Results

**Small Datasets (<20 pages)**
- Hit ratio ~ 99.8%
- All pages fit in memory → replacement strategy irrelevant

**Medium Datasets (100–150 pages)**
- LRU ≈ MRU
- Hit ratio ~ 15%–19%

**Large Dataset (446 pages)**
- Hit ratio ~ 4–5%
- Both LRU and MRU struggle due to random workload + limited buffer

**Main Insights**
- Strategy differences appear only in hotspot workloads
- Random workloads minimize LRU/MRU differences
- Buffer size relative to dataset size is the dominating factor

---

# 🧩 Objective 2 – Slotted Page Implementation for Variable-Length Records

## 🎯 Goal

Design a slotted-page structure capable of storing variable-length student records, supporting:

- Page initialization
- Record insertion
- Record deletion
- Sequential scanning
- Automatic page compaction
- File-level record management
- Space utilization analysis vs fixed-size storage

## 🗂️ File Structure

objective2/
│── slotted_page.c/h
│── student_file.c/h
│── test_objective2_final.c
│── run_objective2_tests.sh
│── IMPLEMENTATION_GUIDE_OBJ2.md
└── Makefile

## 🧠 How Slotted Pages Work

A 4KB page contains:

- 32-byte page header:  
  free space pointer, slot count, total used bytes

- Slot directory (grows downward)
- Record area (grows upward)

On deletion:

- Slot marked dead
- compact_page() removes fragmentation and repacks records

## 🧪 Running Tests

Build:
```bash
make
```

Full test (10,000 records):
```bash
make test
```

Quick test (1,000 records):
```bash
make test-quick
```

Custom size:
```bash
./test_objective2_final 5000
```

## 📊 Performance Comparison

| Method         | File Size | Utilization | Avg Record Size | Space Savings |
|----------------|-----------|-------------|-----------------|--------------|
| Slotted Page   | ~1 MB     | 94%         | 97 bytes        | Baseline     |
| Static 256B    | ~2.5 MB   | 100%        | 256B            | -60%         |
| Static 512B    | ~5 MB     | 100%        | 512B            | -80%         |
| Static 1024B   | ~10 MB    | 100%        | 1024B           | -90%         |

**Summary**
- Slotted pages save 60–90% space
- Minimal overhead compared to fixed-size storage
- Highly efficient for real datasets with variable-length text fields

---

# 🌳 Objective 3 – B+ Tree Index Construction (Three Methods)

## 🎯 Goal

Implement and compare three index-building strategies inside the AM layer:

- Method 1: Bulk Creation  
  Build index on an existing unsorted data file.

- Method 2: Incremental Insertion  
  Build index as records arrive (simulate OLTP).

- Method 3: Bulk-Loading using Pre-sorted Data  
  Sort file by key → build index sequentially.

## 📁 File Location

toydb/amlayer/
│── test_objective3.c
│── misc.c
│── am.c, aminsert.c, amsearch.c, amfns.c
│── makefile
└── Uses ../pflayer/ and ../../../data/student.txt

## ⚙️ Running Tests

Build:
```bash
make test_objective3
```

Run:
```bash
./test_objective3 1000
./test_objective3 17815
```

Clean:
```bash
make clean
```

## 📊 Performance Summary (17,815 records)

| Method                | Time (sec) | Rate (rec/s) | Notes         |
|-----------------------|------------|--------------|---------------|
| 1: Bulk Build         | 0.016      | 1.11M        | Baseline      |
| 2: Incremental Insert | 0.016      | 1.12M        | Often fastest |
| 3: Bulk-Loading       | 0.019      | 0.94M        | Sorting dominates |

**Interpretation**
- For ~17K records:  
  Sorting cost outweighs the structural benefits → Bulk-loading appears slower.
- For 100K+ records:  
  Bulk-loading becomes significantly faster than random insertions.

**Output Files Created**
- student_method1.0
- student_method2.0
- student_method3.0

Each file is a fully constructed B+ tree index.

---

# 🧠 Overall Takeaways

**PF Layer**
- Fully functional buffer manager
- LRU & MRU with real-world workloads
- Statistical analysis + graph generation

**Slotted Pages**
- Highly space-efficient
- Supports insert, delete, scan, compaction
- Ideal for variable-length data

**AM Layer**
- Three index construction strategies
- Full comparison using real student dataset
- Proper integration with ToyDB's AM APIs

---

# ⚙️ Build Instructions Summary

| Component    | Build                      | Run                       | Clean             |
|--------------|---------------------------|---------------------------|-------------------|
| Objective 1  | make tests / make graphs   | ./test_realdata           | make clean-results|
| Objective 2  | make                      | make test                 | make clean-all    |
| Objective 3  | make test_objective3       | ./test_objective3 <N>     | make clean        |

---

# 📦 Repository Structure

project/
├── pflayer/              # Objective 1
├── objective2/           # Objective 2
├── amlayer/              # Objective 3
├── data/                 # Provided student datasets
└── README.md             # Combined project documentation
