#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "student_file.h"

#define MAX_TESTS 10

/* Structure to hold results for each experiment */
typedef struct
{
    char method_name[50];
    int records;
    int file_size;
    double utilization;
    int avg_record_size;
    double insert_rate;
    double scan_rate;
    int pages;
    int wasted_space;
} TestResult;

/* Collection of experiment outcomes */
TestResult outcomes[MAX_TESTS];
int outcome_count = 0;

/* Save one experiment outcome */
void store_result(const char *method, int records, int file_size, double util,
                  int avg_size, double ins_rate, double scan_rate,
                  int pages, int wasted)
{
    if (outcome_count < MAX_TESTS)
    {
        strncpy(outcomes[outcome_count].method_name, method, sizeof(outcomes[outcome_count].method_name) - 1);
        outcomes[outcome_count].records = records;
        outcomes[outcome_count].file_size = file_size;
        outcomes[outcome_count].utilization = util;
        outcomes[outcome_count].avg_record_size = avg_size;
        outcomes[outcome_count].insert_rate = ins_rate;
        outcomes[outcome_count].scan_rate = scan_rate;
        outcomes[outcome_count].pages = pages;
        outcomes[outcome_count].wasted_space = wasted;
        outcome_count++;
    }
}

/* Print a summary table and simple analysis */
void print_results_table()
{
    printf("\n\n");
    printf("==================================================================================================\n");
    printf("                           PERFORMANCE COMPARISON TABLE\n");
    printf("==================================================================================================\n");
    printf("%-18s %-10s %-12s %-12s %-12s %-10s\n",
           "Storage Method", "Records", "File Size", "Utilization", "Avg Rec", "Pages");
    printf("%-18s %-10s %-12s %-12s %-12s %-10s\n",
           "", "", "(KB)", "(%)", "(bytes)", "");
    printf("--------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < outcome_count; i++)
    {
        printf("%-18s %-10d %-12.2f %-12.2f %-12d %-10d\n",
               outcomes[i].method_name,
               outcomes[i].records,
               outcomes[i].file_size / 1024.0,
               outcomes[i].utilization,
               outcomes[i].avg_record_size,
               outcomes[i].pages);
    }

    printf("==================================================================================================\n\n");

    printf("==================================================================================================\n");
    printf("                           PERFORMANCE METRICS\n");
    printf("==================================================================================================\n");
    printf("%-18s %-15s %-15s %-15s\n",
           "Storage Method", "Insert Rate", "Scan Rate", "Wasted Space");
    printf("%-18s %-15s %-15s %-15s\n",
           "", "(rec/sec)", "(rec/sec)", "(KB)");
    printf("--------------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < outcome_count; i++)
    {
        printf("%-18s %-15.0f %-15.0f %-15.2f\n",
               outcomes[i].method_name,
               outcomes[i].insert_rate,
               outcomes[i].scan_rate,
               outcomes[i].wasted_space / 1024.0);
    }

    printf("==================================================================================================\n\n");

    /* Simple comparative commentary */
    printf("==================================================================================================\n");
    printf("                                    ANALYSIS\n");
    printf("==================================================================================================\n\n");

    int best_space_idx = 0, worst_space_idx = 0;
    for (int i = 1; i < outcome_count; i++)
    {
        if (outcomes[i].file_size < outcomes[best_space_idx].file_size)
        {
            best_space_idx = i;
        }
        if (outcomes[i].file_size > outcomes[worst_space_idx].file_size)
        {
            worst_space_idx = i;
        }
    }

    printf("Space Efficiency:\n");
    printf("  Best: %s (%.2f KB)\n",
           outcomes[best_space_idx].method_name,
           outcomes[best_space_idx].file_size / 1024.0);
    printf("  Worst: %s (%.2f KB)\n",
           outcomes[worst_space_idx].method_name,
           outcomes[worst_space_idx].file_size / 1024.0);
    printf("  Relative savings: %.2f%% (%.2f KB saved)\n",
           (1.0 - (double)outcomes[best_space_idx].file_size / outcomes[worst_space_idx].file_size) * 100,
           (outcomes[worst_space_idx].file_size - outcomes[best_space_idx].file_size) / 1024.0);

    printf("\n");
    printf("Key Observations:\n");
    printf("  1. Slotted pages offer noticeable savings for variable-length entries\n");
    if (outcome_count > 0)
    {
        printf("  2. Observed average record size: ~%d bytes\n",
               outcomes[0].avg_record_size);
        printf("  3. Observed utilization: %.2f%%\n", outcomes[0].utilization);
    }
    printf("  4. There is a complexity vs. space-efficiency trade-off\n");

    printf("\n==================================================================================================\n");
}

/* Return time in seconds with microsecond precision */
double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/* Parse one line from student.txt into StudentRecord (simple parser) */
int parse_student_data(char *line, StudentRecord *rec)
{
    char *token;
    char *saveptr;

    char *nl = strchr(line, '\n');
    if (nl)
        *nl = '\0';

    /* skip header line if present */
    if (strstr(line, "Database dummy"))
    {
        return -1;
    }

    memset(rec, 0, sizeof(StudentRecord));

    token = strtok_r(line, ";", &saveptr);
    if (!token)
        return -1;
    strncpy(rec->roll_no, token, sizeof(rec->roll_no) - 1);

    token = strtok_r(NULL, ";", &saveptr);
    if (token)
        strncpy(rec->name, token, sizeof(rec->name) - 1);

    token = strtok_r(NULL, ";", &saveptr);
    if (token)
        strncpy(rec->batch, token, sizeof(rec->batch) - 1);

    token = strtok_r(NULL, ";", &saveptr);
    if (token)
        strncpy(rec->sex, token, sizeof(rec->sex) - 1);

    for (int i = 0; i < 10; i++)
    {
        token = strtok_r(NULL, ";", &saveptr);
        if (!token)
            break;

        if (i == 0)
            strncpy(rec->father_name, token, sizeof(rec->father_name) - 1);
        else if (i == 1)
            strncpy(rec->address, token, sizeof(rec->address) - 1);
        else if (i == 2)
            strncpy(rec->city, token, sizeof(rec->city) - 1);
        else if (i == 3)
            strncpy(rec->state, token, sizeof(rec->state) - 1);
        else if (i == 4)
            strncpy(rec->birthdate, token, sizeof(rec->birthdate) - 1);
        else if (i == 5)
            strncpy(rec->pincode, token, sizeof(rec->pincode) - 1);
        else if (i == 6)
            rec->join_yr = atoi(token);
        else if (i == 7)
            strncpy(rec->degree, token, sizeof(rec->degree) - 1);
        else if (i == 8)
            strncpy(rec->dept, token, sizeof(rec->dept) - 1);
        else if (i == 9)
            strncpy(rec->categ, token, sizeof(rec->categ) - 1);
    }

    return 0;
}

/* Run the slotted-page experiment and collect stats */
void test_slotted_page(int max_records)
{
    printf("\n========================================\n");
    printf("Running Slotted Page (variable-length)\n");
    printf("========================================\n");

    StudentFile sf;
    char *filename = "test_slotted.db";

    SF_CreateFile(filename);
    SF_OpenFile(filename, &sf);

    FILE *fp = fopen("../data/student.txt", "r");
    if (!fp)
    {
        printf("Error: Cannot open data/student.txt\n");
        return;
    }

    char line[2048];
    int loaded = 0;
    double start = get_time();

    while (fgets(line, sizeof(line), fp))
    {
        if (max_records > 0 && loaded >= max_records)
            break;

        StudentRecord rec;
        if (parse_student_data(line, &rec) == 0)
        {
            RecordID rid;
            if (SF_InsertStudent(&sf, &rec, &rid) == SP_OK)
            {
                loaded++;
                if (loaded % 5000 == 0)
                {
                    printf("  Loaded %d records...\n", loaded);
                }
            }
        }
    }

    double insert_time = get_time() - start;
    fclose(fp);

    printf("\nInsertion summary:\n");
    printf("  Total records: %d\n", loaded);
    printf("  Duration: %.3f seconds\n", insert_time);
    printf("  Throughput: %.0f records/sec\n", loaded / insert_time);

    SpaceStats stats;
    SF_GetSpaceStats(&sf, &stats);

    printf("\nSpace summary:\n");
    printf("  Pages: %d\n", stats.total_pages);
    printf("  File size: %.2f KB\n", stats.total_space / 1024.0);
    printf("  Used: %.2f KB\n", stats.used_space / 1024.0);
    printf("  Utilization: %.2f%%\n", stats.utilization_pct);
    printf("  Avg record size: %d bytes\n", stats.used_space / (loaded > 0 ? loaded : 1));
    printf("  Records per page: %.2f\n", stats.avg_records_per_page);

    SF_ScanHandle scan;
    StudentRecord rec;
    int scan_count = 0;

    start = get_time();
    SF_OpenScan(&scan, &sf);
    while (SF_GetNextStudent(&scan, &rec) == SP_OK)
    {
        scan_count++;
    }
    double scan_time = get_time() - start;
    SF_CloseScan(&scan);

    printf("\nScan summary:\n");
    printf("  Records scanned: %d\n", scan_count);
    printf("  Duration: %.3f seconds\n", scan_time);
    printf("  Throughput: %.0f records/sec\n", scan_count / scan_time);

    SF_CloseFile(&sf);

    store_result("Slotted Page", loaded, stats.total_space, stats.utilization_pct,
                 stats.used_space / (loaded > 0 ? loaded : 1), loaded / insert_time,
                 scan_count / scan_time, stats.total_pages, stats.fragmented_space);
}

/* Fixed-length (static) storage experiment */
void test_static_records(int max_records, int record_size)
{
    printf("\n========================================\n");
    printf("Running Static Records (%d bytes)\n", record_size);
    printf("========================================\n");

    char filename[256];
    sprintf(filename, "test_static_%d.db", record_size);

    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    char header[4096] = {0};
    write(fd, header, 4096);

    FILE *fp = fopen("../data/student.txt", "r");
    if (!fp)
    {
        printf("Error: Cannot open data/student.txt\n");
        close(fd);
        return;
    }

    char line[2048];
    int loaded = 0;
    double start = get_time();

    int total_actual_data = 0;

    while (fgets(line, sizeof(line), fp))
    {
        if (max_records > 0 && loaded >= max_records)
            break;

        StudentRecord rec;
        if (parse_student_data(line, &rec) == 0)
        {
            char buffer[2048];
            short len;
            serialize_student(&rec, buffer, &len);

            total_actual_data += len;

            char *slot = malloc(record_size);
            memset(slot, 0, record_size);
            int copy_len = (len < record_size) ? len : record_size - 1;
            memcpy(slot, buffer, copy_len);

            write(fd, slot, record_size);
            free(slot);
            loaded++;

            if (loaded % 5000 == 0)
            {
                printf("  Loaded %d records...\n", loaded);
            }
        }
    }

    double insert_time = get_time() - start;
    fclose(fp);

    printf("\nInsertion summary:\n");
    printf("  Total records: %d\n", loaded);
    printf("  Duration: %.3f seconds\n", insert_time);
    printf("  Throughput: %.0f records/sec\n", loaded / insert_time);

    off_t file_size = lseek(fd, 0, SEEK_END);
    int total_space = file_size - 4096;
    int pages = (total_space + 4095) / 4096;

    printf("\nSpace summary:\n");
    printf("  Pages (equivalent): %d\n", pages);
    printf("  File size: %.2f KB\n", total_space / 1024.0);
    printf("  Fixed record size: %d bytes\n", record_size);
    printf("  Actual data used: %.2f KB\n", total_actual_data / 1024.0);
    printf("  Avg wasted per record: %.2f bytes\n", record_size - (total_actual_data / (float)loaded));
    double util_pct = (double)total_actual_data / total_space * 100;
    printf("  Utilization: %.2f%%\n", util_pct);

    lseek(fd, 4096, SEEK_SET);
    char *record = malloc(record_size);
    int scan_count = 0;

    start = get_time();
    while (read(fd, record, record_size) == record_size)
    {
        scan_count++;
    }
    double scan_time = get_time() - start;

    printf("\nScan summary:\n");
    printf("  Records scanned: %d\n", scan_count);
    printf("  Duration: %.3f seconds\n", scan_time);
    printf("  Throughput: %.0f records/sec\n", scan_count / scan_time);

    free(record);
    close(fd);

    char method_name[50];
    sprintf(method_name, "Static (%dB)", record_size);
    int wasted = total_space - total_actual_data;
    double actual_utilization = (double)total_actual_data / total_space * 100;
    store_result(method_name, loaded, total_space, actual_utilization, record_size,
                 loaded / insert_time, scan_count / scan_time, pages, wasted);
}

int main(int argc, char *argv[])
{
    int max_records = 0;

    if (argc > 1)
    {
        max_records = atoi(argv[1]);
    }

    printf("===========================================================\n");
    printf("   OBJECTIVE 2: Slotted Page Performance Analysis\n");
    printf("===========================================================\n");
    if (max_records > 0)
    {
        printf("Running with up to %d records from data/student.txt\n", max_records);
    }
    else
    {
        printf("Running with ALL records from data/student.txt\n");
    }

    outcome_count = 0;

    test_slotted_page(max_records);
    test_static_records(max_records, 256);
    test_static_records(max_records, 512);
    test_static_records(max_records, 1024);

    print_results_table();

    printf("\nGenerated test files:\n");
    printf("  - test_slotted.db\n");
    printf("  - test_static_256.db\n");
    printf("  - test_static_512.db\n");
    printf("  - test_static_1024.db\n\n");

    return 0;
}
