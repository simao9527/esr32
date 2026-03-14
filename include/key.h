
#ifndef KEY_H
#define KEY_H

#include <stdint.h>
#include <stdbool.h>

// 按键引脚定义
#define KEY_PIN 35

// 档位定义
typedef enum {
    SPEED_LOW = 0,    // 低速档
    SPEED_MEDIUM = 1, // 中速档
    SPEED_HIGH = 2    // 高速档
} SpeedLevel;

// 按键状态枚举
typedef enum {
    KEY_RELEASED = 0, // 按键释放状态
    KEY_PRESSED = 1   // 按键按下状态
} KeyState;

// 按键初始化函数
void Key_Init(void);

// 获取按键状态
KeyState Key_GetState(void);

// 获取当前档位
SpeedLevel Key_GetSpeedLevel(void);

// 设置当前档位
void Key_SetSpeedLevel(SpeedLevel level);

// 按键扫描函数，在主循环中调用
void Key_Scan(void);

#endif // KEY_H
