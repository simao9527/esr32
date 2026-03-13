/**
 * @file motor.c
 * @brief TB6612FNG电机驱动实现
 * @details 提供基于TB6612FNG的双路电机驱动功能，支持PWM调速和方向控制
 */

#include "motor.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MOTOR";

// PWM通道定义
#define MOTOR_PWM_CHANNEL_LEFT  LEDC_CHANNEL_0  // 左电机PWM通道
#define MOTOR_PWM_CHANNEL_RIGHT LEDC_CHANNEL_1  // 右电机PWM通道
#define MOTOR_PWM_SPEED_MODE    LEDC_LOW_SPEED_MODE  // PWM速度模式
#define MOTOR_PWM_TIMER         LEDC_TIMER_0   // PWM定时器

// 电机控制引脚状态
static bool motor_enabled = false;

// 保存当前电机速度
static int8_t left_motor_speed = 0;
static int8_t right_motor_speed = 0;

// 电机配置数组
static const MotorConfig motor_configs[2] = {
    {MOTOR_L_AIN1_PIN, MOTOR_L_AIN2_PIN, MOTOR_PWM_CHANNEL_LEFT},   // 左电机配置
    {MOTOR_L_BIN1_PIN, MOTOR_L_BIN2_PIN, MOTOR_PWM_CHANNEL_RIGHT}   // 右电机配置
};

/**
 * @brief 将速度值(-100到100)转换为PWM占空比(0到1023)
 * @details 将速度值转换为PWM占空比
 * 
 * @param speed 速度值(-100到100)
 * @return uint32_t PWM占空比(0到1023)
 */
static uint32_t Motor_SpeedToDuty(int8_t speed)
{
    // 限制速度范围在-100到100之间
    if (speed > 100) speed = 100;
    if (speed < -100) speed = -100;

    // 计算绝对值
    uint8_t abs_speed = (speed < 0) ? -speed : speed;

    // 将速度值转换为PWM占空比
    return (abs_speed * (1 << MOTOR_PWM_RESOLUTION)) / 100;
}

/**
 * @brief 设置电机控制引脚状态
 * @details 设置电机控制引脚状态，控制电机方向
 * 
 * @param pin1 控制引脚1
 * @param pin2 控制引脚2
 * @param direction 电机方向
 */
static void Motor_SetDirection(gpio_num_t pin1, gpio_num_t pin2, MotorDirection direction)
{
    switch (direction) {
        case MOTOR_DIR_FORWARD:
            gpio_set_level(pin1, 1);
            gpio_set_level(pin2, 0);
            break;
        case MOTOR_DIR_BACKWARD:
            gpio_set_level(pin1, 0);
            gpio_set_level(pin2, 1);
            break;
        case MOTOR_DIR_STOP:
        default:
            gpio_set_level(pin1, 0);
            gpio_set_level(pin2, 0);
            break;
    }
}

/**
 * @brief 电机初始化函数
 * @details 初始化电机控制引脚和PWM输出
 */
void Motor_Init(void)
{
    ESP_LOGI(TAG, "初始化TB6612FNG电机驱动");

    // 配置电机控制引脚
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_L_AIN1_PIN) | (1ULL << MOTOR_L_AIN2_PIN) |
                        (1ULL << MOTOR_L_BIN1_PIN) | (1ULL << MOTOR_L_BIN2_PIN) |
                        (1ULL << MOTOR_STBY_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // 初始化电机控制引脚为低电平
    gpio_set_level(MOTOR_L_AIN1_PIN, 0);
    gpio_set_level(MOTOR_L_AIN2_PIN, 0);
    gpio_set_level(MOTOR_L_BIN1_PIN, 0);
    gpio_set_level(MOTOR_L_BIN2_PIN, 0);
    gpio_set_level(MOTOR_STBY_PIN, 0);

    // 配置PWM
    ledc_timer_config_t pwm_timer = {
        .speed_mode = MOTOR_PWM_SPEED_MODE,
        .duty_resolution = (ledc_timer_bit_t)MOTOR_PWM_RESOLUTION,
        .timer_num = MOTOR_PWM_TIMER,
        .freq_hz = MOTOR_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&pwm_timer);

    // 配置左电机PWM通道
    ledc_channel_config_t pwm_channel_left = {
        .gpio_num = MOTOR_L_PWMA_PIN,
        .speed_mode = MOTOR_PWM_SPEED_MODE,
        .channel = MOTOR_PWM_CHANNEL_LEFT,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&pwm_channel_left);

    // 配置右电机PWM通道
    ledc_channel_config_t pwm_channel_right = {
        .gpio_num = MOTOR_L_PWMB_PIN,
        .speed_mode = MOTOR_PWM_SPEED_MODE,
        .channel = MOTOR_PWM_CHANNEL_RIGHT,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_PWM_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&pwm_channel_right);

    // 停止电机
    Motor_Stop();

    ESP_LOGI(TAG, "TB6612FNG电机驱动初始化完成");
}

/**
 * @brief 统一的电机速度设置函数
 * @details 根据电机索引设置电机的速度和方向
 *
 * @param motor_index 电机索引(0为左电机，1为右电机)
 * @param speed 速度值(-100到100，负值表示后退)
 */
static void Motor_SetSpeedByIndex(uint8_t motor_index, int8_t speed)
{
    if (motor_index >= 2) {
        ESP_LOGE(TAG, "无效的电机索引: %d", motor_index);
        return;
    }

    if (!motor_enabled) {
        ESP_LOGW(TAG, "电机未使能，无法设置速度");
        return;
    }

    // 根据速度值确定电机方向
    MotorDirection direction = (speed > 0) ? MOTOR_DIR_FORWARD : 
                              (speed < 0) ? MOTOR_DIR_BACKWARD : MOTOR_DIR_STOP;

    // 设置电机方向
    Motor_SetDirection(motor_configs[motor_index].pin1, motor_configs[motor_index].pin2, direction);

    // 设置PWM占空比
    ledc_set_duty(MOTOR_PWM_SPEED_MODE, motor_configs[motor_index].channel, Motor_SpeedToDuty(speed));
    ledc_update_duty(MOTOR_PWM_SPEED_MODE, motor_configs[motor_index].channel);

    // 保存当前速度值
    if (motor_index == 0) {
        left_motor_speed = speed;
    } else {
        right_motor_speed = speed;
    }
}

/**
 * @brief 设置左电机速度和方向
 * @details 设置左电机的速度和方向
 * 
 * @param speed 速度值(-100到100，负值表示后退)
 */
void Motor_SetLeftSpeed(int8_t speed)
{
    Motor_SetSpeedByIndex(0, speed);
}

/**
 * @brief 设置右电机速度和方向
 * @details 设置右电机的速度和方向
 * 
 * @param speed 速度值(-100到100，负值表示后退)
 */
void Motor_SetRightSpeed(int8_t speed)
{
    Motor_SetSpeedByIndex(1, speed);
}

/**
 * @brief 设置双电机速度和方向
 * @details 同时设置左右电机的速度和方向
 * 
 * @param speed 电机速度结构体
 */
void Motor_SetSpeed(const MotorSpeed *speed)
{
    if (speed == NULL) {
        ESP_LOGE(TAG, "速度指针为空");
        return;
    }

    Motor_SetLeftSpeed(speed->left_speed);
    Motor_SetRightSpeed(speed->right_speed);
}

/**
 * @brief 控制左电机方向和速度
 * @details 控制左电机的方向和速度
 * 
 * @param direction 电机方向
 * @param speed 速度值(0到100)
 */
void Motor_ControlLeft(MotorDirection direction, uint8_t speed)
{
    if (!motor_enabled) {
        ESP_LOGW(TAG, "电机未使能，无法控制电机");
        return;
    }

    // 设置电机方向
    Motor_SetDirection(MOTOR_L_AIN1_PIN, MOTOR_L_AIN2_PIN, direction);

    // 设置PWM占空比
    int8_t signed_speed = (direction == MOTOR_DIR_BACKWARD) ? -speed : speed;
    ledc_set_duty(MOTOR_PWM_SPEED_MODE, MOTOR_PWM_CHANNEL_LEFT, Motor_SpeedToDuty(signed_speed));
    ledc_update_duty(MOTOR_PWM_SPEED_MODE, MOTOR_PWM_CHANNEL_LEFT);
}

/**
 * @brief 控制右电机方向和速度
 * @details 控制右电机的方向和速度
 * 
 * @param direction 电机方向
 * @param speed 速度值(0到100)
 */
void Motor_ControlRight(MotorDirection direction, uint8_t speed)
{
    if (!motor_enabled) {
        ESP_LOGW(TAG, "电机未使能，无法控制电机");
        return;
    }

    // 设置电机方向
    Motor_SetDirection(MOTOR_L_BIN1_PIN, MOTOR_L_BIN2_PIN, direction);

    // 设置PWM占空比
    int8_t signed_speed = (direction == MOTOR_DIR_BACKWARD) ? -speed : speed;
    ledc_set_duty(MOTOR_PWM_SPEED_MODE, MOTOR_PWM_CHANNEL_RIGHT, Motor_SpeedToDuty(signed_speed));
    ledc_update_duty(MOTOR_PWM_SPEED_MODE, MOTOR_PWM_CHANNEL_RIGHT);
}

/**
 * @brief 停止所有电机
 * @details 停止左右电机
 */
void Motor_Stop(void)
{
    // 设置电机方向为停止
    Motor_SetDirection(MOTOR_L_AIN1_PIN, MOTOR_L_AIN2_PIN, MOTOR_DIR_STOP);
    Motor_SetDirection(MOTOR_L_BIN1_PIN, MOTOR_L_BIN2_PIN, MOTOR_DIR_STOP);

    // 设置PWM占空比为0
    ledc_set_duty(MOTOR_PWM_SPEED_MODE, MOTOR_PWM_CHANNEL_LEFT, 0);
    ledc_update_duty(MOTOR_PWM_SPEED_MODE, MOTOR_PWM_CHANNEL_LEFT);
    ledc_set_duty(MOTOR_PWM_SPEED_MODE, MOTOR_PWM_CHANNEL_RIGHT, 0);
    ledc_update_duty(MOTOR_PWM_SPEED_MODE, MOTOR_PWM_CHANNEL_RIGHT);

    // 重置速度值
    left_motor_speed = 0;
    right_motor_speed = 0;
}

/**
 * @brief 使能电机驱动
 * @details 使能TB6612FNG电机驱动芯片
 */
void Motor_Enable(void)
{
    gpio_set_level(MOTOR_STBY_PIN, 1);
    motor_enabled = true;
    ESP_LOGI(TAG, "电机驱动已使能");
}

/**
 * @brief 禁用电机驱动
 * @details 禁用TB6612FNG电机驱动芯片
 */
void Motor_Disable(void)
{
    Motor_Stop();
    gpio_set_level(MOTOR_STBY_PIN, 0);
    motor_enabled = false;
    ESP_LOGI(TAG, "电机驱动已禁用");
}

/**
 * @brief 获取左电机当前速度
 * @details 获取左电机的当前速度值
 *
 * @return 速度值(-100到100)
 */
int8_t Motor_GetLeftSpeed(void)
{
    return left_motor_speed;
}

/**
 * @brief 获取右电机当前速度
 * @details 获取右电机的当前速度值
 *
 * @return 速度值(-100到100)
 */
int8_t Motor_GetRightSpeed(void)
{
    return right_motor_speed;
}

/**
 * @brief 获取电机使能状态
 * @details 获取电机驱动芯片的使能状态
 *
 * @return true表示已使能，false表示已禁用
 */
bool Motor_IsEnabled(void)
{
    return motor_enabled;
}