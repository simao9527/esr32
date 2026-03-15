#ifndef TB6612_DRIVER_H
#define TB6612_DRIVER_H

#include <Arduino.h>

/*
电机连接说明

左电机  = Motor A
右电机  = Motor B

小车方向说明：

前进  : 左轮前进 + 右轮前进
后退  : 左轮后退 + 右轮后退
左转  : 左轮后退 + 右轮前进
右转  : 左轮前进 + 右轮后退

GPIO连接：

左电机方向
AIN1 -> GPIO4
AIN2 -> GPIO5

右电机方向
BIN1 -> GPIO6
BIN2 -> GPIO7

PWM调速
PWMA -> GPIO16  (左电机速度)
PWMB -> GPIO17  (右电机速度)
*/

#define TB6612_AIN1 4
#define TB6612_AIN2 5
#define TB6612_BIN1 6
#define TB6612_BIN2 7

#define TB6612_PWMA 16
#define TB6612_PWMB 17

#define TB6612_PWM_FREQ 20000
#define TB6612_PWM_RESOLUTION 8

void tb6612_init();

void tb6612_set_motor(int16_t left_speed, int16_t right_speed);

void tb6612_car_forward(uint8_t speed);
void tb6612_car_backward(uint8_t speed);

void tb6612_car_turn_left(uint8_t speed);
void tb6612_car_turn_right(uint8_t speed);

void tb6612_car_stop();

#endif