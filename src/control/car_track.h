#ifndef _CAR_TRACK_H_
#define _CAR_TRACK_H_

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 循迹参数配置 */
typedef struct
{
    uint8_t track_speed;        // 基础速度
    uint8_t track_max_speed;    // 最大速度
    uint8_t track_min_speed;    // 最小速度

    uint32_t max_lost_count;    // 最大丢线次数
    uint32_t check_interval_ms; // 检测周期

    /* PID参数 */
    float kp;
    float ki;
    float kd;

} car_track_config_t;


/* =============================
   速度档位
   ============================= */

typedef enum
{
    CAR_TRACK_SPEED_LOW  = 1,
    CAR_TRACK_SPEED_MID  = 2,
    CAR_TRACK_SPEED_HIGH = 3

} car_track_speed_level_t;


/* 设置速度档位
   输入1低速 2中速 3高速
   返回当前档位
*/
uint8_t car_track_set_speed_level(uint8_t level);


/* 启动循迹任务 */
TaskHandle_t car_track_start(const car_track_config_t *config);


/* 停止循迹任务 */
void car_track_stop(TaskHandle_t task_handle);


/* 获取当前电机PWM值
   参数：
     left_speed - 左电机PWM值
     right_speed - 右电机PWM值
*/
void car_track_get_motor_pwm(int16_t *left_speed, int16_t *right_speed);


#ifdef __cplusplus
}
#endif

#endif