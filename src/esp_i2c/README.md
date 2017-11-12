## I2C Protocol.

Library provides methods needed to quickly create driver for any device which 
uses I2C protocol.

Before you start interacting with I2C bus you have to initialize it with
`esp_i2c_init` function. There is no need to call this function many times 
unless you change GPIO pin configuration for SDA or SDL in some other 
part of your code. 

Library is able to communicate with four speeds 100k 200k 300k and 400k. By
default 100k speed is used but you can change it at any time with 
`esp_i2c_set_speed` function.

Library also provides `esp_i2c_scan` function to scan for I2C devices 
on the bus. The scan result is a linked list of `esp_i2c_dev` structures which 
can be used by your driver to identify device in your code. 

It is user responsibility to release memory associated with the list. For 
convenience library provides `esp_i2c_free_device_list`.

For more information see [esp_i2c.h](include/esp_i2c.h) header file.
