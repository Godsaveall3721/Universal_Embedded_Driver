/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: dma.h
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: CIMC_2026_GD32F470
 * Version: 1.01 (2026/7/22) - Original
 */

#ifndef __HARDWARE_DMA_DMA_H__
#define __HARDWARE_DMA_DMA_H__

#include <stdbool.h>

#ifdef __GD32F470__
#include "HeaderFiles.h"
#endif /* __GD32F470__ */

#ifdef __MSPM0G3507__
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_dma.h>
#endif /* __MSPM0G3507__ */

typedef enum {
    DMA_DIR_MEM_TO_PERIPH = 0,
    DMA_DIR_PERIPH_TO_MEM = 1
} dma_dir_t;

/* DMA 硬件配置描述符 */
typedef struct {
    #ifdef __GD32F470__
    uint32_t rcu_dma;               // RCU_DMA1
    uint32_t dma_periph;            // DMA1
    dma_channel_enum channel;       // DMA_CH7
    dma_subperipheral_enum subperi; // DMA_SUBPERI4
    uint32_t periph_addr;           // 外设寄存器物理地址
    uint32_t direction;             // DMA_MEMORY_TO_PERIPH 或 DMA_PERIPH_TO_MEMORY
    uint32_t priority;              // DMA_PRIORITY_ULTRA_HIGH
    #endif /* __GD32F470__ */

    #ifdef __MSPM0G3507__
    DMA_Regs *dma_regs;         // 统一为 DMA (即 DMA0 基址)
    uint8_t   channel_chan_id;  // 使用 SysConfig 生成的宏，如 DMA_UART_0_TX_CH
    uint32_t  periph_addr;      // 外设寄存器地址，如 (uint32_t)&UART0->TXDATA
    dma_dir_t direction;        // 仅用于驱动层区分 TX/RX 逻辑行为
    #endif /* __MSPM0G3507__ */

} dma_hw_config_t;

/* DMA 通道对象基类 */
typedef struct dma_channel_dev {
    const char *name;
    dma_hw_config_t hw;

    void (*init)(struct dma_channel_dev *dev, uint32_t mem_addr, uint16_t buffer_size);
    void (*start_tx)(struct dma_channel_dev *dev, uint32_t src_addr, uint16_t len);
    uint16_t (*get_remain_num)(struct dma_channel_dev *dev);
    void (*reload_rx)(struct dma_channel_dev *dev, uint32_t dest_addr, uint16_t buffer_size);
} dma_channel_dev_t;

#ifdef __GD32F470__
#define DEFINE_DMA_CHANNEL_DEVICE(name_var, init_var, start_tx_var, \
    get_remain_var, reload_rx_var, \
    rcu_dma_val, dma_periph_val, channel_val, \
    subperi_val, periph_addr_val, \
    direction_val, priority_val) \
    dma_channel_dev_t name_var = { \
        .name = #name_var, .init = init_var, .start_tx = start_tx_var, \
        .get_remain_num = get_remain_var, .reload_rx = reload_rx_var, \
        .hw = { .rcu_dma = rcu_dma_val, .dma_periph = dma_periph_val, .channel = channel_val, \
                .subperi = subperi_val, .periph_addr = periph_addr_val, .direction = direction_val, .priority = priority_val } \
    }

/* GD32F4 API */
void dma_channel_init_gd32(dma_channel_dev *self, uint32_t mem_addr, uint16_t buffer_size);
void dma_channel_start_tx_gd32(dma_channel_dev *self, uint32_t src_addr, uint16_t len);
uint16_t dma_channel_get_remain_num_gd32(dma_channel_dev *self);
void dma_channel_reload_rx_gd32(dma_channel_dev *self, uint32_t dest_addr, uint16_t buffer_size);

/*
 * 实例化gd32的DMA通道的使用方法，请在 gd32f4_driver/Function/Function.c 里操作
 * 
 * TX通道
 * DEFINE_DMA_CHANNEL_DEVICE(usart1_dma_tx, dma_channel_init_gd32, dma_channel_start_tx_gd32, dma_channel_get_remain_num_gd32, NULL, \
 * RCU_DMA0, DMA0, DMA_CH6, DMA_SUBPERI4, (uint32_t)&USART_DATA(USART1), DMA_MEMORY_TO_PERIPH, DMA_PRIORITY_ULTRA_HIGH);
 * 
 * RX通道
 * DEFINE_DMA_CHANNEL_DEVICE(usart1_dma_rx, dma_channel_init_gd32, NULL, dma_channel_get_remain_num_gd32, dma_channel_reload_rx_gd32, \
 * RCU_DMA0, DMA0, DMA_CH5, DMA_SUBPERI4, (uint32_t)&USART_DATA(USART1), DMA_PERIPH_TO_MEMORY, DMA_PRIORITY_ULTRA_HIGH);
 * 
 */

#endif /* __GD32F470__ */


#ifdef __MSPM0G3507__

#define DEFINE_DMA_CHANNEL_DEVICE_MSP(name_var, start_tx_var, reload_rx_var, \
    dma_regs_val, channel_id_val, periph_addr_val, trigger_val, direction_val) \
    dma_channel_dev_t name_var = { \
        .name = #name_var, \
        .init = dma_channel_init_msp, \
        .start_tx = start_tx_var, \
        .get_remain_num = dma_channel_get_remain_num_msp, \
        .reload_rx = reload_rx_var, \
        .hw = { \
            .dma_regs = dma_regs_val, \
            .channel_chan_id = channel_id_val, \
            .periph_addr = periph_addr_val, \
            .trigger = trigger_val, \
            .direction = direction_val \
        } \
    }

/* MSPM0 API */
void dma_channel_init_msp(dma_channel_dev_t *self, uint32_t mem_addr, uint16_t buffer_size);
void dma_channel_start_tx_msp(dma_channel_dev_t *self, uint32_t src_addr, uint16_t len);
uint16_t dma_channel_get_remain_num_msp(dma_channel_dev_t *self);
void dma_channel_reload_rx_msp(dma_channel_dev_t *self, uint32_t dest_addr, uint16_t buffer_size);

/*
 * TX 通道：DMA_DIR_MEM_TO_PERIPH
 * DEFINE_DMA_CHANNEL_DEVICE_MSP(usart0_dma_tx, dma_channel_start_tx_msp, NULL, \
 *     DMA, DMA_CH0_CHAN_ID, (uint32_t)&UART0->TXDATA, DL_DMA_TRIGGER_UART0_TX, DMA_DIR_MEM_TO_PERIPH);
 * 
 * RX 通道：DMA_DIR_PERIPH_TO_MEM
 * DEFINE_DMA_CHANNEL_DEVICE_MSP(usart0_dma_rx, NULL, dma_channel_reload_rx_msp, \
 *     DMA, DMA_CH1_CHAN_ID, (uint32_t)&UART0->RXDATA, DL_DMA_TRIGGER_UART0_RX, DMA_DIR_PERIPH_TO_MEM);
 */

#endif /* __MSPM0G3507__ */

#endif /* __HARDWARE_DMA_DMA_H__ */