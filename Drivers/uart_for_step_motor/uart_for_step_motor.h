
#ifndef __UART_FOR_STEP_MPTOR_H
#define __UART_FOR_STEP_MPTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "uart_all.h"


extern uart_232_t uart_for_step_motor_dev;


/* 校验字节默认定义 */
#define STEPPER_CHECK_BYTE    0x6B

/* 方向枚举 */
typedef enum {
    STEPPER_DIR_CW  = 0x00, // 顺时针
    STEPPER_DIR_CCW = 0x01  // 逆时针
} stepper_dir_t;

/* 位置模式类型 */
typedef enum {
    STEPPER_POS_REL = 0x00, // 相对位置
    STEPPER_POS_ABS = 0x01  // 绝对位置
} stepper_pos_mode_t;

/* 电机句柄结构体 */
typedef struct {
    uart_base_t *bus;       // 绑定的总线 (支持 RS232 / RS485)
    uint8_t addr;           // 电机地址
} stepper_motor_t;

/* API 接口声明 */
void uart_for_step_motor_init(void);
void stepper_init(stepper_motor_t *motor, void *uart_dev, uint8_t addr);

/* 控制指令 */
void stepper_set_enable(stepper_motor_t *motor, bool enable, bool sync);
void stepper_control_speed(stepper_motor_t *motor, stepper_dir_t dir, uint16_t rpm, uint8_t acc, bool sync);
void stepper_control_pos(stepper_motor_t *motor, stepper_dir_t dir, uint16_t rpm, uint8_t acc, uint32_t pulses, stepper_pos_mode_t mode, bool sync);
void stepper_stop_immediately(stepper_motor_t *motor, bool sync);
void stepper_sync_motion(stepper_motor_t *motor);

/* 触发与状态指令 */
void stepper_zero_position(stepper_motor_t *motor);
void stepper_clear_stall_protection(stepper_motor_t *motor);
void stepper_restore_factory(stepper_motor_t *motor);
void stepper_trigger_homing(stepper_motor_t *motor, uint8_t home_mode, bool sync);


#endif