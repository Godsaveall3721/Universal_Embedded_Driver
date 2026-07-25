/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: uart_all.h
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: CIMC_2026_GD32F470
 * Version: 1.01 (2026/7/24) - Original
 */

#ifndef __HARDWARE_UART_ALL_UART_ALL_H__
#define __HARDWARE_UART_ALL_UART_ALL_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "dma.h"

#ifdef __GD32F470__
#include "HeaderFiles.h"
#endif /* __GD32F470__ */

#ifdef __MSPM0G3507__
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_uart_main.h>
#include <ti/driverlib/dl_gpio.h>
#endif /* __MSPM0G3507__ */




/* --- 公共结构体 --- */

/* uart状态 */
typedef enum {
    uart_receive_state = 0,
    uart_send_state,
    uart_state_max
} uart_state_t;

/* 硬件管脚描述符 */
typedef struct {
    #ifdef __GD32F470__
    uint32_t rcu_gpio;
    uint32_t port;
    uint32_t pin;
    #endif /* __GD32F470__ */

    #ifdef __MSPM0G3507__
    GPIO_Regs *port;
    uint32_t   pin;
    #endif /* __MSPM0G3507__ */
} uart_gpio_config_t;

/* 基类 */
typedef struct {
    const char *name;

    uint8_t *tx_buf;                // 发送缓存指针
    volatile const uint8_t tx_buf_size;      // 显式记录缓冲区容量
    volatile uint8_t tx_len;                 // 总长度
    volatile uint8_t tx_index;               // 当前偏移量
    volatile bool send_busy;        // 忙标志

    uint8_t *rx_buf;                // 接收缓冲区指针
    volatile uint16_t rx_buf_size;           // 缓冲区大小
    volatile uint16_t rx_len;                // 当前帧长度
    volatile uint16_t rx_index;              // 当前偏移量
    volatile bool rx_complete; // 接收完成标志

    struct dma_channel_dev *dma_tx; // 指向 dma.h 的 DMA class
    struct dma_channel_dev *dma_rx; // 指向 dma.h 的 DMA class

    uint32_t usart_periph;           // 属于基类，用于链表在中断匹配
    void (*isr_handler)(void *self); // 动态多态中断桥
    struct uart_base_tag *next;      // 区块链链表指针

    void (*init)(void *self);   // 传入 self 指针以访问私有数据
    void (*send)(void *self, const uint8_t *data, uint8_t len);
    void (*receive)(void *self);
} uart_base_t;


///////////////
/* rs485部分 */
//////////////

/* --- 提供数据容器 --- */
static uint8_t rs485_buf_send_test[128];
static uint8_t rs485_buf_receive_test[128];

/* --- 类型定义与结构体 --- */

/* 构造 RS-485 硬件结构 */
typedef struct {
#ifdef __GD32F470__
    uart_gpio_config_t de_pin;
    uart_gpio_config_t tx_pin;
    uart_gpio_config_t rx_pin;
    uint32_t rcu_usart;
    uint32_t usart_periph;
    uint32_t usart_nvic;
    #endif /* __GD32F470__ */

    #ifdef __MSPM0G3507__
    uart_gpio_config_t de_pin;
    UART_Regs *usart_periph;
    #endif /* __MSPM0G3507__ */
} rs485_hw_config_t;

/* 派生子类 RS-485 */
typedef struct {
    uart_base_t base;
    rs485_hw_config_t hw;
    void (*set_dir)(void *self, uart_state_t de_var);
} uart_485_t;

/* 实例化宏 RS-485 */
#ifdef __GD32F470__
#define DEFINE_RS485_DEVICE(name_var, init_var, send_var, receive_var, set_dir_var, \
    tx_buf_var, rx_buf_var, \
    de_rcu_gpio_var, de_port_var, de_pin_var, \
    tx_rcu_gpio_var, tx_port_var, tx_pin_var, \
    rx_rcu_gpio_var, rx_port_var, rx_pin_var, \
    rcu_usart_var, usart_periph_var, usart_nvic_var, \
    dma_tx_var, dma_rx_var) \
    uart_485_t name_var = { \
        .base = { .name = #name_var, \
            .tx_buf = tx_buf_var, .tx_buf_size = sizeof(tx_buf_var), .tx_len = 0, .tx_index = 0, .send_busy = false, \
            .rx_buf = rx_buf_var, .rx_buf_size = sizeof(rx_buf_var), .rx_len = 0, .rx_index = 0, .rx_complete = false, \
            .init = init_var, .send = send_var, .receive = receive_var, \
            .dma_tx = dma_tx_var, .dma_rx = dma_rx_var, \
            .usart_periph = usart_periph_var, .isr_handler = NULL, .next = NULL }, \
        .set_dir = set_dir_var, \
        .hw = { \
                .de_pin = { .rcu_gpio = de_rcu_gpio_var, .port = de_port_var, .pin = de_pin_var}, \
                .tx_pin = { .rcu_gpio = tx_rcu_gpio_var, .port = tx_port_var, .pin = tx_pin_var}, \
                .rx_pin = { .rcu_gpio = rx_rcu_gpio_var, .port = rx_port_var, .pin = rx_pin_var}, \
                .rcu_usart = rcu_usart_var, \
                .usart_periph = usart_periph_var, \
                .usart_nvic = usart_nvic_var \
        } \
    }


/* --- GD32F4 API --- */
void rs485_init_gd32(void *self);
void rs485_send_gd32(void *self, const uint8_t *data, uint8_t len);
void rs485_receive_gd32(void *self);
void set_dir_gd32(void *self, uart_state_t de_var);
inline void uart_irq_handler_rs485_gd32(void *self);
inline void uart_receive_handle_rs485_gd32(void *self);

/*
 * 
 * 实例化gd32的rs485的使用方法，请在 gd32f4_driver/Function/Function.c 里操作
 * 如果有需要就在需要的地方 extern uart_485_t rs485_gd32;
 *  
 * DEFINE_RS485_DEVICE(rs485_gd32, 
 *    rs485_init_gd32, rs485_send_gd32, rs485_receive_gd32, set_dir_gd32, \
 *    rs485_buf_send_test, rs485_buf_receive_test, \
 *    RCU_GPIOE, GPIOE, GPIO_PIN_8, \
 *    RCU_GPIOD, GPIOD, GPIO_PIN_5, \
 *    RCU_GPIOD, GPIOD, GPIO_PIN_6, \
 *    RCU_USART1, USART1, USART1_IRQn, \
 *    NULL, NULL);
 *  
 *  void USART1_IRQHandler(void)
 *  {
 *      uart_irq_handler_rs485_gd32(&rs485_gd32);
 *      uart_receive_handle_rs485_gd32(&rs485_gd32);
 *  }
 *  
 *  发送的简单测试
 *  rs485_gd32.base.init(&rs485_gd32);
 *  uint8_t test_data[] = "Hello John Fox\n";
 *  while(1) rs485_gd32.base.send(&rs485_gd32, test_data, sizeof(test_data) - 1);
 *  
 *  fputc绑定实例化
 *  rs485_gd32.base.init(&rs485_gd32);
 *  uart_set_console(&rs485_gd32.base);
 *  printf("are u ok?\n");
 *  
 *  接收的简单测试
 *  rs485_gd32.base.init(&rs485_gd32);
 *  while(1) rs485_gd32.base.receive(&rs485_gd32);
 *  
 *  如果需要，可提取核心逻辑，放到自己的逻辑
 *  
 *      if (rs485_gd32_001.base.rx_complete) {
 *      // 1. 业务逻辑
 *  	uint16_t len = rs485_gd32_001.base.rx_len;
 *      uint8_t *data = rs485_gd32_001.base.rx_buf;
 *      rs485_gd32_001.base.send(&rs485_gd32_001, data, len);
 *  
 *      // 2. 自动重置接收标志位和长度，为下一次接收做准备
 *      rs485_gd32_001.base.rx_complete = false;
 *      rs485_gd32_001.base.rx_len = 0;
 *      rs485_gd32_001.base.rx_index = 0;
 *      }
 *  
 *  DMA模式的支持
 *  DEFINE_DMA_CHANNEL_DEVICE(usart1_dma_tx, dma_channel_init_gd32, dma_channel_start_tx_gd32, \
 *      dma_channel_get_remain_num_gd32, NULL, \
 *      RCU_DMA0, DMA0, DMA_CH6, \
 *      DMA_SUBPERI4, (uint32_t)&USART_DATA(USART1), \
 *      DMA_MEMORY_TO_PERIPH, DMA_PRIORITY_ULTRA_HIGH);
 *  
 *  DEFINE_DMA_CHANNEL_DEVICE(usart1_dma_rx, dma_channel_init_gd32, NULL, \
 *      dma_channel_get_remain_num_gd32, dma_channel_reload_rx_gd32, \
 *      RCU_DMA0, DMA0, DMA_CH5, \
 *      DMA_SUBPERI4, (uint32_t)&USART_DATA(USART1), \
 *      DMA_PERIPH_TO_MEMORY, DMA_PRIORITY_ULTRA_HIGH);
 *  
 * DEFINE_RS485_DEVICE(rs485_gd32, 
 *    rs485_init_gd32, rs485_send_gd32, rs485_receive_gd32, set_dir_gd32, \
 *    rs485_buf_send_test, rs485_buf_receive_test, \
 *    RCU_GPIOE, GPIOE, GPIO_PIN_8, \
 *    RCU_GPIOD, GPIOD, GPIO_PIN_5, \
 *    RCU_GPIOD, GPIOD, GPIO_PIN_6, \
 *    RCU_USART1, USART1, USART1_IRQn, \
 *    &usart1_dma_tx, &usart1_dma_rx);
*/
#endif /* __GD32F470__ */


#ifdef __MSPM0G3507__
#define DEFINE_RS485_DEVICE(name_var, init_var, send_var, receive_var, set_dir_var, \
    tx_buf_var, rx_buf_var, \
    de_port_var, de_pin_var, \
    usart_periph_var, \
    dma_tx_var, dma_rx_var) \
    uart_485_t name_var = { \
        .base = { .name = #name_var, \
            .tx_buf = tx_buf_var, .tx_buf_size = sizeof(tx_buf_var), .tx_len = 0, .tx_index = 0, .send_busy = false, \
            .rx_buf = rx_buf_var, .rx_buf_size = sizeof(rx_buf_var), .rx_len = 0, .rx_index = 0, .rx_complete = false, \
            .init = init_var, .send = send_var, .receive = receive_var, \
            .dma_tx = dma_tx_var, .dma_rx = dma_rx_var, \
            .usart_periph = (uint32_t)usart_periph_var, .isr_handler = NULL, .next = NULL }, \
        .set_dir = set_dir_var, \
        .hw = { \
            .de_pin = { .port = de_port_var, .pin = de_pin_var }, \
            .usart_periph = usart_periph_var \
        } \
    }

/* --- MSPM0 API --- */
void rs485_init_msp(void *self);
void rs485_send_msp(void *self, const uint8_t *data, uint8_t len);
void rs485_receive_msp(void *self);
void set_dir_msp(void *self, uart_state_t de_var);
inline void uart_irq_handler_rs485_msp(void *self, DL_UART_IIDX pending_stat);
inline void uart_receive_handle_rs485_msp(void *self, DL_UART_IIDX pending_stat);
/*
 * MSPM0 RS-485 实例化与使用说明：
 *
 * DEFINE_RS485_DEVICE(rs485_msp,
 *     rs485_init_msp, rs485_send_msp, rs485_receive_msp, set_dir_msp,
 *     rs485_buf_send_test, rs485_buf_receive_test,
 *     GPIOA, DL_GPIO_PIN_8,
 *     UART0,
 *     NULL, NULL);
 *
 * void UART0_IRQHandler(void)
 * {
 *     uart_irq_handler_rs485_msp(&rs485_msp);
 *     uart_receive_handle_rs485_msp(&rs485_msp);
 * }
 */
#endif /* __MSPM0G3507__ */




///////////////
/* rs232部分 */
//////////////

/* --- 提供数据容器 --- */
static uint8_t rs232_buf_send_test[128];
static uint8_t rs232_buf_receive_test[128];

/* --- 类型定义与结构体 --- */

/* 构造 RS-232 硬件结构 */
typedef struct {
    #ifdef __GD32F470__
    uart_gpio_config_t tx_pin;
    uart_gpio_config_t rx_pin;
    uint32_t rcu_usart;
    uint32_t usart_periph;
    uint32_t usart_nvic;
    #endif /* __GD32F470__ */

    #ifdef __MSPM0G3507__
    UART_Regs *usart_periph;
    #endif /* __MSPM0G3507__ */
} rs232_hw_config_t;

/* 派生子类 RS-232 */
typedef struct {
    uart_base_t base;
    rs232_hw_config_t hw;
} uart_232_t;


/* 实例化宏 RS-232 */

#ifdef __GD32F470__
#define DEFINE_RS232_DEVICE(name_var, init_var, send_var, receive_var, \
    tx_buf_var, rx_buf_var, \
    tx_rcu_gpio_var, tx_port_var, tx_pin_var, \
    rx_rcu_gpio_var, rx_port_var, rx_pin_var, \
    rcu_usart_var, usart_periph_var, usart_nvic_var, \
    dma_tx_var, dma_rx_var) \
    uart_232_t name_var = { \
        .base = { .name = #name_var, \
            .tx_buf = tx_buf_var, .tx_buf_size = sizeof(tx_buf_var), .tx_len = 0, .tx_index = 0, .send_busy = false, \
            .rx_buf = rx_buf_var, .rx_buf_size = sizeof(rx_buf_var), .rx_len = 0, .rx_index = 0, .rx_complete = false, \
            .init = init_var, .send = send_var, .receive = receive_var, \
            .dma_tx = dma_tx_var, .dma_rx = dma_rx_var, \
            .usart_periph = usart_periph_var, .isr_handler = NULL, .next = NULL }, \
        .hw = { \
                .tx_pin = { .rcu_gpio = tx_rcu_gpio_var, .port = tx_port_var, .pin = tx_pin_var}, \
                .rx_pin = { .rcu_gpio = rx_rcu_gpio_var, .port = rx_port_var, .pin = rx_pin_var}, \
                .rcu_usart = rcu_usart_var, \
                .usart_periph = usart_periph_var, \
                .usart_nvic = usart_nvic_var \
        } \
    }

/* --- GD32F4 API --- */

void rs232_init_gd32(void *self);
void rs232_send_gd32(void *self, const uint8_t *data, uint8_t len);
void rs232_receive_gd32(void *self);
inline void uart_irq_handler_rs232_gd32(void *self);
inline void uart_receive_handle_rs232_gd32(void *self);


/*
 *  
 * 实例化gd32的rs232的使用方法
 * 如果有需要就在需要的地方 extern uart_232_t rs232_gd32;
 *  
 *  DEFINE_RS232_DEVICE(rs232_gd32, rs232_init_gd32, rs232_send_gd32, rs232_receive_gd32, \
 *  rs232_buf_send_test, rs232_buf_receive_test, \
 *  RCU_GPIOD, GPIOD, GPIO_PIN_5, \
 *  RCU_GPIOD, GPIOD, GPIO_PIN_6, \
 *  RCU_USART1, USART1, USART1_IRQn, \
 *  NULL, NULL);
 *  
 *  void USART1_IRQHandler(void)
 *  {
 *      uart_irq_handler_rs232_gd32(&rs232_gd32);
 *      uart_receive_handle_rs232_gd32(&rs232_gd32);
 *  }
 *  
 *  发送的简单测试
 *  rs232_gd32.base.init(&rs232_gd32);
 *  uint8_t test_data[] = "Hello John Fox\n";
 *  while(1) rs232_gd32.base.send(&rs232_gd32, test_data, sizeof(test_data) - 1);
 *  
 *  fputc绑定实例化
 *  rs232_gd32.base.init(&rs232_gd32);
 *  uart_set_console(&rs232_gd32.base);
 *  printf("are u ok?\n");
 *  
 *  接收的简单测试
 *  rs232_gd32.base.init(&rs232_gd32);
 *  while(1) rs232_gd32.base.receive(&rs232_gd32);
 *  
 *  如果需要，可提取核心逻辑，放到自己的逻辑
 *  
 *      if (rs232_gd32.base.rx_complete) {
 *      // 1. 业务逻辑
 *  	uint16_t len = rs232_gd32.base.rx_len;
 *      uint8_t *data = rs232_gd32.base.rx_buf;
 *      rs232_gd32.base.send(&rs232_gd32, data, len);
 *  
 *      // 2. 自动重置接收标志位和长度，为下一次接收做准备
 *      rs232_gd32.base.rx_complete = false;
 *      rs232_gd32.base.rx_len = 0;
 *      rs232_gd32.base.rx_index = 0;
 *      }
 *  
 *  DMA模式的支持
 *  DEFINE_DMA_CHANNEL_DEVICE(usart1_dma_tx, dma_channel_init_gd32, dma_channel_start_tx_gd32, \
 *      dma_channel_get_remain_num_gd32, NULL, \
 *      RCU_DMA0, DMA0, DMA_CH6, \
 *      DMA_SUBPERI4, (uint32_t)&USART_DATA(USART1), \
 *      DMA_MEMORY_TO_PERIPH, DMA_PRIORITY_ULTRA_HIGH);
 *  
 *  DEFINE_DMA_CHANNEL_DEVICE(usart1_dma_rx, dma_channel_init_gd32, NULL, \
 *      dma_channel_get_remain_num_gd32, dma_channel_reload_rx_gd32, \
 *      RCU_DMA0, DMA0, DMA_CH5, \
 *      DMA_SUBPERI4, (uint32_t)&USART_DATA(USART1), \
 *      DMA_PERIPH_TO_MEMORY, DMA_PRIORITY_ULTRA_HIGH);
 *  
 *  DEFINE_RS232_DEVICE(rs232_gd32, rs232_init_gd32, rs232_send_gd32, rs232_receive_gd32, \
 *    rs232_buf_send_test, rs232_buf_receive_test, \
 *    RCU_GPIOD, GPIOD, GPIO_PIN_5, \
 *    RCU_GPIOD, GPIOD, GPIO_PIN_6, \
 *    RCU_USART1, USART1, USART1_IRQn, \
 *    &usart1_dma_tx, &usart1_dma_rx);
 *  
*/
#endif /* __GD32F470__ */


#ifdef __MSPM0G3507__
/* 实例化宏 RS-232 (MSPM0) */
#define DEFINE_RS232_DEVICE(name_var, init_var, send_var, receive_var, \
    tx_buf_var, rx_buf_var, \
    usart_periph_var, \
    dma_tx_var, dma_rx_var) \
    uart_232_t name_var = { \
        .base = { .name = #name_var, \
            .tx_buf = tx_buf_var, .tx_buf_size = sizeof(tx_buf_var), .tx_len = 0, .tx_index = 0, .send_busy = false, \
            .rx_buf = rx_buf_var, .rx_buf_size = sizeof(rx_buf_var), .rx_len = 0, .rx_index = 0, .rx_complete = false, \
            .init = init_var, .send = send_var, .receive = receive_var, \
            .dma_tx = dma_tx_var, .dma_rx = dma_rx_var, \
            .usart_periph = (uint32_t)usart_periph_var, .isr_handler = NULL, .next = NULL }, \
        .hw = { \
            .usart_periph = usart_periph_var \
        } \
    }

/* --- MSPM0 API --- */
void rs232_init_msp(void *self);
void rs232_send_msp(void *self, const uint8_t *data, uint8_t len);
void rs232_receive_msp(void *self);
inline void uart_irq_handler_rs232_msp(void *self, DL_UART_IIDX pending_stat);
inline void uart_receive_handle_rs232_msp(void *self, DL_UART_IIDX pending_stat);

/*
 * Syscfg配置步骤:
 *   TIMER:
 *     1. 添加一个TIMER.
 *     2. 命名为 "TIMER_0".
 *     3. 设置 "Timer Clock Divider" 为 "Divided by 8". 
 *     4. 设置 "Timer Clock Prescaler " 为 "100". 
 *     5. 设置 "Timer Mode" 为 "Periodic Down Counting". 
 *     6. 设置 "Actual Timer Period" 为 "1.00 ms". 
 *     7. 勾选 "Start Timer".
 *     8. 设置 "Enable Interrupts" 为 "Zero event". 
 *     9. 设置 "Interrupt Priority" 为 "Default". 
 *     UART:
 *     1. 添加一个UART.
 *     2. UART命名为 "UART_0".
 *     3. 看向 "Basic Configuration" 模块. 
 *     3. 设置 "Clock Source" 设置 "BUSCLK". 
 *     5. 设置 "Target Baud Rate" 为 "115200". 
 *     5. 设置 "Word Length" 为 "8 bits". 
 *     5. 设置 "Parity" 为 "None". 
 *     5. 设置 "Stop Bits" 为 "One".
 *     5. 设置 "HW Flow Control" 为 "Disable HW flow control". 
 *     3. 看向 "Advanced Configuration" 模块. 
 *     5. 设置 "UART Mode" 为 "Normal UART Mode". 
 *     5. 设置 "Communication Direction" 为 "TX and RX". 
 *     5. 设置 "Oversampling" 为 "16x". 
 *     5. 设置 "RX Timeout Interrupt Counts" 为 "15". 
 *     3. 看向 "Interrupt Configuration" 模块. 
 *     5. 设置 "Enable Interrupts" 为 "Transmit" + "Receive" + "RX timeout". 
 *     
 * 实例化mspm0的rs232的使用方法
 * 如果有需要就在需要的地方 extern uart_232_t rs232_gd32;
 *  
 * DEFINE_RS232_DEVICE(
 *     rs232_dev, 
 *     rs232_init_msp, 
 *     rs232_send_msp, 
 *     rs232_receive_msp,
 *     rs232_buf_send_test, 
 *     rs232_buf_receive_test,
 *     UART0,         // SysConfig 生成的 UART 硬件句柄
 *     NULL, NULL     // 不使用 DMA 时填 NULL
 * );
 *  
 * void UART0_IRQHandler(void)
 * {
 *     uart_vector_dispatch((uint32_t)UART0);
 * }
 *  
 *  发送的简单测试
 *  rs232_dev.base.init(&rs232_dev);
 *  uint8_t test_data[] = "Hello John Fox\n";
 *  while(1) rs232_dev.base.send(&rs232_dev, test_data, sizeof(test_data) - 1);
 *  
 *  fputc绑定实例化
 *  rs232_dev.base.init(&rs232_dev);
 *  uart_set_console(&rs232_dev.base);
 *  printf("are u ok?\n");
 *  
 *  接收的简单测试
 *  rs232_dev.base.init(&rs232_dev);
 *  while(1) rs232_dev.base.receive(&rs232_dev);
 *  
 *  如果需要，可提取核心逻辑，放到自己的逻辑
 *  
 *      if (rs232_dev.base.rx_complete) {
 *      // 1. 业务逻辑
 *  	uint16_t len = rs232_dev.base.rx_len;
 *      uint8_t *data = rs232_dev.base.rx_buf;
 *      rs232_dev.base.send(&rs232_dev, data, len);
 *  
 *      // 2. 自动重置接收标志位和长度，为下一次接收做准备
 *      rs232_dev.base.rx_complete = false;
 *      rs232_dev.base.rx_len = 0;
 *      rs232_dev.base.rx_index = 0;
 *      }
 *  
 *  DMA模式的支持
*/


#endif /* __MSPM0G3507__ */



/* 通用 API */
void uart_vector_dispatch(uint32_t usart_periph); // 中央分发总线
void uart_set_console(uart_base_t *dev);

#endif /* __HARDWARE_UART_ALL_UART_ALL_H__ */
