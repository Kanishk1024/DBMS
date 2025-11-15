#include "student_file.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// Serialize student record to buffer
void serialize_student(StudentRecord *rec, char *buffer, short *len) {
    sprintf(buffer, "%s;%s;%s;%s;%s;%d;%s;%s;%s;%s;%s;%s;%s;%s",
            rec->roll_no, rec->name, rec->batch, rec->degree,
            rec->dept, rec->join_yr, rec->categ, rec->sex,
            rec->father_name, rec->birthdate, rec->address,
            rec->city, rec->state, rec->pincode);
    
    *len = strlen(buffer) + 1;
}

// Deserialize buffer to student record
void deserialize_student(char *buffer, short len, StudentRecord *rec) {
    char temp[2048];
    strncpy(temp, buffer, len);
    temp[len] = '\0';
    
    char *token = strtok(temp, ";");
    if (token) strncpy(rec->roll_no, token, sizeof(rec->roll_no)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->name, token, sizeof(rec->name)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->batch, token, sizeof(rec->batch)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->degree, token, sizeof(rec->degree)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->dept, token, sizeof(rec->dept)-1);
    
    token = strtok(NULL, ";");
    if (token) rec->join_yr = atoi(token);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->categ, token, sizeof(rec->categ)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->sex, token, sizeof(rec->sex)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->father_name, token, sizeof(rec->father_name)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->birthdate, token, sizeof(rec->birthdate)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->address, token, sizeof(rec->address)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->city, token, sizeof(rec->city)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->state, token, sizeof(rec->state)-1);
    
    token = strtok(NULL, ";");
    if (token) strncpy(rec->pincode, token, sizeof(rec->pincode)-1);
}

// Create student file
int SF_CreateFile(char *filename) {
    int fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return SP_ERROR;
    }
    
    // Write file header (4096 bytes)
    char header[4096];
    memset(header, 0, 4096);
    if (write(fd, header, 4096) != 4096) {
        close(fd);
        return SP_ERROR;
    }
    
    close(fd);
    return SP_OK;
}

// Open student file
int SF_OpenFile(char *filename, StudentFile *sf) {
    sf->fd = open(filename, O_RDWR);
    if (sf->fd < 0) {
        perror("Failed to open file");
        return SP_ERROR;
    }
    
    // Get file size to determine number of pages
    off_t file_size = lseek(sf->fd, 0, SEEK_END);
    sf->num_pages = (file_size - 4096) / SP_PAGE_SIZE;
    sf->num_records = 0;
    strncpy(sf->filename, filename, sizeof(sf->filename)-1);
    
    return SP_OK;
}

// Close student file
int SF_CloseFile(StudentFile *sf) {
    if (sf->fd >= 0) {
        close(sf->fd);
        sf->fd = -1;
    }
    return SP_OK;
}

// Insert student record
int SF_InsertStudent(StudentFile *sf, StudentRecord *rec, RecordID *rid) {
    char buffer[2048];
    short len;
    
    serialize_student(rec, buffer, &len);
    
    // Try to insert into existing pages first
    char page_data[SP_PAGE_SIZE];
    for (int page_num = 0; page_num < sf->num_pages; page_num++) {
        off_t offset = page_num * SP_PAGE_SIZE + 4096;
        if (lseek(sf->fd, offset, SEEK_SET) < 0) continue;
        if (read(sf->fd, page_data, SP_PAGE_SIZE) != SP_PAGE_SIZE) continue;
        
        int slot_num;
        if (SP_InsertRecord(page_data, buffer, len, &slot_num) == SP_OK) {
            // Write page back
            if (lseek(sf->fd, offset, SEEK_SET) < 0) continue;
            if (write(sf->fd, page_data, SP_PAGE_SIZE) != SP_PAGE_SIZE) continue;
            
            rid->page_num = page_num;
            rid->slot_num = slot_num;
            sf->num_records++;
            return SP_OK;
        }
    }
    
    // Allocate new page
    SP_InitPage(page_data);
    
    int slot_num;
    if (SP_InsertRecord(page_data, buffer, len, &slot_num) != SP_OK) {
        return SP_ERROR;
    }
    
    // Write new page
    off_t offset = sf->num_pages * SP_PAGE_SIZE + 4096;
    if (lseek(sf->fd, offset, SEEK_SET) < 0) return SP_ERROR;
    if (write(sf->fd, page_data, SP_PAGE_SIZE) != SP_PAGE_SIZE) return SP_ERROR;
    
    rid->page_num = sf->num_pages;
    rid->slot_num = slot_num;
    sf->num_pages++;
    sf->num_records++;
    
    return SP_OK;
}

// Delete student record
int SF_DeleteStudent(StudentFile *sf, RecordID rid) {
    char page_data[SP_PAGE_SIZE];
    
    off_t offset = rid.page_num * SP_PAGE_SIZE + 4096;
    if (lseek(sf->fd, offset, SEEK_SET) < 0) return SP_ERROR;
    if (read(sf->fd, page_data, SP_PAGE_SIZE) != SP_PAGE_SIZE) return SP_ERROR;
    
    int result = SP_DeleteRecord(page_data, rid.slot_num);
    
    if (result == SP_OK) {
        // Write page back
        if (lseek(sf->fd, offset, SEEK_SET) < 0) return SP_ERROR;
        if (write(sf->fd, page_data, SP_PAGE_SIZE) != SP_PAGE_SIZE) return SP_ERROR;
        sf->num_records--;
    }
    
    return result;
}

// Get student record
int SF_GetStudent(StudentFile *sf, RecordID rid, StudentRecord *rec) {
    char page_data[SP_PAGE_SIZE];
    
    off_t offset = rid.page_num * SP_PAGE_SIZE + 4096;
    if (lseek(sf->fd, offset, SEEK_SET) < 0) return SP_ERROR;
    if (read(sf->fd, page_data, SP_PAGE_SIZE) != SP_PAGE_SIZE) return SP_ERROR;
    
    char buffer[2048];
    short len;
    int result = SP_GetRecord(page_data, rid.slot_num, buffer, &len);
    
    if (result == SP_OK) {
        deserialize_student(buffer, len, rec);
    }
    
    return result;
}

// Open scan
int SF_OpenScan(SF_ScanHandle *handle, StudentFile *sf) {
    handle->sf = sf;
    return SP_OpenScan(&handle->sp_handle, sf->fd, sf->num_pages);
}

// Get next student
int SF_GetNextStudent(SF_ScanHandle *handle, StudentRecord *rec) {
    char buffer[2048];
    short len;
    RecordID rid;
    
    if (SP_GetNextRecord(&handle->sp_handle, buffer, &len, &rid) != SP_OK) {
        return SP_ERROR;
    }
    
    deserialize_student(buffer, len, rec);
    return SP_OK;
}

// Close scan
int SF_CloseScan(SF_ScanHandle *handle) {
    return SP_CloseScan(&handle->sp_handle);
}

// Get space statistics
void SF_GetSpaceStats(StudentFile *sf, SpaceStats *stats) {
    stats->total_pages = sf->num_pages;
    stats->total_space = sf->num_pages * SP_PAGE_SIZE;
    stats->used_space = 0;
    stats->slot_overhead = 0;
    stats->header_overhead = sf->num_pages * SP_HEADER_SIZE;
    stats->free_space = 0;
    
    char page_data[SP_PAGE_SIZE];
    for (int page_num = 0; page_num < sf->num_pages; page_num++) {
        off_t offset = page_num * SP_PAGE_SIZE + 4096;
        if (lseek(sf->fd, offset, SEEK_SET) < 0) continue;
        if (read(sf->fd, page_data, SP_PAGE_SIZE) != SP_PAGE_SIZE) continue;
        
        SlottedPageHeader *header = (SlottedPageHeader *)page_data;
        SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
        
        stats->slot_overhead += header->num_slots * SP_SLOT_SIZE;
        stats->free_space += header->free_space_size;
        
        // Calculate actual used space
        for (int i = 0; i < header->num_slots; i++) {
            if (slots[i].offset != 0) {
                stats->used_space += slots[i].length;
            }
        }
    }
    
    stats->fragmented_space = stats->total_space - stats->used_space - 
                              stats->slot_overhead - stats->header_overhead - 
                              stats->free_space;
    
    stats->utilization_pct = (stats->total_space > 0) ? 
        (double)stats->used_space / stats->total_space * 100 : 0;
    
    stats->avg_records_per_page = (sf->num_pages > 0) ?
        (double)sf->num_records / sf->num_pages : 0;
}
