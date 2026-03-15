#include "tb6612_driver.h"

#define PWM_CHANNEL_A 0
#define PWM_CHANNEL_B 1

static bool initialized = false;

void tb6612_init()
{
    if (initialized) return;

    // 设置方向控制引脚
    pinMode(TB6612_AIN1, OUTPUT);
    pinMode(TB6612_AIN2, OUTPUT);
    pinMode(TB6612_BIN1, OUTPUT);
    pinMode(TB6612_BIN2, OUTPUT);

    // 配置PWM
    ledcSetup(PWM_CHANNEL_A, TB6612_PWM_FREQ, TB6612_PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_B, TB6612_PWM_FREQ, TB6612_PWM_RESOLUTION);

    ledcAttachPin(TB6612_PWMA, PWM_CHANNEL_A);
    ledcAttachPin(TB6612_PWMB, PWM_CHANNEL_B);

    tb6612_car_stop();

    initialized = true;
}

void tb6612_set_motor(int16_t left_speed, int16_t right_speed)
{
    if (!initialized) return;

    left_speed = constrain(left_speed, -255, 255);
    right_speed = constrain(right_speed, -255, 255);

    /*
    左电机控制
    */

    if (left_speed > 0)
    {
        digitalWrite(TB6612_AIN1, HIGH);
        digitalWrite(TB6612_AIN2, LOW);
        ledcWrite(PWM_CHANNEL_A, left_speed);
    }
    else if (left_speed < 0)
    {
        digitalWrite(TB6612_AIN1, LOW);
        digitalWrite(TB6612_AIN2, HIGH);
        ledcWrite(PWM_CHANNEL_A, -left_speed);
    }
    else
    {
        digitalWrite(TB6612_AIN1, LOW);
        digitalWrite(TB6612_AIN2, LOW);
        ledcWrite(PWM_CHANNEL_A, 0);
    }

    /*
    右电机控制
    */

    if (right_speed > 0)
    {
        digitalWrite(TB6612_BIN1, HIGH);
        digitalWrite(TB6612_BIN2, LOW);
        ledcWrite(PWM_CHANNEL_B, right_speed);
    }
    else if (right_speed < 0)
    {
        digitalWrite(TB6612_BIN1, LOW);
        digitalWrite(TB6612_BIN2, HIGH);
        ledcWrite(PWM_CHANNEL_B, -right_speed);
    }
    else
    {
        digitalWrite(TB6612_BIN1, LOW);
        digitalWrite(TB6612_BIN2, LOW);
        ledcWrite(PWM_CHANNEL_B, 0);
    }
}

/*
小车前进
左轮前进 + 右轮前进
*/
void tb6612_car_forward(uint8_t speed)
{
    tb6612_set_motor(speed, speed);
}

/*
小车后退
左轮后退 + 右轮后退
*/
void tb6612_car_backward(uint8_t speed)
{
    tb6612_set_motor(-speed, -speed);
}

/*
小车左转
左轮后退 + 右轮前进
*/
void tb6612_car_turn_left(uint8_t speed)
{
    tb6612_set_motor(-speed, speed);
}

/*
小车右转
左轮前进 + 右轮后退
*/
void tb6612_car_turn_right(uint8_t speed)
{
    tb6612_set_motor(speed, -speed);
}

/*
小车停止
*/
void tb6612_car_stop()
{
    tb6612_set_motor(0, 0);
}