#include "slotted_page.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// Initialize a new slotted page
int SP_InitPage(char *page_data) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    
    header->page_id = 0;
    header->num_slots = 0;
    header->free_space_offset = SP_PAGE_SIZE;
    header->free_space_size = SP_PAGE_SIZE - SP_HEADER_SIZE;
    header->next_page = -1;
    header->prev_page = -1;
    memset(header->reserved, 0, 8);
    
    return SP_OK;
}

// Insert record into page
int SP_InsertRecord(char *page_data, char *record, short rec_len, int *slot_num) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    
    // Check if enough space
    int space_needed = rec_len + SP_SLOT_SIZE;
    if (header->free_space_size < space_needed) {
        return SP_NO_SPACE;
    }
    
    // Find free slot or create new one
    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    int slot_idx = -1;
    
    for (int i = 0; i < header->num_slots; i++) {
        if (slots[i].offset == 0 && slots[i].length == 0) {
            slot_idx = i;
            break;
        }
    }
    
    if (slot_idx == -1) {
        slot_idx = header->num_slots;
        header->num_slots++;
    }
    
    // Insert record at end (grows upward from end)
    short new_offset = header->free_space_offset - rec_len;
    memcpy(page_data + new_offset, record, rec_len);
    
    // Update slot directory
    slots[slot_idx].offset = new_offset;
    slots[slot_idx].length = rec_len;
    
    // Update header
    header->free_space_offset = new_offset;
    header->free_space_size -= space_needed;
    
    *slot_num = slot_idx;
    return SP_OK;
}

// Delete record from page
int SP_DeleteRecord(char *page_data, int slot_num) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    
    if (slot_num >= header->num_slots) {
        return SP_INVALID_SLOT;
    }
    
    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    
    // Mark slot as deleted
    int rec_len = slots[slot_num].length;
    slots[slot_num].offset = 0;
    slots[slot_num].length = 0;
    
    // Free space now available (but fragmented)
    header->free_space_size += rec_len;
    
    return SP_OK;
}

// Get record from page
int SP_GetRecord(char *page_data, int slot_num, char *record, short *rec_len) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    
    if (slot_num >= header->num_slots) {
        return SP_INVALID_SLOT;
    }
    
    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    
    if (slots[slot_num].offset == 0) {
        return SP_INVALID_SLOT;  // Deleted record
    }
    
    *rec_len = slots[slot_num].length;
    memcpy(record, page_data + slots[slot_num].offset, *rec_len);
    
    return SP_OK;
}

// Get free space in page
int SP_GetFreeSpace(char *page_data) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    return header->free_space_size;
}

// Compact page (remove fragmentation)
int SP_CompactPage(char *page_data) {
    SlottedPageHeader *header = (SlottedPageHeader *)page_data;
    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    
    char *temp_page = malloc(SP_PAGE_SIZE);
    if (!temp_page) return SP_ERROR;
    
    memcpy(temp_page, page_data, SP_PAGE_SIZE);
    
    // Reset page
    SP_InitPage(page_data);
    SlottedPageHeader *new_header = (SlottedPageHeader *)page_data;
    SlotEntry *new_slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    
    // Copy back valid records
    short new_offset = SP_PAGE_SIZE;
    SlottedPageHeader *old_header = (SlottedPageHeader *)temp_page;
    SlotEntry *old_slots = (SlotEntry *)(temp_page + SP_HEADER_SIZE);
    
    int new_slot_count = 0;
    for (int i = 0; i < old_header->num_slots; i++) {
        if (old_slots[i].offset != 0) {  // Valid record
            short rec_len = old_slots[i].length;
            new_offset -= rec_len;
            
            memcpy(page_data + new_offset, 
                   temp_page + old_slots[i].offset, 
                   rec_len);
            
            new_slots[new_slot_count].offset = new_offset;
            new_slots[new_slot_count].length = rec_len;
            new_slot_count++;
        }
    }
    
    new_header->num_slots = new_slot_count;
    new_header->free_space_offset = new_offset;
    new_header->free_space_size = new_offset - SP_HEADER_SIZE - 
                                  (new_slot_count * SP_SLOT_SIZE);
    
    free(temp_page);
    return SP_OK;
}

// Open scan
int SP_OpenScan(SP_ScanHandle *handle, int fd, int total_pages) {
    handle->fd = fd;
    handle->curr_page = 0;
    handle->curr_slot = 0;
    handle->total_pages = total_pages;
    return SP_OK;
}

// Get next record
int SP_GetNextRecord(SP_ScanHandle *handle, char *record, short *rec_len, RecordID *rid) {
    char page_data[SP_PAGE_SIZE];
    
    while (handle->curr_page < handle->total_pages) {
        // Read current page
        off_t offset = handle->curr_page * SP_PAGE_SIZE + 4096;  // Skip file header
        if (lseek(handle->fd, offset, SEEK_SET) < 0) {
            return SP_ERROR;
        }
        if (read(handle->fd, page_data, SP_PAGE_SIZE) != SP_PAGE_SIZE) {
            return SP_ERROR;
        }
        
        SlottedPageHeader *header = (SlottedPageHeader *)page_data;
        SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
        
        // Scan slots in current page
        while (handle->curr_slot < header->num_slots) {
            if (slots[handle->curr_slot].offset != 0) {
                // Found valid record
                *rec_len = slots[handle->curr_slot].length;
                memcpy(record, page_data + slots[handle->curr_slot].offset, *rec_len);
                
                if (rid) {
                    rid->page_num = handle->curr_page;
                    rid->slot_num = handle->curr_slot;
                }
                
                handle->curr_slot++;
                return SP_OK;
            }
            handle->curr_slot++;
        }
        
        // Move to next page
        handle->curr_page++;
        handle->curr_slot = 0;
    }
    
    return SP_ERROR;  // End of scan
}

// Close scan
int SP_CloseScan(SP_ScanHandle *handle) {
    return SP_OK;
}
