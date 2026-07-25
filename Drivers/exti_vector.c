/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: exti_vector.c
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: CIMC_2026_GD32F470
 * Version: 1.01 (2026/7/22) - Original
 */

#include "key.h"

#ifdef __GD32F470__
#include "HeaderFiles.h"

#define NEST_EXTI_IRQ_HANDLER_FACTORY(num) \
void EXTI##num##_IRQHandler(void) \
{ \
    if (RESET != exti_interrupt_flag_get(EXTI_##num)) \
    { \
        /* 分发驱动业务逻辑 */ \
        key_exti_dispatch(EXTI_##num); \
        \
        /* 必须清除硬件中断标志位 */ \
        exti_interrupt_flag_clear(EXTI_##num); \
    } \
}

/* 批量生产中断服务函数 */
NEST_EXTI_IRQ_HANDLER_FACTORY(0)
NEST_EXTI_IRQ_HANDLER_FACTORY(1)
NEST_EXTI_IRQ_HANDLER_FACTORY(2)
NEST_EXTI_IRQ_HANDLER_FACTORY(3)
NEST_EXTI_IRQ_HANDLER_FACTORY(4)
#endif /* __GD32F470__ */



#ifdef __MSPM0G3507__

#define CONSUME_GPIO_GROUP_INT(port_suf) \
    do { \
        uint32_t iidx = DL_GPIO_getPendingInterrupt(GPIO##port_suf); \
        if (iidx != DL_GPIO_IIDX_NO_INTR) \
        { \
            key_exti_dispatch_msp(GPIO##port_suf, iidx); \
        } \
    } while(0)

void GROUP1_IRQHandler(void)
{
    /* 一键批量消费各端口的中断向量 */
    CONSUME_GPIO_GROUP_INT(A);
    CONSUME_GPIO_GROUP_INT(B);
    
}


#endif /* __MSPM0G3507__ */



