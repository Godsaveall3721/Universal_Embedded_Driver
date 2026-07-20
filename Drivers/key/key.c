/* 
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: key.c
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: GD32F470 / MSPM0G3507
 * Version: 1.00 (2026/6/3) - Original
 */

#include "key.h"

#ifdef __GD32F470__

/* 运行期动态对象注册表 */
static key_dev_t *key_head = NULL;

/* 普通轮询模式 */
void key_init_gd32(key_dev_t *self)
{
    key_dev_t *dev = (key_dev_t *)self;
    rcu_periph_clock_enable((rcu_periph_enum)dev->hw.rcu_gpio);
    gpio_mode_set(dev->hw.port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, dev->hw.pin);
}

/* 外部中断初始化 */
void key_init_exti_gd32(key_dev_t *self)
{
    key_dev_t *dev = (key_dev_t *)self;

    /* 头插法将当前实例挂入链表 */
    dev->next = key_head;
    key_head = dev;

    /* 基本 GPIO 配置 */
    rcu_periph_clock_enable((rcu_periph_enum)dev->hw.rcu_gpio);
    gpio_mode_set(dev->hw.port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, dev->hw.pin);

    /* EXTI 时钟与系统配置 */
    rcu_periph_clock_enable(RCU_SYSCFG);
    syscfg_exti_line_config(dev->hw.exti_port_src, dev->hw.exti_pin_src);

    /* 配置触发源 */
    exti_init((exti_line_enum)dev->hw.exti_line, EXTI_INTERRUPT, EXTI_TRIG_FALLING); //配置中断为下降沿触发
    exti_interrupt_flag_clear((exti_line_enum)dev->hw.exti_line);

    /* 使能 NVIC 中断 */
    nvic_irq_enable(dev->hw.exti_irqn, 2, 0);

    /* 清除中断标志位 */
    exti_interrupt_flag_clear((exti_line_enum)dev->hw.exti_line);
}

bool key_read_gd32(key_dev_t *self)
{
    key_dev_t *dev = (key_dev_t *)self;
    if (SET != gpio_input_bit_get(dev->hw.port, dev->hw.pin)) {
        delay_1ms(20);
        if (SET != gpio_input_bit_get(dev->hw.port, dev->hw.pin)) {
            while(SET != gpio_input_bit_get(dev->hw.port, dev->hw.pin));
            return true;
        }
    }
    return false;
}


/////////////////////////////
/* 以下是KEY中断状态机部分 */
/////////////////////////////

/* 非阻塞状态机 */
void key_process_loop(void)
{
    key_dev_t *curr = key_head;
    while (curr != NULL)
    {
        if (curr->is_triggered)
        {
            curr->debounce_cnt++;
            /* 灵敏调大，迟钝调小 */
            if (curr->debounce_cnt > 2500)
            {
                /* 状态机时间到，非阻塞审查引脚真实状态 */
                if (RESET == gpio_input_bit_get(curr->hw.port, curr->hw.pin))
                {
                    if (curr->on_pressed_cb != NULL) {
                        curr->on_pressed_cb(curr); // 开始消费
                    }
                }
                /* 消费完毕，状态重置 */
                curr->is_triggered = false;
                curr->debounce_cnt = 0;
                exti_interrupt_flag_clear((exti_line_enum)curr->hw.exti_line);
                nvic_irq_enable(curr->hw.exti_irqn, 2, 0);
            }
        }
        curr = curr->next;
    }
}

void key_exti_dispatch_gd32(uint32_t exti_line)
{
    key_dev_t *curr = key_head;
    while (curr != NULL) 
    {
        /* 动态匹配 */ 
        if (curr->hw.exti_line == exti_line) 
        {
            nvic_irq_disable(curr->hw.exti_irqn);
            curr->is_triggered = true;  // 丢给主循环去异步消费
            curr->debounce_cnt = 0;     // 重置消抖计数器
        }
        curr = curr->next; // 顺着链表继续往下找，支持同一条中断线挂载多个按键
    }
}

#endif /* __GD32F470__ */



#ifdef __MSPM0G3507__

/* 运行期动态对象注册表 */
static key_dev_t *key_head = NULL;

/* 轮询模式初始化 */
void key_init_msp(key_dev_t *self)
{
    key_dev_t *dev = (key_dev_t *)self;
    /* 物理初始化由自动生成的 SYSCFG_DL_init() 完成 */
    dev->is_triggered = false;
    dev->debounce_cnt = 0;
}

/* 外部中断模式的链表挂载 */
void key_init_exti_msp(key_dev_t *self)
{
    key_dev_t *dev = (key_dev_t *)self;
    
    /* 头插法挂入链表 */
    dev->next = key_head;
    key_head = dev;

    /* 物理初始化已经由自动生成的 SYSCFG_DL_init() 完成 */
    /* 只需要保证 CPU 的 NVIC 中断总开关打开即可 */
    DL_Interrupt_clearGroup(DL_INTERRUPT_GROUP_1, 0xFFFFFFFF);
    NVIC_EnableIRQ(dev->hw.irqn);
}

/* 轮询模式读取 */
bool key_read_msp(key_dev_t *self)
{
    key_dev_t *dev = (key_dev_t *)self;
    /* 假设低电平按下 */
    if ((DL_GPIO_readPins(dev->hw.port, dev->hw.pin) & dev->hw.pin) == 0) {
        delay_cycles(3200000); // 约20ms的软件消抖（基于主频）
        if ((DL_GPIO_readPins(dev->hw.port, dev->hw.pin) & dev->hw.pin) == 0) {
            while((DL_GPIO_readPins(dev->hw.port, dev->hw.pin) & dev->hw.pin) == 0);
            return true;
        }
    }
    return false;
}

//////////////////////////
/* 以下是KEY中断状态机部分 */
/////////////////////////

/* 异步状态机高频轮询 */
void key_process_loop(void)
{
    key_dev_t *curr = key_head;
    while (curr != NULL)
    {
        if (curr->is_triggered)
        {
            curr->debounce_cnt++;
            if (curr->debounce_cnt > 2500) // 根据主频和调用频率调整
            {
                /* 检查引脚是否仍为低电平（按下状态） */
                if ((DL_GPIO_readPins(curr->hw.port, curr->hw.pin) & curr->hw.pin) == 0)
                {
                    if (curr->on_pressed_cb != NULL) {
                        curr->on_pressed_cb(curr);
                    }
                }
                curr->is_triggered = false;
                curr->debounce_cnt = 0;
                /* 重新使能该组中断 */
                DL_GPIO_clearInterruptStatus(curr->hw.port, curr->hw.pin);
                NVIC_EnableIRQ(curr->hw.irqn);
            }
        }
        curr = curr->next;
    }
}

/* 中断分发器 */
void key_exti_dispatch_msp(GPIO_Regs *port, uint32_t pin_iidx)
{
    key_dev_t *curr = key_head;
    while (curr != NULL) 
    {
        /* 同时匹配 端口(PORTA/B) 和 引脚中断向量(IIDX) */
        if (curr->hw.port == port && curr->hw.pin_iidx == pin_iidx) 
        {
            NVIC_DisableIRQ(curr->hw.irqn); // 关中断，交给主循环消抖
            curr->is_triggered = true;  
            curr->debounce_cnt = 0;     
            break; // 找到了就跳出
        }
        curr = curr->next;
    }
}

#endif /* __MSP0G3507__ */