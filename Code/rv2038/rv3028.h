#ifndef RV3028_H
#define RV3028_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <time.h>

// I2C Configuration
#define RV3028_I2C_ADDR         0x52
#define RV3028_I2C_PORT         i2c0

// Register Addresses
#define RV3028_SECONDS_REG      0x00
#define RV3028_MINUTES_REG      0x01
#define RV3028_HOURS_REG        0x02
#define RV3028_WEEKDAY_REG      0x03
#define RV3028_DATE_REG         0x04
#define RV3028_MONTH_REG        0x05
#define RV3028_YEAR_REG         0x06
#define RV3028_STATUS_REG       0x0E
#define RV3028_ALARM_MINUTES_REG 0x08
#define RV3028_ALARM_HOURS_REG   0x09
#define RV3028_ALARM_DATE_REG    0x0A
#define RV3028_CONTROL2_REG      0x11
 
 // Register Bit Masks
#define RV3028_STATUS_AF        0x04  // Alarm Flag
#define RV3028_STATUS_AIE       0x04  // Alarm Interrupt Enable
#define RV3028_ALARM_AE         0x80  // Alarm Enable (in each alarm register)
#define RV3028_SECONDS_MASK     0x7F  // Mask VL bit
#define RV3028_MINUTES_MASK     0x7F  // Mask unused bit
#define RV3028_HOURS_MASK       0x3F  // 24-hour format, mask AM/PM bits
#define RV3028_WEEKDAY_MASK     0x07  // Weekday 0-6
#define RV3028_DATE_MASK        0x3F  // Day of month 1-31
#define RV3028_MONTH_MASK       0x1F  // Month 1-12

// Buffer Sizes
#define RV3028_TIME_REGISTER_COUNT  7
#define RV3028_WRITE_BUFFER_SIZE    8

// Error Codes
typedef enum {
    RV3028_SUCCESS = 0,
    RV3028_ERROR_NULL_POINTER,
    RV3028_ERROR_INVALID_TIME,
    RV3028_ERROR_I2C_WRITE_FAILED,
    RV3028_ERROR_I2C_READ_FAILED,
    RV3028_ERROR_DEVICE_NOT_RESPONDING
} rv3028_result_t;

// Public API
rv3028_result_t rv3028_initialize(void);
rv3028_result_t rv3028_set_current_time(const struct tm *time_to_set);
rv3028_result_t rv3028_get_current_time(struct tm *time_buffer);
rv3028_result_t rv3028_set_alarm(const struct tm *unlock_date);
rv3028_result_t rv3028_check_alarm_flag(bool *is_triggered);
rv3028_result_t rv3028_clear_alarm_flag(void);
rv3028_result_t rv3028_disable_alarm_interrupt(void);
 
 // BCD Conversion Utilities
 uint8_t decimal_to_bcd(uint8_t decimal_value);
uint8_t bcd_to_decimal(uint8_t bcd_value);

#endif