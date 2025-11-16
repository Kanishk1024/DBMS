#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "pf.h"

#define MAX_LINE_LENGTH 1024
#define RECORDS_PER_PAGE_LIMIT 40

/* CSV handles */
static FILE *out_csv_lru = NULL;
static FILE *out_csv_mru = NULL;
static int enable_csv = 0;

typedef struct
{
    const char *text_path;
    const char *db_name;
    int record_count;
} TextDataset;

/* relative paths from pflayer */
static TextDataset datasets[] = {
    {"../../../data/student.txt", "student.db", 0},
    {"../../../data/courses.txt", "courses.db", 0},
    {"../../../data/department.txt", "department.db", 0},
    {"../../../data/program.txt", "program.db", 0},
    {"../../../data/studemail.txt", "studemail.db", 0},
    {NULL, NULL, 0}};

static int import_text_into_db(const char *txtfile, const char *dbfile, int *out_count)
{
    FILE *fin = fopen(txtfile, "r");
    if (!fin)
    {
        printf("Warning: %s not found, skipping\n", txtfile);
        return -1;
    }

    static int pf_is_ready = 0;
    if (!pf_is_ready)
    {
        PF_Init();
        pf_is_ready = 1;
    }

    if (PF_CreateFile(dbfile) != PFE_OK)
    {
        fclose(fin);
        return -1;
    }

    int fd = PF_OpenFile(dbfile);
    if (fd < 0)
    {
        fclose(fin);
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int total = 0;
    int in_page = 0;
    char *pagebuf = NULL;
    int pnum = 0;
    int used = 0;

    /* skip header line */
    if (fgets(line, sizeof(line), fin) == NULL)
    {
        fclose(fin);
        PF_CloseFile(fd);
        PF_DestroyFile(dbfile);
        return -1;
    }

    if (PF_AllocPage(fd, &pnum, &pagebuf) != PFE_OK)
    {
        fclose(fin);
        PF_CloseFile(fd);
        PF_DestroyFile(dbfile);
        return -1;
    }

    while (fgets(line, sizeof(line), fin) != NULL)
    {
        int len = strlen(line);
        if (used + len >= PF_PAGE_SIZE || in_page >= RECORDS_PER_PAGE_LIMIT)
        {
            PF_UnfixPage(fd, pnum, TRUE);
            if (PF_AllocPage(fd, &pnum, &pagebuf) != PFE_OK)
            {
                fclose(fin);
                PF_CloseFile(fd);
                PF_DestroyFile(dbfile);
                return -1;
            }
            used = 0;
            in_page = 0;
        }
        memcpy(pagebuf + used, line, len);
        used += len;
        in_page++;
        total++;
    }

    if (pagebuf)
        PF_UnfixPage(fd, pnum, TRUE);

    fclose(fin);
    PF_CloseFile(fd);

    *out_count = total;
    return 0;
}

static void simulate_io_mix(const char *dbfile, int page_count, int ops, int read_percent, int write_percent)
{
    int fd = PF_OpenFile(dbfile);
    if (fd < 0)
    {
        printf("Unable to open %s\n", dbfile);
        return;
    }

    char *pagebuf;
    for (int i = 0; i < ops; ++i)
    {
        int pnum = rand() % page_count;
        int choice = rand() % 100;
        if (choice < read_percent)
        {
            if (PF_GetThisPage(fd, pnum, &pagebuf) == PFE_OK)
            {
                volatile int tmp = *(int *)pagebuf;
                (void)tmp;
                PF_UnfixPage(fd, pnum, FALSE);
            }
        }
        else
        {
            if (PF_GetThisPage(fd, pnum, &pagebuf) == PFE_OK)
            {
                *(int *)pagebuf = i;
                PF_UnfixPage(fd, pnum, TRUE);
            }
        }
    }

    PF_CloseFile(fd);
}

static void evaluate_dataset(TextDataset *ds, ReplacementStrategy strat, const char *strat_name)
{
    printf("\n--- Dataset: %s (strategy: %s) ---\n", ds->text_path, strat_name);

    BUF_SetStrategy(strat);

    int estimated_pages = (ds->record_count / RECORDS_PER_PAGE_LIMIT) + 1;
    if (estimated_pages < 10)
        estimated_pages = 10;

    printf("Records: %d  Estimated pages: %d\n\n", ds->record_count, estimated_pages);

    int mixes[][2] = {
        {100, 0}, {90, 10}, {80, 20}, {70, 30}, {60, 40}, {50, 50}, {40, 60}, {30, 70}, {20, 80}, {10, 90}, {0, 100}};
    const int mixes_n = sizeof(mixes) / sizeof(mixes[0]);
    const int ops = 5000;

    printf("Read%%\tWrite%%\tHits\tMisses\tPhysRead\tPhysWrite\tHitRatio\n");
    printf("--------------------------------------------------------------------\n");

    FILE *csv = (strat == REPLACE_LRU) ? out_csv_lru : out_csv_mru;

    for (int i = 0; i < mixes_n; ++i)
    {
        BUF_ResetStatistics();
        simulate_io_mix(ds->db_name, estimated_pages, ops, mixes[i][0], mixes[i][1]);

        BufferStats s;
        BUF_GetStatistics(&s);

        printf("%d\t%d\t%ld\t%ld\t%ld\t\t%ld\t\t%.4f\n",
               mixes[i][0], mixes[i][1],
               s.buffer_hits, s.buffer_misses,
               s.physical_reads, s.physical_writes,
               s.hit_ratio);

        if (enable_csv && csv)
        {
            const char *base = strrchr(ds->text_path, '/');
            base = base ? base + 1 : ds->text_path;
            fprintf(csv, "%s,%d,%d,%d,%ld,%ld,%ld,%ld,%ld,%ld,%.4f\n",
                    base,
                    mixes[i][0], mixes[i][1], estimated_pages,
                    s.logical_reads, s.logical_writes,
                    s.physical_reads, s.physical_writes,
                    s.buffer_hits, s.buffer_misses,
                    s.hit_ratio);
        }
    }
}

int main(int argc, char *argv[])
{
    printf("=========================================\n");
    printf("Real Data Buffer Management Test\n");
    printf("=========================================\n\n");

    if (argc > 1 && strcmp(argv[1], "--csv") == 0)
    {
        enable_csv = 1;
        out_csv_lru = fopen("realdata_lru.csv", "w");
        out_csv_mru = fopen("realdata_mru.csv", "w");
        if (!out_csv_lru || !out_csv_mru)
        {
            fprintf(stderr, "Error: cannot create CSV output\n");
            return 1;
        }
        fprintf(out_csv_lru, "Dataset,ReadPct,WritePct,NumPages,LogicalReads,LogicalWrites,PhysicalReads,PhysicalWrites,BufferHits,BufferMisses,HitRatio\n");
        fprintf(out_csv_mru, "Dataset,ReadPct,WritePct,NumPages,LogicalReads,LogicalWrites,PhysicalReads,PhysicalWrites,BufferHits,BufferMisses,HitRatio\n");
        printf("CSV mode on. Producing realdata_lru.csv and realdata_mru.csv\n\n");
    }

    srand((unsigned)time(NULL));

    printf("Importing data files...\n");
    for (int i = 0; datasets[i].text_path != NULL; ++i)
    {
        printf("  %s ... ", datasets[i].text_path);
        fflush(stdout);
        if (import_text_into_db(datasets[i].text_path, datasets[i].db_name, &datasets[i].record_count) == 0)
        {
            printf("loaded (%d records)\n", datasets[i].record_count);
        }
        else
        {
            printf("skipped\n");
        }
    }

    printf("\n");

    for (int i = 0; datasets[i].text_path != NULL; ++i)
    {
        if (datasets[i].record_count > 0)
        {
            evaluate_dataset(&datasets[i], REPLACE_LRU, "LRU");
            evaluate_dataset(&datasets[i], REPLACE_MRU, "MRU");
            printf("\n");
        }
    }

    printf("Removing temporary DB files...\n");
    for (int i = 0; datasets[i].text_path != NULL; ++i)
    {
        if (datasets[i].record_count > 0)
            PF_DestroyFile(datasets[i].db_name);
    }

    if (enable_csv)
    {
        if (out_csv_lru)
        {
            fclose(out_csv_lru);
            printf("\nCSV file generated: realdata_lru.csv\n");
        }
        if (out_csv_mru)
        {
            fclose(out_csv_mru);
            printf("CSV file generated: realdata_mru.csv\n");
        }
    }

    printf("\nTest completed successfully!\n");
    return 0;
}
