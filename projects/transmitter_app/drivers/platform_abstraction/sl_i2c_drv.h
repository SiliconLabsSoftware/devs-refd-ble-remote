/***************************************************************************//**
 * @file sl_i2c_drv.h
 *
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#ifndef SL_I2C_DRV_H_
#define SL_I2C_DRV_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>

/// @brief I2C operation flags
typedef enum {
  SL_I2C_OPERATION_WRITE, ///< Write operation
  SL_I2C_OPERATION_READ, ///< Read operation
  SL_I2C_OPERATION_WRITE_READ, ///< Write-read operation
  SL_I2C_OPERATION_WRITE_WRITE, ///< Write-write operation
  //
  SL_I2C_OPERATION_MAX,///< Maximum number of operation flags. DO NOT TOUCH!
} sl_i2c_operation_flag_t;

/**
 * @brief Initializes the I2C peripheral.
 *
 * @param i2c_number   I2C peripheral index to initialize.
 * @param frequency_hz Desired I2C bus frequency in Hz.
 * @param scl_port_pin SCL GPIO port and pin number encoded in a single value (by HW_CNF_PORT_PIN_SET).
 * @param sda_port_pin SDA GPIO port and pin number encoded in a single value (by HW_CNF_PORT_PIN_SET).
 */
int sl_i2c_init(uint8_t i2c_number, uint32_t frequency_hz, uint32_t scl_port_pin, uint32_t sda_port_pin);

/**
 * @brief Reads data from an I2C device.
 *
 * @param i2c_number     I2C peripheral index to use.
 * @param device_address 7-bit I2C device address.
 * @param buf            Pointer to buffer to store read data.
 * @param buf_len        Number of bytes to read.
 * @return 0 on success, negative value on error.
 */
int sl_i2c_read(uint8_t i2c_number, uint8_t device_address, void *buf, size_t buf_len);

/**
 * @brief Writes data to an I2C device.
 *
 * @param i2c_number     I2C peripheral index to use.
 * @param device_address 7-bit I2C device address.
 * @param buf            Pointer to buffer with data to write.
 * @param buf_len        Number of bytes to write.
 * @return 0 on success, negative value on error.
 */
int sl_i2c_write(uint8_t i2c_number, uint8_t device_address, const void *buf, size_t buf_len);

/**
 * @brief Reads data from a specific register of an I2C device.
 *
 * @param i2c_number       I2C peripheral index to use.
 * @param device_address   7-bit I2C device address.
 * @param register_address Register address to read from.
 * @param buf              Pointer to buffer to store read data.
 * @param buf_len          Number of bytes to read.
 * @return 0 on success, negative value on error.
 */
int sl_i2c_read_register(uint8_t i2c_number,
                         uint8_t device_address,
                         uint8_t register_address,
                         void *buf,
                         size_t buf_len);

/**
 * @brief Writes data to a specific register of an I2C device.
 *
 * @param i2c_number       I2C peripheral index to use.
 * @param device_address   7-bit I2C device address.
 * @param register_address Register address to write to.
 * @param buf              Pointer to buffer with data to write.
 * @param buf_len          Number of bytes to write.
 * @return 0 on success, negative value on error.
 */
int sl_i2c_write_register(uint8_t i2c_number,
                          uint8_t device_address,
                          uint8_t register_address,
                          const void *buf,
                          size_t buf_len);

/**
 * @brief Performs a blocking I2C transfer with flexible buffer configuration.
 *
 * This function performs a synchronous I2C transfer operation that blocks until completion.
 * The operation type and buffer usage depends on the operation_flag parameter.
 *
 * @param i2c_number     I2C peripheral index to use.
 * @param device_address 7-bit I2C device address.
 * @param operation_flag Type of I2C operation to perform (read, write, write-read, write-write).
 * @param buf0           Pointer to first buffer (usage depends on operation_flag).
 * @param buf0_len       Length of first buffer in bytes.
 * @param buf1           Pointer to second buffer (usage depends on operation_flag, can be NULL).
 * @param buf1_len       Length of second buffer in bytes.
 * @return 0 on success, negative value on error.
 */
int sl_i2c_transfer_blocking(uint8_t i2c_number,
                             uint8_t device_address,
                             sl_i2c_operation_flag_t operation_flag,
                             void *buf0,
                             size_t buf0_len,
                             void *buf1,
                             size_t buf1_len);

/**
 * @brief Performs an asynchronous I2C transfer with flexible buffer configuration.
 *
 * This function initiates an asynchronous I2C transfer operation that returns immediately.
 * The sl_i2c_transfer_complete callback will be invoked upon completion of the transfer.
 * The operation type and buffer usage depends on the operation_flag parameter.
 *
 * @param i2c_number     I2C peripheral index to use.
 * @param device_address 7-bit I2C device address.
 * @param operation_flag Type of I2C operation to perform (read, write, write-read, write-write).
 * @param buf0           Pointer to first buffer (usage depends on operation_flag).
 * @param buf0_len       Length of first buffer in bytes.
 * @param buf1           Pointer to second buffer (usage depends on operation_flag, can be NULL).
 * @param buf1_len       Length of second buffer in bytes.
 * @return 0 on success, negative value on error.
 */
int sl_i2c_transfer_async(uint8_t i2c_number,
                          uint8_t device_address,
                          sl_i2c_operation_flag_t operation_flag,
                          void *buf0,
                          size_t buf0_len,
                          void *buf1,
                          size_t buf1_len);
/**
 * @brief Callback function invoked upon completion of an asynchronous I2C transfer.
 * @param i2c_number I2C peripheral index.
 * @param result     Result of the I2C transfer (0 on success, negative value on error).
 */
void sl_i2c_transfer_complete(uint8_t i2c_number, int result);

#ifdef __cplusplus
}
#endif
#endif
