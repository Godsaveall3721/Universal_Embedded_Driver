/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: dma.c
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: CIMC_2026_GD32F470
 * Version: 1.01 (2026/7/22) - Original
 */

#include "dma.h"



#ifdef __GD32F470__
void dma_channel_init_gd32(dma_channel_dev *self, uint32_t mem_addr, uint16_t buffer_size)
{
    dma_channel_dev_t *dev = (dma_channel_dev_t *)self;
    dma_single_data_parameter_struct dma_init_struct;

    /* 开启对应的全局DMA外设时钟 */
    rcu_periph_clock_enable((rcu_periph_enum)dev->hw.rcu_dma);

    /* 复位指定的通道寄存器 */
    dma_deinit(dev->hw.dma_periph, dev->hw.channel);

    /* 通用单维参数基础绑定 */
    dma_init_struct.direction           = dev->hw.direction;
    dma_init_struct.periph_addr         = dev->hw.periph_addr;
    dma_init_struct.periph_inc          = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    dma_init_struct.memory0_addr        = mem_addr;
    dma_init_struct.memory_inc          = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.number              = buffer_size;
    dma_init_struct.priority            = dev->hw.priority;
    
    dma_single_data_mode_init(dev->hw.dma_periph, dev->hw.channel, &dma_init_struct);
    
    /* 选择通道硬件子复用器 */
    dma_channel_subperipheral_select(dev->hw.dma_periph, dev->hw.channel, dev->hw.subperi);
    dma_circulation_disable(dev->hw.dma_periph, dev->hw.channel);

    /* 如果是接收通道，默认直接拉开接收大门 */
    if (dev->hw.direction == DMA_PERIPH_TO_MEMORY) {
        dma_channel_enable(dev->hw.dma_periph, dev->hw.channel);
    }
}

void dma_channel_start_tx_gd32(dma_channel_dev *self, uint32_t src_addr, uint16_t len)
{
    dma_channel_dev_t *dev = (dma_channel_dev_t *)self;

    /* 临时闭锁通道 */
    dma_channel_disable(dev->hw.dma_periph, dev->hw.channel);
    
    /* 清除上次传输完成状态标记 */
    dma_flag_clear(dev->hw.dma_periph, dev->hw.channel, DMA_FLAG_FTF);
    
    /* 动态重载内存源首地址与长度参数 */
    dma_memory_address_config(dev->hw.dma_periph, dev->hw.channel, DMA_MEMORY_0, src_addr);
    dma_transfer_number_config(dev->hw.dma_periph, dev->hw.channel, len);
    
    /* 重开物理通道传输 */
    dma_channel_enable(dev->hw.dma_periph, dev->hw.channel);
    
    /* 等待本次DMA通道物理传输结束闭环 */
    while (!dma_flag_get(dev->hw.dma_periph, dev->hw.channel, DMA_FLAG_FTF));
}


uint16_t dma_channel_get_remain_num_gd32(dma_channel_dev *self)
{
    dma_channel_dev_t *dev = (dma_channel_dev_t *)self;
    return dma_transfer_number_get(dev->hw.dma_periph, dev->hw.channel);
}

void dma_channel_reload_rx_gd32(dma_channel_dev *self, uint32_t dest_addr, uint16_t buffer_size)
{
    dma_channel_dev_t *dev = (dma_channel_dev_t *)self;

    dma_channel_disable(dev->hw.dma_periph, dev->hw.channel);
    dma_memory_address_config(dev->hw.dma_periph, dev->hw.channel, DMA_MEMORY_0, dest_addr);
    dma_transfer_number_config(dev->hw.dma_periph, dev->hw.channel, buffer_size);
    dma_flag_clear(dev->hw.dma_periph, dev->hw.channel, DMA_FLAG_FTF);
    dma_channel_enable(dev->hw.dma_periph, dev->hw.channel);
}
#endif /* __GD32F470__ */


#ifdef __MSPM0G3507__

/**
 * @brief 运行时的 DMA 通道动态绑定（例如在串口初始化时配置首缓冲区）
 */
void dma_channel_init_msp(dma_channel_dev_t *self, uint32_t mem_addr, uint16_t buffer_size)
{
    dma_channel_dev_t *dev = (dma_channel_dev_t *)self;

    /* 先禁用通道 */
    DL_DMA_disableChannel(dev->hw.dma_regs, dev->hw.channel_chan_id);

    /* 仅更新运行时变化的地址与传输长度 */
    if (dev->hw.direction == DMA_DIR_MEM_TO_PERIPH) {
        DL_DMA_setSrcAddr(dev->hw.dma_regs, dev->hw.channel_chan_id, mem_addr);
        DL_DMA_setDestAddr(dev->hw.dma_regs, dev->hw.channel_chan_id, dev->hw.periph_addr);
    } else {
        DL_DMA_setSrcAddr(dev->hw.dma_regs, dev->hw.channel_chan_id, dev->hw.periph_addr);
        DL_DMA_setDestAddr(dev->hw.dma_regs, dev->hw.channel_chan_id, mem_addr);
    }

    DL_DMA_setTransferSize(dev->hw.dma_regs, dev->hw.channel_chan_id, buffer_size);

    /* RX 接收通道默认使能等待外设数据触发 */
    if (dev->hw.direction == DMA_DIR_PERIPH_TO_MEM) {
        DL_DMA_enableChannel(dev->hw.dma_regs, dev->hw.channel_chan_id);
    }
}

/**
 * @brief 发送模式：重新载入内存地址并启动单次 TX 传输
 */
void dma_channel_start_tx_msp(dma_channel_dev_t *self, uint32_t src_addr, uint16_t len)
{
    dma_channel_dev_t *dev = (dma_channel_dev_t *)self;

    DL_DMA_disableChannel(dev->hw.dma_regs, dev->hw.channel_chan_id);

    DL_DMA_setSrcAddr(dev->hw.dma_regs, dev->hw.channel_chan_id, src_addr);
    DL_DMA_setTransferSize(dev->hw.dma_regs, dev->hw.channel_chan_id, len);

    DL_DMA_enableChannel(dev->hw.dma_regs, dev->hw.channel_chan_id);
}

/**
 * @brief 接收模式：重新加载 RX 目标缓冲区 (如 Idle 中断或 Ping-Pong 切换时)
 */
void dma_channel_reload_rx_msp(dma_channel_dev_t *self, uint32_t dest_addr, uint16_t buffer_size)
{
    dma_channel_dev_t *dev = (dma_channel_dev_t *)self;

    DL_DMA_disableChannel(dev->hw.dma_regs, dev->hw.channel_chan_id);

    DL_DMA_setDestAddr(dev->hw.dma_regs, dev->hw.channel_chan_id, dest_addr);
    DL_DMA_setTransferSize(dev->hw.dma_regs, dev->hw.channel_chan_id, buffer_size);

    DL_DMA_enableChannel(dev->hw.dma_regs, dev->hw.channel_chan_id);
}

/**
 * @brief 查询当前通道剩余未传输数据量
 */
uint16_t dma_channel_get_remain_num_msp(dma_channel_dev_t *self)
{
    dma_channel_dev_t *dev = (dma_channel_dev_t *)self;
    return (uint16_t)DL_DMA_getTransferSize(dev->hw.dma_regs, dev->hw.channel_chan_id);
}

#endif /* __MSPM0G3507__ */