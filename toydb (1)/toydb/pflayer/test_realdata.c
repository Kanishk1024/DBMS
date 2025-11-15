#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "pf.h"

#define MAX_LINE_LENGTH 1024
#define MAX_RECORDS_PER_PAGE 40

// CSV output file handles
FILE *csv_lru = NULL;
FILE *csv_mru = NULL;
int csv_mode = 0;  // 0 = console only, 1 = console + CSV

typedef struct {
    char *filename;
    char *db_filename;
    int num_records;
} DataFile;

// Relative paths from pflayer directory to data folder
DataFile data_files[] = {
    {"../../../data/student.txt", "student.db", 0},
    {"../../../data/courses.txt", "courses.db", 0},
    {"../../../data/department.txt", "department.db", 0},
    {"../../../data/program.txt", "program.db", 0},
    {"../../../data/studemail.txt", "studemail.db", 0},
    {NULL, NULL, 0}
};

int load_data_to_db(const char *text_file, const char *db_file, int *record_count) {
    FILE *fp = fopen(text_file, "r");
    if (!fp) {
        printf("Warning: Cannot open %s, skipping...\n", text_file);
        return -1;
    }
    
    static int pf_initialized = 0;
    if (!pf_initialized) {
        PF_Init();
        pf_initialized = 1;
    }
    
    if (PF_CreateFile(db_file) != PFE_OK) {
        fclose(fp);
        return -1;
    }
    
    int fd = PF_OpenFile(db_file);
    if (fd < 0) {
        fclose(fp);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    int total_records = 0;
    int records_in_page = 0;
    char *pagebuf = NULL;
    int pagenum;
    int offset = 0;
    
    // Skip header
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        PF_CloseFile(fd);
        PF_DestroyFile(db_file);
        return -1;
    }
    
    // Allocate first page
    if (PF_AllocPage(fd, &pagenum, &pagebuf) != PFE_OK) {
        fclose(fp);
        PF_CloseFile(fd);
        PF_DestroyFile(db_file);
        return -1;
    }
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        int line_len = strlen(line);
        
        if (offset + line_len >= PF_PAGE_SIZE || records_in_page >= MAX_RECORDS_PER_PAGE) {
            PF_UnfixPage(fd, pagenum, TRUE);
            
            if (PF_AllocPage(fd, &pagenum, &pagebuf) != PFE_OK) {
                fclose(fp);
                PF_CloseFile(fd);
                PF_DestroyFile(db_file);
                return -1;
            }
            offset = 0;
            records_in_page = 0;
        }
        
        memcpy(pagebuf + offset, line, line_len);
        offset += line_len;
        records_in_page++;
        total_records++;
    }
    
    if (pagebuf != NULL) {
        PF_UnfixPage(fd, pagenum, TRUE);
    }
    
    fclose(fp);
    PF_CloseFile(fd);
    
    *record_count = total_records;
    return 0;
}

void run_read_write_mixture(const char *db_file, int num_pages, int num_ops, int read_pct, int write_pct) {
    int fd = PF_OpenFile(db_file);
    if (fd < 0) {
        printf("Failed to open %s\n", db_file);
        return;
    }
    
    char *pagebuf;
    
    for (int i = 0; i < num_ops; i++) {
        int pagenum = rand() % num_pages;
        int op = rand() % 100;
        
        if (op < read_pct) {
            // Read operation
            if (PF_GetThisPage(fd, pagenum, &pagebuf) == PFE_OK) {
                volatile int value = *(int*)pagebuf;
                (void)value;
                PF_UnfixPage(fd, pagenum, FALSE);
            }
        } else {
            // Write operation
            if (PF_GetThisPage(fd, pagenum, &pagebuf) == PFE_OK) {
                *(int*)pagebuf = i;
                PF_UnfixPage(fd, pagenum, TRUE);
            }
        }
    }
    
    PF_CloseFile(fd);
}

void test_with_real_data(DataFile *df, ReplacementStrategy strategy, const char *strategy_name) {
    printf("\n=== Testing %s with %s strategy ===\n", df->filename, strategy_name);
    
    BUF_SetStrategy(strategy);
    
    int num_pages = (df->num_records / MAX_RECORDS_PER_PAGE) + 1;
    if (num_pages < 10) num_pages = 10;
    
    printf("Records: %d, Estimated pages: %d\n\n", df->num_records, num_pages);
    
    // Different read/write mixtures as per project requirements
    int mixtures[][2] = {
        {100, 0},   // 100% read
        {90, 10},   // 90% read, 10% write
        {80, 20},   // 80% read, 20% write
        {70, 30},   // 70% read, 30% write
        {60, 40},   // 60% read, 40% write
        {50, 50},   // 50% read, 50% write
        {40, 60},   // 40% read, 60% write
        {30, 70},   // 30% read, 70% write
        {20, 80},   // 20% read, 80% write
        {10, 90},   // 10% read, 90% write
        {0, 100}    // 100% write
    };
    int num_mixtures = 11;
    int num_ops = 5000;
    
    printf("Read%%\tWrite%%\tHits\tMisses\tPhysRead\tPhysWrite\tHitRatio\n");
    printf("--------------------------------------------------------------------\n");
    
    // Select CSV file based on strategy
    FILE *csv_file = (strategy == REPLACE_LRU) ? csv_lru : csv_mru;
    
    for (int i = 0; i < num_mixtures; i++) {
        BUF_ResetStatistics();
        run_read_write_mixture(df->db_filename, num_pages, num_ops, mixtures[i][0], mixtures[i][1]);
        
        BufferStats stats;
        BUF_GetStatistics(&stats);
        
        printf("%d\t%d\t%ld\t%ld\t%ld\t\t%ld\t\t%.4f\n",
            mixtures[i][0], mixtures[i][1],
            stats.buffer_hits, stats.buffer_misses,
            stats.physical_reads, stats.physical_writes,
            stats.hit_ratio);
        
        // Write to CSV if in CSV mode
        if (csv_mode && csv_file) {
            // Extract just the filename without path
            const char *fname = strrchr(df->filename, '/');
            fname = fname ? fname + 1 : df->filename;
            
            fprintf(csv_file, "%s,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%.4f\n",
                fname, mixtures[i][0], mixtures[i][1], num_pages,
                stats.logical_reads, stats.logical_writes,
                stats.physical_reads, stats.physical_writes,
                stats.buffer_hits, stats.buffer_misses,
                stats.hit_ratio);
        }
    }
}

int main(int argc, char *argv[]) {
    printf("=========================================\n");
    printf("Real Data Buffer Management Test\n");
    printf("=========================================\n\n");
    
    // Check for CSV mode
    if (argc > 1 && strcmp(argv[1], "--csv") == 0) {
        csv_mode = 1;
        printf("CSV mode enabled - generating realdata_lru.csv and realdata_mru.csv\n\n");
        
        csv_lru = fopen("realdata_lru.csv", "w");
        csv_mru = fopen("realdata_mru.csv", "w");
        
        if (!csv_lru || !csv_mru) {
            fprintf(stderr, "Error: Failed to create CSV files\n");
            return 1;
        }
        
        // Write CSV headers
        fprintf(csv_lru, "Dataset,ReadPct,WritePct,NumPages,LogicalReads,LogicalWrites,PhysicalReads,PhysicalWrites,BufferHits,BufferMisses,HitRatio\n");
        fprintf(csv_mru, "Dataset,ReadPct,WritePct,NumPages,LogicalReads,LogicalWrites,PhysicalReads,PhysicalWrites,BufferHits,BufferMisses,HitRatio\n");
    }
    
    srand(time(NULL));
    
    printf("Loading data files...\n");
    for (int i = 0; data_files[i].filename != NULL; i++) {
        printf("  Loading %s... ", data_files[i].filename);
        fflush(stdout);
        
        if (load_data_to_db(data_files[i].filename, 
                           data_files[i].db_filename,
                           &data_files[i].num_records) == 0) {
            printf("Done (%d records)\n", data_files[i].num_records);
        } else {
            printf("Skipped\n");
        }
    }
    
    printf("\n");
    
    for (int i = 0; data_files[i].filename != NULL; i++) {
        if (data_files[i].num_records > 0) {
            test_with_real_data(&data_files[i], REPLACE_LRU, "LRU");
            test_with_real_data(&data_files[i], REPLACE_MRU, "MRU");
            printf("\n");
        }
    }
    
    printf("Cleaning up database files...\n");
    for (int i = 0; data_files[i].filename != NULL; i++) {
        if (data_files[i].num_records > 0) {
            PF_DestroyFile(data_files[i].db_filename);
        }
    }
    
    // Close CSV files if open
    if (csv_mode) {
        if (csv_lru) {
            fclose(csv_lru);
            printf("\nCSV file generated: realdata_lru.csv\n");
        }
        if (csv_mru) {
            fclose(csv_mru);
            printf("CSV file generated: realdata_mru.csv\n");
        }
    }
    
    printf("\nTest completed successfully!\n");
    return 0;
}
