/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: key.h
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: CIMC_2026_GD32F470
 * Version: 1.00 (2026/6/3) - Original
 */

#ifndef __HARDWARE_KEY_KEY_H__
#define __HARDWARE_KEY_KEY_H__

#ifdef __GD32F470__
#include "HeaderFiles.h"
#include <stdbool.h>
#endif /* __GD32F470__ */

#ifdef __MSPM0G3507__
#include "ti_msp_dl_config.h"
#endif /* __MSPM0G3507__ */


typedef struct {
#ifdef __GD32F470__
    uint32_t rcu_gpio;
    uint32_t port;
    uint32_t pin;

    uint32_t exti_line;             // EXTI_2
    uint32_t exti_irqn;             // EXTI2_IRQn
    uint8_t  exti_port_src;         // EXTI_SOURCE_GPIOE
    uint8_t  exti_pin_src;          // EXTI_SOURCE_PIN2
    uint8_t nvic_irq_pre_priority;  // 2
    uint8_t nvic_irq_sub_priority;  // 0
#endif /* __GD32F470__ */

#ifdef __MSPM0G3507__
    GPIO_Regs     *port;        /* 对应 GPIOA 或 GPIOB */
    uint32_t      pin;         /* 对应 DL_GPIO_PIN_X */
    uint32_t      pin_iidx;    /* 核心：对应中断向量值 DL_GPIO_IIDX_DIOX */
    IRQn_Type     irqn;        /* 对应 GPIOA_INT_IRQn */
#endif 

} key_gpio_config_t;

typedef struct key_dev {
    const char *name;

    key_gpio_config_t hw;
    
    void (*init)(struct key_dev *dev);
    void (*on_pressed_cb)(void *self);
    bool (*read)(struct key_dev *dev); // 轮询才需要read读取

    volatile bool is_triggered;     // 中断触发标记
    uint16_t debounce_cnt;          // 非阻塞消抖计数器
    struct key_dev *next;           // 区块链链表
} key_dev_t;

#ifdef __GD32F470__
#define DEFINE_KEY_DEVICE(name_var, init_var, read_var, cb_var, \
    rcu_val, port_val, pin_val, exti_line_val, exti_irqn_val, exti_port_src_val, exti_pin_src_val, \
    nvic_irq_pre_priority_val, nvic_irq_sub_priority_val) \
    key_dev_t name_var = { \
        .name = #name_var, .init = init_var, .read = read_var, .on_pressed_cb = cb_var, \
        .hw = { .rcu_gpio = rcu_val, .port = port_val, .pin = pin_val, \
                .exti_line = exti_line_val, .exti_irqn = exti_irqn_val, \
                .exti_port_src = exti_port_src_val, .exti_pin_src = exti_pin_src_val, \
                .nvic_irq_pre_priority = nvic_irq_pre_priority_val, .nvic_irq_sub_priority = nvic_irq_sub_priority_val \
        }, \
        .is_triggered = false, .debounce_cnt = 0, \
        .next = NULL \
    }

/* --- GD32 API --- */
void key_init_gd32(key_dev_t *self);
inline bool key_read_gd32(key_dev_t *self);
void key_init_exti_gd32(key_dev_t *self);
void key_exti_dispatch_gd32(uint32_t exti_line);

/*
 * 实例化gd32的key的使用方法，请在 gd32f4_driver/Function/Function.c 里操作    
 * 编写与底层完全解耦的业务层回调逻辑:
 * void key1_press_cb(void *self) 
 * { 
 * LED1.toggle(&LED1); 
 * }
 *  
 * 外部中断模式（EXTI）的实例化方法:
 * DEFINE_KEY_DEVICE(KEY1, key_init_exti_gd32, NULL, key1_press_cb, \
 * RCU_GPIOE, GPIOE, GPIO_PIN_2, EXTI_2, EXTI2_IRQn, EXTI_SOURCE_GPIOE, EXTI_SOURCE_PIN2, 2, 0);
 *
 * 普通轮询模式的实例化方法:
 * DEFINE_KEY_DEVICE(KEY2, key_init_gd32, key_read_gd32, NULL, \
 * RCU_GPIOE, GPIOE, GPIO_PIN_4, 0, 0, 0, 0, 0, 0);
 *  
 * 硬件向量中断服务函数绑定（在 exti_vector.c 中）：
 * NEST_EXTI_IRQ_HANDLER_FACTORY(2)
 *  
 * 业务层应用初始化与高频消费：
 * void UsrFunction(void)
 * {
 * KEY1.init(&KEY1); // 自动挂入中央区块链链表，开辟异步状态机
 *  
 * KEY2.init(&KEY2); // 轮询模式配置
 * * while(1)
 * {
 *  
 * key_process_loop(); // 无 delay 的外部中断异步消抖状态机高频轮询
 *  
 *  
 * if (KEY2.read(&KEY2)) { // 轮询类型设备的同步消费
 * // 轮询按键处理逻辑
 *          }
 *      }
 * }
 *  
 *  
 *  最后特别注意不能反复构造EXTI_X，就算是GPIOX不同也不能共用一个EXTI_X！
 */ 
#endif /* __GD32F470__ */


#ifdef __MSPM0G3507__

#define DEFINE_KEY_DEVICE_MSP(name_var, init_var, read_var, cb_var, port_val, pin_val, pin_iidx_val, irqn_val) \
    key_dev_t name_var = { \
        .name = #name_var, .init = init_var, .read = read_var, .on_pressed_cb = cb_var, \
        .hw = { .port = port_val, .pin = pin_val, .pin_iidx = pin_iidx_val, .irqn = irqn_val }, \
        .is_triggered = false, .debounce_cnt = 0, \
        .next = NULL \
    }

/* --- MSPM0 API --- */
void key_init_msp(key_dev_t *self);
void key_init_exti_msp(key_dev_t *self);
bool key_read_msp(key_dev_t *self);
void key_exti_dispatch_msp(GPIO_Regs *port, uint32_t pin_iidx);

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
 *   GPIO(中断/状态机):
 *     1. 添加一个GPIO.
 *     2. group命名为 "GPIO_KEY".
 *     3. 添加一个PIN,命名为PIN_KEY_1.
 *     5. 设置 "Direction" 为 "Input". 
 *     6. 设置 "Digital IOMUX Features" 里的 "Internal Resistor" 为 "Pull-Up Resistor"
 *     7. 设置 "Interrupts/Events" 勾选 "Enable Interrupts" 并设置 "Trigger Polarity" 为 "Trigger on Falling Edge”
 *   GPIO(阻塞轮询):
 *     1. 添加一个GPIO.
 *     2. group命名为 "GPIO_KEY".
 *     3. 添加一个PIN,命名为PIN_KEY_2.
 *     5. 设置 "Direction" 为 "Input". 
 *     6. 设置 "Digital IOMUX Features" 里的 "Internal Resistor" 为 "Pull-Up Resistor"
 *     
  *  编写与底层完全解耦的业务层回调逻辑:
  *   void key_press_cb(void *self) {
  *   { 
  *   DL_GPIO_togglePins(GPIO_test_PORT, GPIO_test_PIN_test_PIN);
  *   }
  *  
  *  外部中断模式（EXTI）的实例化方法:
  *  DEFINE_KEY_DEVICE_MSP(KEY1, key_init_exti_msp, NULL, key_press_cb, 
  *                      GPIO_KEY_PORT, GPIO_KEY_PIN_KEY_1_PIN, GPIO_KEY_PIN_KEY_1_IIDX, GPIOA_INT_IRQn);
  * 
  * 普通轮询模式的实例化方法:
  *  DEFINE_KEY_DEVICE_MSP(KEY2, key_init_msp,   key_read_msp,   NULL,
  *                      GPIO_KEY_PORT, GPIO_KEY_PIN_KEY_2_PIN, GPIO_KEY_PIN_KEY_2_IIDX, GPIOA_INT_IRQn);
  * 
  *  硬件向量中断服务函数绑定（在 exti_vector.c 中）：
  *  CONSUME_GPIO_GROUP_INT(A);
  *
  *  业务层应用初始化与高频消费：
  *  void main(void)
  *  {
  *  KEY1.init(&KEY1); // 自动挂入中央区块链链表，开辟异步状态机
  *
  *  KEY2.init(&KEY2); // 轮询模式配置
  *  while(1)
  *  {
  *   
  *   key_process_loop(); // 无 delay 的外部中断异步消抖状态机高频轮询
  *
  * 
  *   if (KEY2.read(&KEY2)) { // 轮询类型设备的同步消费
  * 		  // 轮询按键处理逻辑
  *            }
  *      }
  * }
*/
#endif /* __MSPM0G3507__ */


/* --- 公共 API --- */
void key_process_loop(void);

#endif /* __HARDWARE_KEY_KEY_H__ */