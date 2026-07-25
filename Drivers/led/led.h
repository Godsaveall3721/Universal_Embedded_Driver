/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 John Fox. All rights reserved.
 *
 * File: led.h
 * Author: John Fox <wmoyanren@gmail.com>
 * Platform: CIMC_2026_GD32F470
 * Version: 1.01 (2026/7/20) - Original
 */

#ifndef __HARDWARE_LED_LED_H__
#define __HARDWARE_LED_LED_H__

#ifdef __GD32F470__
#include "HeaderFiles.h"
#endif /* __GD32F470__ */

#ifdef __MSPM0G3507__
#include "ti_msp_dl_config.h"
#endif /* __MSPM0G3507__ */

typedef enum {
    LED_OFF = 0, 
    LED_ON = !LED_OFF
} led_state_t;

typedef struct {
#ifdef __GD32F470__
    uint32_t rcu_gpio;
    uint32_t port;
    uint32_t pin;
#endif /* __GD32F470__ */

#ifdef __MSPM0G3507__
    GPIO_Regs *port;    // GPIO_laser_PORT (GPIOA)
    uint32_t   pin;     // GPIO_laser_PIN_laser_PIN (DL_GPIO_PIN_23)
#endif /* __MSPM0G3507__ */
} led_gpio_config_t;

typedef struct led_dev {
    const char *name;

    led_gpio_config_t hw;

    led_state_t active;

    void (*init)(struct led_dev *dev);
    void (*on)(struct led_dev *dev);
    void (*off)(struct led_dev *dev);
    void (*toggle)(struct led_dev *dev);
} led_dev_t;

#ifdef __GD32F470__

#define DEFINE_LED_DEVICE(name_var, init_var, on_var, off_var, toggle_var, \
    rcu_val, port_val, pin_val, active_val) \
    led_dev_t name_var = { \
        .name = #name_var, .init = init_var, .on = on_var, .off = off_var, .toggle = toggle_var, \
        .hw = { .rcu_gpio = rcu_val, .port = port_val, .pin = pin_val }, \
        .active = active_val \
    }

/* --- GD32 API --- */
void led_init_gd32(led_dev_t *self);
void led_on_gd32(led_dev_t *self);
void led_off_gd32(led_dev_t *self);
void led_toggle_gd32(led_dev_t *self);
/*
 * 
 * 实例化gd32的led的使用方法，请在 gd32f4_driver/Function/Function.c 里操作
 * 
 * DEFINE_LED_DEVICE(LED1, led_init_gd32, led_on_gd32, led_off_gd32, led_toggle_gd32, \
 *  RCU_GPIOA, GPIOA, GPIO_PIN_4, LED_ON);
 *  
 *  LED1.init(&LED1);
 *  LED1.on(&LED1);
 *  LED1.toggle(&LED1);
*/

#endif /* __GD32F470__ */

#ifdef __MSPM0G3507__
#define DEFINE_LED_DEVICE_MSP(name_var, port_val, pin_val, active_val) \
    led_dev_t name_var = { \
        .name = #name_var, \
        .init = led_init_msp, .on = led_on_msp, .off = led_off_msp, .toggle = led_toggle_msp, \
        .hw = { .port = port_val, .pin = pin_val }, \
        .active = active_val \
    }

/* --- MSPM0 API --- */
void led_init_msp(led_dev_t *self);
void led_on_msp(led_dev_t *self);
void led_off_msp(led_dev_t *self);
void led_toggle_msp(led_dev_t *self);
/*
 * 实例化 MSPM0 的 LED 使用方法：
 * 
 * DEFINE_LED_DEVICE_MSP(LED1, GPIO_LED_PORT, GPIO_LED_PIN_LED_1_PIN, LED_ON);
 * 
 * LED1.init(&LED1);
 * LED1.on(&LED1);
 * LED1.toggle(&LED1);
 */

#endif /* __MSPM0G3507__ */

#endif /* __HARDWARE_LED_LED_H__ */
