#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "bsp/board.h"
#include "tusb.h"

#include <time.h>
#include "rv3028.h"
#include "fs_manager.h"

#define FLASH_TARGET_BASE_ADDR (XIP_BASE)
#define FLASH_FILESYSTEM_OFFSET (2 * 1024 * 1024)

const uint32_t FLASH_TOTAL_SIZE = 16 * 1024 * 1024;
const uint32_t DISK_SIZE = FLASH_TOTAL_SIZE - FLASH_FILESYSTEM_OFFSET;

void tud_msc_capability_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    *block_size = 512;
    *block_count = DISK_SIZE / *block_size;
}

int32_t tud_msc_read_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    const uint32_t flash_addr = FLASH_TARGET_BASE_ADDR + FLASH_FILESYSTEM_OFFSET + (lba * 512);
    flash_read(flash_addr, buffer, bufsize);
    return bufsize;
}

static uint8_t sector_buffer[FLASH_SECTOR_SIZE];
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    uint32_t block_addr_in_flash = FLASH_FILESYSTEM_OFFSET + (lba * 512);
    uint32_t sector_addr_in_flash = block_addr_in_flash & ~(FLASH_SECTOR_SIZE - 1);
    flash_read(FLASH_TARGET_BASE_ADDR + sector_addr_in_flash, sector_buffer, FLASH_SECTOR_SIZE);
    
    uint32_t offset_in_sector = block_addr_in_flash - sector_addr_in_flash;
    memcpy(sector_buffer + offset_in_sector, buffer, bufsize);

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(sector_addr_in_flash, FLASH_SECTOR_SIZE);
    flash_range_program(sector_addr_in_flash, sector_buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);

    return bufsize;
}

void setup_rtc(void) {
    i2c_init(RV3028_I2C_PORT, 100 * 1000);
    gpio_set_function(4, GPIO_FUNC_I2C); // Corrected I2C pins
    gpio_set_function(5, GPIO_FUNC_I2C);
    gpio_pull_up(4);
    gpio_pull_up(5);
    sleep_ms(100);

    if (rv3028_initialize() != RV3028_SUCCESS) {
        printf("Error: RTC not found!\n");
        return;
    }

    struct tm current_time;
    rv3028_get_current_time(&current_time);

    if (current_time.tm_year < 123) { // Check if RTC year < 2023
        printf("Setting RTC to compile time...\n");
        char s_month[5];
        int month, day, year, hour, min, sec;
        static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
        sscanf(__DATE__, "%s %d %d", s_month, &day, &year);
        sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);
        month = (strstr(month_names, s_month) - month_names) / 3;

        struct tm compile_time = { .tm_sec = sec, .tm_min = min, .tm_hour = hour, .tm_mday = day, .tm_mon = month, .tm_year = year - 1900 };
        rv3028_set_current_time(&compile_time);
    }
}

void check_and_process_files(void) {
    bool is_alarm_triggered;
    rv3028_check_alarm_flag(&is_alarm_triggered);

    if (is_alarm_triggered) {
        printf("Alarm triggered! Checking for unlock.\n");
        
        // Clear the alarm flag first to avoid re-triggering
        rv3028_clear_alarm_flag();

        if (fs_is_file_in_private()) {
            struct tm unlock_date, current_time;
            fs_get_unlock_date(&unlock_date);
            rv3028_get_current_time(&current_time);

            if (mktime(&current_time) >= mktime(&unlock_date)) {
                printf("Unlock date reached! Moving file to public.\n");
                fs_move_to_public();

                // After unlocking, check for the next locked file and set a new alarm
                if (fs_is_file_in_private()) {
                    struct tm next_unlock_date;
                    fs_get_unlock_date(&next_unlock_date);
                    rv3028_set_alarm(&next_unlock_date);
                    printf("Next alarm set for new file.\n");
                } else {
                    printf("No more locked files. Disabling alarm.\n");
                    rv3028_disable_alarm_interrupt();
                }
            }
        }
    }

    // This part handles the initial locking of a new file
    if (!fs_is_file_in_private()) {
        char filename[256];
        if (fs_find_file_in_public(filename, sizeof(filename))) {
            if (fs_is_file_stable(filename)) {
                printf("New file found: %s. Moving to private.\n", filename);
                fs_move_to_private(filename);

                // Set alarm for the new file's unlock date
                struct tm unlock_date;
                fs_get_unlock_date(&unlock_date);
                rv3028_set_alarm(&unlock_date);
                printf("Alarm set for the new locked file.\n");
            }
        }
    }
}

void check_and_disable_latch(void) {
    struct tm current_time, unlock_date;
    
    rv3028_get_current_time(&current_time);

    if (fs_is_file_in_private() && fs_get_unlock_date(&unlock_date)) {
        if (mktime(&current_time) >= mktime(&unlock_date)) {
            printf("Unlock date has passed. Disabling MOSFET latch.\n");

            // Temporarily take control of GPIO5
            gpio_set_function(5, GPIO_FUNC_SIO);
            gpio_set_dir(5, GPIO_OUT);
            
            gpio_put(5, 1); // Drive GPIO5 high
            sleep_ms(500);  // Keep it high for a moment
            gpio_put(5, 0); // Drive GPIO5 low again

            // Return GPIO5 to I2C function
            gpio_set_function(5, GPIO_FUNC_I2C);
            gpio_pull_up(5);
        }
    }
}

int main(void) {
    board_init();
    stdio_init_all();
    printf("Pico Time Capsule Initializing...\n");

    setup_rtc();
    fs_init();
    fs_mount_partitions();

    check_and_disable_latch();

    tusb_init();

    while (1) {
        tud_task();
        check_and_process_files();
    }

    return 0;
}