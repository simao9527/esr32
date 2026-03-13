/**
 * @file motor.h
 * @brief TB6612FNG电机驱动头文件
 * @details 提供基于TB6612FNG的双路电机驱动功能，支持PWM调速和方向控制
 */

#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/ledc.h"

// TB6612FNG电机驱动引脚定义
// 左电机引脚定义
#define MOTOR_L_AIN1_PIN 4    // 左电机控制引脚1
#define MOTOR_L_AIN2_PIN 5    // 左电机控制引脚2
#define MOTOR_L_PWMA_PIN 1    // 左电机PWM控制引脚

// 右电机引脚定义
#define MOTOR_L_BIN1_PIN 6    // 右电机控制引脚1
#define MOTOR_L_BIN2_PIN 7    // 右电机控制引脚2
#define MOTOR_L_PWMB_PIN 2    // 右电机PWM控制引脚

// TB6612FNG控制引脚
#define MOTOR_STBY_PIN 8     // 待机控制引脚

// PWM频率定义
#define MOTOR_PWM_FREQ_HZ 10000  // 电机PWM频率10kHz
#define MOTOR_PWM_RESOLUTION 10  // PWM分辨率10位(0-1023)

// 电机方向定义
typedef enum {
    MOTOR_DIR_FORWARD = 0,  // 前进
    MOTOR_DIR_BACKWARD = 1, // 后退
    MOTOR_DIR_STOP = 2      // 停止
} MotorDirection;

// 电机控制结构体
typedef struct {
    int8_t left_speed;      // 左电机速度(-100到100，负值表示后退)
    int8_t right_speed;     // 右电机速度(-100到100，负值表示后退)
} MotorSpeed;

// 电机配置结构体
typedef struct {
    gpio_num_t pin1;         // 电机控制引脚1
    gpio_num_t pin2;         // 电机控制引脚2
    ledc_channel_t channel;  // PWM通道
} MotorConfig;

/**
 * @brief 电机初始化函数
 * @details 初始化电机控制引脚和PWM输出
 */
void Motor_Init(void);

/**
 * @brief 设置左电机速度和方向
 * @details 设置左电机的速度和方向
 * 
 * @param speed 速度值(-100到100，负值表示后退)
 */
void Motor_SetLeftSpeed(int8_t speed);

/**
 * @brief 设置右电机速度和方向
 * @details 设置右电机的速度和方向
 * 
 * @param speed 速度值(-100到100，负值表示后退)
 */
void Motor_SetRightSpeed(int8_t speed);

/**
 * @brief 设置双电机速度和方向
 * @details 同时设置左右电机的速度和方向
 * 
 * @param speed 电机速度结构体
 */
void Motor_SetSpeed(const MotorSpeed *speed);

/**
 * @brief 控制左电机方向和速度
 * @details 控制左电机的方向和速度
 * 
 * @param direction 电机方向
 * @param speed 速度值(0到100)
 */
void Motor_ControlLeft(MotorDirection direction, uint8_t speed);

/**
 * @brief 控制右电机方向和速度
 * @details 控制右电机的方向和速度
 * 
 * @param direction 电机方向
 * @param speed 速度值(0到100)
 */
void Motor_ControlRight(MotorDirection direction, uint8_t speed);

/**
 * @brief 停止所有电机
 * @details 停止左右电机
 */
void Motor_Stop(void);

/**
 * @brief 使能电机驱动
 * @details 使能TB6612FNG电机驱动芯片
 */
void Motor_Enable(void);

/**
 * @brief 禁用电机驱动
 * @details 禁用TB6612FNG电机驱动芯片
 */
void Motor_Disable(void);

#endif // MOTOR_H
