#include "rv3028.h"
#include <time.h>

// Constants for validation
#define MAX_SECONDS     59
#define MAX_MINUTES     59
#define MAX_HOURS       23
#define MAX_DAY_OF_MONTH 31
#define MAX_MONTH       11  // tm_mon is 0-11
#define MAX_WEEKDAY     6
#define MAX_BCD_VALUE   99
#define BCD_ERROR_VALUE 0xFF
#define YEAR_OFFSET_1900 100  // Years since 1900 for 2000+ dates

// BCD Conversion Functions
uint8_t decimal_to_bcd(uint8_t decimal_value) {
    if (decimal_value > MAX_BCD_VALUE) {
        return BCD_ERROR_VALUE;
    }
    return ((decimal_value / 10) << 4) | (decimal_value % 10);
}

uint8_t bcd_to_decimal(uint8_t bcd_value) {
    return ((bcd_value >> 4) * 10) + (bcd_value & 0x0F);
}

// I2C Communication Helpers
static rv3028_result_t write_register_address(uint8_t register_address) {
    int result = i2c_write_blocking(RV3028_I2C_PORT, RV3028_I2C_ADDR, 
                                   &register_address, 1, true);
    return (result == 1) ? RV3028_SUCCESS : RV3028_ERROR_I2C_WRITE_FAILED;
}

static rv3028_result_t read_registers(uint8_t *buffer, size_t buffer_size) {
    int result = i2c_read_blocking(RV3028_I2C_PORT, RV3028_I2C_ADDR, 
                                  buffer, buffer_size, false);
    return (result == (int)buffer_size) ? RV3028_SUCCESS : RV3028_ERROR_I2C_READ_FAILED;
}

static rv3028_result_t write_data_to_device(const uint8_t *data, size_t data_size) {
    int result = i2c_write_blocking(RV3028_I2C_PORT, RV3028_I2C_ADDR, 
                                   data, data_size, false);
    return (result == (int)data_size) ? RV3028_SUCCESS : RV3028_ERROR_I2C_WRITE_FAILED;
}

// Input Validation Functions
static bool is_valid_time_component(const struct tm *time_data) {
    return (time_data->tm_sec <= MAX_SECONDS &&
            time_data->tm_min <= MAX_MINUTES && 
            time_data->tm_hour <= MAX_HOURS &&
            time_data->tm_mday <= MAX_DAY_OF_MONTH &&
            time_data->tm_mon <= MAX_MONTH &&
            time_data->tm_wday <= MAX_WEEKDAY);
}

static rv3028_result_t validate_time_input(const struct tm *time_to_validate) {
    if (time_to_validate == NULL) {
        return RV3028_ERROR_NULL_POINTER;
    }
    
    if (!is_valid_time_component(time_to_validate)) {
        return RV3028_ERROR_INVALID_TIME;
    }
    
    return RV3028_SUCCESS;
}

// Device Communication Functions
rv3028_result_t rv3028_initialize(void) {
    rv3028_result_t result = write_register_address(RV3028_STATUS_REG);
    if (result != RV3028_SUCCESS) {
        return result;
    }
    
    uint8_t status_data;
    result = read_registers(&status_data, 1);
    if (result != RV3028_SUCCESS) {
        return RV3028_ERROR_DEVICE_NOT_RESPONDING;
    }

    return RV3028_SUCCESS;
}

// Time Setting Functions
static void populate_time_write_buffer(uint8_t *buffer, const struct tm *time_data) {
    buffer[0] = RV3028_SECONDS_REG;  // Starting register address
    buffer[1] = decimal_to_bcd(time_data->tm_sec);
    buffer[2] = decimal_to_bcd(time_data->tm_min);
    buffer[3] = decimal_to_bcd(time_data->tm_hour);
    buffer[4] = decimal_to_bcd(time_data->tm_wday);
    buffer[5] = decimal_to_bcd(time_data->tm_mday);
    buffer[6] = decimal_to_bcd(time_data->tm_mon + 1);    // RTC expects 1-12, tm_mon is 0-11
    buffer[7] = decimal_to_bcd(time_data->tm_year % 100); // RTC stores 2-digit year
}

rv3028_result_t rv3028_set_current_time(const struct tm *time_to_set) {
    rv3028_result_t validation_result = validate_time_input(time_to_set);
    if (validation_result != RV3028_SUCCESS) {
        return validation_result;
    }

    uint8_t write_buffer[RV3028_WRITE_BUFFER_SIZE];
    populate_time_write_buffer(write_buffer, time_to_set);
    
    return write_data_to_device(write_buffer, sizeof(write_buffer));
}

// Time Reading Functions
static void convert_rtc_registers_to_tm(const uint8_t *rtc_registers, struct tm *time_buffer) {
    time_buffer->tm_sec  = bcd_to_decimal(rtc_registers[0] & RV3028_SECONDS_MASK);
    time_buffer->tm_min  = bcd_to_decimal(rtc_registers[1] & RV3028_MINUTES_MASK);
    time_buffer->tm_hour = bcd_to_decimal(rtc_registers[2] & RV3028_HOURS_MASK);
    time_buffer->tm_wday = bcd_to_decimal(rtc_registers[3] & RV3028_WEEKDAY_MASK);
    time_buffer->tm_mday = bcd_to_decimal(rtc_registers[4] & RV3028_DATE_MASK);
    time_buffer->tm_mon  = bcd_to_decimal(rtc_registers[5] & RV3028_MONTH_MASK) - 1; // Convert 1-12 to 0-11
    time_buffer->tm_year = bcd_to_decimal(rtc_registers[6]) + YEAR_OFFSET_1900;      // Convert to years since 1900

    // Set fields that the RTC doesn't provide
    time_buffer->tm_isdst = -1; // Unknown DST status
    time_buffer->tm_yday = 0;   // Day of year not calculated
}

rv3028_result_t rv3028_get_current_time(struct tm *time_buffer) {
    if (time_buffer == NULL) {
        return RV3028_ERROR_NULL_POINTER;
    }
    
    rv3028_result_t result = write_register_address(RV3028_SECONDS_REG);
    if (result != RV3028_SUCCESS) {
        return result;
    }

    uint8_t rtc_registers[RV3028_TIME_REGISTER_COUNT];
    result = read_registers(rtc_registers, sizeof(rtc_registers));
    if (result != RV3028_SUCCESS) {
        return result;
    }

    convert_rtc_registers_to_tm(rtc_registers, time_buffer);
    return RV3028_SUCCESS;
}

// Alarm Functions
rv3028_result_t rv3028_set_alarm(const struct tm *unlock_date) {
    rv3028_result_t validation_result = validate_time_input(unlock_date);
    if (validation_result != RV3028_SUCCESS) {
        return validation_result;
    }

    // Disable alarm interrupt to prevent race conditions
    uint8_t control_buffer[] = {RV3028_CONTROL2_REG, 0};
    rv3028_result_t result = write_data_to_device(control_buffer, sizeof(control_buffer));
    if (result != RV3028_SUCCESS) {
        return result;
    }

    // Set alarm registers
    // Set alarm registers, enabling minute, hour, and day of month alarms.
    // The AE_x bit is active-low, so we clear it to enable the alarm.
    uint8_t alarm_buffer[] = {
        RV3028_ALARM_MINUTES_REG,
        decimal_to_bcd(unlock_date->tm_min) & ~RV3028_ALARM_AE,
        decimal_to_bcd(unlock_date->tm_hour) & ~RV3028_ALARM_AE,
        decimal_to_bcd(unlock_date->tm_mday) & ~RV3028_ALARM_AE
    };
    result = write_data_to_device(alarm_buffer, sizeof(alarm_buffer));
    if (result != RV3028_SUCCESS) {
        return result;
    }

    // Enable alarm interrupt
    control_buffer[1] = RV3028_STATUS_AIE;
    return write_data_to_device(control_buffer, sizeof(control_buffer));
}

rv3028_result_t rv3028_check_alarm_flag(bool *is_triggered) {
    if (is_triggered == NULL) {
        return RV3028_ERROR_NULL_POINTER;
    }

    rv3028_result_t result = write_register_address(RV3028_STATUS_REG);
    if (result != RV3028_SUCCESS) {
        return result;
    }

    uint8_t status_reg;
    result = read_registers(&status_reg, 1);
    if (result != RV3028_SUCCESS) {
        return result;
    }

    *is_triggered = (status_reg & RV3028_STATUS_AF) != 0;
    return RV3028_SUCCESS;
}

rv3028_result_t rv3028_clear_alarm_flag(void) {
    rv3028_result_t result = write_register_address(RV3028_STATUS_REG);
    if (result != RV3028_SUCCESS) {
        return result;
    }

    uint8_t status_reg;
    result = read_registers(&status_reg, 1);
    if (result != RV3028_SUCCESS) {
        return result;
    }

    // Write back the status register with the alarm flag cleared
    uint8_t write_buffer[] = {RV3028_STATUS_REG, status_reg & ~RV3028_STATUS_AF};
    return write_data_to_device(write_buffer, sizeof(write_buffer));
}

rv3028_result_t rv3028_disable_alarm_interrupt(void) {
    rv3028_result_t result = write_register_address(RV3028_CONTROL2_REG);
    if (result != RV3028_SUCCESS) {
        return result;
    }

    uint8_t control2_reg;
    result = read_registers(&control2_reg, 1);
    if (result != RV3028_SUCCESS) {
        return result;
    }

    // Write back the control register with the alarm interrupt enable bit cleared
    uint8_t write_buffer[] = {RV3028_CONTROL2_REG, control2_reg & ~RV3028_STATUS_AIE};
    return write_data_to_device(write_buffer, sizeof(write_buffer));
}