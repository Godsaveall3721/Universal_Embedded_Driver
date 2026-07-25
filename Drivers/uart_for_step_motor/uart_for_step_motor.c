#include "uart_for_step_motor.h"

static uint8_t rs232_tx_buf_for_step_motor[128];
static uint8_t rs232_rx_buf_for_step_motor[128];

DEFINE_RS232_DEVICE(
    uart_for_step_motor_dev, 
    rs232_init_msp, 
    rs232_send_msp, 
    rs232_receive_msp,
    rs232_tx_buf_for_step_motor, 
    rs232_rx_buf_for_step_motor,
    UART0,         /* SysConfig 生成的 UART 硬件句柄 */
    NULL, NULL     /* 不使用 DMA 时填 NULL */
);

void uart_for_step_motor_init(void)
{
    uart_for_step_motor_dev.base.init(&uart_for_step_motor_dev);
    uart_set_console(&uart_for_step_motor_dev.base);
    NVIC_ClearPendingIRQ(UART0_INT_IRQn);
    NVIC_EnableIRQ(UART0_INT_IRQn);
}


/**
 * @brief 初始化步进电机对象
 */
void stepper_init(stepper_motor_t *motor, void *uart_dev, uint8_t addr)
{
    motor->bus = (uart_base_t *)uart_dev;
    motor->addr = addr;
}

/**
 * @brief 电机使能控制 (0xF3)
 */
void stepper_set_enable(stepper_motor_t *motor, bool enable, bool sync)
{
    uint8_t cmd[6];
    cmd[0] = motor->addr;
    cmd[1] = 0xF3;
    cmd[2] = 0xAB;
    cmd[3] = enable ? 0x01 : 0x00;
    cmd[4] = sync ? 0x01 : 0x00;
    cmd[5] = STEPPER_CHECK_BYTE;

    motor->bus->send(motor->bus, cmd, sizeof(cmd));
}

/**
 * @brief 速度模式控制 (0xF6)
 */
void stepper_control_speed(stepper_motor_t *motor, stepper_dir_t dir, uint16_t rpm, uint8_t acc, bool sync)
{
    uint8_t cmd[8];
    cmd[0] = motor->addr;
    cmd[1] = 0xF6;
    cmd[2] = (uint8_t)dir;
    cmd[3] = (uint8_t)(rpm >> 8);   // RPM 高 8 位
    cmd[4] = (uint8_t)(rpm & 0xFF); // RPM 低 8 位
    cmd[5] = acc;
    cmd[6] = sync ? 0x01 : 0x00;
    cmd[7] = STEPPER_CHECK_BYTE;

    motor->bus->send(motor->bus, cmd, sizeof(cmd));
}

/**
 * @brief 位置模式控制 (0xFD)
 */
void stepper_control_pos(stepper_motor_t *motor, stepper_dir_t dir, uint16_t rpm, uint8_t acc, uint32_t pulses, stepper_pos_mode_t mode, bool sync)
{
    uint8_t cmd[13];
    cmd[0] = motor->addr;
    cmd[1] = 0xFD;
    cmd[2] = (uint8_t)dir;
    cmd[3] = (uint8_t)(rpm >> 8);
    cmd[4] = (uint8_t)(rpm & 0xFF);
    cmd[5] = acc;
    cmd[6] = (uint8_t)(pulses >> 24); // 脉冲数 32-bit 大端序
    cmd[7] = (uint8_t)(pulses >> 16);
    cmd[8] = (uint8_t)(pulses >> 8);
    cmd[9] = (uint8_t)(pulses & 0xFF);
    cmd[10] = (uint8_t)mode;
    cmd[11] = sync ? 0x01 : 0x00;
    cmd[12] = STEPPER_CHECK_BYTE;

    motor->bus->send(motor->bus, cmd, sizeof(cmd));
}

/**
 * @brief 立即停止 (0xFE)
 */
void stepper_stop_immediately(stepper_motor_t *motor, bool sync)
{
    uint8_t cmd[5] = {
        motor->addr, 
        0xFE, 
        0x98, 
        (uint8_t)(sync ? 0x01 : 0x00), 
        STEPPER_CHECK_BYTE
    };
    motor->bus->send(motor->bus, cmd, sizeof(cmd));
}

/**
 * @brief 多机同步运动触发 (0xFF)
 */
void stepper_sync_motion(stepper_motor_t *motor)
{
    uint8_t cmd[4] = {motor->addr, 0xFF, 0x66, STEPPER_CHECK_BYTE};
    motor->bus->send(motor->bus, cmd, sizeof(cmd));
}

/**
 * @brief 位置角度清零 (0x0A)
 */
void stepper_zero_position(stepper_motor_t *motor)
{
    uint8_t cmd[4] = {motor->addr, 0x0A, 0x6D, STEPPER_CHECK_BYTE};
    motor->bus->send(motor->bus, cmd, sizeof(cmd));
}

/**
 * @brief 解除堵转保护 (0x0E)
 */
void stepper_clear_stall_protection(stepper_motor_t *motor)
{
    uint8_t cmd[4] = {motor->addr, 0x0E, 0x52, STEPPER_CHECK_BYTE};
    motor->bus->send(motor->bus, cmd, sizeof(cmd));
}

/**
 * @brief 恢复出厂设置 (0x0F)
 */
void stepper_restore_factory(stepper_motor_t *motor)
{
    uint8_t cmd[4] = {motor->addr, 0x0F, 0x5F, STEPPER_CHECK_BYTE};
    motor->bus->send(motor->bus, cmd, sizeof(cmd));
}

/**
 * @brief 触发回零 (0x9A)
 * @param home_mode 0:单圈就近, 1:单圈方向, 2:多圈碰撞, 3:多圈限位开关
 */
void stepper_trigger_homing(stepper_motor_t *motor, uint8_t home_mode, bool sync)
{
    uint8_t cmd[5] = {
        motor->addr,
        0x9A,
        home_mode,
        (uint8_t)(sync ? 0x01 : 0x00),
        STEPPER_CHECK_BYTE
    };
    motor->bus->send(motor->bus, cmd, sizeof(cmd));
}