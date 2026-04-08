/**
 * \file
 * \brief STM32 HAL based I2C transport for CryptoAuthLib.
 *
 * This HAL is intended for STM32 projects that use the Cube HAL I2C driver.
 * Add this source file to your MCU project and do not compile any of the other
 * platform I2C HAL implementations at the same time.
 */

#ifndef HAL_STM32_I2C_H_
#define HAL_STM32_I2C_H_

#include "atca_hal.h"

#if defined(ATCA_STM32_HAL_HEADER)
#include ATCA_STM32_HAL_HEADER
#elif defined(STM32F0xx)
#include "stm32f0xx_hal.h"
#elif defined(STM32F1xx)
#include "stm32f1xx_hal.h"
#elif defined(STM32F2xx)
#include "stm32f2xx_hal.h"
#elif defined(STM32F3xx)
#include "stm32f3xx_hal.h"
#elif defined(STM32F4xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32F7xx)
#include "stm32f7xx_hal.h"
#elif defined(STM32G0xx)
#include "stm32g0xx_hal.h"
#elif defined(STM32G4xx)
#include "stm32g4xx_hal.h"
#elif defined(STM32H5xx)
#include "stm32h5xx_hal.h"
#elif defined(STM32H7xx)
#include "stm32h7xx_hal.h"
#elif defined(STM32L0xx)
#include "stm32l0xx_hal.h"
#elif defined(STM32L1xx)
#include "stm32l1xx_hal.h"
#elif defined(STM32L4xx)
#include "stm32l4xx_hal.h"
#elif defined(STM32L5xx)
#include "stm32l5xx_hal.h"
#elif defined(STM32U0xx)
#include "stm32u0xx_hal.h"
#elif defined(STM32U5xx)
#include "stm32u5xx_hal.h"
#elif defined(STM32WBxx)
#include "stm32wbxx_hal.h"
#elif defined(STM32WLxx)
#include "stm32wlxx_hal.h"
#else
#error "Unsupported STM32 family. Define ATCA_STM32_HAL_HEADER or include the correct stm32xx_hal.h before this header."
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ATCA_STM32_I2C_TIMEOUT_MS_DEFAULT
#define ATCA_STM32_I2C_TIMEOUT_MS_DEFAULT   (100U)
#endif

#ifndef ATCA_STM32_I2C_WAKE_BAUD_DEFAULT
#define ATCA_STM32_I2C_WAKE_BAUD_DEFAULT    (100000U)
#endif

typedef ATCA_STATUS (*atca_stm32_i2c_change_baud_cb)(I2C_HandleTypeDef *handle, uint32_t baud, void *context);
typedef void (*atca_stm32_i2c_lock_cb)(void *context);

typedef struct atca_stm32_i2c_s
{
    I2C_HandleTypeDef *handle;
    uint32_t timeout_ms;
    uint32_t wake_baud;
    uint32_t normal_baud;
    atca_stm32_i2c_change_baud_cb change_baud;
    atca_stm32_i2c_lock_cb lock;
    atca_stm32_i2c_lock_cb unlock;
    void *user_context;
    uint16_t ref_ct;
} atca_stm32_i2c_t;

ATCA_STATUS hal_i2c_init(ATCAIface iface, ATCAIfaceCfg *cfg);
ATCA_STATUS hal_i2c_post_init(ATCAIface iface);
ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t word_address, uint8_t *txdata, int txlength);
ATCA_STATUS hal_i2c_receive(ATCAIface iface, uint8_t word_address, uint8_t *rxdata, uint16_t *rxlength);
#if !defined(ATCA_HAL_LEGACY_API)
ATCA_STATUS hal_i2c_control(ATCAIface iface, uint8_t option, void* param, size_t paramlen);
#endif
ATCA_STATUS hal_i2c_wake(ATCAIface iface);
ATCA_STATUS hal_i2c_idle(ATCAIface iface);
ATCA_STATUS hal_i2c_sleep(ATCAIface iface);
ATCA_STATUS hal_i2c_release(void *hal_data);

#ifdef __cplusplus
}
#endif

#endif /* HAL_STM32_I2C_H_ */