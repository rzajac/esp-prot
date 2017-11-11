/*
 * Copyright 2017 Rafal Zajac <rzajac@gmail.com>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#ifndef ESP_I2C_H
#define ESP_I2C_H

#include <c_types.h>
#include <esp_gpio.h>

#define ESP_I2C_ACK false
#define ESP_I2C_NACK true

#define ESP_I2C_SPEED_100 9
#define ESP_I2C_SPEED_200 5
#define ESP_I2C_SPEED_300 3
#define ESP_I2C_SPEED_400 1

// Get address with read bit set.
#define ESP_I2C_ADDR_READ(addr) ((uint8_t) (((addr) << 1) | 0x01))
// Get address with write bit set.
#define ESP_I2C_ADDR_WRITE(addr) ((uint8_t) (((addr) << 1) & 0xFE))

// Linked list node used when finding devices on the I2C bus.
typedef struct esp_i2c_dev {
  uint8_t address;          // The I2C address.
  void *custom;             // Custom data to associate with the device.
  struct esp_i2c_dev *next; // The next device on the list.
} esp_i2c_dev;

// I2C error codes.
typedef enum {
  ESP_I2C_OK,
  ESP_I2C_ERR_INIT_CONFLICT,
  ESP_I2C_ERR_STOP_OUTSIDE_TRANS,
  ESP_I2C_ERR_ARB_LOST,
  ESP_I2C_ERR_LONG_STRETCH,
  ESP_I2C_ERR_NO_ACK,
  ESP_I2C_ERR_ROOT_NOT_NULL,
  ESP_I2C_ERR_DATA_CORRUPTED,
} esp_i2c_err;

// Espressif SDK missing includes.
void ets_isr_mask(unsigned intr);
void ets_isr_unmask(unsigned intr);

/**
 * Initialize I2C bus.
 *
 * @param scl_gpio_num The GPIO number to use for clock.
 * @param sda_gpio_num The GPIO number to use for data.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_init(uint8_t scl_gpio_num, uint8_t sda_gpio_num);

/**
 * Set I2C bus speed.
 *
 * @param speed The one of ESP_I2C_SPEED_* defines.
 */
void ICACHE_FLASH_ATTR
esp_i2c_set_speed(uint8_t speed);

/**
 * Send start condition to I2C bus.
 *
 * It also initializes clock to known initial state.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_start();

/**
 * Send stop condition to I2C bus.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_stop();

/**
 * Release SCL and SDA on fatal error.
 *
 * @param err The I2C error code to return.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_fail_fast(esp_i2c_err err);

/**
 * Write byte to I2C bus.
 *
 * @param byte     The byte to write.
 * @param ack_resp The response after sending 8th bit (ESP_I2C_ACK or ESP_I2C_NACK).
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_write_byte(uint8_t byte, bool *ack_resp);

/**
 * Read byte from I2C bus.
 *
 * @param dst      The destination buffer.
 * @param ack_type The ESP_I2C_ACK or ESP_I2C_NACK to send after receiving the byte.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_read_byte(uint8_t *dst, bool ack_type);

/**
 * Write bytes to I2C bytes.
 *
 * @param buf The data to write.
 * @param len  The number of bytes to write.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_write_bytes(uint8_t *buf, uint8_t len);

/**
 * Read bytes from I2C bus.
 *
 * ESP_I2C_NACK is send after last byte.
 *
 * @param buf The buffer to read data to.
 * @param len The number of bytes to read.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_read_bytes(uint8_t *buf, uint8_t len);

/**
 * Start reading or writing process. Expect address with read/write bit already set.
 *
 * @param address      The I2C address with read/write bit already set.
 * @param stop_on_nack Send stop after getting ESP_I2C_NACK.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_start_read_write(uint8_t address, bool stop_on_nack);

/**
 * Start writing to the slave.
 *
 * @param address The slave address.
 * @param reg     The register address to write to.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_start_write(uint8_t address, uint8_t reg);

/**
 * Start reading from the slave.
 *
 * @param address The slave address.
 * @param reg     The register address to read from.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_start_read(uint8_t address, uint8_t reg);

/**
 * Scan I2C bus for devices.
 *
 * @param root The root of linked list of found devices. Must be NULL initially.
 *
 * @return The I2C error code.
 */
esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_scan(esp_i2c_dev **root);

/**
 * Free memory allocated for device list returned from esp_i2c_scan.
 *
 * @param root        The root node of the list.
 * @param free_custom Free memory pointed by custom.
 */
void ICACHE_FLASH_ATTR
esp_i2c_free_device_list(esp_i2c_dev *root, bool free_custom);

#endif //ESP_I2C_H
