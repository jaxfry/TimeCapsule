#include "fs_manager.h"
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "fatfs/ff.h"
#include <string.h>
#include <time.h>

#define PRIVATE_STORAGE_OFFSET (15 * 1024 * 1024)
#define PRIVATE_STORAGE_SIZE (1 * 1024 * 1024)
#define METADATA_SIZE 256

static const uint32_t private_flash_address = XIP_BASE + PRIVATE_STORAGE_OFFSET;

typedef struct {
    char filename[256];
    struct tm unlock_date;
    uint32_t file_size;
    bool is_valid;
} metadata_t;

static const char* public_path = "0:";
static FATFS fs_public;

bool fs_init(void) {
    return true;
}

bool fs_mount_partitions(void) {
    FRESULT fr = f_mount(&fs_public, public_path, 1);
    if (fr != FR_OK) {
        printf("Failed to mount public partition: %d\n", fr);
        return false;
    }
    return true;
}

bool fs_move_to_private(const char* filename) {
    FIL fil;
    FRESULT fr;
    char public_filepath[256];
    snprintf(public_filepath, sizeof(public_filepath), "%s/%s", public_path, filename);

    fr = f_open(&fil, public_filepath, FA_READ);
    if (fr != FR_OK) return false;

    UINT br;
    uint8_t buffer[FLASH_SECTOR_SIZE];
    metadata_t metadata;

    strncpy(metadata.filename, filename, sizeof(metadata.filename) - 1);
    metadata.filename[sizeof(metadata.filename) - 1] = '\0';
    metadata.file_size = f_size(&fil);
    metadata.is_valid = true;

    sscanf(filename, "%d-%d-%d", &metadata.unlock_date.tm_year, &metadata.unlock_date.tm_mon, &metadata.unlock_date.tm_mday);
    metadata.unlock_date.tm_year -= 1900;
    metadata.unlock_date.tm_mon -= 1;

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(PRIVATE_STORAGE_OFFSET, PRIVATE_STORAGE_SIZE);
    flash_range_program(PRIVATE_STORAGE_OFFSET, (uint8_t*)&metadata, sizeof(metadata_t));
    restore_interrupts(ints);

    uint32_t offset = METADATA_SIZE;
    while (f_read(&fil, buffer, sizeof(buffer), &br) == FR_OK && br > 0) {
        ints = save_and_disable_interrupts();
        flash_range_program(PRIVATE_STORAGE_OFFSET + offset, buffer, br);
        restore_interrupts(ints);
        offset += br;
    }

    f_close(&fil);
    f_unlink(public_filepath);
    return true;
}

bool fs_is_file_in_private(void) {
    metadata_t metadata;
    memcpy(&metadata, (void*)private_flash_address, sizeof(metadata_t));
    return metadata.is_valid;
}

bool fs_get_unlock_date(struct tm* unlock_date) {
    metadata_t metadata;
    memcpy(&metadata, (void*)private_flash_address, sizeof(metadata_t));
    if (!metadata.is_valid) return false;
    *unlock_date = metadata.unlock_date;
    return true;
}

bool fs_move_to_public(void) {
    metadata_t metadata;
    memcpy(&metadata, (void*)private_flash_address, sizeof(metadata_t));
    if (!metadata.is_valid) return false;

    FIL fil;
    char public_filepath[256];
    snprintf(public_filepath, sizeof(public_filepath), "%s/%s", public_path, metadata.filename);

    if (f_open(&fil, public_filepath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) return false;

    UINT bw;
    uint32_t offset = METADATA_SIZE;
    uint32_t remaining = metadata.file_size;
    uint8_t buffer[FLASH_SECTOR_SIZE];

    while (remaining > 0) {
        uint32_t to_read = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
        memcpy(buffer, (void*)(private_flash_address + offset), to_read);
        f_write(&fil, buffer, to_read, &bw);
        offset += bw;
        remaining -= bw;
    }

    f_close(&fil);

    metadata.is_valid = false;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(PRIVATE_STORAGE_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(PRIVATE_STORAGE_OFFSET, (uint8_t*)&metadata, sizeof(metadata_t));
    restore_interrupts(ints);

    return true;
}

bool fs_find_file_in_public(char* found_filename, size_t max_len) {
    FRESULT fr;
    DIR dir;
    static FILINFO fno;

    fr = f_opendir(&dir, public_path);
    if (fr == FR_OK) {
        for (;;) {
            fr = f_readdir(&dir, &fno);
            if (fr != FR_OK || fno.fname[0] == 0) break; // Break on error or end of dir
            if (fno.fattrib & AM_DIR) continue; // Skip directories

            // For now, just find the first file that isn't a system file.
            if (strcmp(fno.fname, "SYSTEM~1") != 0 && strcmp(fno.fname, "System Volume Information") != 0) {
                strncpy(found_filename, fno.fname, max_len - 1);
                found_filename[max_len - 1] = '\0';
                f_closedir(&dir);
                return true;
            }
        }
        f_closedir(&dir);
    }
    return false; // No suitable file found
}

bool fs_is_file_stable(const char* filename) {
    char public_filepath[256];
    snprintf(public_filepath, sizeof(public_filepath), "%s/%s", public_path, filename);

    FILINFO fno1, fno2;
    FRESULT fr;

    fr = f_stat(public_filepath, &fno1);
    if (fr != FR_OK) return false; // File not found or other error

    sleep_ms(500); // Wait a bit to see if the file size changes

    fr = f_stat(public_filepath, &fno2);
    if (fr != FR_OK) return false;

    return fno1.fsize == fno2.fsize;
}