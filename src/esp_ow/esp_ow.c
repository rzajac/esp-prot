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


#include <esp_ow.h>
#include <esp_gpio.h>
#include <osapi.h>
#include <mem.h>


#define OW_LOW(gpio_num) (GPIO_OUT_EN_S = (0x1 << (gpio_num)))
#define OW_HIGH(gpio_num) (GPIO_OUT_EN_C = (0x1 << (gpio_num)))
#define OW_RELEASE(gpio_num) (GPIO_OUT_EN_C = (0x1 << (gpio_num)))
#define OW_READ(gpio_num) ((GPIO_IN & (0x1 << (gpio_num))) != 0)

// The CRC lookup table.
static uint8_t crc_lookup[] = {
  0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
  157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
  35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
  190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
  70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
  219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
  101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
  248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
  140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
  17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
  175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
  50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
  202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
  87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
  233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
  116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};

uint8_t ICACHE_FLASH_ATTR
esp_ow_crc8(uint8_t crc8, uint8_t value)
{
  return crc_lookup[crc8 ^ value];
}

void ICACHE_FLASH_ATTR
esp_ow_free_device_list(esp_ow_device *node, bool free_custom)
{
  esp_ow_device *curr;
  esp_ow_device *next = node;

  while (next != NULL) {
    curr = next;
    next = next->next;
    if (free_custom && curr->custom != NULL) os_free(curr->custom);
    os_free(curr);
  }
}

bool ICACHE_FLASH_ATTR
esp_ow_read_bit(uint8_t gpio_num)
{
  bool bit = 0;

  OW_LOW(gpio_num);
  os_delay_us(2);
  OW_RELEASE(gpio_num);
  os_delay_us(5);
  bit = OW_READ(gpio_num);
  os_delay_us(53);

  return bit;
}

void ICACHE_FLASH_ATTR
esp_ow_write_bit(uint8_t gpio_num, bool bit)
{
  OW_LOW(gpio_num);

  if (bit) {
    // Write 1.
    os_delay_us(5);
    OW_RELEASE(gpio_num);
    os_delay_us(55);
  } else {
    // Write 0.
    os_delay_us(55);
    OW_RELEASE(gpio_num);
    os_delay_us(5);
  }
}

void ICACHE_FLASH_ATTR
esp_ow_init(uint8_t gpio_num)
{
  esp_gpio_setup(gpio_num, GPIO_MODE_INPUT_PULLUP);
}

bool ICACHE_FLASH_ATTR
esp_ow_reset(uint8_t gpio_num)
{
  bool atr = false;
  int8_t retries = 0;

  // Hold bus low for 480us (Reset Pulse).
  OW_LOW(gpio_num);
  os_delay_us(480);

  // Releasing bus high and sampling
  // for 240us for Answer to Reset (ATR)
  // from devices on the line.
  OW_RELEASE(gpio_num);

  do {
    os_delay_us(5);
    retries++;
    // At least one device on the line.
    if (OW_READ(gpio_num) == false) {
      atr = true;
      break;
    }
  } while (retries < 48);

  // The total time of reset pulse must be minimum 2*480us.
  os_delay_us((uint16_t) (480 - (retries * 5)));

  return atr;
}

void ICACHE_FLASH_ATTR
esp_ow_match_rom(uint8_t gpio_num, uint8_t *rom)
{
  esp_ow_write(gpio_num, ESP_OW_CMD_MATCH_ROM);
  esp_ow_write_bytes(gpio_num, rom, 8);
}

void ICACHE_FLASH_ATTR
esp_ow_match_dev(esp_ow_device *device)
{
  esp_ow_match_rom(device->gpio_num, device->rom);
}

uint8_t ICACHE_FLASH_ATTR
esp_ow_read(uint8_t gpio_num)
{
  uint8_t byte = 0;
  uint8_t mask;

  for (mask = 1; mask; mask <<= 1) {
    if (esp_ow_read_bit(gpio_num)) byte |= mask;
  }

  return byte;
}

void ICACHE_FLASH_ATTR
esp_ow_read_bytes(uint8_t gpio_num, uint8_t *buf, uint8_t len)
{
  uint8_t idx;

  for (idx = 0; idx < len; idx++) {
    buf[idx] = esp_ow_read(gpio_num);
  }
}

void ICACHE_FLASH_ATTR
esp_ow_write(uint8_t gpio_num, uint8_t byte)
{
  uint8_t mask;

  for (mask = 1; mask; mask <<= 1) {
    esp_ow_write_bit(gpio_num, byte & mask);
  }
}

void ICACHE_FLASH_ATTR
esp_ow_write_bytes(uint8_t gpio_num, uint8_t *buf, uint8_t len)
{
  uint8_t idx;

  for (idx = 0; idx < len; idx++) {
    esp_ow_write(gpio_num, buf[idx]);
  }
}

/**
 * Find deice on the OneWire bus.
 *
 * Uses binary search algorithm to find devices on the bus.
 * Users should call it again and again till the is_last argument is set to true.
 * Each call updates prev_search with ROM address of newly found device.
 *
 * When function returns false caller must start search for devices on the bus all over again.
 *
 * @param sch_type    The search type.
 * @param prev_search The previous device found on the bus. For the first search the address will contain all zeros.
 * @param prev_dis    The discrepancy address position found during the previous search. This function sets
 *                    it to the last discrepancy found during current search.
 * @param is_last     Set by current function call to true if this is the last device on the bus.
 *
 * @return The error code.
 */
static esp_ow_err ICACHE_FLASH_ATTR
search(uint8_t gpio_num, esp_ow_cmd sch_type, esp_ow_device *prev_search, uint8_t *prev_dis, bool *is_last)
{
  // The last ROM address found on the bus.
  uint8_t *prev_adr = prev_search->rom;
  // Current byte index in ROM address array (0-7).
  uint8_t rom_byte_idx = 0;
  // Current ROM address bit index (1-64).
  uint8_t rom_bit_idx = 1;
  // Current mask for ROM address byte.
  uint8_t rom_byte_mask = 1;
  // Last discrepancy found during this search.
  uint8_t found_dis = 0;
  // The bit read from the bus (0 or 1).
  bool bit;
  // The bit complement read from the bus (0 - 1).
  bool bit_com;
  // The search direction.
  bool sch_dir = false;
  // The ROM address CRC8 value.
  uint8_t crc = 0;
  // Number of ones seen on the bus.
  uint8_t one_count = 0;

  if (esp_ow_reset(gpio_num) == false) {
    return ESP_OW_ERR_NO_DEV;
  }

  esp_ow_write(gpio_num, sch_type);

  do {
    bit = esp_ow_read_bit(gpio_num);
    bit_com = esp_ow_read_bit(gpio_num);

    // No devices on the bus or error.
    if (bit == 1 && bit_com == 1) {
      return ESP_OW_ERR_NO_DEV;
    }

    if (bit == 0 && bit_com == 0) {
      // Discrepancy.
      if (rom_bit_idx < *prev_dis) {
        // Discrepancy is before the previous search discrepancy.
        // We use search direction from the last search.
        sch_dir = (uint8_t) ((prev_adr[rom_byte_idx] & rom_bit_idx) ? 1 : 0);
      } else {
        // We have reached the last discrepancy from previous
        // search or this is the first search and first
        // discrepancy found. We know that during the first search
        // prev_dis is set to 0 so it will never be equal to rom_bit_idx
        // which always starts from 1. So for the first search we always pick
        // 0 direction for all consecutive ones we pick 1.
        sch_dir = (uint8_t) (rom_bit_idx == *prev_dis);
      }

      // Record the last found discrepancy during this search only if the path taken is 0.
      // When we take path 1 we are resolving previous discrepancy so there is no
      // need to mark this position for next try.
      if (sch_dir == 0) found_dis = rom_bit_idx;
    } else {
      // No discrepancy we use the bit as the direction.
      sch_dir = bit;
    }

    if (sch_dir) {
      prev_adr[rom_byte_idx] |= rom_byte_mask;
      one_count++;
    } else {
      prev_adr[rom_byte_idx] &= ~rom_byte_mask;
    }

    esp_ow_write_bit(gpio_num, sch_dir);
    //os_printf("w: %d %d %d | %d %d\n", rom_bit_idx, rom_byte_idx, sch_dir, bit, bit_com);

    // Move to the next ROM address bit.
    rom_bit_idx++;
    // Move to the next bit in ROM address byte.
    rom_byte_mask <<= 1;

    // When rom_byte_mask is 0 it means we finished with current ROM address byte.
    if (rom_byte_mask == 0) {
      crc = esp_ow_crc8(crc, prev_adr[rom_byte_idx]);
      rom_byte_mask = 1;
      rom_byte_idx++;
    }
  } while (rom_byte_idx < 8);

  //os_printf("%d bits read (%d) (d:%d) 1s:%d.\n", rom_bit_idx, crc, found_dis, one_count);

  // Search is successful if we were able to get all 64 bits
  // of the ROM address and CRC8 is 0.
  if (rom_bit_idx == 65 && crc == 0) {
    // There is no way we haven't received any ones.
    if (one_count == 0) return ESP_OW_ERR_PIN_FLAPPING;
    // When no discrepancies found it means this is the last device on the bus.
    if (found_dis == 0) *is_last = true;
    *prev_dis = found_dis;

    return ESP_OW_OK;
  }

  if (crc != 0) {
    return ESP_OW_ERR_BAD_CRC;
  }

  // Something went wrong.
  return ESP_OW_ERR;
}

/**
 * Create device node based on previous node.
 *
 * The new device address is copied from the previous device.
 *
 * @param prev_node The previous node.
 *
 * @return The new device.
 */
static esp_ow_device *ICACHE_FLASH_ATTR
add_device(esp_ow_device *prev_node)
{
  esp_ow_device *new_node = os_zalloc(sizeof(esp_ow_device));
  if (new_node == NULL) return NULL;

  os_memcpy(new_node->rom, prev_node->rom, (8 * sizeof(uint8_t)));
  new_node->gpio_num = prev_node->gpio_num;
  prev_node->next = new_node;

  return new_node;
}

esp_ow_err ICACHE_FLASH_ATTR
esp_ow_search(uint8_t gpio_num, esp_ow_cmd sch_type, esp_ow_device **root)
{
  // The ROM address bit number where we encountered last discrepancy.
  uint8_t last_disc = 0;
  // Set to true by search function when last device ROM address is acquired.
  bool is_last_dev = false;
  // Search error code.
  esp_ow_err err;
  // The current device.
  esp_ow_device *dev_curr;
  // Number of found devices on the bus.
  int8_t found_count = 0;

  if (*root != NULL) {
    return ESP_OW_ERR_ROOT_NOT_NULL;
  }

  // Make sure only search commands are passed.
  if (!(sch_type == ESP_OW_CMD_SEARCH_ROM || sch_type == ESP_OW_CMD_SEARCH_ROM_ALERT)) {
    return ESP_OW_ERR_BAD_CMD;
  }

  // Initialize rom list.
  dev_curr = *root = os_zalloc(sizeof(esp_ow_device));
  if (dev_curr == NULL) {
    return ESP_OW_ERR_MEM;
  }
  dev_curr->gpio_num = gpio_num;

  do {
    err = search(gpio_num, sch_type, dev_curr, &last_disc, &is_last_dev);
    //os_printf("SCH: %d\n", err);

    if (err == ESP_OW_OK && is_last_dev == false) {
      found_count++;
      dev_curr = add_device(dev_curr);
      if (dev_curr == NULL) {
        err = ESP_OW_ERR_MEM;
        break;
      }
      continue;
    }

    break;
  } while (is_last_dev == false);

  if (err != ESP_OW_OK) {
    esp_ow_free_device_list(*root, false);
    *root = NULL;
  }

  return err;
}

esp_ow_err ICACHE_FLASH_ATTR
esp_ow_search_family(uint8_t gpio_num, esp_ow_cmd sch_type, uint8_t family_code, esp_ow_device **root)
{
  esp_ow_err err;
  esp_ow_device *prev = NULL;
  esp_ow_device *curr = NULL;

  err = esp_ow_search(gpio_num, sch_type, root);
  if (err != ESP_OW_OK) return err;

  // Filter out devices not matching family code.
  curr = *root;
  while (curr != NULL) {
    if (curr->rom[0] != family_code) {
      if (prev == NULL) {
        // Removing head.
        *root = curr->next;
        os_free(curr);
        curr = *root;
        if (curr == NULL) break; // No more devices.
      } else {
        prev->next = curr->next;
        os_free(curr);
        curr = prev->next;
      }
    } else {
      prev = curr;
      curr = curr->next;
    }
  }

  if (*root == NULL) {
    return ESP_OW_ERR_NO_DEV;
  }

  return ESP_OW_OK;
}

void ICACHE_FLASH_ATTR
esp_ow_read_rom(uint8_t gpio_num, uint8_t *rom)
{
  esp_ow_reset(gpio_num);
  esp_ow_write(gpio_num, ESP_OW_CMD_READ_ROM);
  esp_ow_read_bytes(gpio_num, rom, 8);
}

esp_ow_device *ICACHE_FLASH_ATTR
esp_ow_read_rom_dev(uint8_t gpio_num)
{
  esp_ow_device *dev = os_zalloc(sizeof(esp_ow_device));
  esp_ow_read_rom(gpio_num, dev->rom);

  return dev;
}

void ICACHE_FLASH_ATTR
esp_ow_dump_found(esp_ow_device *root)
{
  esp_ow_device *curr;

  if (root == NULL) {
    os_printf("No OneWire devices found.\n");
    return;
  }

  curr = root;
  while (curr != NULL) {
    os_printf("Found ROM: ");
    os_printf("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
              curr->rom[7],
              curr->rom[6],
              curr->rom[5],
              curr->rom[4],
              curr->rom[3],
              curr->rom[2],
              curr->rom[1],
              curr->rom[0]);
    curr = curr->next;
  }
}
