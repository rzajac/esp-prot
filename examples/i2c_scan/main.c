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


#include <esp_i2c.h>
#include <esp_sdo.h>
#include <user_interface.h>

#define SCL GPIO0
#define SDA GPIO2

os_timer_t timer;


/** Scan I2C bus for devices. */
static void ICACHE_FLASH_ATTR
scan_i2c()
{
  esp_i2c_err err;
  esp_i2c_dev *root = NULL;
  esp_i2c_dev *curr;

  err = esp_i2c_init(SCL, SDA);
  if (err != ESP_I2C_OK) {
    os_printf("I2C init error: %d\n", err);
    return;
  }

  err = esp_i2c_scan(&root);
  if (err != ESP_I2C_OK) {
    os_printf("I2C scan error: %d\n", err);
    return;
  }

  if (root == NULL) {
    os_printf("No devices found.\n");
    return;
  }

  curr = root;
  while (curr != NULL) {
    os_printf("Found device: 0x%02X\n", curr->address);
    curr = curr->next;
  }

  esp_i2c_free_device_list(root, false);
}

void ICACHE_FLASH_ATTR
user_init()
{
  // We don't need WiFi for this example.
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);

  stdout_init(BIT_RATE_74880);
  os_printf("Starting...\n");

  os_timer_disarm(&timer);
  os_timer_setfn(&timer, (os_timer_func_t *) scan_i2c, NULL);
  os_timer_arm(&timer, 1500, false);
}
