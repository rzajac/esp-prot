## ESP8266 protocol library.

The collection of low level protocol drivers for ESP8266.

- [I2C](src/include/esp_i2c.h)
- [OneWire](src/include/esp_ow.h)

For more details about the libraries check library header file where you will 
find documentation for all the APIs.

## Build environment.

This library is part of my build system for ESP8266 based on CMake.
To compile / flash examples you will have to have the ESP development 
environment setup as described at https://github.com/rzajac/esp-dev-env.

## Examples.

- [Scan I2C bus](examples/i2c_scan)
- [Search OneWire bus](examples/ow_search)

# Dependencies.

This library depends on:

- https://github.com/rzajac/esp-ecl

to install dependency run:

```
$ wget -O - https://raw.githubusercontent.com/rzajac/esp-ecl/master/install.sh | bash
```

## License.

[Apache License Version 2.0](LICENSE) unless stated otherwise.
