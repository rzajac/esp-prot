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
#include <mem.h>

// Set to true when I2C is initialized.
static bool init;
// GPIO number assigned to clock.
static uint8_t gpio_scl;
// GPIO number assigned to data.
static uint8_t gpio_sda;
// In transaction when between START and STOP conditions.
static bool in_trans;

// Maximum time slave can stretch the clock.
static uint32_t max_cs;

// The default delays to use in functions.
static uint8_t i2c_delay_short;
static uint8_t i2c_delay_long;

// Makes GPIO an output and because GPIO_OUT for this bit
// is 0 it will pull bus low.
#define SDA_LOW() (GPIO_OUT_EN_S = (0x1 << (gpio_sda)))
#define SCL_LOW() (GPIO_OUT_EN_S = (0x1 << (gpio_scl)))

// Make GPIO an input and since there are pull up resistors
// on the bus they will pull bus line high unless
// some device is actively driving it low.
// This is handy for SCL because when it's high we want to read it state.
#define SDA_RELEASE() (GPIO_OUT_EN_C = (0x1 << (gpio_sda)))
#define SCL_RELEASE() (GPIO_OUT_EN_C = (0x1 << (gpio_scl)))

// Read GPIO state. Can be called only after *_RELEASE since only then
// GPIO is setup as input.
#define SDA_READ() ((GPIO_IN & (0x1 << (gpio_sda))) != 0)
#define SCL_READ() ((GPIO_IN & (0x1 << (gpio_scl))) != 0)

// I2C bus states.
#define ESP_I2C_HIGH true
#define ESP_I2C_LO false

/**
 * Delay doing nothing.
 *
 * @param count The number of noop loops.
 */
static void ICACHE_FLASH_ATTR
delay(uint8_t count)
{
  uint8_t idx;
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  uint32_t reg;
  #pragma GCC diagnostic pop

  for (idx = 0; idx < count; idx++) reg = GPIO_IN;
}

/**
 * Check clock stretching.
 *
 * @return The clock stretching time.
 */
static uint8_t ICACHE_FLASH_ATTR
chk_cs()
{
  uint8_t idx = 0;

  while (SCL_READ() == ESP_I2C_LO && (idx++) < max_cs);

  return idx;
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_fail_fast(esp_i2c_err err)
{
  SCL_RELEASE();
  SDA_RELEASE();
  in_trans = false;

  return err;
}

/**
 * Advance the clock one cycle.
 *
 * One clock tick:
 *    ____
 * __/    \__
 *
 * SHORT LOW -> LONG HIGH -> SHORT LOW
 *
 * LONG = 2 * SHORT
 *
 * @return The I2C error code.
 */
static esp_i2c_err ICACHE_FLASH_ATTR
tick()
{
  delay(i2c_delay_short);
  SCL_RELEASE();

  if (chk_cs() == max_cs) return esp_i2c_fail_fast(ESP_I2C_ERR_LONG_STRETCH);

  // Keep clock high.
  delay(i2c_delay_long);
  SCL_LOW();
  delay(i2c_delay_short);

  return ESP_I2C_OK;
}

void ICACHE_FLASH_ATTR
esp_i2c_set_speed(uint8_t speed)
{
  // The default delays.
  i2c_delay_short = speed;
  i2c_delay_long = (uint8_t) (2 * i2c_delay_short);
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_init(uint8_t scl_gpio_num, uint8_t sda_gpio_num)
{
  // Check double initialization.
  if (init) {
    if (gpio_scl == scl_gpio_num && gpio_sda == sda_gpio_num) return ESP_I2C_OK;
    else return ESP_I2C_ERR_INIT_CONFLICT;
  }

  init = true;

  // Configure as GPIOs.
  esp_gpio_setup(scl_gpio_num, GPIO_MODE_INPUT);
  esp_gpio_setup(sda_gpio_num, GPIO_MODE_INPUT);

  // Set global variables.
  gpio_scl = scl_gpio_num;
  gpio_sda = sda_gpio_num;

  // Maximum clock stretch.
  max_cs = 230 * 3;

  // The default speed.
  esp_i2c_set_speed(ESP_I2C_SPEED_100);

  in_trans = false;

  return ESP_I2C_OK;
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_start()
{
  uint8_t idx = 0;

  if (in_trans) {
    // We are within transaction so we can assume:
    // - SCL is LO.
    // - SDA is HIGH.
    // - We are in the middle of low clock cycle.

    delay(i2c_delay_short);
    SCL_RELEASE();
    delay(i2c_delay_short);

    if ((idx = chk_cs()) == max_cs) return esp_i2c_fail_fast(ESP_I2C_ERR_LONG_STRETCH);

    // If there was clock stretching we need to delay
    // before driving SDA low.
    if (idx > 0) delay(i2c_delay_short);

    SDA_LOW();
    delay(i2c_delay_short);
    SCL_LOW();
    delay(i2c_delay_short);

    SDA_RELEASE();

    return ESP_I2C_OK;
  }

  // We are not in transaction so SCL and SDA might be in any state.

  SCL_RELEASE();
  SDA_RELEASE();

  // Check for arbitration.
  if (SDA_READ() == ESP_I2C_LO) return ESP_I2C_ERR_ARB_LOST;

  if (chk_cs() == max_cs) return esp_i2c_fail_fast(ESP_I2C_ERR_LONG_STRETCH);

  // Drive SDA low while SCL is high.
  delay(i2c_delay_short);
  SDA_LOW();
  delay(i2c_delay_short);

  // Drive SCL low.
  SCL_LOW();
  delay(i2c_delay_short);

  // Release SDA so the next function has
  // a known state (SCL low, SDA high).
  SDA_RELEASE();
  in_trans = true;

  return ESP_I2C_OK;
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_stop()
{
  // You can issue STOP condition only when in transaction.
  if (!in_trans) return esp_i2c_fail_fast(ESP_I2C_ERR_STOP_OUTSIDE_TRANS);

  SDA_LOW();
  delay(i2c_delay_short);

  SCL_RELEASE();

  if (chk_cs() == max_cs) return esp_i2c_fail_fast(ESP_I2C_ERR_LONG_STRETCH);

  delay(i2c_delay_short);
  SDA_RELEASE();
  delay(i2c_delay_short);

  SCL_LOW();
  delay(i2c_delay_short);

  in_trans = false;

  return ESP_I2C_OK;
}

static esp_i2c_err ICACHE_FLASH_ATTR
write_bit(bool bit)
{
  esp_i2c_err err;

  // Set SDA value before next clock tick.
  (bit) ? SDA_RELEASE() : SDA_LOW();

  err = tick();
  if (err != ESP_I2C_OK) return esp_i2c_fail_fast(err);

  // Make sure SDA is released so next function has
  // a known state (SCL low, SDA high).
  if (!bit) SDA_RELEASE();

  return ESP_I2C_OK;
}

static esp_i2c_err ICACHE_FLASH_ATTR
read_bit(bool *bit)
{
  delay(i2c_delay_short);
  SCL_RELEASE();

  if (chk_cs() == max_cs) return esp_i2c_fail_fast(ESP_I2C_ERR_LONG_STRETCH);

  // Keep clock high.
  delay(i2c_delay_short);

  // Sample SDA.
  *bit = SDA_READ();

  delay(i2c_delay_short);

  // Set clock low.
  SCL_LOW();
  delay(i2c_delay_short);

  return ESP_I2C_OK;
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_write_byte(uint8_t byte, bool *ack_resp)
{
  uint8_t mask;
  esp_i2c_err err;

  for (mask = 0x80; mask; mask >>= 1) {
    err = write_bit(((byte & mask) != 0));
    if (err != ESP_I2C_OK) return err;
  }

  return read_bit(ack_resp);
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_read_byte(uint8_t *dst, bool ack_type)
{
  bool bit;
  uint8_t idx;
  esp_i2c_err err;

  for (idx = 0; idx < 8; idx++) {
    err = read_bit(&bit);
    if (err != ESP_I2C_OK) return err;
    *dst = (*dst << 1) | bit;
  }

  return write_bit(ack_type);
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_write_bytes(uint8_t *buf, uint8_t len)
{
  uint8_t idx;
  bool ack_resp;
  esp_i2c_err err;

  for (idx = 0; idx < len; idx++) {
    err = esp_i2c_write_byte(buf[idx], &ack_resp);
    if (err != ESP_I2C_OK) return esp_i2c_fail_fast(err);
    if (ack_resp != ESP_I2C_ACK) esp_i2c_fail_fast(ESP_I2C_ERR_NO_ACK);
  }

  return ESP_I2C_OK;
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_read_bytes(uint8_t *buf, uint8_t len)
{
  uint8_t idx;
  bool ack_type = ESP_I2C_ACK;
  esp_i2c_err err;

  for (idx = 0; idx < len; idx++) {
    if (idx == len - 1) ack_type = ESP_I2C_NACK;
    err = esp_i2c_read_byte(&buf[idx], ack_type);
    if (err != ESP_I2C_OK) return esp_i2c_fail_fast(err);
  }

  return ESP_I2C_OK;
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_start_read_write(uint8_t address, bool stop_on_nack)
{
  bool ack_resp;
  esp_i2c_err err;

  err = esp_i2c_start();
  if (err != ESP_I2C_OK) return err;

  err = esp_i2c_write_byte(address, &ack_resp);
  if (err != ESP_I2C_OK) return err;
  if (ack_resp != ESP_I2C_ACK) {
    if (stop_on_nack) esp_i2c_stop();
    return esp_i2c_fail_fast(ESP_I2C_ERR_NO_ACK);
  }

  return ESP_I2C_OK;
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_start_write(uint8_t address, uint8_t reg)
{
  bool ack_resp;
  esp_i2c_err err;

  err = esp_i2c_start_read_write(ESP_I2C_ADDR_WRITE(address), true);
  if (err != ESP_I2C_OK) return err;

  err = esp_i2c_write_byte(reg, &ack_resp);
  if (err != ESP_I2C_OK) return err;
  if (ack_resp != ESP_I2C_ACK) return esp_i2c_fail_fast(ESP_I2C_ERR_NO_ACK);

  return ESP_I2C_OK;
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_start_read(uint8_t address, uint8_t reg)
{
  esp_i2c_err err;

  err = esp_i2c_start_write(address, reg);
  if (err != ESP_I2C_OK) return err;

  return esp_i2c_start_read_write(ESP_I2C_ADDR_READ(address), true);
}

void ICACHE_FLASH_ATTR
esp_i2c_free_device_list(esp_i2c_dev *root, bool free_custom)
{
  esp_i2c_dev *curr;
  esp_i2c_dev *next = root;

  while (next != NULL) {
    curr = next;
    next = next->next;
    if (free_custom && curr->custom != NULL) os_free(curr->custom);
    os_free(curr);
  }
}

esp_i2c_err ICACHE_FLASH_ATTR
esp_i2c_scan(esp_i2c_dev **root)
{
  uint8_t address;
  esp_i2c_err err;
  esp_i2c_dev *curr;

  if (*root != NULL) return ESP_I2C_ERR_ROOT_NOT_NULL;
  curr = *root = os_zalloc(sizeof(esp_i2c_dev));

  for (address = 1; address < 128; address++) {
    // Skip reserved addresses.
    // 0000 0XXX
    // 0111 1XXX
    if ((address >> 0x3) == 0xF) continue;
    if ((address & 0x78) == 0 && (address & 0x3) > 0) continue;

    err = esp_i2c_start_read_write(ESP_I2C_ADDR_WRITE(address), true);
    if (err != ESP_I2C_OK) continue;

    // First found device.
    if (curr->address == 0) {
      curr->address = address;
      esp_i2c_stop();
      continue;
    }

    // Add device to the list.
    curr->next = os_zalloc(sizeof(esp_i2c_dev));
    curr->next->address = address;
    curr = curr->next;
    esp_i2c_stop();
  }

  if ((*root)->address == 0x0) {
    esp_i2c_free_device_list(*root, false);
    *root = NULL;
  }

  return ESP_I2C_OK;
}
