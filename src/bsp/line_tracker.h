#ifndef LINE_TRACKER_H
#define LINE_TRACKER_H

#include <Arduino.h>

// 传感器数量
#define LINE_TRACKER_NUM 5

// 根据你的实际接线修改
#define LINE_TRACKER_GPIO_1 10
#define LINE_TRACKER_GPIO_2 11
#define LINE_TRACKER_GPIO_3 12
#define LINE_TRACKER_GPIO_4 13
#define LINE_TRACKER_GPIO_5 14

// 传感器状态结构体
typedef struct
{
    bool sensor1;
    bool sensor2;
    bool sensor3;
    bool sensor4;
    bool sensor5;

} line_tracker_state_t;


// 初始化
bool line_tracker_init(void);

// 读取单个传感器
bool line_tracker_read_sensor(uint8_t sensor_num);

// 读取全部传感器
bool line_tracker_read_all(line_tracker_state_t *state);

// 获取原始数组值
bool line_tracker_get_raw_values(bool values[LINE_TRACKER_NUM]);

// 是否检测到线
bool line_tracker_detect_line(void);

// 反初始化
void line_tracker_deinit(void);

#endif