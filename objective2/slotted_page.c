#include "slotted_page.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/* Initialize fresh page structure */
int SP_InitPage(char *page_data)
{
    SlottedPageHeader *hdr = (SlottedPageHeader *)page_data;
    hdr->page_id = 0;
    hdr->num_slots = 0;
    hdr->free_space_offset = SP_PAGE_SIZE;
    hdr->free_space_size = SP_PAGE_SIZE - SP_HEADER_SIZE;
    hdr->next_page = -1;
    hdr->prev_page = -1;
    memset(hdr->reserved, 0, sizeof(hdr->reserved));
    return SP_OK;
}

/* Put a record into the page, update slot directory */
int SP_InsertRecord(char *page_data, char *record, short rec_len, int *slot_num)
{
    SlottedPageHeader *hdr = (SlottedPageHeader *)page_data;
    int need = rec_len + SP_SLOT_SIZE;
    if (hdr->free_space_size < need)
        return SP_NO_SPACE;

    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    int idx = -1;
    for (int i = 0; i < hdr->num_slots; ++i)
    {
        if (slots[i].offset == 0 && slots[i].length == 0)
        {
            idx = i;
            break;
        }
    }
    if (idx == -1)
    {
        idx = hdr->num_slots++;
    }

    short new_off = hdr->free_space_offset - rec_len;
    memcpy(page_data + new_off, record, rec_len);

    slots[idx].offset = new_off;
    slots[idx].length = rec_len;

    hdr->free_space_offset = new_off;
    hdr->free_space_size -= need;

    if (slot_num)
        *slot_num = idx;
    return SP_OK;
}

/* Mark a slot as deleted (fragmentation left) */
int SP_DeleteRecord(char *page_data, int slot_num)
{
    SlottedPageHeader *hdr = (SlottedPageHeader *)page_data;
    if (slot_num >= hdr->num_slots)
        return SP_INVALID_SLOT;

    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    int rec_len = slots[slot_num].length;
    slots[slot_num].offset = 0;
    slots[slot_num].length = 0;
    hdr->free_space_size += rec_len;
    return SP_OK;
}

/* Retrieve a record's bytes from a slot */
int SP_GetRecord(char *page_data, int slot_num, char *record, short *rec_len)
{
    SlottedPageHeader *hdr = (SlottedPageHeader *)page_data;
    if (slot_num >= hdr->num_slots)
        return SP_INVALID_SLOT;

    SlotEntry *slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);
    if (slots[slot_num].offset == 0)
        return SP_INVALID_SLOT;

    *rec_len = slots[slot_num].length;
    memcpy(record, page_data + slots[slot_num].offset, *rec_len);
    return SP_OK;
}

/* Return amount of free bytes available in page */
int SP_GetFreeSpace(char *page_data)
{
    SlottedPageHeader *hdr = (SlottedPageHeader *)page_data;
    return hdr->free_space_size;
}

/* Repack page to eliminate fragmentation */
int SP_CompactPage(char *page_data)
{
    SlottedPageHeader *old_hdr = (SlottedPageHeader *)page_data;
    SlotEntry *old_slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);

    char *backup = malloc(SP_PAGE_SIZE);
    if (!backup)
        return SP_ERROR;
    memcpy(backup, page_data, SP_PAGE_SIZE);

    SP_InitPage(page_data);
    SlottedPageHeader *new_hdr = (SlottedPageHeader *)page_data;
    SlotEntry *new_slots = (SlotEntry *)(page_data + SP_HEADER_SIZE);

    SlottedPageHeader *src_hdr = (SlottedPageHeader *)backup;
    SlotEntry *src_slots = (SlotEntry *)(backup + SP_HEADER_SIZE);

    short offset = SP_PAGE_SIZE;
    int new_count = 0;
    for (int i = 0; i < src_hdr->num_slots; ++i)
    {
        if (src_slots[i].offset != 0)
        {
            short rlen = src_slots[i].length;
            offset -= rlen;
            memcpy(page_data + offset, backup + src_slots[i].offset, rlen);
            new_slots[new_count].offset = offset;
            new_slots[new_count].length = rlen;
            new_count++;
        }
    }

    new_hdr->num_slots = new_count;
    new_hdr->free_space_offset = offset;
    new_hdr->free_space_size = offset - SP_HEADER_SIZE - (new_count * SP_SLOT_SIZE);

    free(backup);
    return SP_OK;
}

/* Prepare scan state */
int SP_OpenScan(SP_ScanHandle *handle, int fd, int total_pages)
{
    handle->fd = fd;
    handle->curr_page = 0;
    handle->curr_slot = 0;
    handle->total_pages = total_pages;
    return SP_OK;
}

/* Walk through pages and slots returning next valid record */
int SP_GetNextRecord(SP_ScanHandle *handle, char *record, short *rec_len, RecordID *rid)
{
    char page_buf[SP_PAGE_SIZE];

    while (handle->curr_page < handle->total_pages)
    {
        off_t off = handle->curr_page * SP_PAGE_SIZE + 4096; /* skip file header */
        if (lseek(handle->fd, off, SEEK_SET) < 0)
            return SP_ERROR;
        if (read(handle->fd, page_buf, SP_PAGE_SIZE) != SP_PAGE_SIZE)
            return SP_ERROR;

        SlottedPageHeader *hdr = (SlottedPageHeader *)page_buf;
        SlotEntry *slots = (SlotEntry *)(page_buf + SP_HEADER_SIZE);

        while (handle->curr_slot < hdr->num_slots)
        {
            if (slots[handle->curr_slot].offset != 0)
            {
                *rec_len = slots[handle->curr_slot].length;
                memcpy(record, page_buf + slots[handle->curr_slot].offset, *rec_len);
                if (rid)
                {
                    rid->page_num = handle->curr_page;
                    rid->slot_num = handle->curr_slot;
                }
                handle->curr_slot++;
                return SP_OK;
            }
            handle->curr_slot++;
        }

        handle->curr_page++;
        handle->curr_slot = 0;
    }

    return SP_ERROR;
}

/* No cleanup required for scan in this implementation */
int SP_CloseScan(SP_ScanHandle *handle)
{
    (void)handle;
    return SP_OK;
}
