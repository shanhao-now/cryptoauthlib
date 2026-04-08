/**
 * \file
 * \brief STM32 HAL based I2C transport for CryptoAuthLib.
 */

#include "hal_stm32_i2c.h"

#include <string.h>

#ifndef ATCA_STM32_I2C_TX_BUFFER_SIZE
#define ATCA_STM32_I2C_TX_BUFFER_SIZE  (MAX_PACKET_SIZE + 1U)
#endif

static uint8_t hal_stm32_i2c_address(const ATCAIfaceCfg *cfg)
{
#ifdef ATCA_ENABLE_DEPRECATED
    return ATCA_IFACECFG_VALUE(cfg, atcai2c.slave_address);
#else
    return ATCA_IFACECFG_VALUE(cfg, atcai2c.address);
#endif
}

static ATCA_STATUS hal_stm32_i2c_get(ATCAIface iface, ATCAIfaceCfg **cfg, atca_stm32_i2c_t **hal)
{
    if ((NULL == iface) || (NULL == cfg) || (NULL == hal))
    {
        return ATCA_BAD_PARAM;
    }

    *cfg = atgetifacecfg(iface);
    *hal = (atca_stm32_i2c_t*)atgetifacehaldat(iface);

    if ((NULL == *cfg) || (NULL == *hal) || (NULL == (*hal)->handle))
    {
        return ATCA_NOT_INITIALIZED;
    }

    return ATCA_SUCCESS;
}

static void hal_stm32_i2c_lock_if_needed(atca_stm32_i2c_t *hal)
{
    if ((NULL != hal) && (NULL != hal->lock))
    {
        hal->lock(hal->user_context);
    }
}

static void hal_stm32_i2c_unlock_if_needed(atca_stm32_i2c_t *hal)
{
    if ((NULL != hal) && (NULL != hal->unlock))
    {
        hal->unlock(hal->user_context);
    }
}

static ATCA_STATUS hal_stm32_to_atca_status(HAL_StatusTypeDef status)
{
    switch (status)
    {
    case HAL_OK:
        return ATCA_SUCCESS;
    case HAL_TIMEOUT:
        return ATCA_RX_NO_RESPONSE;
    case HAL_BUSY:
        return ATCA_COMM_FAIL;
    case HAL_ERROR:
    default:
        return ATCA_COMM_FAIL;
    }
}

static ATCA_STATUS hal_stm32_i2c_change_baud_internal(atca_stm32_i2c_t *hal, uint32_t baud)
{
    if ((NULL == hal) || (0U == baud))
    {
        return ATCA_BAD_PARAM;
    }

    if (NULL == hal->change_baud)
    {
        return ATCA_UNIMPLEMENTED;
    }

    return hal->change_baud(hal->handle, baud, hal->user_context);
}

static HAL_StatusTypeDef hal_stm32_i2c_tx(atca_stm32_i2c_t *hal, uint16_t address, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Master_Transmit(hal->handle, address, data, len, hal->timeout_ms);
}

static HAL_StatusTypeDef hal_stm32_i2c_rx(atca_stm32_i2c_t *hal, uint16_t address, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Master_Receive(hal->handle, address, data, len, hal->timeout_ms);
}

ATCA_STATUS hal_i2c_init(ATCAIface iface, ATCAIfaceCfg *cfg)
{
    atca_stm32_i2c_t *hal_data;

    if ((NULL == iface) || (NULL == cfg))
    {
        return ATCA_BAD_PARAM;
    }

    if (NULL == cfg->cfg_data)
    {
        return ATCA_BAD_PARAM;
    }

    hal_data = (atca_stm32_i2c_t*)cfg->cfg_data;
    if (NULL == hal_data->handle)
    {
        return ATCA_BAD_PARAM;
    }

    if (0U == hal_data->timeout_ms)
    {
        hal_data->timeout_ms = ATCA_STM32_I2C_TIMEOUT_MS_DEFAULT;
    }

    if (0U == hal_data->wake_baud)
    {
        hal_data->wake_baud = ATCA_STM32_I2C_WAKE_BAUD_DEFAULT;
    }

    if (0U == hal_data->normal_baud)
    {
        hal_data->normal_baud = cfg->atcai2c.baud;
    }

    hal_data->ref_ct++;
    iface->hal_data = hal_data;

    return ATCA_SUCCESS;
}

ATCA_STATUS hal_i2c_post_init(ATCAIface iface)
{
    (void)iface;
    return ATCA_SUCCESS;
}

ATCA_STATUS hal_i2c_send(ATCAIface iface, uint8_t word_address, uint8_t *txdata, int txlength)
{
    ATCAIfaceCfg *cfg;
    atca_stm32_i2c_t *hal;
    uint8_t buffer[ATCA_STM32_I2C_TX_BUFFER_SIZE];
    uint16_t total = 0U;
    uint8_t address;
    HAL_StatusTypeDef hal_status;
    ATCA_STATUS status;

    status = hal_stm32_i2c_get(iface, &cfg, &hal);
    if (ATCA_SUCCESS != status)
    {
        return status;
    }

    if (txlength < 0)
    {
        return ATCA_BAD_PARAM;
    }

    if ((size_t)txlength > MAX_PACKET_SIZE)
    {
        return ATCA_SMALL_BUFFER;
    }

    if (0xFFu != word_address)
    {
        buffer[total++] = word_address;
    }

    if ((NULL != txdata) && (0 < txlength))
    {
        (void)memcpy(&buffer[total], txdata, (size_t)txlength);
        total = (uint16_t)(total + (uint16_t)txlength);
    }

    if (0U == total)
    {
        return ATCA_BAD_PARAM;
    }

    address = hal_stm32_i2c_address(cfg);

    hal_stm32_i2c_lock_if_needed(hal);
    hal_status = hal_stm32_i2c_tx(hal, address, buffer, total);
    hal_stm32_i2c_unlock_if_needed(hal);

    return hal_stm32_to_atca_status(hal_status);
}

ATCA_STATUS hal_i2c_receive(ATCAIface iface, uint8_t word_address, uint8_t *rxdata, uint16_t *rxlength)
{
    ATCAIfaceCfg *cfg;
    atca_stm32_i2c_t *hal;
    ATCA_STATUS status;
    uint8_t address;

    (void)word_address;

    if ((NULL == rxdata) || (NULL == rxlength))
    {
        return ATCA_BAD_PARAM;
    }

    status = hal_stm32_i2c_get(iface, &cfg, &hal);
    if (ATCA_SUCCESS != status)
    {
        return status;
    }

    if (*rxlength < 1U)
    {
        return ATCA_SMALL_BUFFER;
    }

    address = hal_stm32_i2c_address(cfg);

    hal_stm32_i2c_lock_if_needed(hal);
    if (HAL_OK != hal_stm32_i2c_rx(hal, address, rxdata, *rxlength))
    {
        hal_stm32_i2c_unlock_if_needed(hal);
        return ATCA_RX_FAIL;
    }
    hal_stm32_i2c_unlock_if_needed(hal);

    return ATCA_SUCCESS;
}

ATCA_STATUS hal_i2c_wake(ATCAIface iface)
{
    ATCAIfaceCfg *cfg;
    atca_stm32_i2c_t *hal;
    uint8_t response[4];
    uint8_t dummy = 0U;
    ATCA_STATUS status;
    int retries;
    bool restore_baud = false;
    uint32_t normal_baud;

    status = hal_stm32_i2c_get(iface, &cfg, &hal);
    if (ATCA_SUCCESS != status)
    {
        return status;
    }

    normal_baud = (0U != hal->normal_baud) ? hal->normal_baud : cfg->atcai2c.baud;

    if ((normal_baud != hal->wake_baud) && (NULL != hal->change_baud))
    {
        status = hal_stm32_i2c_change_baud_internal(hal, hal->wake_baud);
        if (ATCA_SUCCESS != status)
        {
            return status;
        }
        restore_baud = true;
    }

    hal_stm32_i2c_lock_if_needed(hal);
    (void)HAL_I2C_Master_Transmit(hal->handle, 0x00U, &dummy, 0U, hal->timeout_ms);
    hal_stm32_i2c_unlock_if_needed(hal);

    atca_delay_us(cfg->wake_delay);

    retries = cfg->rx_retries;
    while (retries-- > 0)
    {
        hal_stm32_i2c_lock_if_needed(hal);
        if (HAL_OK == hal_stm32_i2c_rx(hal, hal_stm32_i2c_address(cfg), response, sizeof(response)))
        {
            hal_stm32_i2c_unlock_if_needed(hal);
            status = hal_check_wake(response, (int)sizeof(response));
            break;
        }
        hal_stm32_i2c_unlock_if_needed(hal);
        status = ATCA_WAKE_FAILED;
    }

    if (restore_baud)
    {
        ATCA_STATUS restore_status = hal_stm32_i2c_change_baud_internal(hal, normal_baud);
        if ((ATCA_SUCCESS == status) && (ATCA_SUCCESS != restore_status))
        {
            status = restore_status;
        }
    }

    return status;
}

ATCA_STATUS hal_i2c_idle(ATCAIface iface)
{
    ATCAIfaceCfg *cfg;
    atca_stm32_i2c_t *hal;
    uint8_t idle_cmd = 0x02U;
    ATCA_STATUS status;
    HAL_StatusTypeDef hal_status;

    status = hal_stm32_i2c_get(iface, &cfg, &hal);
    if (ATCA_SUCCESS != status)
    {
        return status;
    }

    hal_stm32_i2c_lock_if_needed(hal);
    hal_status = hal_stm32_i2c_tx(hal, hal_stm32_i2c_address(cfg), &idle_cmd, 1U);
    hal_stm32_i2c_unlock_if_needed(hal);

    return hal_stm32_to_atca_status(hal_status);
}

ATCA_STATUS hal_i2c_sleep(ATCAIface iface)
{
    ATCAIfaceCfg *cfg;
    atca_stm32_i2c_t *hal;
    uint8_t sleep_cmd = 0x01U;
    ATCA_STATUS status;
    HAL_StatusTypeDef hal_status;

    status = hal_stm32_i2c_get(iface, &cfg, &hal);
    if (ATCA_SUCCESS != status)
    {
        return status;
    }

    hal_stm32_i2c_lock_if_needed(hal);
    hal_status = hal_stm32_i2c_tx(hal, hal_stm32_i2c_address(cfg), &sleep_cmd, 1U);
    hal_stm32_i2c_unlock_if_needed(hal);

    return hal_stm32_to_atca_status(hal_status);
}

ATCA_STATUS hal_i2c_release(void *hal_data)
{
    atca_stm32_i2c_t *hal = (atca_stm32_i2c_t*)hal_data;

    if ((NULL != hal) && (0U < hal->ref_ct))
    {
        hal->ref_ct--;
    }

    return ATCA_SUCCESS;
}

#if !defined(ATCA_HAL_LEGACY_API)
ATCA_STATUS hal_i2c_control(ATCAIface iface, uint8_t option, void* param, size_t paramlen)
{
    ATCAIfaceCfg *cfg;
    atca_stm32_i2c_t *hal;
    ATCA_STATUS status;

    (void)paramlen;

    status = hal_stm32_i2c_get(iface, &cfg, &hal);
    if (ATCA_SUCCESS != status)
    {
        return status;
    }

    (void)cfg;

    switch (option)
    {
    case ATCA_HAL_CONTROL_WAKE:
        return hal_i2c_wake(iface);
    case ATCA_HAL_CONTROL_IDLE:
        return hal_i2c_idle(iface);
    case ATCA_HAL_CONTROL_SLEEP:
        return hal_i2c_sleep(iface);
    case ATCA_HAL_CONTROL_SELECT:
    case ATCA_HAL_CONTROL_DESELECT:
        return ATCA_SUCCESS;
    case ATCA_HAL_CHANGE_BAUD:
        if ((NULL == param) || (sizeof(uint32_t) != paramlen))
        {
            return ATCA_BAD_PARAM;
        }
        return hal_stm32_i2c_change_baud_internal(hal, *(uint32_t*)param);
    default:
        return ATCA_UNIMPLEMENTED;
    }
}
#endif

__weak void hal_delay_ms(uint32_t delay)
{
    HAL_Delay(delay);
}

__weak void hal_delay_us(uint32_t delay)
{
#if defined(DWT) && defined(DWT_CTRL_CYCCNTENA_Msk)
    uint32_t start;
    uint32_t ticks;

    if (0U == (CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    if (0U == (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))
    {
        DWT->CYCCNT = 0U;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    start = DWT->CYCCNT;
    ticks = delay * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks)
    {
    }
#else
    uint32_t cycles = delay * (SystemCoreClock / 5000000U);
    while (cycles-- > 0U)
    {
        __NOP();
    }
#endif
}