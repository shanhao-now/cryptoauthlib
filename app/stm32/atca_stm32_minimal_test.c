#include "atca_stm32_minimal_test.h"

#include <string.h>

#include "hal_stm32_i2c.h"

extern I2C_HandleTypeDef hi2c2;

static atca_stm32_i2c_t g_atca_i2c_hal = {
    .handle = &hi2c2,
    .timeout_ms = 100U,
    .wake_baud = 100000U,
    .normal_baud = 100000U,
    .change_baud = NULL,
    .lock = NULL,
    .unlock = NULL,
    .user_context = NULL,
    .ref_ct = 0U
};

static ATCAIfaceCfg g_atca_cfg = {
    .iface_type = ATCA_I2C_IFACE,
    .devtype = ATECC608,
    {
#ifdef ATCA_ENABLE_DEPRECATED
        .atcai2c.slave_address = 0xC0,
#else
        .atcai2c.address = 0xC0,
#endif
        .atcai2c.bus = 2,
        .atcai2c.baud = 100000U,
    },
    .wake_delay = 1500,
    .rx_retries = 20,
    .cfg_data = &g_atca_i2c_hal
};

ATCA_STATUS atca_stm32_test_init(void)
{
    return atcab_init(&g_atca_cfg);
}

ATCA_STATUS atca_stm32_test_read_info(atca_stm32_minimal_result_t *result)
{
    ATCA_STATUS status;

    if (NULL == result)
    {
        return ATCA_BAD_PARAM;
    }

    (void)memset(result, 0, sizeof(*result));

    status = atcab_info(result->revision);
    if (ATCA_SUCCESS != status)
    {
        return status;
    }

    status = atcab_read_serial_number(result->serial_number);
    return status;
}

ATCA_STATUS atca_stm32_test_release(void)
{
    return atcab_release();
}

ATCA_STATUS atca_stm32_test_run_minimal(atca_stm32_minimal_result_t *result)
{
    ATCA_STATUS status;

    status = atca_stm32_test_init();
    if (ATCA_SUCCESS != status)
    {
        return status;
    }

    status = atca_stm32_test_read_info(result);

    {
        ATCA_STATUS release_status = atca_stm32_test_release();
        if ((ATCA_SUCCESS == status) && (ATCA_SUCCESS != release_status))
        {
            status = release_status;
        }
    }

    return status;
}
