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


#ifndef ESP_ONE_WIRE_H
#define ESP_ONE_WIRE_H

#include <c_types.h>

// Linked list of devices found on OneWire bus.
//
// The device ROM address structure:
//   0   - family code (LSB)
//   1,6 - serial number (48 bits)
//   7   - CRC (MSB)
//
typedef struct ow_device {
  uint8_t rom[8];         // The device ROM address.
  void *custom;           // Custom data to associate with the device.
  uint8_t gpio_num;       // The GPIO this device is connected to.
  struct ow_device *next; // The next device on the list.
} esp_ow_device;

// OneWire commands.
typedef enum {
  ESP_OW_CMD_READ_ROM = 0x33,
  ESP_OW_CMD_MATCH_ROM = 0x55,
  ESP_OW_CMD_SEARCH_ROM = 0xF0,
  ESP_OW_CMD_SEARCH_ROM_ALERT = 0xEC,
  ESP_OW_CMD_SKIP_ROM = 0xCC
} esp_ow_cmd;

// Error codes.
typedef enum {
  ESP_OW_OK,
  ESP_OW_ERR,
  ESP_OW_ERR_MEM,
  ESP_OW_ERR_BAD_CMD,
  ESP_OW_ERR_BAD_CRC,
  ESP_OW_ERR_NO_DEV,
  ESP_OW_ERR_ROOT_NOT_NULL,
  ESP_OW_ERR_PIN_FLAPPING,
} esp_ow_err;


/**
 * Initialize OneWire bus.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 */
void ICACHE_FLASH_ATTR
esp_ow_init(uint8_t gpio_num);

/**
 * Find deices on the OneWire bus.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 * @param sch_type ESP_OW_CMD_SEARCH_ROM or ESP_OW_CMD_SEARCH_ROM_ALERT.
 * @param root     The pointer to root of linked list of found devices on the OneWire bus.
 *                 Must be set initially to NULL by the caller.
 *
 * @return The error code.
 */
esp_ow_err ICACHE_FLASH_ATTR
esp_ow_search(uint8_t gpio_num, esp_ow_cmd sch_type, esp_ow_device **root);

/**
 * Construct new device with given ROM address.
 *
 * @param rom The pointer to 8 byte ROM address.
 *
 * @return The device or NULL on error
 */
esp_ow_device *ICACHE_FLASH_ATTR
esp_ow_new_dev(uint8_t *rom);

/**
 * Find deices on the OneWire bus matching family code.
 *
 * @param gpio_num    The GPIO connected to OneWire data bus.
 * @param sch_type    ESP_OW_CMD_SEARCH_ROM or ESP_OW_CMD_SEARCH_ROM_ALERT.
 * @param family_code The family code.
 * @param root        The pointer to root of linked list of found devices on the OneWire bus.
 *                    Must be set initially to NULL by the caller.
 *
 * @return The error code.
 */
esp_ow_err ICACHE_FLASH_ATTR
esp_ow_search_family(uint8_t gpio_num, esp_ow_cmd sch_type, uint8_t family_code, esp_ow_device **root);

/**
 * Read ROM.
 *
 * Can be used only when there is only one device on the OneWire bus.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 * @param rom      The pointer to 8 byte array.
 */
void ICACHE_FLASH_ATTR
esp_ow_read_rom(uint8_t gpio_num, uint8_t *rom);

/**
 * Read ROM.
 *
 * Can be used only when there is only one device on the OneWire bus.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 *
 * @return The OneWire device.
 */
esp_ow_device *ICACHE_FLASH_ATTR
esp_ow_read_rom_dev(uint8_t gpio_num);

/**
 * Free ROM address list.
 *
 * @param node        The root node of the list.
 * @param free_custom Free memory pointed by custom.
 */
void ICACHE_FLASH_ATTR
esp_ow_free_device_list(esp_ow_device *node, bool free_custom);

/**
 * Reset OneWire bus.
 *
 * @return true if at least one slave on the OneWire bus, false otherwise.
 */
bool ICACHE_FLASH_ATTR
esp_ow_reset(uint8_t gpio_num);

/**
 * Send rom address to the OneWire bus.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 * @param buf      The 8 byte array least significant octet first.
 */
void ICACHE_FLASH_ATTR
esp_ow_write_bytes(uint8_t gpio_num, uint8_t *buf, uint8_t len);

/**
 * Send match rom command.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 * @param rom      The 8 byte array least significant octet first.
 */
void ICACHE_FLASH_ATTR
esp_ow_match_rom(uint8_t gpio_num, uint8_t *rom);

/**
 * Send match rom command.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 * @param device   The device.
 */
void ICACHE_FLASH_ATTR
esp_ow_match_dev(esp_ow_device *device);

/**
 * Calculate the CRC8 of the byte value.
 *
 * @param crc8  The previously calculated CRC8 (0 if used first time).
 * @param value The value to calculate CRC8 for.
 *
 * @return The calculated CRC8.
 */
uint8_t ICACHE_FLASH_ATTR
esp_ow_crc8(uint8_t crc8, uint8_t value);

/**
 * Read one byte from the OneWire bus.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 *
 * @return The byte send by the slave.
 */
uint8_t ICACHE_FLASH_ATTR
esp_ow_read(uint8_t gpio_num);

/**
 * Reads len bytes to buffer.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 * @param buf      The buffer to read bytes to.
 * @param len      The number of bytes to read to the buffer.
 */
void ICACHE_FLASH_ATTR
esp_ow_read_bytes(uint8_t gpio_num, uint8_t *buf, uint8_t len);

/**
 * Write one byte to OneWire bus.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 */
void ICACHE_FLASH_ATTR
esp_ow_write(uint8_t gpio_num, uint8_t byte);

/**
 * Read one bit from OneWire bus.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 *
 * @return 0 or 1
 */
bool ICACHE_FLASH_ATTR
esp_ow_read_bit(uint8_t gpio_num);

/**
 * Write one bit to OneWire bus.
 *
 * @param gpio_num The GPIO connected to OneWire data bus.
 */
void ICACHE_FLASH_ATTR
esp_ow_write_bit(uint8_t gpio_num, bool bit);

/**
 * Dump found ROMs.
 *
 * @param root The devices list.
 */
void ICACHE_FLASH_ATTR
esp_ow_dump_found(esp_ow_device *root);

#endif //ESP_ONE_WIRE_H
