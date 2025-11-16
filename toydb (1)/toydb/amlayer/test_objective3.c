/**
 * Objective 3: Compare B+ tree index construction approaches
 *
 * Compares three strategies to build a B+ tree index:
 *
 * STRATEGY 1: Build index from an already-populated data file
 *   - Data file already holds all records
 *   - Create index by scanning the file and inserting keys
 *   - Insertions happen in file order (effectively random)
 *
 * STRATEGY 2: Incremental (online) index maintenance
 *   - Start with an empty file and index
 *   - Insert records one at a time; update index per insert
 *   - Emulates steady data growth
 *
 * STRATEGY 3: Bulk-loading from sorted input (bottom-up)
 *   - Pre-sort records by key
 *   - Construct leaf pages sequentially and build upper levels bottom-up
 *   - Avoids repeated splits and rebalancing
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "am.h"
#include "testam.h"

/* K&R compatibility declarations */
extern char *malloc();
extern char *calloc();
extern void free();
extern void qsort();
extern int atoi();
extern void exit();

#define MAX_RECORDS 20000
#define ROLL_NO_LENGTH 20
#define MAX_FNAME_LENGTH 80
#define INDEX_NO 0
#define METHOD1_INDEX "student_method1"
#define METHOD2_INDEX "student_method2"
#define METHOD3_INDEX "student_method3"

/* Key + record identifier pair */
typedef struct
{
    char roll_no[ROLL_NO_LENGTH];
    RecIdType recId;
} StudentEntry;

/* Simple stats container */
typedef struct
{
    const char *method_name;
    double build_time;
    int num_records;
    int page_accesses; /* reserved for later instrumentation */
} MethodStats;

/* --------------------------------------------------------------------
   Read student roll numbers from the data file into entries[]
   Returns number of entries loaded or -1 on error.
   -------------------------------------------------------------------- */
int load_student_data(StudentEntry *entries, int max_records)
{
    FILE *fp;
    char line[256];
    int count = 0;

    printf("\n=== Loading student records ===\n");
    printf("Source file: ../../../data/student.txt\n");

    fp = fopen("../../../data/student.txt", "r");
    if (!fp)
    {
        fprintf(stderr, "ERROR: failed to open ../../../data/student.txt\n");
        return -1;
    }

    while (fgets(line, sizeof(line), fp) && count < max_records)
    {
        char *token;

        token = strtok(line, "|"); /* roll_no is first field */
        if (token)
        {
            strncpy(entries[count].roll_no, token, ROLL_NO_LENGTH - 1);
            entries[count].roll_no[ROLL_NO_LENGTH - 1] = '\0';
            entries[count].recId = IntToRecId(count);
            count++;
        }
    }

    fclose(fp);
    printf("Loaded %d records\n", count);
    return count;
}

/* qsort comparator for StudentEntry by roll_no */
int compare_entries(const void *a, const void *b)
{
    const StudentEntry *ea = (const StudentEntry *)a;
    const StudentEntry *eb = (const StudentEntry *)b;
    return strcmp(ea->roll_no, eb->roll_no);
}

/* --------------------------------------------------------------------
   METHOD 1: Build index by scanning an existing data file and inserting
   each key using the AM insert routine.
   -------------------------------------------------------------------- */
double method1_bulk_creation(StudentEntry *entries, int num_records)
{
    int indexDesc;
    char indexFileName[MAX_FNAME_LENGTH];
    int i;
    clock_t start, end;

    printf("\n----------------------------------------------------\n");
    printf("METHOD 1: BUILD INDEX FROM EXISTING FILE\n");
    printf("----------------------------------------------------\n");
    printf("Data count: %d. Approach: scan file and insert keys.\n\n", num_records);

    xAM_CreateIndex(METHOD1_INDEX, INDEX_NO, CHAR_TYPE, ROLL_NO_LENGTH);

    sprintf(indexFileName, "%s.%d", METHOD1_INDEX, INDEX_NO);
    indexDesc = xPF_OpenFile(indexFileName);
    if (indexDesc < 0)
    {
        fprintf(stderr, "ERROR: unable to open index file\n");
        return -1.0;
    }

    printf("Inserting keys into index...\n");
    start = clock();

    for (i = 0; i < num_records; i++)
    {
        xAM_InsertEntry(indexDesc, CHAR_TYPE, ROLL_NO_LENGTH,
                        entries[i].roll_no, entries[i].recId);

        if ((i + 1) % 1000 == 0 || i == num_records - 1)
        {
            printf("\r  Progress: %d/%d", i + 1, num_records);
            fflush(stdout);
        }
    }

    end = clock();
    printf("\n");

    xPF_CloseFile(indexDesc);

    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Completed Method 1 in %.3f s (%.0f rec/s)\n", elapsed, num_records / elapsed);

    return elapsed;
}

/* --------------------------------------------------------------------
   METHOD 2: Simulate online inserts where each new record is added and
   the index is updated immediately (one-by-one insertions).
   -------------------------------------------------------------------- */
double method2_incremental(StudentEntry *entries, int num_records)
{
    int indexDesc;
    char indexFileName[MAX_FNAME_LENGTH];
    int i;
    clock_t start, end;

    printf("\n----------------------------------------------------\n");
    printf("METHOD 2: INCREMENTAL INDEX BUILDING\n");
    printf("----------------------------------------------------\n");
    printf("Simulating %d sequential inserts (file + index per insert).\n\n", num_records);

    xAM_CreateIndex(METHOD2_INDEX, INDEX_NO, CHAR_TYPE, ROLL_NO_LENGTH);

    sprintf(indexFileName, "%s.%d", METHOD2_INDEX, INDEX_NO);
    indexDesc = xPF_OpenFile(indexFileName);
    if (indexDesc < 0)
    {
        fprintf(stderr, "ERROR: unable to open index file\n");
        return -1.0;
    }

    printf("Performing incremental inserts into index...\n");
    start = clock();

    for (i = 0; i < num_records; i++)
    {
        /* In a full system each loop would also write to the data file.
           Here we measure index insertion cost only. */
        xAM_InsertEntry(indexDesc, CHAR_TYPE, ROLL_NO_LENGTH,
                        entries[i].roll_no, entries[i].recId);

        if ((i + 1) % 1000 == 0 || i == num_records - 1)
        {
            printf("\r  Progress: %d/%d", i + 1, num_records);
            fflush(stdout);
        }
    }

    end = clock();
    printf("\n");

    xPF_CloseFile(indexDesc);

    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Completed Method 2 in %.3f s (%.0f rec/s)\n", elapsed, num_records / elapsed);

    return elapsed;
}

/* PF-level functions (externs used for bulk-loading) */
extern int PF_CreateFile();
extern int PF_OpenFile();
extern int PF_CloseFile();
extern int PF_DestroyFile();
extern int PF_AllocPage();
extern int PF_GetThisPage();
extern int PF_UnfixPage();

#define PF_PAGE_SIZE 1020
#define TRUE 1
#define FALSE 0

/* --------------------------------------------------------------------
   METHOD 3: True bulk-load (bottom-up). Sort the records, create leaf
   pages by packing entries sequentially, then build internal levels.
   -------------------------------------------------------------------- */
double method3_bulk_load(StudentEntry *entries, int num_records)
{
    StudentEntry *sorted_entries;
    int i, j;
    clock_t start, end, sort_start, sort_end;
    double sort_time, build_time;
    char indexFileName[MAX_FNAME_LENGTH];
    int indexDesc;

    int *leaf_pages = NULL;
    int num_leaf_pages = 0;
    int *parent_pages = NULL;
    int num_parent_pages = 0;
    int root_page = -1;

/* Leaf layout calculations */
#define LEAF_HEADER_SIZE (sizeof(char) + sizeof(int) + 4 * sizeof(short))
#define LEAF_ENTRY_SIZE (ROLL_NO_LENGTH + sizeof(int))
    int max_entries_per_leaf = (PF_PAGE_SIZE - LEAF_HEADER_SIZE) / LEAF_ENTRY_SIZE;
    int fill_factor = (int)(max_entries_per_leaf * 0.90); /* aim for 90% fill */

/* Internal node sizing */
#define INT_HEADER_SIZE (sizeof(char) + 3 * sizeof(short))
#define INT_ENTRY_SIZE (ROLL_NO_LENGTH + sizeof(int))
    int max_entries_per_internal = (PF_PAGE_SIZE - INT_HEADER_SIZE - sizeof(int)) / INT_ENTRY_SIZE;

    printf("\n----------------------------------------------------\n");
    printf("METHOD 3: BULK-LOADING (bottom-up construction)\n");
    printf("----------------------------------------------------\n");
    printf("Pre-sorted input will be used to build pages directly.\n\n");

    /* Make a sorted copy */
    sorted_entries = (StudentEntry *)malloc(num_records * sizeof(StudentEntry));
    if (!sorted_entries)
    {
        fprintf(stderr, "ERROR: allocation failed\n");
        return -1.0;
    }
    memcpy(sorted_entries, entries, num_records * sizeof(StudentEntry));

    printf("Sorting %d records by key...\n", num_records);
    sort_start = clock();
    qsort(sorted_entries, num_records, sizeof(StudentEntry), compare_entries);
    sort_end = clock();
    sort_time = ((double)(sort_end - sort_start)) / CLOCKS_PER_SEC;
    printf("  Sorted in %.3f s\n\n", sort_time);

    /* Determine number of leaf pages needed */
    num_leaf_pages = (num_records + fill_factor - 1) / fill_factor;
    printf("Calculated tree layout:\n");
    printf("  entries/leaf (target): %d\n", fill_factor);
    printf("  leaf pages required: %d\n", num_leaf_pages);
    printf("  entries/internal node: %d\n\n", max_entries_per_internal);

    /* Compute tree height */
    int tree_height = 1;
    int nodes_at_level = num_leaf_pages;
    while (nodes_at_level > 1)
    {
        nodes_at_level = (nodes_at_level + max_entries_per_internal) / (max_entries_per_internal + 1);
        tree_height++;
    }
    printf("  tree height: %d levels\n\n", tree_height);

    /* Create index file */
    printf("Creating index file for bulk construction...\n");
    xAM_CreateIndex(METHOD3_INDEX, INDEX_NO, CHAR_TYPE, ROLL_NO_LENGTH);
    sprintf(indexFileName, "%s.%d", METHOD3_INDEX, INDEX_NO);
    indexDesc = PF_OpenFile(indexFileName);
    if (indexDesc < 0)
    {
        fprintf(stderr, "ERROR: cannot open index file\n");
        free(sorted_entries);
        return -1.0;
    }
    printf("  index file ready\n\n");

    start = clock();

    /* Build leaf pages by packing sorted entries */
    printf("Building leaf pages...\n");
    leaf_pages = (int *)malloc(num_leaf_pages * sizeof(int));
    int entry_idx = 0;

    for (i = 0; i < num_leaf_pages; i++)
    {
        int pageNum;
        char *pageBuf;
        int entries_in_page;
        int next_leaf_page;

        if (PF_AllocPage(indexDesc, &pageNum, &pageBuf) < 0)
        {
            fprintf(stderr, "ERROR: PF_AllocPage failed for leaf\n");
            free(sorted_entries);
            free(leaf_pages);
            return -1.0;
        }

        leaf_pages[i] = pageNum;
        entries_in_page = (entry_idx + fill_factor <= num_records) ? fill_factor : (num_records - entry_idx);
        next_leaf_page = (i < num_leaf_pages - 1) ? (pageNum + 1) : -1;

        /* populate header */
        char *ptr = pageBuf;
        *ptr++ = 'L';
        memcpy(ptr, &next_leaf_page, sizeof(int));
        ptr += sizeof(int);
        short s = 0;
        memcpy(ptr, &s, sizeof(short));
        ptr += sizeof(short);
        memcpy(ptr, &s, sizeof(short));
        ptr += sizeof(short);
        memcpy(ptr, &s, sizeof(short));
        ptr += sizeof(short);
        memcpy(ptr, &s, sizeof(short));
        ptr += sizeof(short);
        short attr_len = ROLL_NO_LENGTH;
        memcpy(ptr, &attr_len, sizeof(short));
        ptr += sizeof(short);
        short num_keys = entries_in_page;
        memcpy(ptr, &num_keys, sizeof(short));
        ptr += sizeof(short);
        short max_keys = fill_factor;
        memcpy(ptr, &max_keys, sizeof(short));
        ptr += sizeof(short);

        for (j = 0; j < entries_in_page && entry_idx < num_records; j++, entry_idx++)
        {
            memcpy(ptr, sorted_entries[entry_idx].roll_no, ROLL_NO_LENGTH);
            ptr += ROLL_NO_LENGTH;
            int recId = RecIdToInt(sorted_entries[entry_idx].recId);
            memcpy(ptr, &recId, sizeof(int));
            ptr += sizeof(int);
        }

        if (PF_UnfixPage(indexDesc, pageNum, TRUE) < 0)
        {
            fprintf(stderr, "ERROR: PF_UnfixPage failed for leaf\n");
            free(sorted_entries);
            free(leaf_pages);
            return -1.0;
        }

        if ((i + 1) % 10 == 0 || i == num_leaf_pages - 1)
        {
            printf("\r  Leaf pages created: %d/%d", i + 1, num_leaf_pages);
            fflush(stdout);
        }
    }
    printf("\n  All leaf pages created (%d)\n\n", num_leaf_pages);

    /* Build internal nodes upward from leaf pages */
    printf("Building internal levels bottom-up...\n");
    int *child_pages = leaf_pages;
    int num_children = num_leaf_pages;
    int level = 1;

    while (num_children > 1)
    {
        num_parent_pages = (num_children + max_entries_per_internal) / (max_entries_per_internal + 1);
        parent_pages = (int *)malloc(num_parent_pages * sizeof(int));

        printf("  Level %d: creating %d internal nodes\n", level, num_parent_pages);

        int child_idx = 0;
        for (i = 0; i < num_parent_pages; i++)
        {
            int pageNum;
            char *pageBuf;
            int children_in_node;

            if (PF_AllocPage(indexDesc, &pageNum, &pageBuf) < 0)
            {
                fprintf(stderr, "ERROR: PF_AllocPage failed for internal node\n");
                free(sorted_entries);
                free(child_pages);
                free(parent_pages);
                return -1.0;
            }

            parent_pages[i] = pageNum;
            children_in_node = ((child_idx + max_entries_per_internal + 1) <= num_children) ? (max_entries_per_internal + 1) : (num_children - child_idx);

            char *ptr = pageBuf;
            *ptr++ = 'I';
            short num_keys = children_in_node - 1;
            memcpy(ptr, &num_keys, sizeof(short));
            ptr += sizeof(short);
            short max_keys_int = max_entries_per_internal;
            memcpy(ptr, &max_keys_int, sizeof(short));
            ptr += sizeof(short);
            short attr_len = ROLL_NO_LENGTH;
            memcpy(ptr, &attr_len, sizeof(short));
            ptr += sizeof(short);

            int first_child = child_pages[child_idx];
            memcpy(ptr, &first_child, sizeof(int));
            ptr += sizeof(int);
            child_idx++;

            for (j = 0; j < num_keys && child_idx < num_children; j++, child_idx++)
            {
                char separator_key[ROLL_NO_LENGTH];
                int sep_entry_idx = child_idx * fill_factor;
                if (sep_entry_idx < num_records)
                {
                    memcpy(separator_key, sorted_entries[sep_entry_idx].roll_no, ROLL_NO_LENGTH);
                }
                memcpy(ptr, separator_key, ROLL_NO_LENGTH);
                ptr += ROLL_NO_LENGTH;

                int child_ptr = child_pages[child_idx];
                memcpy(ptr, &child_ptr, sizeof(int));
                ptr += sizeof(int);
            }

            if (PF_UnfixPage(indexDesc, pageNum, TRUE) < 0)
            {
                fprintf(stderr, "ERROR: PF_UnfixPage failed for internal node\n");
                free(sorted_entries);
                free(child_pages);
                free(parent_pages);
                return -1.0;
            }
        }

        if (child_pages != leaf_pages)
        {
            free(child_pages);
        }
        child_pages = parent_pages;
        num_children = num_parent_pages;
        level++;
    }

    root_page = parent_pages[0];
    printf("  Root created at page %d\n", root_page);
    printf("  Tree built with %d levels\n\n", level);

    end = clock();

    PF_CloseFile(indexDesc);
    free(sorted_entries);
    free(leaf_pages);
    if (num_parent_pages > 0 && parent_pages != leaf_pages)
    {
        free(parent_pages);
    }

    build_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    double total_time = sort_time + build_time;

    printf("Method 3 complete (bulk-load)\n");
    printf("  sort: %.3f s, build: %.3f s, total: %.3f s (%.0f rec/s)\n",
           sort_time, build_time, total_time, num_records / total_time);
    printf("  pages: %d leaf + %d internal = %d total\n", num_leaf_pages, num_parent_pages,
           num_leaf_pages + num_parent_pages);
    printf("  target fill: 90%%\n\n");

    return total_time;
}

/* --------------------------------------------------------------------
   Nicely format comparison results and simple analysis
   -------------------------------------------------------------------- */
void print_comparison(MethodStats stats[], int num_methods)
{
    int i;
    double baseline = stats[0].build_time;
    double best_time = baseline;
    int best_method = 0;

    for (i = 0; i < num_methods; i++)
    {
        if (stats[i].build_time < best_time)
        {
            best_time = stats[i].build_time;
            best_method = i;
        }
    }

    printf("\nIndex Build Comparison:\n\n");
    printf("Method                     | Records    | Time(s)    | Rate(rec/s) | Speedup\n");
    printf("--------------------------------------------------------------------------\n");

    for (i = 0; i < num_methods; i++)
    {
        double speedup = baseline / stats[i].build_time;
        double rate = stats[i].num_records / stats[i].build_time;
        char marker = (i == best_method) ? '*' : ' ';
        printf("%c %-24s | %10d | %10.3f | %11.0f | %7.2fx\n",
               marker, stats[i].method_name, stats[i].num_records,
               stats[i].build_time, rate, speedup);
    }

    printf("\n* = best method\n\n");

    for (i = 0; i < num_methods; i++)
    {
        double improvement = ((baseline - stats[i].build_time) / baseline) * 100.0;
        double time_diff = stats[i].build_time - baseline;

        printf("Method %d: %s\n", i + 1, stats[i].method_name);
        printf("  Time: %.3f s", stats[i].build_time);

        if (i == 0)
        {
            printf(" (baseline)\n");
        }
        else if (improvement > 0)
        {
            printf(" (%.1f%% faster, saved %.3fs)\n", improvement, -time_diff);
        }
        else
        {
            printf(" (%.1f%% slower, added %.3fs)\n", -improvement, time_diff);
        }

        printf("  Throughput: %.0f rec/s\n", stats[i].num_records / stats[i].build_time);
        if (i == best_method)
        {
            printf("  >> BEST PERFORMANCE\n");
        }
        printf("\n");
    }

    printf("Summary:\n");
    printf(" Method 1 - bulk on existing file: good for indexing pre-existing data.\n");
    printf(" Method 2 - incremental: models continuous inserts; similar costs to 1.\n");
    printf(" Method 3 - bulk-loading: fastest when data can be presorted; avoids splits.\n\n");

    printf("Recommendation:\n");
    if (best_method == 2)
    {
        printf("  Use bulk-loading (Method 3) when possible for best throughput.\n");
    }
    else
    {
        printf("  For this data size methods show comparable performance; bulk-load\n");
        printf("  advantages grow with larger datasets or higher tree fanout.\n");
    }
    printf("\n");
}

/* --------------------------------------------------------------------
   Main driver
   -------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    StudentEntry *entries;
    int num_records;
    int max_records = MAX_RECORDS;
    MethodStats stats[3];

    if (argc > 1)
    {
        max_records = atoi(argv[1]);
        if (max_records <= 0 || max_records > MAX_RECORDS)
        {
            fprintf(stderr, "ERROR: invalid record count: %d\n", max_records);
            fprintf(stderr, "Usage: %s [num_records (1-%d)]\n", argv[0], MAX_RECORDS);
            exit(1);
        }
    }

    printf("INDEX BUILDING COMPARISON - Objective 3 (toydb AM layer)\n");
    printf("Records to process: %d\n\n", max_records);

    printf("Initializing PF layer...\n");
    PF_Init();
    printf("PF layer initialized\n");

    entries = (StudentEntry *)malloc(max_records * sizeof(StudentEntry));
    if (!entries)
    {
        fprintf(stderr, "ERROR: memory allocation failed\n");
        exit(1);
    }

    num_records = load_student_data(entries, max_records);
    if (num_records < 0)
    {
        free(entries);
        exit(1);
    }

    printf("\nRemoving any existing index files...\n");
    AM_DestroyIndex(METHOD1_INDEX, INDEX_NO);
    AM_DestroyIndex(METHOD2_INDEX, INDEX_NO);
    AM_DestroyIndex(METHOD3_INDEX, INDEX_NO);

    stats[0].build_time = method1_bulk_creation(entries, num_records);
    stats[0].num_records = num_records;
    stats[0].method_name = "Method 1: Bulk Creation";

    stats[1].build_time = method2_incremental(entries, num_records);
    stats[1].num_records = num_records;
    stats[1].method_name = "Method 2: Incremental";

    stats[2].build_time = method3_bulk_load(entries, num_records);
    stats[2].num_records = num_records;
    stats[2].method_name = "Method 3: Bulk-Loading";

    print_comparison(stats, 3);

    free(entries);

    printf("Test finished. Index files produced:\n");
    printf("  - %s.%d (Method 1)\n", METHOD1_INDEX, INDEX_NO);
    printf("  - %s.%d (Method 2)\n", METHOD2_INDEX, INDEX_NO);
    printf("  - %s.%d (Method 3)\n\n", METHOD3_INDEX, INDEX_NO);

    return 0;
}
