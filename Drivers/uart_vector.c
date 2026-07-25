/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: uart_vector.c
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: CIMC_2026_GD32F470
 * Version: 1.01 (2026/7/22) - Original
 */

#include "uart_all.h"
#include "dma.h"

#ifdef __GD32F470__
#include "HeaderFiles.h"
#endif /* __GD32F470__ */

#ifdef __MSPM0G3507__
#include <ti/devices/msp/msp.h>
#endif /* __MSPM0G3507__ */

#ifdef __GD32F470__
void USART0_IRQHandler(void)
{
    uart_vector_dispatch(USART0); 
}
#endif /* __GD32F470__ */

#ifdef __MSPM0G3507__
void UART0_IRQHandler(void)
{
    uart_vector_dispatch((uint32_t)UART0);
}
#endif /* __MSPM0G3507__ */