#ifndef STUDENT_FILE_H
#define STUDENT_FILE_H

#include "slotted_page.h"

#define MAX_NAME_LEN 100
#define MAX_ADDR_LEN 200

// Student record structure
typedef struct {
    char roll_no[20];
    char name[MAX_NAME_LEN];
    char batch[10];
    char degree[20];
    char dept[10];
    int join_yr;
    char categ[10];
    char sex[2];
    char father_name[MAX_NAME_LEN];
    char birthdate[20];
    char address[MAX_ADDR_LEN];
    char city[50];
    char state[50];
    char pincode[10];
} StudentRecord;

// Student file structure
typedef struct {
    int fd;
    int num_pages;
    int num_records;
    char filename[256];
} StudentFile;

// Scan handle
typedef struct {
    StudentFile *sf;
    SP_ScanHandle sp_handle;
} SF_ScanHandle;

// Space statistics
typedef struct {
    int total_pages;
    int total_space;
    int used_space;
    int slot_overhead;
    int header_overhead;
    int free_space;
    int fragmented_space;
    double utilization_pct;
    double avg_records_per_page;
} SpaceStats;

// Function prototypes
int SF_CreateFile(char *filename);
int SF_OpenFile(char *filename, StudentFile *sf);
int SF_CloseFile(StudentFile *sf);
int SF_InsertStudent(StudentFile *sf, StudentRecord *rec, RecordID *rid);
int SF_DeleteStudent(StudentFile *sf, RecordID rid);
int SF_GetStudent(StudentFile *sf, RecordID rid, StudentRecord *rec);
int SF_OpenScan(SF_ScanHandle *handle, StudentFile *sf);
int SF_GetNextStudent(SF_ScanHandle *handle, StudentRecord *rec);
int SF_CloseScan(SF_ScanHandle *handle);
void SF_GetSpaceStats(StudentFile *sf, SpaceStats *stats);

// Utility functions
void serialize_student(StudentRecord *rec, char *buffer, short *len);
void deserialize_student(char *buffer, short len, StudentRecord *rec);

#endif
