#ifndef SLOTTED_PAGE_H
#define SLOTTED_PAGE_H

#define SP_PAGE_SIZE 4096
#define SP_HEADER_SIZE 32
#define SP_SLOT_SIZE 4

// Error codes
#define SP_OK 0
#define SP_ERROR -1
#define SP_NO_SPACE -2
#define SP_INVALID_SLOT -3

// Slotted page header
typedef struct {
    int page_id;
    short num_slots;
    short free_space_offset;
    short free_space_size;
    int next_page;
    int prev_page;
    char reserved[8];
} SlottedPageHeader;

// Slot entry
typedef struct {
    short offset;
    short length;
} SlotEntry;

// Record ID
typedef struct {
    int page_num;
    int slot_num;
} RecordID;

// Scan handle
typedef struct {
    int fd;
    int curr_page;
    int curr_slot;
    int total_pages;
} SP_ScanHandle;

// Function prototypes
int SP_InitPage(char *page_data);
int SP_InsertRecord(char *page_data, char *record, short rec_len, int *slot_num);
int SP_DeleteRecord(char *page_data, int slot_num);
int SP_GetRecord(char *page_data, int slot_num, char *record, short *rec_len);
int SP_GetFreeSpace(char *page_data);
int SP_CompactPage(char *page_data);
int SP_OpenScan(SP_ScanHandle *handle, int fd, int total_pages);
int SP_GetNextRecord(SP_ScanHandle *handle, char *record, short *rec_len, RecordID *rid);
int SP_CloseScan(SP_ScanHandle *handle);

#endif
