## Maxim OneWire Protocol.

Library provides methods needed to quickly create driver for any device which 
uses Maxim OneWire protocol.

Before you start interacting with OneWire bus you have to initialize it with
`esp_ow_init` function. There is no need to call this function many times 
unless you change GPIO pin configuration in some other part of your code. 

Library provides few ways to discover devices on the bus:

Function               | Description
-----------------------|------------
`esp_ow_search`        | Search for devices on the bus.
`esp_ow_search_family` | Search bus for specific device family.
`esp_ow_read_rom`      | Read ROM address if there is only one device on the bus.
`esp_ow_read_rom_dev`  | Same as `esp_ow_read_rom` but returns `esp_ow_device`.

Both `esp_ow_search` and `esp_ow_search_family` provide a way to search for all
devices or only the ones which are in alert state. Also both return linked list 
of `esp_ow_device` structures. User is responsible to free list memory. 

For operations on linked list Library provides helper functions:

Function                  | Description
--------------------------|------------
`esp_ow_free_device_list` | Release memory allocated for the list.
`esp_ow_dump_found`       | Dump all devices to serial. 

For more information see [esp_ow.h](include/esp_ow.h) header file.
