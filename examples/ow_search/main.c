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
#include <esp_sdo.h>
#include <user_interface.h>

os_timer_t timer;

void ICACHE_FLASH_ATTR
sys_init_done(void *arg)
{
  // Head of the linked list of found devices.
  esp_ow_device *root = NULL;

  // Initialize OneWire.
  esp_ow_init(GPIO2);
  //esp_ow_reset(GPIO2);

  // Search for devices.
  esp_ow_err err = esp_ow_search(GPIO2, ESP_OW_CMD_SEARCH_ROM, &root);
  if (err != ESP_OW_OK) {
    os_printf("Search error %d\n", err);
    return;
  }

  // Print found devices.
  esp_ow_dump_found(root);

  // Release linked list memory.
  esp_ow_free_device_list(root, false);
}

void ICACHE_FLASH_ATTR
user_init()
{
  // No need for wifi for this example.
  wifi_station_disconnect();
  wifi_set_opmode_current(NULL_MODE);

  stdout_init(BIT_RATE_74880);

  // Wait before running main code.
  os_printf("Initialized.\n");
  os_timer_disarm(&timer);
  os_timer_setfn(&timer, sys_init_done, NULL);
  os_timer_arm(&timer, 1500, false);
}
