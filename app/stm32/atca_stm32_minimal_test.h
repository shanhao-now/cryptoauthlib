#ifndef ATCA_STM32_MINIMAL_TEST_H
#define ATCA_STM32_MINIMAL_TEST_H

#include "cryptoauthlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct atca_stm32_minimal_result_s
{
    uint8_t revision[4];
    uint8_t serial_number[9];
} atca_stm32_minimal_result_t;

ATCA_STATUS atca_stm32_test_init(void);
ATCA_STATUS atca_stm32_test_read_info(atca_stm32_minimal_result_t *result);
ATCA_STATUS atca_stm32_test_release(void);
ATCA_STATUS atca_stm32_test_run_minimal(atca_stm32_minimal_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* ATCA_STM32_MINIMAL_TEST_H */
