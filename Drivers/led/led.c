/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: led.c
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: GD32F470 / MSPM0G3507
 * Version: 1.01 (2026/7/20) - Original
 */

#include "led.h"

#ifdef __GD32F470__

void led_init_gd32(led_dev_t *self)
{
    led_dev_t *dev = (led_dev_t *)self;
    rcu_periph_clock_enable((rcu_periph_enum)dev->hw.rcu_gpio);
	gpio_mode_set(dev->hw.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, dev->hw.pin);
    gpio_output_options_set(dev->hw.port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, dev->hw.pin);
	gpio_bit_reset(dev->hw.port, dev->hw.pin);

    dev->active ? dev->on(self) : dev->off(self);
}

void led_on_gd32(led_dev_t *self)
{
    led_dev_t *dev = (led_dev_t *)self;
    gpio_bit_set(dev->hw.port, dev->hw.pin);
    dev->active = LED_ON;
}

void led_off_gd32(led_dev_t *self)
{
    led_dev_t *dev = (led_dev_t *)self;
    gpio_bit_reset(dev->hw.port, dev->hw.pin);
    dev->active = LED_OFF;
}

void led_toggle_gd32(led_dev_t *self) {
    led_dev_t *dev = (led_dev_t *)self;
    dev->active ? dev->off(self) : dev->on(self);
}

#endif /* __GD32F470__ */


#ifdef __MSPM0G3507__

void led_init_msp(led_dev_t *self)
{
    led_dev_t *dev = (led_dev_t *)self;
    /* 物理 GPIO 初始化已由 SYSCFG_DL_init() 自动装载 */
    /* 根据硬件初始期望状态设置引脚 */
    dev->active ? dev->on(self) : dev->off(self);
}

void led_on_msp(led_dev_t *self)
{
    led_dev_t *dev = (led_dev_t *)self;
    /* 高电平点亮 LED */
    DL_GPIO_setPins(dev->hw.port, dev->hw.pin);
    dev->active = LED_ON;
}

void led_off_msp(led_dev_t *self)
{
    led_dev_t *dev = (led_dev_t *)self;
    /* 低电平熄灭 LED */
    DL_GPIO_clearPins(dev->hw.port, dev->hw.pin);
    dev->active = LED_OFF;
}

void led_toggle_msp(led_dev_t *self)
{
    led_dev_t *dev = (led_dev_t *)self;
    DL_GPIO_togglePins(dev->hw.port, dev->hw.pin);
    dev->active = (dev->active == LED_ON) ? LED_OFF : LED_ON;
}

#endif /* __MSPM0G3507__ */