/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: uart_all.c
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: CIMC_2026_GD32F470
 * Version: 1.00 (2026/5/30) - Original
 */

#include "uart_all.h"


/* 运行期动态串口对象区块链链表头 */
static uart_base_t *uart_head = NULL;


#ifdef __GD32F470__

/* --- RS-485 部分 --- */

/* --- 辅助函数 --- */

/* 485 中断处理自适应状态机 */
static void rs485_isr_blockchain_bridge(void *self)
{
    // 对象的 base 是第一个成员，指针物理等价，直接强转回子类
    uart_485_t *dev = (uart_485_t *)self;
    uint32_t usart = dev->hw.usart_periph;

if (dev->base.dma_tx == NULL) {
            uart_irq_handler_rs485_gd32(dev);
    }

    /* 如果启用了 DMA 接收，中断只负责剥离(总线空闲 IDLE 断帧) */
    if (dev->base.dma_rx != NULL)
    {
        if (RESET != usart_interrupt_flag_get(usart, USART_INT_FLAG_IDLE)) 
        {
            usart_data_receive(usart); // 清除 IDLE 标志
            uint16_t remain = dev->base.dma_rx->get_remain_num(dev->base.dma_rx);
            dev->base.rx_len = dev->base.rx_buf_size - remain;

            if (dev->base.rx_len > 0 && dev->base.rx_len < dev->base.rx_buf_size) 
            {
                dev->base.dma_rx->reload_rx(dev->base.dma_rx, (uint32_t)dev->base.rx_buf, dev->base.rx_buf_size);
                dev->base.rx_complete = true; // 异步抛给主循环消费
            }
        }
    }
    else 
    {
        /* 未启用 DMA 接收，则退回到纯传统中断服务中接收流量(处理 RBNE 和 IDLE) */
        uart_receive_handle_rs485_gd32(dev);
    }
}

/* 485 的发送中断处理函数 */
void uart_irq_handler_rs485_gd32(void *self)
{
    uart_485_t *dev = (uart_485_t *)self;

    if (usart_interrupt_flag_get(dev->hw.usart_periph, USART_INT_FLAG_TBE))
    {
        if (dev->base.tx_index < dev->base.tx_len)
        {
            usart_data_transmit(dev->hw.usart_periph, dev->base.tx_buf[dev->base.tx_index]);
            dev->base.tx_index++;
        }
        else
        {
            dev->base.tx_buf[dev->base.tx_index] = '\0';
            dev->base.tx_index = 0;
            dev->base.tx_len = 0;
            usart_interrupt_disable(dev->hw.usart_periph, USART_INT_TBE);
            usart_interrupt_enable(dev->hw.usart_periph, USART_INT_TC);
        }
    }

    if (usart_interrupt_flag_get(dev->hw.usart_periph, USART_INT_FLAG_TC))
    {
        usart_interrupt_flag_clear(dev->hw.usart_periph, USART_INT_FLAG_TC);
        usart_interrupt_disable(dev->hw.usart_periph, USART_INT_TC);
        dev->set_dir(dev, uart_receive_state);
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_RBNE);
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_IDLE);
        dev->base.send_busy = false;
    }
}

/* 485 的接收中断处理函数 */
void uart_receive_handle_rs485_gd32(void *self)
{
    uart_485_t *dev = (uart_485_t *)self;
    /* 接收数据寄存器非空中断 (RBNE) */
    if (usart_interrupt_flag_get(dev->hw.usart_periph, USART_INT_FLAG_RBNE))
    {
        uint8_t rx_data = usart_data_receive(dev->hw.usart_periph);
        
        // 存入缓冲区，防止溢出
        if (dev->base.rx_index < dev->base.rx_buf_size)
        {
            dev->base.rx_buf[dev->base.rx_index++] = rx_data;
        }
    }

    /* 总线空闲中断 (IDLE) - 一帧数据接收完成的信号 */
    if (usart_interrupt_flag_get(dev->hw.usart_periph, USART_INT_FLAG_IDLE))
    {
        // 清除 IDLE 标志 (GD32 读一下数据寄存器即可)
        usart_data_receive(dev->hw.usart_periph); 

        // 记录这一帧的长度
        dev->base.rx_len = dev->base.rx_index;
        
        // 重置索引以便接收下一帧
        dev->base.rx_index = 0;
        
        // 设置标志位，通知主程序有数据到达
        dev->base.rx_complete = true;
    }
}

/* --- 公共 API 实现 --- */

/* 485 的初始化函数 */
void rs485_init_gd32(void *self)
{
    uart_485_t *dev = (uart_485_t *)self; // 强制转换回派生类

    dev->base.isr_handler = rs485_isr_blockchain_bridge; // 绑定子类专属中断桥
    dev->base.next = uart_head;                          // 头插法串联成区块链
    uart_head = (uart_base_t *)dev;

    nvic_irq_enable(dev->hw.usart_nvic, 5, 0);
    rcu_periph_clock_enable((rcu_periph_enum)dev->hw.rcu_usart);

    rcu_periph_clock_enable((rcu_periph_enum)dev->hw.tx_pin.rcu_gpio);
    rcu_periph_clock_enable((rcu_periph_enum)dev->hw.rx_pin.rcu_gpio);
    rcu_periph_clock_enable((rcu_periph_enum)dev->hw.de_pin.rcu_gpio);


    gpio_mode_set(dev->hw.de_pin.port, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN, dev->hw.de_pin.pin);
    gpio_output_options_set(dev->hw.de_pin.port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, dev->hw.de_pin.pin);

    gpio_af_set(dev->hw.tx_pin.port, GPIO_AF_7, dev->hw.tx_pin.pin);
    gpio_mode_set(dev->hw.tx_pin.port, GPIO_MODE_AF, GPIO_PUPD_NONE, dev->hw.tx_pin.pin);
    gpio_output_options_set(dev->hw.tx_pin.port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, dev->hw.tx_pin.pin);

    gpio_af_set(dev->hw.rx_pin.port, GPIO_AF_7, dev->hw.rx_pin.pin);
    gpio_mode_set(dev->hw.rx_pin.port, GPIO_MODE_AF, GPIO_PUPD_NONE, dev->hw.rx_pin.pin);
    gpio_output_options_set(dev->hw.rx_pin.port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, dev->hw.rx_pin.pin);

    /* 配置USART */
    usart_deinit(dev->hw.usart_periph);
    usart_baudrate_set(dev->hw.usart_periph, 115200U);
    usart_transmit_config(dev->hw.usart_periph, USART_TRANSMIT_ENABLE);
    usart_receive_config(dev->hw.usart_periph, USART_RECEIVE_ENABLE);
    usart_enable(dev->hw.usart_periph);

    if (dev->base.dma_tx != NULL && dev->base.dma_rx != NULL) {
        /* DMA 模式 */
        usart_dma_transmit_config(dev->hw.usart_periph, USART_TRANSMIT_DMA_ENABLE);
        usart_dma_receive_config(dev->hw.usart_periph, USART_RECEIVE_DMA_ENABLE);

        /* 面向对象动态重载，直接把实例里绑定的既有数组指针和容量甩给 DMA，完全不改名！ */
        dev->base.dma_tx->init(dev->base.dma_tx, (uint32_t)dev->base.tx_buf, dev->base.tx_buf_size);
        dev->base.dma_rx->init(dev->base.dma_rx, (uint32_t)dev->base.rx_buf, dev->base.rx_buf_size);
        
        /* 开启空闲帧断帧中断，配合 DMA 接收 */
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_IDLE);
    }
    else 
    {
        /* 接收缓冲区非空中断模式 */
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_RBNE);  
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_IDLE);  
    }

    dev->set_dir(dev, uart_receive_state);
}

/* 485 的特定函数 */
void set_dir_gd32(void *self, uart_state_t de_var)
{
    uart_485_t *dev = (uart_485_t *)self;
    if (de_var == uart_send_state)
            gpio_bit_set(dev->hw.de_pin.port, dev->hw.de_pin.pin);
    else if (de_var == uart_receive_state)
            gpio_bit_reset(dev->hw.de_pin.port, dev->hw.de_pin.pin);
}

/* 485 的发送函数 */
void rs485_send_gd32(void *self, const uint8_t *data, uint8_t len)
{
    uart_485_t *dev = (uart_485_t *)self;

    if (len > dev->base.tx_buf_size) return;

    while (dev->base.send_busy); // 忙检查

    dev->base.send_busy = true;

    dev->set_dir(dev, uart_send_state);

    memcpy(dev->base.tx_buf, data, len);

    if (dev->base.dma_tx != NULL)
    {
        /* DMA中断发送总线模式 */
        dev->base.dma_tx->start_tx(dev->base.dma_tx, (uint32_t)dev->base.tx_buf, len);

        while (RESET == usart_flag_get(dev->hw.usart_periph, USART_FLAG_TC));
        usart_flag_clear(dev->hw.usart_periph, USART_FLAG_TC);

        dev->set_dir(dev, uart_receive_state);
        dev->base.send_busy = false; // start_tx 内部是阻塞等完的，直接释放忙状态
    }
    else
    {
        /* 标准传统中断发送总线模式 */
        usart_interrupt_disable(dev->hw.usart_periph, USART_INT_RBNE);
        usart_interrupt_disable(dev->hw.usart_periph, USART_INT_IDLE);

        dev->base.tx_len = len;
        dev->base.tx_index = 0;
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_TBE);
    }
}


/* 485 的接收函数 */
void rs485_receive_gd32(void *self) // 注，把rx_buf单独参数指针传入会卡死
{
    uart_485_t *dev = (uart_485_t *)self;

    if (dev->base.send_busy)
                    goto end;

    /* 应当将此过程视为模板 */
    if (dev->base.rx_complete) {

        // 业务逻辑
		if (memcmp(dev->base.rx_buf , "break" , 5) == 0)
                                                goto break_out;
        dev->base.send(dev, dev->base.rx_buf, dev->base.rx_len);
break_out:

        dev->base.rx_complete = false;
        dev->base.rx_len = 0;
        dev->base.rx_index = 0;
    }

end:
    return;
}


/* --- RS-232 部分 --- */

/* --- 辅助函数 --- */

static void rs232_isr_blockchain_bridge(void *self)
{
    // 对象的 base 是第一个成员，指针物理等价，直接强转回子类
    uart_232_t *dev = (uart_232_t *)self;
    uint32_t usart = dev->hw.usart_periph;

    if (dev->base.dma_tx == NULL) {
        uart_irq_handler_rs232_gd32(dev);
    }

    if (dev->base.dma_rx != NULL) 
    {
        if (RESET != usart_interrupt_flag_get(usart, USART_INT_FLAG_IDLE)) 
        {
            usart_data_receive(usart); 
            uint16_t remain = dev->base.dma_rx->get_remain_num(dev->base.dma_rx);
            dev->base.rx_len = dev->base.rx_buf_size - remain;

            if (dev->base.rx_len > 0 && dev->base.rx_len < dev->base.rx_buf_size) 
            {
                dev->base.dma_rx->reload_rx(dev->base.dma_rx, (uint32_t)dev->base.rx_buf, dev->base.rx_buf_size);
                dev->base.rx_complete = true; 
            }
        }
    } 
    else 
    {
        uart_receive_handle_rs232_gd32(dev);
    }
}

void uart_irq_handler_rs232_gd32(void *self)
{
uart_232_t *dev = (uart_232_t *)self;
    uint32_t usart = dev->hw.usart_periph;
    if (usart_interrupt_flag_get(usart, USART_INT_FLAG_TBE))
    {
        if (dev->base.tx_index < dev->base.tx_len)
        {
            usart_data_transmit(usart, dev->base.tx_buf[dev->base.tx_index++]);
        }
        else
        {
            usart_interrupt_disable(usart, USART_INT_TBE);
            dev->base.send_busy = false; 
        }
    }
}

void uart_receive_handle_rs232_gd32(void *self)
{
uart_232_t *dev = (uart_232_t *)self;
    uint32_t usart = dev->hw.usart_periph;

    // 接收 RBNE
    if (usart_interrupt_flag_get(usart, USART_INT_FLAG_RBNE))
    {
        if (dev->base.rx_index < dev->base.rx_buf_size)
            dev->base.rx_buf[dev->base.rx_index++] = usart_data_receive(usart);
    }

    // 空闲中断 IDLE
    if (usart_interrupt_flag_get(usart, USART_INT_FLAG_IDLE))
    {
        usart_data_receive(usart); // 清除 IDLE
        dev->base.rx_len = dev->base.rx_index;
        dev->base.rx_index = 0;
        dev->base.rx_complete = true;
    }
}


/* --- 公共 API 实现 --- */

void rs232_init_gd32(void *self)
{
    uart_232_t *dev = (uart_232_t *)self;

    dev->base.isr_handler = rs232_isr_blockchain_bridge; // 绑定子类专属中断桥
    dev->base.next = uart_head;                          // 头插法串联成区块链
    uart_head = (uart_base_t *)dev;

	/* 开启对应的中断处理函数 */     
	nvic_irq_enable(dev->hw.usart_nvic , 3 , 0);

    /* 使能USART时钟 */
	rcu_periph_clock_enable((rcu_periph_enum)dev->hw.rcu_usart);
	rcu_periph_clock_enable((rcu_periph_enum)dev->hw.tx_pin.rcu_gpio);
	rcu_periph_clock_enable((rcu_periph_enum)dev->hw.rx_pin.rcu_gpio);

	gpio_af_set(dev->hw.tx_pin.port , GPIO_AF_7 , dev->hw.tx_pin.pin);
	gpio_mode_set(dev->hw.tx_pin.port , GPIO_MODE_AF , GPIO_PUPD_NONE , dev->hw.tx_pin.pin);
	gpio_output_options_set(dev->hw.tx_pin.port , GPIO_OTYPE_PP , GPIO_OSPEED_50MHZ , dev->hw.tx_pin.pin);

	gpio_af_set(dev->hw.rx_pin.port , GPIO_AF_7 , dev->hw.rx_pin.pin);
	gpio_mode_set(dev->hw.rx_pin.port , GPIO_MODE_AF , GPIO_PUPD_NONE , dev->hw.rx_pin.pin);
	gpio_output_options_set(dev->hw.rx_pin.port , GPIO_OTYPE_PP , GPIO_OSPEED_50MHZ , dev->hw.rx_pin.pin);

	/* 配置USART */
	usart_deinit(dev->hw.usart_periph);
	usart_baudrate_set(dev->hw.usart_periph , 115200U);
	usart_receive_config(dev->hw.usart_periph , USART_RECEIVE_ENABLE);
	usart_transmit_config(dev->hw.usart_periph , USART_TRANSMIT_ENABLE);
	usart_enable(dev->hw.usart_periph);

    if (dev->base.dma_tx != NULL && dev->base.dma_rx != NULL) {
        /* DMA 模式 */
        usart_dma_transmit_config(dev->hw.usart_periph, USART_TRANSMIT_DMA_ENABLE);
        usart_dma_receive_config(dev->hw.usart_periph, USART_RECEIVE_DMA_ENABLE);

        /* 面向对象动态重载，绑定的既有数组指针和容量传递给 DMA */
        dev->base.dma_tx->init(dev->base.dma_tx, (uint32_t)dev->base.tx_buf, dev->base.tx_buf_size);
        dev->base.dma_rx->init(dev->base.dma_rx, (uint32_t)dev->base.rx_buf, dev->base.rx_buf_size);
        
        /* 开启空闲帧断帧中断，配合 DMA 接收 */
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_IDLE);
    }
    else 
    {
        /* 接收缓冲区非空中断模式 */
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_RBNE);  
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_IDLE);  
    }
}

void rs232_send_gd32(void *self, const uint8_t *data, uint8_t len)
{
    uart_232_t *dev = (uart_232_t *)self;

    if (len > dev->base.tx_buf_size) return;

    while (dev->base.send_busy); // 忙检查

    dev->base.send_busy = true;

    memcpy(dev->base.tx_buf, data, len);

    if (dev->base.dma_tx != NULL)
    {
        /* DMA中断发送总线模式 */
        dev->base.dma_tx->start_tx(dev->base.dma_tx, (uint32_t)dev->base.tx_buf, len);
        dev->base.send_busy = false; // start_tx 内部是阻塞等完的，直接释放忙状态
    }
    else
    {
        /* 标准传统中断发送总线模式 */
        dev->base.tx_len = len;
        dev->base.tx_index = 0;
        usart_interrupt_enable(dev->hw.usart_periph, USART_INT_TBE);
    }
}

void rs232_receive_gd32(void *self) // 注，把rx_buf单独参数指针传入会卡死
{
uart_232_t *dev = (uart_232_t *)self;
    
    // 如果你在发送，就不处理接收
    if (dev->base.send_busy) goto end;

    if (dev->base.rx_complete) {
        // 业务逻辑
		if (memcmp(dev->base.rx_buf , "break" , 5) == 0)
                                                goto break_out;
        dev->base.send(dev, dev->base.rx_buf, dev->base.rx_len);
break_out:

        dev->base.rx_complete = false;
        dev->base.rx_len = 0;
        dev->base.rx_index = 0;
    }
end:
    return;
}

#endif /* __GD32F470__ */


#ifdef __MSPM0G3507__

/* --- RS-485 部分 --- */

void uart_irq_handler_rs485_msp(void *self, DL_UART_IIDX pending_stat)
{
    uart_485_t *dev = (uart_485_t *)self;
    UART_Regs *usart = dev->hw.usart_periph;

    switch (pending_stat) {
        case DL_UART_IIDX_TX:
            /* 尽量填满 TX FIFO */
            while (dev->base.tx_index < dev->base.tx_len &&
                   !DL_UART_Main_isTXFIFOFull(usart)) {
                DL_UART_Main_transmitData(usart, dev->base.tx_buf[dev->base.tx_index++]);
            }

            /* RAM 中的数据全部塞入 FIFO 后，关闭 TX 中断，开启 EOT(传输完成) 中断等待移位寄存器发完 */
            if (dev->base.tx_index >= dev->base.tx_len) {
                DL_UART_Main_disableInterrupt(usart, DL_UART_MAIN_INTERRUPT_TX);
                DL_UART_Main_enableInterrupt(usart, DL_UART_MAIN_INTERRUPT_EOT_DONE);
            }
            break;

        case DL_UART_IIDX_EOT_DONE:
            /* 物理总线数据彻底发送完毕，切回接收方向 */
            DL_UART_Main_disableInterrupt(usart, DL_UART_MAIN_INTERRUPT_EOT_DONE);
            dev->set_dir(dev, uart_receive_state);
            
            /* 重新使能接收中断 */
            DL_UART_Main_enableInterrupt(usart, DL_UART_MAIN_INTERRUPT_RX | DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);
            dev->base.send_busy = false;
            break;

        default:
            break;
    }
}

void uart_receive_handle_rs485_msp(void *self, DL_UART_IIDX pending_stat)
{
    uart_485_t *dev = (uart_485_t *)self;
    UART_Regs *usart = dev->hw.usart_periph;

    switch (pending_stat) {
        case DL_UART_IIDX_RX:
        case DL_UART_IIDX_RX_TIMEOUT_ERROR:
            /* 只要有数据到达，全部读入 rx_buf */
            while (!DL_UART_Main_isRXFIFOEmpty(usart)) {
                if (dev->base.rx_index < dev->base.rx_buf_size) {
                    dev->base.rx_buf[dev->base.rx_index++] = DL_UART_Main_receiveData(usart);
                } else {
                    (void)DL_UART_Main_receiveData(usart); // 溢出防护
                }
            }

            /* 只要收到数据即更新状态，彻底摆脱 Timeout 硬件依赖 */
            if (dev->base.rx_index > 0) {
                dev->base.rx_len = dev->base.rx_index;
                dev->base.rx_complete = true;
            }
            break;

        default:
            break;
    }
}

static void rs485_isr_blockchain_bridge_msp(void *self)
{
    uart_485_t *dev = (uart_485_t *)self;

    /* 顶级 ISR 单次读取硬件 IIDX 寄存器并保存 */
    DL_UART_IIDX pending = DL_UART_Main_getPendingInterrupt(dev->hw.usart_periph);

    if (pending == DL_UART_IIDX_NO_INTERRUPT) {
        return;
    }

    /* 非 DMA 模式下的发送/方向切换中断处理 */
    if (dev->base.dma_tx == NULL) {
        uart_irq_handler_rs485_msp(dev, pending);
    }

    /* 接收中断处理 */
    if (dev->base.dma_rx != NULL) {
        if (pending == DL_UART_IIDX_DMA_DONE_RX) {
            uint16_t remain = dev->base.dma_rx->get_remain_num(dev->base.dma_rx);
            dev->base.rx_len = dev->base.rx_buf_size - remain;

            if (dev->base.rx_len > 0 && dev->base.rx_len < dev->base.rx_buf_size) {
                dev->base.dma_rx->reload_rx(dev->base.dma_rx, (uint32_t)dev->base.rx_buf, dev->base.rx_buf_size);
                dev->base.rx_complete = true;
            }
        }
    } else {
        uart_receive_handle_rs485_msp(dev, pending);
    }
}

void rs485_init_msp(void *self)
{
    uart_485_t *dev = (uart_485_t *)self;

    dev->base.isr_handler = rs485_isr_blockchain_bridge_msp;
    dev->base.next = (struct uart_base_tag *)uart_head;
    uart_head = (uart_base_t *)dev;

    /* 配置 DE/RE 控制引脚 */
    DL_GPIO_initDigitalOutput(dev->hw.de_pin.pin);
    DL_GPIO_clearPins(dev->hw.de_pin.port, dev->hw.de_pin.pin);
    DL_GPIO_enableOutput(dev->hw.de_pin.port, dev->hw.de_pin.pin);

    if (dev->base.dma_tx != NULL && dev->base.dma_rx != NULL) {
        dev->base.dma_tx->init(dev->base.dma_tx, (uint32_t)dev->base.tx_buf, dev->base.tx_buf_size);
        dev->base.dma_rx->init(dev->base.dma_rx, (uint32_t)dev->base.rx_buf, dev->base.rx_buf_size);
        DL_UART_Main_enableInterrupt(dev->hw.usart_periph, DL_UART_MAIN_INTERRUPT_DMA_DONE_RX);
    } else {
        DL_UART_Main_clearInterruptStatus(dev->hw.usart_periph, 0xFFFFFFFF);
        DL_UART_Main_enableInterrupt(dev->hw.usart_periph, DL_UART_MAIN_INTERRUPT_RX | DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);
    }

    /* 默认为接收状态 */
    dev->set_dir(dev, uart_receive_state);
}

void set_dir_msp(void *self, uart_state_t de_var)
{
    uart_485_t *dev = (uart_485_t *)self;
    if (de_var == uart_send_state)
        DL_GPIO_setPins(dev->hw.de_pin.port, dev->hw.de_pin.pin);
    else if (de_var == uart_receive_state)
        DL_GPIO_clearPins(dev->hw.de_pin.port, dev->hw.de_pin.pin);
}

void rs485_send_msp(void *self, const uint8_t *data, uint8_t len)
{
    uart_485_t *dev = (uart_485_t *)self;

    if (len == 0 || len > dev->base.tx_buf_size) return;

    while (dev->base.send_busy);

    dev->base.send_busy = true;
    memcpy(dev->base.tx_buf, data, len);

    /* 切换为发送模式 */
    dev->set_dir(dev, uart_send_state);

    if (dev->base.dma_tx != NULL) {
        dev->base.dma_tx->start_tx(dev->base.dma_tx, (uint32_t)dev->base.tx_buf, len);

        /* 硬件等待 UART 发送完成 */
        while (DL_UART_Main_isBusy(dev->hw.usart_periph));

        dev->set_dir(dev, uart_receive_state);
        dev->base.send_busy = false;
    } else {
        /* 关闭接收中断，防止发送时自发自收（如果收发引脚未硬隔离） */
        DL_UART_Main_disableInterrupt(dev->hw.usart_periph, DL_UART_MAIN_INTERRUPT_RX | DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);

        dev->base.tx_len = len;
        dev->base.tx_index = 0;

        /* 先压入 FIFO */
        while (dev->base.tx_index < dev->base.tx_len &&
               !DL_UART_Main_isTXFIFOFull(dev->hw.usart_periph)) {
            DL_UART_Main_transmitData(dev->hw.usart_periph, dev->base.tx_buf[dev->base.tx_index++]);
        }

        /* 使能 TX 中断继续后续发送 */
        DL_UART_Main_enableInterrupt(dev->hw.usart_periph, DL_UART_MAIN_INTERRUPT_TX);
    }
}

void rs485_receive_msp(void *self)
{
    uart_485_t *dev = (uart_485_t *)self;

    /* 移除 if (dev->base.send_busy) goto end; */

    if (dev->base.rx_complete) {
        if (memcmp(dev->base.rx_buf, "break", 5) == 0)
            goto break_out;

        /* 回显接收到的数据 */
        dev->base.send(dev, dev->base.rx_buf, dev->base.rx_len);

break_out:
        dev->base.rx_complete = false;
        dev->base.rx_len = 0;
        dev->base.rx_index = 0;
    }
}

/* --- RS-232 部分 --- */

void uart_irq_handler_rs232_msp(void *self, DL_UART_IIDX pending_stat)
{
    uart_232_t *dev = (uart_232_t *)self;
    UART_Regs *usart = dev->hw.usart_periph;

    if (pending_stat == DL_UART_IIDX_TX) {
        while (dev->base.tx_index < dev->base.tx_len &&
               !DL_UART_Main_isTXFIFOFull(usart)) {
            DL_UART_Main_transmitData(usart, dev->base.tx_buf[dev->base.tx_index++]);
        }

        if (dev->base.tx_index >= dev->base.tx_len) {
            DL_UART_Main_disableInterrupt(usart, DL_UART_MAIN_INTERRUPT_TX);
            dev->base.send_busy = false;
        }
    }
}

void uart_receive_handle_rs232_msp(void *self, DL_UART_IIDX pending_stat)
{
    uart_232_t *dev = (uart_232_t *)self;
    UART_Regs *usart = dev->hw.usart_periph;

    switch (pending_stat) {
        case DL_UART_IIDX_RX:
        case DL_UART_IIDX_RX_TIMEOUT_ERROR:
            /* 只要进中断，就把 FIFO 里的数据全部搬到 rx_buf */
            while (!DL_UART_Main_isRXFIFOEmpty(usart)) {
                if (dev->base.rx_index < dev->base.rx_buf_size) {
                    dev->base.rx_buf[dev->base.rx_index++] = DL_UART_Main_receiveData(usart);
                } else {
                    (void)DL_UART_Main_receiveData(usart); // 溢出抛弃
                }
            }

            /* 【降级兼容策略】
             * 只要缓冲区里有数据，无论是因为 Timeout 触发，还是普通 RX 触发，
             * 只要收到了数据就更新 rx_len，并把 rx_complete 置 true！
             * 彻底摆脱对 Timeout 硬件中断的绝对依赖。
             */
            if (dev->base.rx_index > 0) {
                dev->base.rx_len = dev->base.rx_index;
                dev->base.rx_complete = true;
            }
            break;

        default:
            break;
    }
}

static void rs232_isr_blockchain_bridge_msp(void *self)
{
    uart_232_t *dev = (uart_232_t *)self;

    /* 单次读取 IIDX 寄存器 */
    DL_UART_IIDX pending = DL_UART_Main_getPendingInterrupt(dev->hw.usart_periph);

    if (pending == DL_UART_IIDX_NO_INTERRUPT) {
        return;
    }

    /* 处理 TX */
    if (dev->base.dma_tx == NULL) {
        uart_irq_handler_rs232_msp(dev, pending);
    }

    /* 处理 RX */
    if (dev->base.dma_rx != NULL) {
        if (pending == DL_UART_IIDX_DMA_DONE_RX) {
            uint16_t remain = dev->base.dma_rx->get_remain_num(dev->base.dma_rx);
            dev->base.rx_len = dev->base.rx_buf_size - remain;

            if (dev->base.rx_len > 0 && dev->base.rx_len < dev->base.rx_buf_size) {
                dev->base.dma_rx->reload_rx(dev->base.dma_rx, (uint32_t)dev->base.rx_buf, dev->base.rx_buf_size);
                dev->base.rx_complete = true;
            }
        }
    } else {
        uart_receive_handle_rs232_msp(dev, pending);
    }
}

void rs232_init_msp(void *self)
{
    uart_232_t *dev = (uart_232_t *)self;

    dev->base.isr_handler = rs232_isr_blockchain_bridge_msp;
    dev->base.next = (struct uart_base_tag *)uart_head;
    uart_head = (uart_base_t *)dev;

    if (dev->base.dma_tx != NULL && dev->base.dma_rx != NULL) {
        dev->base.dma_tx->init(dev->base.dma_tx, (uint32_t)dev->base.tx_buf, dev->base.tx_buf_size);
        dev->base.dma_rx->init(dev->base.dma_rx, (uint32_t)dev->base.rx_buf, dev->base.rx_buf_size);

        DL_UART_Main_enableInterrupt(dev->hw.usart_periph, DL_UART_MAIN_INTERRUPT_DMA_DONE_RX);
    } else {
        /* 全局清除所有中断标志，防止上电残余标志锁死中断线 */
        DL_UART_Main_clearInterruptStatus(dev->hw.usart_periph, 0xFFFFFFFF);
        
        /* 开启所有接收相关中断（RX + RX_TIMEOUT） */
        DL_UART_Main_enableInterrupt(dev->hw.usart_periph, 
            DL_UART_MAIN_INTERRUPT_RX | DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);
    }
}

void rs232_send_msp(void *self, const uint8_t *data, uint8_t len)
{
    uart_232_t *dev = (uart_232_t *)self;
    if (len == 0 || len > dev->base.tx_buf_size) return;

    while (dev->base.send_busy);
    dev->base.send_busy = true;

    memcpy(dev->base.tx_buf, data, len);
    dev->base.tx_len   = len;
    dev->base.tx_index = 0;

    while (dev->base.tx_index < dev->base.tx_len &&
           !DL_UART_Main_isTXFIFOFull(dev->hw.usart_periph)) {
        DL_UART_Main_transmitData(dev->hw.usart_periph,
                                  dev->base.tx_buf[dev->base.tx_index++]);
    }

    if (dev->base.tx_index >= dev->base.tx_len) {
        dev->base.send_busy = false;
    } else {
        DL_UART_Main_enableInterrupt(dev->hw.usart_periph, DL_UART_MAIN_INTERRUPT_TX);
    }
}

void rs232_receive_msp(void *self)
{
    uart_232_t *dev = (uart_232_t *)self;

    if (dev->base.rx_complete) {
        if (memcmp(dev->base.rx_buf, "break", 5) == 0)
            goto break_out;

        /* 回显数据 */
        dev->base.send(dev, dev->base.rx_buf, dev->base.rx_len);

break_out:
        dev->base.rx_complete = false;
        dev->base.rx_len = 0;
        dev->base.rx_index = 0;
    }
}
#endif /* __MSPM0G3507__ */

///////////////////////////////
/* 以下是通用部分与 fputc 多态  */
//////////////////////////////

static uart_base_t *default_console = NULL;

void uart_set_console(uart_base_t *dev) {
    default_console = dev;
}

int fputc(int ch, FILE* f)
{
    if (default_console != NULL) {
        uint8_t c = (uint8_t)ch;
        default_console->send((void*)default_console, &c, 1);
    }
    return ch;
}

/* 区块链分发总线 */
void uart_vector_dispatch(uint32_t usart_periph)
{
    uart_base_t *curr = uart_head;
    while (curr != NULL)
    {
        if (curr->usart_periph == usart_periph && curr->isr_handler != NULL)
        {
            curr->isr_handler(curr);
        }
        curr = (uart_base_t *)curr->next;
    }
}
